#ifndef __WEB_HANDLER_HEADER__
#define __WEB_HANDLER_HEADER__

#include <queue>
#include "CString.h"
#include "JTC.h"
#include "CMutex.h"
#include "Socket.h"
#include "HttpParser.h"
#include "Digest.h"
#include "StatsDB.h"
#include "UsersDB.h"
#include "XmlJsonUtil.h"
#include "Html.h"

#define MAX_HTTP_REQUEST_SIZE 8192

// Session entry for cookie-based auth
struct WebSession {
	CString user_name;
	CString session_id;
	time_t created;
	time_t last_access;
};

class CWebHandler : public JTCThread, public JTCMonitor
{
public:
	CWebHandler();
	virtual ~CWebHandler();

	virtual void run();
	void AddToJobQueue(TCPSocket* job);
	void Close();
	void Signal();

private:
	void HandleRequest(TCPSocket* sock, char* buffer,CHttpReply& reply);
	void InitReply(CHttpReply& reply);

	bool SendEIBCommand(const CString& addr, unsigned char *apci, unsigned char apci_len, CString& err);
	bool GetByteArrayFromHexString(const CString& str, unsigned char *val, unsigned char &val_len);

	void HandleFavoritsIconRequest(CHttpReply& reply);
	void HandleImageRequest(const CString& file_name, CHttpReply& reply);
	void FillRawFile(const CString& file_name, CDataBuffer& buf, int& total_size);

	// --- API & Static File Serving ---
	bool HandleApiRequest(CHttpRequest& request, CHttpReply& reply);
	bool HandleStaticFile(const CString& path, CHttpReply& reply);
	void SetJsonResponse(CHttpReply& reply, const CString& json, HTTP_STATUS_CODE status = STATUS_OK);
	void SetJsonError(CHttpReply& reply, const CString& message, HTTP_STATUS_CODE status = STATUS_INTERNAL_ERROR);

	// API: Session management
	void ApiLogin(CHttpRequest& request, CHttpReply& reply);
	void ApiLogout(CHttpRequest& request, CHttpReply& reply);
	void ApiSessionCheck(CHttpRequest& request, CHttpReply& reply);
	bool ApiAuthenticate(CHttpRequest& request, CUser& user);

	// API: Admin endpoints (direct access to conf classes)
	void ApiGetUsers(CHttpReply& reply);
	void ApiSetUsers(CHttpRequest& request, CHttpReply& reply);
	void ApiGetInterface(CHttpReply& reply);
	void ApiInterfaceStart(CHttpReply& reply);
	void ApiInterfaceStop(CHttpReply& reply);
	void ApiGetBusMonAddresses(CHttpReply& reply);
	void ApiBusMonSendCmd(CHttpRequest& request, CHttpReply& reply);

	// API: Data endpoints (direct StatsDB access)
	void ApiGetGlobalHistory(CHttpReply& reply);
	void ApiGetFunctionHistory(const CString& address, CHttpReply& reply);
	void ApiSendEibCommand(CHttpRequest& request, CHttpReply& reply);
	void ApiScheduleCommand(CHttpRequest& request, CHttpReply& reply);

	// Session helpers
	CString GenerateSessionId();
	CString GetSessionCookie(CHttpRequest& request);

	// MIME type helper
	CString GetMimeType(const CString& file_path);

	// Extract JSON field from body
	CString GetJsonField(const CString& json, const CString& field);

private:
	queue<TCPSocket*> _job_queue;
	bool _stop;

	unsigned char HexToChar(const CString& hexNumber);
	int	GetDigitValue (char digit);

	// Web sessions (cookie-based auth for SPA)
	static map<CString, WebSession> _sessions;
	static JTCMutex _session_mutex;
};

#endif
