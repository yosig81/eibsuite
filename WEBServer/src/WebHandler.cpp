#include "WebHandler.h"
#include "Html.h"
#include "WEBServer.h"
#include "CTime.h"
#include "URLEncoding.h"
#include "Utils.h"
#include <cstdlib>
#include <ctime>

map<CString, WebSession> CWebHandler::_sessions;
JTCMutex CWebHandler::_session_mutex;

CWebHandler::CWebHandler():
_stop(false)
{
	this->setName("WEB Handler");
}

CWebHandler::~CWebHandler()
{
}

void CWebHandler::Close()
{
	do
	{
		JTCSynchronized sync(*this);
		//at this point we sure the handler called "wait".
		_stop = true;
		this->notify();
	}while(0);

	//now, when monitor acuqired we can be sure the the hanlder is dead...
	JTCSynchronized sync(*this);
}

void CWebHandler::run()
{
	JTCSynchronized sync(*this);

	char buffer[MAX_HTTP_REQUEST_SIZE];
	CHttpReply reply;
	CDataBuffer reply_buf;

	while (!_stop)
	{
		this->wait();

		InitReply(reply);

		if(_job_queue.size() == 0){
			continue;
		}

		TCPSocket* sock = _job_queue.front();

		START_TRY
			HandleRequest(sock, buffer,reply);
		END_TRY_START_CATCH(e)
			//send error page to user
			CContentGenerator::Generate_Error_Line(reply_buf,e.what());
			LOG_ERROR("[WEB Handler] %s", e.what());
		END_CATCH

		reply.Finalize(reply_buf);
		sock->Send(reply_buf.GetBuffer(),reply_buf.GetLength());
		reply.Reset();
		memset(buffer,0,MAX_HTTP_REQUEST_SIZE);
		//remove this job from queue
		_job_queue.pop();

		if (sock != NULL){
			delete sock;
		}
	}
}

void CWebHandler::InitReply(CHttpReply& reply)
{
	reply.RemoveAllHeaders();
	reply.SetContentType(CT_TEXT_HTML);
	reply.SetStatusCode(STATUS_OK);
	reply.SetVersion(HTTP_1_0);
	reply.AddHeader("Connection","Close");
}

void CWebHandler::Signal()
{
	JTCSynchronized sync(*this);
	this->notify();
}

void CWebHandler::HandleRequest(TCPSocket* sock, char* buffer,CHttpReply& reply)
{
	int len = 0;
	START_TRY
		len = sock->Recv(buffer,MAX_HTTP_REQUEST_SIZE,INFINITE);
	END_TRY_START_CATCH_SOCKET(e)
		CWEBServer::GetInstance().GetLog().Log(LOG_LEVEL_ERROR,"Socket exception in WEBHandler. Code: %d",e.GetErrorCode());
	END_CATCH

	if (len > MAX_HTTP_REQUEST_SIZE || len == 0){
		//log error and return 404 to client
		return;
	}

	CHttpRequest request;
	CHttpParser parser(request,buffer,len, *sock);
	if (!parser.IsLegalRequest()){
		//return error to client
		throw CEIBException(GeneralError,"Error in request parsing");
	}

	CString uri = request.GetRequestURI();

	// --- API requests (no Basic Auth required, use cookie session) ---
	if (uri.GetLength() >= 4 && uri.SubString(0, 5) == "/api/") {
		HandleApiRequest(request, reply);
		return;
	}

	// --- SPA: serve index.html for root ---
	if (uri == "/" && request.GetNumParams() == 0) {
		if (HandleStaticFile("/index.html", reply)) {
			return;
		}
	}

	// --- Static file serving from www/ ---
	if (uri.GetLength() > 1 && !uri.EndsWith(".jpg") && !uri.EndsWith(".jpeg") &&
		!uri.EndsWith(".png") && uri != "/favicon.ico" && request.GetNumParams() == 0)
	{
		if (HandleStaticFile(uri, reply)) {
			return;
		}
	}

	// --- Legacy path: Basic Auth required ---
	CHttpHeader auth;
	if(!request.GetHeader(AUTHORIZATION_HEADER,auth)){
		// For non-API, non-static requests, require Basic Auth
		reply.AddHeader(AUTHENTICATION_HEADER,"Basic");
		reply.SetStatusCode(STATUS_NOT_AUTHORIZED);
		return;
	}

	CDigest digest(ALGORITHM_BASE64);
	size_t index = auth.GetValue().Find("Basic ");
	CString cipher = auth.GetValue().SubString(index + 6,auth.GetValue().GetLength() - index - 6);
	CString clear;
	if(!digest.Decode(cipher, clear)){
		reply.AddHeader(AUTHENTICATION_HEADER,"Basic");
		reply.SetStatusCode(STATUS_NOT_AUTHORIZED);
		return;
	}

	//user name and password
	CHttpHeader login(clear);

	//find is user exist and password valid
	CUser user;
	if (!CWEBServer::GetInstance().GetUsersDB().AuthenticateUser(login.GetName(),login.GetValue(),user)){
		reply.AddHeader(AUTHENTICATION_HEADER,"Basic");
		reply.SetStatusCode(STATUS_NOT_AUTHORIZED);
		return;
	}

	reply.SetContentType(CT_TEXT_HTML);
	reply.SetStatusCode(STATUS_OK);
	reply.SetVersion(HTTP_1_0);

	if(request.GetRequestURI() == "/favicon.ico" && request.GetNumParams() == 0){
		HandleFavoritsIconRequest(reply);
		return;
	}

	//Check if the request if for an image
	if(request.GetRequestURI().EndsWith(".jpg") || request.GetRequestURI().EndsWith(".png") ||
	   request.GetRequestURI().EndsWith(".jpeg")	)
	{
		HandleImageRequest(request.GetRequestURI().SubString(1, request.GetRequestURI().GetLength() - 1), reply);
		return;
	}

	//logic of parameters (legacy ?cmd= handling)
	CreateContent(request,reply, user);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// API Handler
//////////////////////////////////////////////////////////////////////////////////////////////

bool CWebHandler::HandleApiRequest(CHttpRequest& request, CHttpReply& reply)
{
	CString uri = request.GetRequestURI();

	// Session endpoints (no auth needed)
	if (uri == "/api/login") {
		ApiLogin(request, reply);
		return true;
	}
	if (uri == "/api/session") {
		ApiSessionCheck(request, reply);
		return true;
	}
	if (uri == "/api/logout") {
		ApiLogout(request, reply);
		return true;
	}

	// All other API endpoints require auth
	CUser user;
	if (!ApiAuthenticate(request, user)) {
		SetJsonError(reply, "Not authenticated", STATUS_NOT_AUTHORIZED);
		return true;
	}

	// Data endpoints (existing WEB features)
	if (uri == "/api/history") {
		if (!user.IsReadPolicyAllowed()) {
			SetJsonError(reply, "Insufficient privileges", STATUS_FORBIDDEN);
			return true;
		}
		ApiGetGlobalHistory(reply);
		return true;
	}
	if (uri.GetLength() > 13 && uri.SubString(0, 13) == "/api/history/") {
		if (!user.IsReadPolicyAllowed()) {
			SetJsonError(reply, "Insufficient privileges", STATUS_FORBIDDEN);
			return true;
		}
		CString address = URLEncoder::Decode(uri.SubString(13, uri.GetLength() - 13));
		ApiGetFunctionHistory(address, reply);
		return true;
	}
	if (uri == "/api/eib/send") {
		if (!user.IsWritePolicyAllowed()) {
			SetJsonError(reply, "Insufficient privileges", STATUS_FORBIDDEN);
			return true;
		}
		ApiSendEibCommand(request, reply);
		return true;
	}
	if (uri == "/api/eib/schedule") {
		if (!user.IsWritePolicyAllowed()) {
			SetJsonError(reply, "Insufficient privileges", STATUS_FORBIDDEN);
			return true;
		}
		ApiScheduleCommand(request, reply);
		return true;
	}

	// Admin endpoints (proxy to ConsoleManager)
	if (uri == "/api/admin/users" && request.GetMethod() == GET_M) {
		ApiGetUsers(reply);
		return true;
	}
	if (uri == "/api/admin/users" && request.GetMethod() == POST_M) {
		ApiSetUsers(request, reply);
		return true;
	}
	if (uri == "/api/admin/interface") {
		ApiGetInterface(reply);
		return true;
	}
	if (uri == "/api/admin/interface/start") {
		ApiInterfaceStart(reply);
		return true;
	}
	if (uri == "/api/admin/interface/stop") {
		ApiInterfaceStop(reply);
		return true;
	}
	if (uri == "/api/admin/busmon" && request.GetMethod() == GET_M) {
		ApiGetBusMonAddresses(reply);
		return true;
	}
	if (uri == "/api/admin/busmon/send") {
		ApiBusMonSendCmd(request, reply);
		return true;
	}

	SetJsonError(reply, "Unknown API endpoint", STATUS_NOT_FOUND);
	return true;
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

CString CWebHandler::GetSessionCookie(CHttpRequest& request)
{
	CHttpCookie cookie;
	if (request.GetCookie("WEBSESSIONID", cookie)) {
		return cookie.GetValue();
	}
	return EMPTY_STRING;
}

void CWebHandler::ApiLogin(CHttpRequest& request, CHttpReply& reply)
{
	// Parse user/pass from JSON body or form params
	CString body_str((const char*)request.GetContent().GetBuffer(), request.GetContent().GetLength());
	CString user_name = GetJsonField(body_str, "user");
	CString password = GetJsonField(body_str, "pass");

	if (user_name.GetLength() == 0 || password.GetLength() == 0) {
		SetJsonError(reply, "Missing user or pass", STATUS_NOT_AUTHORIZED);
		return;
	}

	CUser user;
	if (!CWEBServer::GetInstance().GetUsersDB().AuthenticateUser(user_name, password, user)) {
		SetJsonError(reply, "Invalid credentials", STATUS_NOT_AUTHORIZED);
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
		JTCSynchronized sync(_session_mutex);
		_sessions[sid] = session;
	}

	// Set cookie
	CHttpCookie cookie;
	cookie.SetName("WEBSESSIONID");
	cookie.SetValue(sid);
	cookie.SetPath("/");
	reply.AddCookie(cookie);

	CString json = "{\"status\":\"ok\",\"user\":\"";
	json += CXmlJsonUtil::JsonEscape(user_name);
	json += "\",\"read\":";
	json += user.IsReadPolicyAllowed() ? "true" : "false";
	json += ",\"write\":";
	json += user.IsWritePolicyAllowed() ? "true" : "false";
	json += "}";

	SetJsonResponse(reply, json);
}

void CWebHandler::ApiLogout(CHttpRequest& request, CHttpReply& reply)
{
	CString sid = GetSessionCookie(request);
	if (sid.GetLength() > 0) {
		JTCSynchronized sync(_session_mutex);
		_sessions.erase(sid);
	}

	// Clear cookie
	CHttpCookie cookie;
	cookie.SetName("WEBSESSIONID");
	cookie.SetValue("");
	cookie.SetPath("/");
	reply.AddCookie(cookie);

	SetJsonResponse(reply, CXmlJsonUtil::JsonOk());
}

void CWebHandler::ApiSessionCheck(CHttpRequest& request, CHttpReply& reply)
{
	CUser user;
	if (ApiAuthenticate(request, user)) {
		CString sid = GetSessionCookie(request);
		CString user_name;
		{
			JTCSynchronized sync(_session_mutex);
			map<CString, WebSession>::iterator it = _sessions.find(sid);
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
		json += "}";
		SetJsonResponse(reply, json);
	} else {
		SetJsonResponse(reply, "{\"authenticated\":false}");
	}
}

bool CWebHandler::ApiAuthenticate(CHttpRequest& request, CUser& user)
{
	CString sid = GetSessionCookie(request);
	if (sid.GetLength() == 0) {
		// Also try Basic Auth for API backward compat
		CHttpHeader auth;
		if (request.GetHeader(AUTHORIZATION_HEADER, auth)) {
			CDigest digest(ALGORITHM_BASE64);
			size_t index = auth.GetValue().Find("Basic ");
			if (index != string::npos) {
				CString cipher = auth.GetValue().SubString(index + 6, auth.GetValue().GetLength() - index - 6);
				CString clear;
				if (digest.Decode(cipher, clear)) {
					CHttpHeader login(clear);
					if (CWEBServer::GetInstance().GetUsersDB().AuthenticateUser(login.GetName(), login.GetValue(), user)) {
						return true;
					}
				}
			}
		}
		return false;
	}

	JTCSynchronized sync(_session_mutex);
	map<CString, WebSession>::iterator it = _sessions.find(sid);
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

	return CWEBServer::GetInstance().GetUsersDB().AuthenticateUser(user_name, CString(""), user) ||
		   // If AuthenticateUser requires password, look up user directly
		   (CWEBServer::GetInstance().GetUsersDB().GetUsersList().count(user_name) > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Admin API endpoints (proxy to ConsoleManager)
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::ApiGetUsers(CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	CString xml = cc.SendGet("EIB_SERVER_GET_USERS_CONF_CMD");
	CString json = CXmlJsonUtil::XmlToJson(xml);
	SetJsonResponse(reply, json);
}

void CWebHandler::ApiSetUsers(CHttpRequest& request, CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	// The body should be XML (pass through from SPA which converts JSON to XML)
	CString body((const char*)request.GetContent().GetBuffer(), request.GetContent().GetLength());
	CString result = cc.SendPost("EIB_SERVER_SET_USERS_CONF_CMD", body);
	SetJsonResponse(reply, CXmlJsonUtil::JsonOk());
}

void CWebHandler::ApiGetInterface(CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	CString xml = cc.SendGet("EIB_SERVER_GET_INTERFACE_CONF_CMD");
	CString json = CXmlJsonUtil::XmlToJson(xml);
	SetJsonResponse(reply, json);
}

void CWebHandler::ApiInterfaceStart(CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	CString xml = cc.SendGet("EIB_SERVER_INTERFACE_START_CMD");
	CString json = CXmlJsonUtil::XmlToJson(xml);
	SetJsonResponse(reply, json);
}

void CWebHandler::ApiInterfaceStop(CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	CString xml = cc.SendGet("EIB_SERVER_INTERFACE_STOP_CMD");
	CString json = CXmlJsonUtil::XmlToJson(xml);
	SetJsonResponse(reply, json);
}

void CWebHandler::ApiGetBusMonAddresses(CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}
	CString xml = cc.SendGet("EIB_BUS_MON_GET_ADDRESSES_CMD");
	CString json = CXmlJsonUtil::XmlToJson(xml);
	SetJsonResponse(reply, json);
}

void CWebHandler::ApiBusMonSendCmd(CHttpRequest& request, CHttpReply& reply)
{
	CConsoleClient& cc = CWEBServer::GetInstance().GetConsoleClient();
	if (!cc.IsLoggedIn()) {
		SetJsonError(reply, "Console Manager not connected");
		return;
	}

	CString body_str((const char*)request.GetContent().GetBuffer(), request.GetContent().GetLength());
	CString addr = GetJsonField(body_str, "address");
	CString val = GetJsonField(body_str, "value");
	CString mode = GetJsonField(body_str, "mode");

	if (mode.GetLength() == 0) mode = "3"; // WAIT_FOR_CONFIRM default

	CString form_body = "SND_DEST_ADDR=";
	form_body += addr;
	form_body += "&SND_VAL=";
	form_body += val;
	form_body += "&SND_MODE=";
	form_body += mode;

	CString result = cc.SendPost("EIB_BUS_MON_SEND_CMD_TO_ADDR_CMD", form_body, true);
	SetJsonResponse(reply, CXmlJsonUtil::JsonOk());
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Data API endpoints
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::ApiGetGlobalHistory(CHttpReply& reply)
{
	CStatsDB db;
	CString err;
	GetHisotryFromEIB(db, err);

	if (err.GetLength() > 0) {
		SetJsonError(reply, err);
		return;
	}

	// Build JSON from StatsDB
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

	SetJsonResponse(reply, json);
}

void CWebHandler::ApiGetFunctionHistory(const CString& address, CHttpReply& reply)
{
	CStatsDB db;
	CString err;
	GetHisotryFromEIB(db, err);

	if (err.GetLength() > 0) {
		SetJsonError(reply, err);
		return;
	}

	CEibAddress addr(address);
	CEIBObjectRecord rec;
	if (!db.GetRecord(addr, rec)) {
		SetJsonError(reply, CString("No entries found for address: ") + address, STATUS_NOT_FOUND);
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

	SetJsonResponse(reply, json);
}

void CWebHandler::ApiSendEibCommand(CHttpRequest& request, CHttpReply& reply)
{
	CString body_str((const char*)request.GetContent().GetBuffer(), request.GetContent().GetLength());
	CString addr = GetJsonField(body_str, "address");
	CString apci_str = GetJsonField(body_str, "value");

	if (addr.GetLength() == 0 || apci_str.GetLength() == 0) {
		SetJsonError(reply, "Missing address or value");
		return;
	}

	unsigned char apci[MAX_EIB_VALUE_LEN];
	unsigned char apci_len;
	if (!GetByteArrayFromHexString(apci_str, apci, apci_len)) {
		SetJsonError(reply, "Invalid hex value");
		return;
	}

	CString err;
	if (!SendEIBCommand(addr, apci, apci_len, err)) {
		SetJsonError(reply, err);
		return;
	}

	SetJsonResponse(reply, CXmlJsonUtil::JsonOk());
}

void CWebHandler::ApiScheduleCommand(CHttpRequest& request, CHttpReply& reply)
{
	CString body_str((const char*)request.GetContent().GetBuffer(), request.GetContent().GetLength());
	CString addr = GetJsonField(body_str, "address");
	CString apci_str = GetJsonField(body_str, "value");
	CString datetime_str = GetJsonField(body_str, "datetime");

	if (addr.GetLength() == 0 || apci_str.GetLength() == 0 || datetime_str.GetLength() == 0) {
		SetJsonError(reply, "Missing address, value, or datetime");
		return;
	}

	unsigned char apci[MAX_EIB_VALUE_LEN];
	unsigned char apci_len;
	if (!GetByteArrayFromHexString(apci_str, apci, apci_len)) {
		SetJsonError(reply, "Invalid hex value");
		return;
	}

	CTime datetime(datetime_str.GetBuffer(), true);
	CString err;
	if (!CWEBServer::GetInstance().GetCollector()->AddScheduledCommand(
			datetime, CEibAddress(addr), apci, apci_len, err)) {
		SetJsonError(reply, err);
		return;
	}

	SetJsonResponse(reply, CXmlJsonUtil::JsonOk());
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Static File Serving
//////////////////////////////////////////////////////////////////////////////////////////////

bool CWebHandler::HandleStaticFile(const CString& path, CHttpReply& reply)
{
	CString www_root = CWEBServer::GetInstance().GetConfig().GetWwwRoot();
	CString file_path = www_root + path;

	// Security: prevent path traversal
	if (file_path.Find("..") != string::npos) {
		return false;
	}

	int total_size = 0;
	START_TRY
		FillRawFile(file_path, reply.GetContent(), total_size);
	END_TRY_START_CATCH(e)
		// File not found - try SPA fallback (serve index.html for client-side routing)
		START_TRY
			CString index_path = www_root + "/index.html";
			FillRawFile(index_path, reply.GetContent(), total_size);
			reply.SetStatusCode(STATUS_OK);
			reply.SetVersion(HTTP_1_0);
			reply.AddHeader("Content-Type", MIME_TEXT_HTML);
			return true;
		END_TRY_START_CATCH(e2)
			return false;
		END_CATCH
	END_CATCH

	CString mime = GetMimeType(file_path);
	reply.SetStatusCode(STATUS_OK);
	reply.SetVersion(HTTP_1_0);
	reply.AddHeader("Content-Type", mime);

	return true;
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

//////////////////////////////////////////////////////////////////////////////////////////////
// JSON helpers
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::SetJsonResponse(CHttpReply& reply, const CString& json, HTTP_STATUS_CODE status)
{
	reply.SetStatusCode(status);
	reply.SetVersion(HTTP_1_0);
	reply.AddHeader("Content-Type", MIME_TEXT_JSON);
	reply.AddHeader("Access-Control-Allow-Origin", "*");
	reply.GetContent().Clear();
	reply.GetContent().Add(json.GetBuffer(), json.GetLength());
}

void CWebHandler::SetJsonError(CHttpReply& reply, const CString& message, HTTP_STATUS_CODE status)
{
	SetJsonResponse(reply, CXmlJsonUtil::JsonError(message), status);
}

CString CWebHandler::GetJsonField(const CString& json, const CString& field)
{
	// Simple JSON field extraction: finds "field":"value"
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

//////////////////////////////////////////////////////////////////////////////////////////////
// Legacy code (preserved for backward compatibility)
//////////////////////////////////////////////////////////////////////////////////////////////

void CWebHandler::HandleImageRequest(const CString& file_name, CHttpReply& reply)
{
	if(file_name.EndsWith(".jpg") || file_name.EndsWith(".jpeg")){
		reply.SetContentType(CT_IMAGE_JPEG);
	}else if(file_name.EndsWith(".png")){
		reply.SetContentType(CT_IMAGE_PNG);
	}else{
		reply.SetVersion(HTTP_1_0);
		reply.SetStatusCode(STATUS_NOT_FOUND);
		return;
	}

	reply.SetVersion(HTTP_1_0);
	reply.SetStatusCode(STATUS_OK);
	reply.AddHeader("Accept-Ranges","bytes");

	const CString& file = CWEBServer::GetInstance().GetConfig().GetImagesFolder() + PATH_DELIM + file_name;
	int clen;
	CWebHandler::FillRawFile(file, reply.GetContent(), clen);
}

void CWebHandler::HandleFavoritsIconRequest(CHttpReply& reply)
{
	reply.SetVersion(HTTP_1_0);
	reply.SetStatusCode(STATUS_OK);
	reply.SetContentType(CT_IMAGE_X_ICON);

	reply.AddHeader("Accept-Ranges","bytes");

	const CString& icon_file = CWEBServer::GetInstance().GetConfig().GetImagesFolder() + PATH_DELIM + "favicon.ico";
	int clen;
	CWebHandler::FillRawFile(icon_file, reply.GetContent(), clen);
}

void CWebHandler::FillRawFile(const CString& file_name, CDataBuffer& buf, int& total_size)
{
	buf.Clear();
	total_size = 0;
	ifstream file (file_name.GetBuffer(), ios::in | ios::binary | ios::ate);
	if(file.is_open()){
		int size;
		char memblock[1024];
		file.seekg(0, ios::beg);
		while(file.good())
		{
			file.read(memblock, 1024);
			size = (int)file.gcount();
			if (!size) break;
			total_size += size;
			buf.Add(memblock,size);
		}
		file.close();
	}else{
		throw CEIBException(FileError, "[WEB Handler] Cannot find file %s", file_name.GetBuffer());
	}
}

void CWebHandler::CreateContent(CHttpRequest& request,CHttpReply& reply, const CUser& user)
{
	CHttpParameter p;
	CString err;

	if(request.GetNumParams() == 0){
		CContentGenerator gen;
		gen.Page_Main(reply.GetContent(),GetCurrentDomain(),err);
		return;
	}

	if(!request.GetParameter(PARAM_NAME_COMMAND,p)){
		//redirect to main page
		reply.Redirect("/");
		return;
	}

	CStatsDB db;
	CEIBObjectRecord rec;
	CContentGenerator gen;
	CHttpParameter p1,p2,p3;
	unsigned char apci[MAX_EIB_VALUE_LEN];
	unsigned char apci_len;
	CString norm_param;
	CEibAddress addr;
	CTime datetime;

	int cmd_type = p.GetValue().ToInt();
	if((cmd_type == PARAM_COMMAND_VALUE_GLOBAL_HISTORY || cmd_type == PARAM_COMMAND_VALUE_FUNCTION_HISTORY) &&
			!user.IsReadPolicyAllowed()){
		goto unauthorized;
	}
	else if((cmd_type == PARAM_COMMAND_VALUE_SEND_COMMAND || cmd_type == PARAM_COMMAND_VALUE_SCHEDULE_COMMAND) &&
			!user.IsWritePolicyAllowed()){
		goto unauthorized;
	}

	switch (cmd_type)
	{
	case PARAM_COMMAND_VALUE_GLOBAL_HISTORY:
		GetHisotryFromEIB(db,err);
		gen.Page_HistoryGlobal(reply.GetContent(),db,err);
		break;
	case PARAM_COMMAND_VALUE_FUNCTION_HISTORY:
		if(!CWEBServer::GetInstance().IsConnected()){
			err += "Eib server is not connected";
		}

		if(!request.GetParameter(PARAM_NAME_FUNCTION,p1)){
			gen.Form_HistoryPerFunction(reply.GetContent(),err);
			return;
		}
		norm_param = URLEncoder::Decode(p1.GetValue());
		addr = CEibAddress(norm_param);
		GetHisotryFromEIB(db,err);
		if(!db.GetRecord(addr,rec)){
			err += "No Entries found for EIB Address: ";
			err += norm_param;
		}
		gen.Page_HistoryPerFunction(reply.GetContent(),addr,rec,err);
		break;
	case PARAM_COMMAND_VALUE_SEND_COMMAND:
		if(!CWEBServer::GetInstance().IsConnected()){
			err += "Eib server is not connected";
		}
		if(!request.GetParameter(PARAM_NAME_FUNCTION_DEST_ADDRESS,p1)) {
			gen.Form_SendCommandToEIB(reply.GetContent(),err);
			return;
		}
		if(!request.GetParameter(PARAM_NAME_FUNCTION_APCI,p2)){
			err += "Parameter \"";
			err += PARAM_NAME_FUNCTION_APCI;
			err += "\" is missing from request";
			gen.Form_SendCommandToEIB(reply.GetContent(),err);
			return;
		}
		if(!GetByteArrayFromHexString(p2.GetValue(),apci,apci_len)){
			gen.Form_SendCommandToEIB(reply.GetContent(),err);
			return;
		}
		if(!SendEIBCommand(p1.GetValue(),apci,apci_len,err)){
			gen.Page_AckCommandToEIB(reply.GetContent(), false, err);
			return;
		}
		gen.Page_AckCommandToEIB(reply.GetContent(), true, err);
		break;
	case PARAM_COMMAND_VALUE_SCHEDULE_COMMAND:
		if(!CWEBServer::GetInstance().IsConnected()){
			err += "Eib server is not connected";
		}

		if(!request.GetParameter(PARAM_NAME_FUNCTION_DEST_ADDRESS,p1)) {
			gen.Form_ScheduleCommand(reply.GetContent(),err);
			return;
		}
		if(!request.GetParameter(PARAM_NAME_FUNCTION_APCI,p2)){
			err += "Parameter \"";
			err += PARAM_NAME_FUNCTION_APCI;
			err += " missing from request";
			gen.Form_ScheduleCommand(reply.GetContent(),err);
			return;
		}
		if(!request.GetParameter(PARAM_NAME_FUNCTION_DATETIME,p3)){
			err += "Parameter \"";
			err += PARAM_NAME_FUNCTION_DATETIME;
			err += " missing from request";
			gen.Form_ScheduleCommand(reply.GetContent(),err);
			return;
		}

		if(!GetByteArrayFromHexString(p2.GetValue(),apci, apci_len)){
			err += "Illegal unsigned short representation";
			gen.Form_ScheduleCommand(reply.GetContent(),err);
			return;
		}
		norm_param = URLEncoder::Decode(p3.GetValue());
		datetime = CTime(norm_param.GetBuffer(),true);
		if(!CWEBServer::GetInstance().GetCollector()->AddScheduledCommand(datetime,
																	 CEibAddress(URLEncoder::Decode(p1.GetValue())),
																	 apci,
																	 apci_len,
																	 err)){
		    gen.Form_ScheduleCommand(reply.GetContent(),err);
			return;
		}
		reply.Redirect(CString("/?") + CString(PARAM_NAME_COMMAND) + CString("=") + CString(PARAM_COMMAND_VALUE_SCHEDULE_COMMAND));
		break;
	default:
		reply.Redirect("/");
		break;
	}

	return;

unauthorized:
	gen.Page_UnAuthorizedAction(reply.GetContent());
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

bool CWebHandler::SendEIBCommand(const CString& addr, unsigned char *apci, unsigned char apci_len, CString& err)
{
	START_TRY
		CEibAddress dest_addr(URLEncoder::Decode(addr));

		if(CWEBServer::GetInstance().SendEIBNetwork(dest_addr,apci,apci_len,NON_BLOCKING) == 0){
			err += "Eib server is not connected";
			return false;
		}
	END_TRY_START_CATCH(e)
		err += e.what();
		return false;
	END_CATCH

	return true;
}

void CWebHandler::GetHisotryFromEIB(CStatsDB& db, CString& err)
{
	START_TRY
		if(!CWEBServer::GetInstance().IsConnected()){
			err += "EIB Server is not Connected";
			return;
		}
		db = CWEBServer::GetInstance().GetCollector()->GetStatsDB();
	END_TRY_START_CATCH(e)
		err += e.what();
	END_CATCH
}

void CWebHandler::AddToJobQueue(TCPSocket* job)
{
	JTCSynchronized sync(*this);

	ASSERT_ERROR(job != NULL,"NULL Socket cannot be added to queue")
	_job_queue.push(job);
}

const CString& CWebHandler::GetCurrentDomain() const
{
	return CWEBServer::GetInstance().GetDomain();
}
