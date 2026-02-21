#include "WebHandler.h"
#include "Html.h"
#include "EIBServer.h"
#include "CTime.h"
#include "URLEncoding.h"
#include "Utils.h"
#include "Digest.h"
#include "conf/EIBServerUsersConf.h"
#include "conf/EIBInterfaceConf.h"
#include "conf/EIBBusMonConf.h"
#include "CommandScheduler.h"
#include <cstdlib>
#include <ctime>

std::map<CString, WebSession> CWebHandler::_sessions;
std::mutex CWebHandler::_session_mutex;

//////////////////////////////////////////////////////////////////////////////////////////////
// Route Registration
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::RegisterRoutes(httplib::SSLServer& server)
{
	// Session (no auth)
	server.Post("/api/login",   ApiLogin);
	server.Get("/api/session",  ApiSessionCheck);
	server.Post("/api/logout",  ApiLogout);

	// Data (require auth)
	server.Get("/api/history",             ApiGetGlobalHistory);
	server.Get(R"(/api/history/(.+))",     ApiGetAddressHistory);
	server.Post("/api/eib/send",           ApiSendEibCommand);
	server.Post("/api/eib/schedule",       ApiScheduleCommand);

	// Admin (require authentication)
	server.Get("/api/admin/users",             ApiGetUsers);
	server.Post("/api/admin/users",            ApiSetUsers);
	server.Get("/api/admin/interface",         ApiGetInterface);
	server.Post("/api/admin/interface/start",  ApiInterfaceStart);
	server.Post("/api/admin/interface/stop",   ApiInterfaceStop);
	server.Get("/api/admin/busmon",            ApiGetBusMonAddresses);
	server.Post("/api/admin/busmon/send",      ApiBusMonSendCmd);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// JSON helpers
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::SetJsonResponse(httplib::Response& res, const CString& json, int status)
{
	res.status = status;
	res.set_header("Access-Control-Allow-Origin", "*");
	res.set_content(std::string(json.GetBuffer(), json.GetLength()), MIME_TEXT_JSON);
}

void CWebHandler::SetJsonError(httplib::Response& res, const CString& message, int status)
{
	SetJsonResponse(res, CXmlJsonUtil::JsonError(message), status);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Session Management
//////////////////////////////////////////////////////////////////////////////////////////////

CString CWebHandler::GenerateSessionId()
{
	static bool seeded = false;
	if (!seeded) {
		srand((unsigned int)time(NULL));
		seeded = true;
	}
	CString sid;
	const char hex[] = "0123456789abcdef";
	for (int i = 0; i < 32; i++) {
		sid += hex[rand() % 16];
	}
	return sid;
}

CString CWebHandler::GetSessionCookie(const httplib::Request& req)
{
	std::string cookie_hdr = req.get_header_value("Cookie");
	if (cookie_hdr.empty()) return EMPTY_STRING;

	// Parse "WEBSESSIONID=<value>" from the Cookie header
	std::string key = "WEBSESSIONID=";
	size_t pos = cookie_hdr.find(key);
	if (pos == std::string::npos) return EMPTY_STRING;

	size_t start = pos + key.length();
	size_t end = cookie_hdr.find(';', start);
	if (end == std::string::npos) end = cookie_hdr.length();

	std::string val = cookie_hdr.substr(start, end - start);
	return CString(val.c_str());
}

void CWebHandler::ApiLogin(const httplib::Request& req, httplib::Response& res)
{
	CString body_str(req.body.c_str(), (int)req.body.length());
	CString user_name = GetJsonField(body_str, "user");
	CString password = GetJsonField(body_str, "pass");

	if (user_name.GetLength() == 0 || password.GetLength() == 0) {
		SetJsonError(res, "Missing user or pass", 401);
		return;
	}

	CUser user;
	if (!CEIBServer::GetInstance().GetUsersDB().AuthenticateUser(user_name, password, user)) {
		SetJsonError(res, "Invalid credentials", 401);
		return;
	}

	if (!user.IsWebAccessAllowed()) {
		SetJsonError(res, "Web access not allowed for this user", 403);
		return;
	}

	// Create session
	CString sid = GenerateSessionId();
	WebSession session;
	session.user_name = user_name;
	session.session_id = sid;
	session.created = time(NULL);
	session.last_access = session.created;

	{
		std::lock_guard<std::mutex> lock(_session_mutex);
		_sessions[sid] = session;
	}

	// Set cookie
	CString cookie_val = CString("WEBSESSIONID=") + sid + "; Path=/";
	res.set_header("Set-Cookie", cookie_val.GetBuffer());

	CString json = "{\"status\":\"ok\",\"user\":\"";
	json += CXmlJsonUtil::JsonEscape(user_name);
	json += "\",\"read\":";
	json += user.IsReadPolicyAllowed() ? "true" : "false";
	json += ",\"write\":";
	json += user.IsWritePolicyAllowed() ? "true" : "false";
	json += ",\"admin\":";
	json += user.IsAdminAccessAllowed() ? "true" : "false";
	json += "}";

	SetJsonResponse(res, json);
}

void CWebHandler::ApiLogout(const httplib::Request& req, httplib::Response& res)
{
	CString sid = GetSessionCookie(req);
	if (sid.GetLength() > 0) {
		std::lock_guard<std::mutex> lock(_session_mutex);
		_sessions.erase(sid);
	}

	// Clear cookie
	res.set_header("Set-Cookie", "WEBSESSIONID=; Path=/");

	SetJsonResponse(res, CXmlJsonUtil::JsonOk());
}

void CWebHandler::ApiSessionCheck(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (Authenticate(req, user)) {
		CString sid = GetSessionCookie(req);
		CString user_name;
		{
			std::lock_guard<std::mutex> lock(_session_mutex);
			std::map<CString, WebSession>::iterator it = _sessions.find(sid);
			if (it != _sessions.end()) {
				user_name = it->second.user_name;
			}
		}
		CString json = "{\"authenticated\":true,\"user\":\"";
		json += CXmlJsonUtil::JsonEscape(user_name);
		json += "\",\"read\":";
		json += user.IsReadPolicyAllowed() ? "true" : "false";
		json += ",\"write\":";
		json += user.IsWritePolicyAllowed() ? "true" : "false";
		json += ",\"admin\":";
		json += user.IsAdminAccessAllowed() ? "true" : "false";
		json += "}";
		SetJsonResponse(res, json);
	} else {
		SetJsonResponse(res, "{\"authenticated\":false}");
	}
}

bool CWebHandler::Authenticate(const httplib::Request& req, CUser& user)
{
	CString sid = GetSessionCookie(req);
	if (sid.GetLength() == 0) {
		// Also try Basic Auth for API backward compat
		std::string auth_hdr = req.get_header_value("Authorization");
		if (!auth_hdr.empty()) {
			CDigest digest(ALGORITHM_BASE64);
			size_t index = auth_hdr.find("Basic ");
			if (index != std::string::npos) {
				CString cipher(auth_hdr.c_str() + index + 6);
				CString clear;
				if (digest.Decode(cipher, clear)) {
					// clear is "user:pass"
					size_t colon = std::string(clear.GetBuffer()).find(':');
					if (colon != std::string::npos) {
						CString uname(clear.GetBuffer(), (int)colon);
						CString pass(clear.GetBuffer() + colon + 1);
						if (CEIBServer::GetInstance().GetUsersDB().AuthenticateUser(uname, pass, user)) {
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	std::lock_guard<std::mutex> lock(_session_mutex);
	std::map<CString, WebSession>::iterator it = _sessions.find(sid);
	if (it == _sessions.end()) {
		return false;
	}

	// Check session age (expire after 1 hour)
	time_t now = time(NULL);
	if (now - it->second.last_access > 3600) {
		_sessions.erase(it);
		return false;
	}

	it->second.last_access = now;
	CString user_name = it->second.user_name;

	return CEIBServer::GetInstance().GetUsersDB().GetRecord(user_name, user);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Admin API endpoints
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::ApiGetUsers(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}

	START_TRY
		CEIBServerUsersConf conf;
		conf.GetConnectedClients();
		CDataBuffer xml_buf;
		conf.ToXml(xml_buf);
		CString xml((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
		CString json = CXmlJsonUtil::XmlToJson(xml);
		SetJsonResponse(res, json);
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiSetUsers(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsAdminAccessAllowed()) {
		SetJsonError(res, "Admin privileges required", 403);
		return;
	}

	START_TRY
		CEIBServerUsersConf conf;
		CDataBuffer body_buf;
		body_buf.Add(req.body.c_str(), (int)req.body.length());
		conf.FromXml(body_buf);
		conf.SetConnectedClients();
		SetJsonResponse(res, CXmlJsonUtil::JsonOk());
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiGetInterface(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}

	START_TRY
		CEIBInterfaceConf conf;
		CDataBuffer xml_buf;
		conf.ToXml(xml_buf);
		CString xml((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
		CString json = CXmlJsonUtil::XmlToJson(xml);
		SetJsonResponse(res, json);
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiInterfaceStart(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsAdminAccessAllowed()) {
		SetJsonError(res, "Admin privileges required", 403);
		return;
	}

	START_TRY
		CEIBInterfaceConf conf;
		CDataBuffer xml_buf;
		conf.StartInterface(xml_buf);
		CString xml((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
		CString json = CXmlJsonUtil::XmlToJson(xml);
		SetJsonResponse(res, json);
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiInterfaceStop(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsAdminAccessAllowed()) {
		SetJsonError(res, "Admin privileges required", 403);
		return;
	}

	START_TRY
		CEIBInterfaceConf conf;
		CDataBuffer xml_buf;
		conf.StopInterface(xml_buf);
		CString xml((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
		CString json = CXmlJsonUtil::XmlToJson(xml);
		SetJsonResponse(res, json);
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiGetBusMonAddresses(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}

	START_TRY
		CEIBBusMonAddrListConf conf;
		CDataBuffer xml_buf;
		conf.ToXml(xml_buf);
		CString xml((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
		CString json = CXmlJsonUtil::XmlToJson(xml);
		SetJsonResponse(res, json);
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

void CWebHandler::ApiBusMonSendCmd(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsAdminAccessAllowed()) {
		SetJsonError(res, "Admin privileges required", 403);
		return;
	}

	CString body_str(req.body.c_str(), (int)req.body.length());
	CString addr = GetJsonField(body_str, "address");
	CString val = GetJsonField(body_str, "value");
	CString mode = GetJsonField(body_str, "mode");

	if (mode.GetLength() == 0) mode = "3"; // WAIT_FOR_CONFIRM default

	START_TRY
		CEIBBusMonAddrListConf conf;
		conf.SendCmdToAddr(addr, val, mode);
		SetJsonResponse(res, CXmlJsonUtil::JsonOk());
	END_TRY_START_CATCH(e)
		SetJsonError(res, e.what());
	END_CATCH
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Data API endpoints
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::ApiGetGlobalHistory(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsReadPolicyAllowed()) {
		SetJsonError(res, "Insufficient privileges", 403);
		return;
	}

	CStatsDB& db = CEIBServer::GetInstance().GetStatsDB();

	CString json = "{\"records\":[";
	map<CEibAddress, CEIBObjectRecord>& records = db.GetData();
	bool first = true;
	map<CEibAddress, CEIBObjectRecord>::const_iterator it;
	for (it = records.begin(); it != records.end(); ++it) {
		if (!first) json += ",";
		first = false;
		json += "{\"address\":\"";
		json += CXmlJsonUtil::JsonEscape(it->first.ToString());
		json += "\",\"entries\":[";
		const deque<CEIBRecord>& entries = it->second.GetHistory();
		bool first_entry = true;
		deque<CEIBRecord>::const_iterator eit;
		for (eit = entries.begin(); eit != entries.end(); ++eit) {
			if (!first_entry) json += ",";
			first_entry = false;
			json += "{\"time\":\"";
			json += CXmlJsonUtil::JsonEscape(eit->GetTime().Format());
			json += "\",\"value\":\"0x";
			for (int i = 0; i < eit->GetValueLength(); ++i) {
				json += ToHex((int)eit->GetValue()[i]);
			}
			json += "\"}";
		}
		json += "]}";
	}
	json += "]}";

	SetJsonResponse(res, json);
}

void CWebHandler::ApiGetAddressHistory(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsReadPolicyAllowed()) {
		SetJsonError(res, "Insufficient privileges", 403);
		return;
	}

	CString address = URLEncoder::Decode(CString(req.matches[1].str().c_str()));

	CStatsDB& db = CEIBServer::GetInstance().GetStatsDB();

	CEibAddress addr(address);
	CEIBObjectRecord rec;
	if (!db.GetRecord(addr, rec)) {
		SetJsonError(res, CString("No entries found for address: ") + address, 404);
		return;
	}

	CString json = "{\"address\":\"";
	json += CXmlJsonUtil::JsonEscape(address);
	json += "\",\"entries\":[";
	const deque<CEIBRecord>& entries = rec.GetHistory();
	bool first = true;
	deque<CEIBRecord>::const_iterator eit;
	for (eit = entries.begin(); eit != entries.end(); ++eit) {
		if (!first) json += ",";
		first = false;
		json += "{\"time\":\"";
		json += CXmlJsonUtil::JsonEscape(eit->GetTime().Format());
		json += "\",\"value\":\"0x";
		for (int i = 0; i < eit->GetValueLength(); ++i) {
			json += ToHex((int)eit->GetValue()[i]);
		}
		json += "\"}";
	}
	json += "]}";

	SetJsonResponse(res, json);
}

void CWebHandler::ApiSendEibCommand(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsWritePolicyAllowed()) {
		SetJsonError(res, "Insufficient privileges", 403);
		return;
	}

	CString body_str(req.body.c_str(), (int)req.body.length());
	CString addr = GetJsonField(body_str, "address");
	CString apci_str = GetJsonField(body_str, "value");

	if (addr.GetLength() == 0 || apci_str.GetLength() == 0) {
		SetJsonError(res, "Missing address or value");
		return;
	}

	unsigned char apci[MAX_EIB_VALUE_LEN];
	unsigned char apci_len;
	if (!GetByteArrayFromHexString(apci_str, apci, apci_len)) {
		SetJsonError(res, "Invalid hex value");
		return;
	}

	CString err;
	if (!SendEIBCommand(addr, apci, apci_len, err)) {
		SetJsonError(res, err);
		return;
	}

	SetJsonResponse(res, CXmlJsonUtil::JsonOk());
}

void CWebHandler::ApiScheduleCommand(const httplib::Request& req, httplib::Response& res)
{
	CUser user;
	if (!Authenticate(req, user)) {
		SetJsonError(res, "Not authenticated", 401);
		return;
	}
	if (!user.IsWritePolicyAllowed()) {
		SetJsonError(res, "Insufficient privileges", 403);
		return;
	}

	CString body_str(req.body.c_str(), (int)req.body.length());
	CString addr = GetJsonField(body_str, "address");
	CString apci_str = GetJsonField(body_str, "value");
	CString datetime_str = GetJsonField(body_str, "datetime");

	if (addr.GetLength() == 0 || apci_str.GetLength() == 0 || datetime_str.GetLength() == 0) {
		SetJsonError(res, "Missing address, value, or datetime");
		return;
	}

	unsigned char apci[MAX_EIB_VALUE_LEN];
	unsigned char apci_len;
	if (!GetByteArrayFromHexString(apci_str, apci, apci_len)) {
		SetJsonError(res, "Invalid hex value");
		return;
	}

	CTime datetime(datetime_str.GetBuffer(), true);
	CString err;
	if (!CEIBServer::GetInstance().GetScheduler().AddScheduledCommand(
			datetime, CEibAddress(addr), apci, apci_len, err)) {
		SetJsonError(res, err);
		return;
	}

	SetJsonResponse(res, CXmlJsonUtil::JsonOk());
}

//////////////////////////////////////////////////////////////////////////////////////////////
// EIB command sending
//////////////////////////////////////////////////////////////////////////////////////////////

bool CWebHandler::SendEIBCommand(const CString& addr, unsigned char *apci, unsigned char apci_len, CString& err)
{
	START_TRY
		CEibAddress dest_addr(URLEncoder::Decode(addr));
		CEIBInterface& iface = CEIBServer::GetInstance().GetEIBInterface();

		if(iface.GetMode() == UNDEFINED_MODE){
			err += "EIBNet/IP Device is not initialized";
			return false;
		}

		CCemi_L_Data_Frame msg;
		msg.SetMessageControl(L_DATA_REQ);
		msg.SetAddilLength(0);
		msg.SetCtrl1(0);
		msg.SetCtrl2(6);
		msg.SetPriority(PRIORITY_NORMAL);
		msg.SetFrameFormatStandard();
		msg.SetSrcAddress(CEibAddress());
		msg.SetDestAddress(dest_addr);
		msg.SetValue(apci, apci_len);

		iface.GetOutputHandler()->Write(msg, NON_BLOCKING, NULL);
	END_TRY_START_CATCH(e)
		err += e.what();
		return false;
	END_CATCH

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////////////////////////

CString CWebHandler::GetJsonField(const CString& json, const CString& field)
{
	CString search = "\"";
	search += field;
	search += "\":\"";

	size_t pos = json.Find(search);
	if (pos == string::npos) return EMPTY_STRING;

	pos += search.GetLength();
	size_t end = json.Find("\"", pos);
	if (end == string::npos) return EMPTY_STRING;

	return json.SubString(pos, end - pos);
}

bool CWebHandler::GetByteArrayFromHexString(const CString& str, unsigned char *val, unsigned char &val_len)
{
	if(str.FindFirstOf("0x") != 0){
		return false;
	}
	val_len = 0;

	CString temp_str(str.SubString(2,str.GetLength() - 2));
	temp_str.TrimStart('0');
	if ( temp_str.GetLength() % 2 != 0){
		temp_str = CString("0") + temp_str;
	}
	ASSERT_ERROR(temp_str.GetLength() % 2 == 0,"Must be even!!!!");

	for (int i = 0; i < temp_str.GetLength(); i += 2)
	{
		if(!IS_HEX_DIGIT(temp_str[i]) || !IS_HEX_DIGIT(temp_str[i + 1]))
		{
			return false;
		}
		val[i / 2] = HexToChar(temp_str.SubString(i,2));
		++val_len;
	}
	return true;
}

unsigned char CWebHandler::HexToChar(const CString& hexNumber)
{
	unsigned char high_digit = hexNumber[0];
	unsigned char low_digit	= hexNumber[1];
	int lowOrderValue = GetDigitValue(low_digit);
	int highOrderValue = GetDigitValue(high_digit);

	unsigned char res = (unsigned char)(lowOrderValue + 16 * highOrderValue);
	return res;
}

int CWebHandler::GetDigitValue (char digit)
{
	if (digit >= '0' && digit <= '9'){
      return (digit - '0');
	}
	else if (digit >= 'A' && digit <= 'F'){
		return (digit - 'A' + 10);
	}
	else if (digit >= 'a' && digit <= 'f'){
		return (digit - 'a' + 10);
	}

	return -1;
}

CString CWebHandler::GetMimeType(const CString& file_path)
{
	if (file_path.EndsWith(".html") || file_path.EndsWith(".htm")) return MIME_TEXT_HTML;
	if (file_path.EndsWith(".css")) return MIME_TEXT_CSS;
	if (file_path.EndsWith(".js")) return MIME_TEXT_JS;
	if (file_path.EndsWith(".json")) return MIME_TEXT_JSON;
	if (file_path.EndsWith(".png")) return MIME_IMAGE_PNG;
	if (file_path.EndsWith(".jpg") || file_path.EndsWith(".jpeg")) return MIME_IMAGE_JPEG;
	if (file_path.EndsWith(".svg")) return MIME_IMAGE_SVG;
	if (file_path.EndsWith(".ico")) return MIME_IMAGE_ICO;
	return "application/octet-stream";
}
