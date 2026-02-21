#ifndef __WEB_HANDLER_HEADER__
#define __WEB_HANDLER_HEADER__

#include <map>
#include <mutex>
#include <httplib.h>
#include "CString.h"
#include "UsersDB.h"
#include "XmlJsonUtil.h"

#ifndef MAX_EIB_VALUE_LEN
#define MAX_EIB_VALUE_LEN 16
#endif

// Session entry for cookie-based auth
struct WebSession {
	CString user_name;
	CString session_id;
	time_t created;
	time_t last_access;
};

class CWebHandler {
public:
	// Route registration (called from CDispatcher::RegisterRoutes)
	static void RegisterRoutes(httplib::SSLServer& server);

private:
	// Session endpoints
	static void ApiLogin(const httplib::Request& req, httplib::Response& res);
	static void ApiLogout(const httplib::Request& req, httplib::Response& res);
	static void ApiSessionCheck(const httplib::Request& req, httplib::Response& res);

	// Data endpoints
	static void ApiGetGlobalHistory(const httplib::Request& req, httplib::Response& res);
	static void ApiGetAddressHistory(const httplib::Request& req, httplib::Response& res);
	static void ApiSendEibCommand(const httplib::Request& req, httplib::Response& res);
	static void ApiScheduleCommand(const httplib::Request& req, httplib::Response& res);

	// Admin endpoints
	static void ApiGetUsers(const httplib::Request& req, httplib::Response& res);
	static void ApiSetUsers(const httplib::Request& req, httplib::Response& res);
	static void ApiGetInterface(const httplib::Request& req, httplib::Response& res);
	static void ApiInterfaceStart(const httplib::Request& req, httplib::Response& res);
	static void ApiInterfaceStop(const httplib::Request& req, httplib::Response& res);
	static void ApiGetBusMonAddresses(const httplib::Request& req, httplib::Response& res);
	static void ApiBusMonSendCmd(const httplib::Request& req, httplib::Response& res);

	// Helpers
	static bool Authenticate(const httplib::Request& req, CUser& user);
	static CString GetSessionCookie(const httplib::Request& req);
	static CString GenerateSessionId();
	static CString GetJsonField(const CString& json, const CString& field);

	static void SetJsonResponse(httplib::Response& res, const CString& json, int status = 200);
	static void SetJsonError(httplib::Response& res, const CString& message, int status = 500);

	// Existing business logic helpers
	static bool SendEIBCommand(const CString& addr, unsigned char* apci,
							   unsigned char apci_len, CString& err);
	static bool GetByteArrayFromHexString(const CString& str, unsigned char* val,
										  unsigned char& val_len);
	static unsigned char HexToChar(const CString& hexNumber);
	static int GetDigitValue(char digit);
	static CString GetMimeType(const CString& file_path);

	friend class WebHandlerUtilTest;

	static std::map<CString, WebSession> _sessions;
	static std::mutex _session_mutex;
};

#endif
