#ifndef __CONSOLE_CLIENT_HEADER__
#define __CONSOLE_CLIENT_HEADER__

#include "CString.h"
#include "JTC.h"
#include "Socket.h"
#include "Protocol.h"
#include "DataBuffer.h"

class CConsoleClient : public JTCMonitor
{
public:
	CConsoleClient();
	virtual ~CConsoleClient();

	bool Login(const CString& host, int port, const CString& user, const CString& pass);
	bool IsLoggedIn();
	void Logout();
	bool KeepAlive();

	CString SendGet(const CString& uri);
	CString SendPost(const CString& uri, const CString& body, bool form_encoded = false);

private:
	CString DoRequest(const CString& method, const CString& uri, const CString& body, bool form_encoded);
	CString ParseResponseBody(const char* response, int len, CString& out_session);

private:
	CString _host;
	int _port;
	CString _session_id;
	bool _logged_in;
};

#endif
