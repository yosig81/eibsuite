// IntegrationHelpers.h -- Global test environment and HTTP test client
// for EIBServer integration tests.

#ifndef INTEGRATION_HELPERS_H
#define INTEGRATION_HELPERS_H

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <httplib.h>

#include "EIBServer.h"
#include "EmulatorWrapper.h"
#include "CString.h"

namespace IntegrationTest {

// ---------------------------------------------------------------------------
// IntegrationEnvironment -- started once, before all tests.
// ---------------------------------------------------------------------------
class IntegrationEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // 0. Save a copy of Users.db so we can restore it after server shuts down
        //    (server Close() saves its in-memory state to disk).
        std::ifstream db_in("conf/Users.db");
        if (db_in.is_open()) {
            _original_users_db.assign(
                (std::istreambuf_iterator<char>(db_in)),
                 std::istreambuf_iterator<char>());
        }

        // 1. Init emulator (loads conf/Emulator.conf, conf/Emulator.db, binds UDP :3671)
        _emu_ok = InitEmulator();
        ASSERT_TRUE(_emu_ok) << "Emulator Init() failed";

        // 2. Suppress emulator console output -- keep logs in file only
        SuppressEmulatorScreen();

        // 3. Start emulator handler threads
        StartEmulator();

        // 4. Create and init the server singleton
        CEIBServer::Create();
        _srv_ok = CEIBServer::GetInstance().Init();
        ASSERT_TRUE(_srv_ok) << "CEIBServer::Init() failed";

        // 5. Suppress server console output -- keep logs in file only
        CEIBServer::GetInstance().GetLog().SetPrompt(false);

        // 6. Start server threads
        CEIBServer::GetInstance().Start();

        // 7. Wait until the HTTPS server is actually accepting connections
        WaitForWebServer(CEIBServer::GetInstance().GetConfig().GetWEBServerPort());
    }

private:
    std::string _original_users_db;
    bool _emu_ok = false;
    bool _srv_ok = false;

    void WaitForWebServer(int port, int max_attempts = 50) {
        for (int i = 0; i < max_attempts; ++i) {
            try {
                httplib::SSLClient probe("127.0.0.1", port);
                probe.enable_server_certificate_verification(false);
                probe.set_connection_timeout(0, 500000); // 500ms
                auto result = probe.Get("/api/session");
                if (result) return; // connected -- server is ready
            } catch (...) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        FAIL() << "Web server did not start on port " << port;
    }

public:

    void TearDown() override {
        if (_srv_ok) {
            CEIBServer::GetInstance().Close();
        }
        if (_emu_ok) {
            StopEmulator();
        }
        // Restore original Users.db after server Close() has saved its state.
        if (!_original_users_db.empty()) {
            std::ofstream db_out("conf/Users.db", std::ios::trunc);
            db_out << _original_users_db;
        }
    }
};

// NOTE: Do NOT register IntegrationEnvironment here (header included
// from multiple TUs).  Registration happens once in IntegrationMain.cpp.

// ---------------------------------------------------------------------------
// HttpTestClient -- sends HTTPS requests to the server's web port.
// ---------------------------------------------------------------------------
struct HttpResponse {
    int status_code;
    CString body;
    CString set_cookie;    // raw Set-Cookie value, if present
    CString content_type;  // Content-Type header value
};

class HttpTestClient {
public:
    explicit HttpTestClient(int port = 18080, const CString& host = "127.0.0.1")
        : _cli(host.GetBuffer(), port)
    {
        _cli.enable_server_certificate_verification(false);
        _cli.set_connection_timeout(5, 0); // 5 seconds
        _cli.set_read_timeout(5, 0);
    }

    // GET request, optionally with session cookie
    HttpResponse Get(const CString& uri, const CString& session_id = "") {
        httplib::Headers headers;
        if (session_id.GetLength() > 0) {
            headers.emplace("Cookie",
                std::string("WEBSESSIONID=") + session_id.GetBuffer());
        }
        auto result = _cli.Get(uri.GetBuffer(), headers);
        return ToResponse(result);
    }

    // POST request with JSON body, optionally with session cookie
    HttpResponse Post(const CString& uri, const CString& body,
                      const CString& session_id = "") {
        return PostWithContentType(uri, body, "application/json", session_id);
    }

    // POST request with XML body, optionally with session cookie
    HttpResponse PostXml(const CString& uri, const CString& body,
                         const CString& session_id = "") {
        return PostWithContentType(uri, body, "text/xml", session_id);
    }

    // POST request with custom content type
    HttpResponse PostWithContentType(const CString& uri, const CString& body,
                                     const CString& content_type,
                                     const CString& session_id = "") {
        httplib::Headers headers;
        if (session_id.GetLength() > 0) {
            headers.emplace("Cookie",
                std::string("WEBSESSIONID=") + session_id.GetBuffer());
        }
        auto result = _cli.Post(uri.GetBuffer(), headers,
            std::string(body.GetBuffer(), body.GetLength()),
            content_type.GetBuffer());
        return ToResponse(result);
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
    httplib::SSLClient _cli;

    HttpResponse ToResponse(const httplib::Result& result) {
        HttpResponse resp;
        resp.status_code = 0;

        if (!result) {
            resp.status_code = -1;
            resp.body = "Connection failed";
            return resp;
        }

        resp.status_code = result->status;
        resp.body = CString(result->body.c_str(), (int)result->body.length());

        // Extract Set-Cookie header
        if (result->has_header("Set-Cookie")) {
            resp.set_cookie = CString(result->get_header_value("Set-Cookie").c_str());
        }

        // Extract Content-Type header
        if (result->has_header("Content-Type")) {
            resp.content_type = CString(result->get_header_value("Content-Type").c_str());
        }

        return resp;
    }

    CString ExtractSessionId(const CString& set_cookie) {
        // Format: WEBSESSIONID=<hex>; Path=/
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
