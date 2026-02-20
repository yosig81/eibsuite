// IntegrationHelpers.h -- Global test environment and HTTP test client
// for EIBServer integration tests.
//
// Includes EIBServer.h (server macros) and EmulatorWrapper.h (no macros).
// Safe to include in any integration test .cpp file.

#ifndef INTEGRATION_HELPERS_H
#define INTEGRATION_HELPERS_H

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>

#include "EIBServer.h"
#include "EmulatorWrapper.h"
#include "Socket.h"
#include "CString.h"

namespace IntegrationTest {

// ---------------------------------------------------------------------------
// IntegrationEnvironment -- started once, before all tests.
// ---------------------------------------------------------------------------
class IntegrationEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // 1. Init emulator (loads conf/Emulator.conf, conf/Emulator.db, binds UDP :3671)
        bool emu_ok = InitEmulator();
        ASSERT_TRUE(emu_ok) << "Emulator Init() failed";

        // 2. Suppress emulator console output -- keep logs in file only
        SuppressEmulatorScreen();

        // 3. Start emulator handler threads
        StartEmulator();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 4. Create and init the server singleton
        CEIBServer::Create();
        bool srv_ok = CEIBServer::GetInstance().Init();
        ASSERT_TRUE(srv_ok) << "CEIBServer::Init() failed";

        // 5. Suppress server console output -- keep logs in file only
        CEIBServer::GetInstance().GetLog().SetPrompt(false);

        // 6. Start server threads
        CEIBServer::GetInstance().Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    void TearDown() override {
        CEIBServer::GetInstance().Close();
        StopEmulator();
        // Intentionally skip delete/Destroy to avoid JTC static destruction crashes
    }
};

// NOTE: Do NOT register IntegrationEnvironment here (header included
// from multiple TUs).  Registration happens once in IntegrationMain.cpp.

// ---------------------------------------------------------------------------
// HttpTestClient -- sends raw HTTP requests to the server's web port.
// ---------------------------------------------------------------------------
struct HttpResponse {
    int status_code;
    CString body;
    CString set_cookie;   // raw Set-Cookie value, if present
};

class HttpTestClient {
public:
    explicit HttpTestClient(int port = 18080, const CString& host = "127.0.0.1")
        : _port(port), _host(host) {}

    // GET request, optionally with session cookie
    HttpResponse Get(const CString& uri, const CString& session_id = "") {
        CString req("GET ");
        req += uri;
        req += " HTTP/1.0\r\n";
        req += CString("Host: ") + _host + "\r\n";
        if (session_id.GetLength() > 0) {
            req += CString("Cookie: WEBSESSIONID=") + session_id + "\r\n";
        }
        req += "\r\n";
        return SendRequest(req);
    }

    // POST request with JSON body, optionally with session cookie
    HttpResponse Post(const CString& uri, const CString& body,
                      const CString& session_id = "") {
        CString req("POST ");
        req += uri;
        req += " HTTP/1.0\r\n";
        req += CString("Host: ") + _host + "\r\n";
        req += "Content-Type: application/json\r\n";
        req += CString("Content-Length: ") + CString(body.GetLength()) + "\r\n";
        if (session_id.GetLength() > 0) {
            req += CString("Cookie: WEBSESSIONID=") + session_id + "\r\n";
        }
        req += "\r\n";
        req += body;
        return SendRequest(req);
    }

    // Login helper -- returns session ID on success, empty string on failure
    CString Login(const CString& user, const CString& pass) {
        CString body("{\"user\":\"");
        body += user;
        body += "\",\"pass\":\"";
        body += pass;
        body += "\"}";
        HttpResponse resp = Post("/api/login", body);
        if (resp.status_code != 200) return "";
        return ExtractSessionId(resp.set_cookie);
    }

private:
    int _port;
    CString _host;

    HttpResponse SendRequest(const CString& raw_request) {
        HttpResponse resp;
        resp.status_code = 0;

        try {
            TCPSocket sock(_host, (unsigned short)_port);
            sock.Send(raw_request.GetBuffer(), raw_request.GetLength());

            // Read full response
            char buf[8192];
            CString full_response;
            int n;
            while ((n = sock.Recv(buf, sizeof(buf) - 1, 3000)) > 0) {
                buf[n] = '\0';
                full_response += CString(buf, n);
            }

            // Parse status line
            size_t sp1 = full_response.Find(" ");
            if (sp1 != string::npos) {
                size_t sp2 = full_response.Find(" ", sp1 + 1);
                if (sp2 != string::npos && sp2 > sp1) {
                    CString code_str = full_response.SubString(sp1 + 1, sp2 - sp1 - 1);
                    resp.status_code = code_str.ToInt();
                }
            }

            // Extract Set-Cookie header
            size_t cookie_pos = full_response.Find("Set-Cookie:");
            if (cookie_pos != string::npos) {
                size_t val_start = cookie_pos + 11;
                // skip whitespace
                while (val_start < (size_t)full_response.GetLength() && full_response[val_start] == ' ')
                    val_start++;
                size_t val_end = full_response.Find("\r\n", val_start);
                if (val_end == string::npos) val_end = full_response.GetLength();
                resp.set_cookie = full_response.SubString(val_start, val_end - val_start);
            }

            // Extract body (after \r\n\r\n)
            size_t body_start = full_response.Find("\r\n\r\n");
            if (body_start != string::npos) {
                body_start += 4;
                resp.body = full_response.SubString(body_start, full_response.GetLength() - body_start);
            }
        } catch (SocketException& e) {
            resp.status_code = -1;
            resp.body = e.what();
        }
        return resp;
    }

    CString ExtractSessionId(const CString& set_cookie) {
        // Format: WEBSESSIONID=<hex>;Path=/
        size_t eq = set_cookie.Find("WEBSESSIONID=");
        if (eq == string::npos) return "";
        size_t start = eq + 13; // length of "WEBSESSIONID="
        size_t end = set_cookie.Find(";", start);
        if (end == string::npos) end = set_cookie.GetLength();
        return set_cookie.SubString(start, end - start);
    }
};

} // namespace IntegrationTest

#endif // INTEGRATION_HELPERS_H
