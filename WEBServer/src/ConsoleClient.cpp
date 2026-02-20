#include "ConsoleClient.h"
#include "WEBServer.h"

CConsoleClient::CConsoleClient() :
_port(0),
_logged_in(false)
{
}

CConsoleClient::~CConsoleClient()
{
}

bool CConsoleClient::Login(const CString& host, int port, const CString& user, const CString& pass)
{
	JTCSynchronized sync(*this);

	_host = host;
	_port = port;
	_logged_in = false;
	_session_id = EMPTY_STRING;

	START_TRY
		TCPSocket sock(_host, (unsigned short)_port);

		CString request;
		request += "GET /";
		request += EIB_CLIENT_AUTHENTICATE;
		request += " HTTP/1.0\r\n";
		request += "Host: ";
		request += _host;
		request += "\r\n";
		request += USER_NAME_HEADER;
		request += ": ";
		request += user;
		request += "\r\n";
		request += PASSWORD_HEADER;
		request += ": ";
		request += pass;
		request += "\r\n";
		request += "\r\n";

		sock.Send(request.GetBuffer(), request.GetLength());

		char buf[4096];
		int len = sock.Recv(buf, sizeof(buf) - 1, 5000);
		if (len <= 0) {
			return false;
		}
		buf[len] = '\0';

		CString new_session;
		CString body = ParseResponseBody(buf, len, new_session);

		if (body.Find(CString(EIB_CLIENT_AUTHENTICATE_SUCCESS)) != string::npos) {
			if (new_session.GetLength() > 0) {
				_session_id = new_session;
			}
			_logged_in = true;
			LOG_INFO("[ConsoleClient] Logged in to ConsoleManager at %s:%d", _host.GetBuffer(), _port);
			return true;
		}

		LOG_ERROR("[ConsoleClient] Login failed: %s", body.GetBuffer());
	END_TRY_START_CATCH(e)
		LOG_ERROR("[ConsoleClient] Login exception: %s", e.what());
	END_TRY_START_CATCH_SOCKET(e)
		LOG_ERROR("[ConsoleClient] Login socket error: %s", e.what());
	END_TRY_START_CATCH_ANY
		LOG_ERROR("[ConsoleClient] Login unknown exception");
	END_CATCH

	return false;
}

bool CConsoleClient::IsLoggedIn()
{
	JTCSynchronized sync(*this);
	return _logged_in;
}

void CConsoleClient::Logout()
{
	JTCSynchronized sync(*this);
	if (!_logged_in) return;

	START_TRY
		SendGet(EIB_CLIENT_DISCONNECT);
	END_TRY_START_CATCH_ANY
	END_CATCH

	_logged_in = false;
	_session_id = EMPTY_STRING;
	LOG_INFO("[ConsoleClient] Logged out from ConsoleManager");
}

bool CConsoleClient::KeepAlive()
{
	JTCSynchronized sync(*this);
	if (!_logged_in) return false;

	START_TRY
		CString body = SendGet("EIB_CONSOLE_KEEP_ALIVE_CMD");
		if (body == "OK") {
			return true;
		}
	END_TRY_START_CATCH_ANY
	END_CATCH

	_logged_in = false;
	return false;
}

CString CConsoleClient::SendGet(const CString& uri)
{
	return DoRequest("GET", uri, EMPTY_STRING, false);
}

CString CConsoleClient::SendPost(const CString& uri, const CString& body, bool form_encoded)
{
	return DoRequest("POST", uri, body, form_encoded);
}

CString CConsoleClient::DoRequest(const CString& method, const CString& uri, const CString& body, bool form_encoded)
{
	JTCSynchronized sync(*this);

	if (!_logged_in && uri != CString(EIB_CLIENT_AUTHENTICATE)) {
		return EMPTY_STRING;
	}

	START_TRY
		TCPSocket sock(_host, (unsigned short)_port);

		CString request;
		request += method;
		request += " /";
		request += uri;
		request += " HTTP/1.0\r\n";
		request += "Host: ";
		request += _host;
		request += "\r\n";

		if (_session_id.GetLength() > 0) {
			request += "Cookie: ";
			request += EIB_SESSION_ID_COOKIE_NAME;
			request += "=";
			request += _session_id;
			request += "\r\n";
		}

		if (body.GetLength() > 0) {
			if (form_encoded) {
				request += "Content-Type: application/x-www-form-urlencoded\r\n";
			}
			request += "Content-Length: ";
			request += CString(body.GetLength());
			request += "\r\n";
		}

		request += "\r\n";

		if (body.GetLength() > 0) {
			request += body;
		}

		sock.Send(request.GetBuffer(), request.GetLength());

		char buf[8192];
		int total = 0;
		int len;

		// Read response - may come in multiple chunks
		while (total < (int)sizeof(buf) - 1) {
			len = sock.Recv(buf + total, sizeof(buf) - 1 - total, 5000);
			if (len <= 0) break;
			total += len;
		}

		if (total <= 0) {
			return EMPTY_STRING;
		}
		buf[total] = '\0';

		CString new_session;
		CString response_body = ParseResponseBody(buf, total, new_session);

		if (new_session.GetLength() > 0) {
			_session_id = new_session;
		}

		return response_body;

	END_TRY_START_CATCH(e)
		LOG_ERROR("[ConsoleClient] Request exception: %s", e.what());
	END_TRY_START_CATCH_SOCKET(e)
		LOG_ERROR("[ConsoleClient] Socket error: %s", e.what());
	END_TRY_START_CATCH_ANY
		LOG_ERROR("[ConsoleClient] Unknown exception in request");
	END_CATCH

	return EMPTY_STRING;
}

CString CConsoleClient::ParseResponseBody(const char* response, int len, CString& out_session)
{
	out_session = EMPTY_STRING;

	// Find the end of headers (double CRLF)
	const char* body_start = strstr(response, "\r\n\r\n");
	if (body_start == NULL) {
		return EMPTY_STRING;
	}

	// Search for Set-Cookie header with EIBSESSIONID
	const char* cookie_hdr = strstr(response, "Set-Cookie:");
	if (cookie_hdr != NULL && cookie_hdr < body_start) {
		const char* sid_start = strstr(cookie_hdr, EIB_SESSION_ID_COOKIE_NAME);
		if (sid_start != NULL && sid_start < body_start) {
			sid_start += strlen(EIB_SESSION_ID_COOKIE_NAME) + 1; // skip "EIBSESSIONID="
			const char* sid_end = sid_start;
			while (*sid_end != '\0' && *sid_end != ';' && *sid_end != '\r' && *sid_end != '\n') {
				sid_end++;
			}
			if (sid_end > sid_start) {
				out_session = CString(sid_start, sid_end - sid_start);
			}
		}
	}

	body_start += 4; // skip "\r\n\r\n"
	int body_len = len - (body_start - response);
	if (body_len <= 0) {
		return EMPTY_STRING;
	}

	return CString(body_start, body_len);
}
