#include "Dispatcher.h"
#include "EIBServer.h"
#include "WebHandler.h"

CDispatcher::CDispatcher() : _port(0)
{
}

CDispatcher::~CDispatcher()
{
}

void CDispatcher::Init()
{
	CServerConfig& conf = CEIBServer::GetInstance().GetConfig();
	_port = conf.GetWEBServerPort();

	const CString& cert = conf.GetTLSCertFile();
	const CString& key  = conf.GetTLSKeyFile();

	if (cert.GetLength() == 0 || key.GetLength() == 0) {
		throw CEIBException(GeneralError,
			"TLS_CERT_FILE and TLS_KEY_FILE must be set in EIB.conf");
	}

	try {
		_server = std::make_unique<httplib::SSLServer>(
			cert.GetBuffer(), key.GetBuffer());
	} catch (const std::exception& e) {
		throw CEIBException(GeneralError,
			"Failed to create HTTPS server: %s", e.what());
	}

	if (!_server->is_valid()) {
		throw CEIBException(GeneralError,
			"HTTPS server failed to initialize (check cert/key paths)");
	}

	RegisterRoutes();

	// Static file mount points
	CString www_root = conf.GetWwwRoot();
	CString images_folder = conf.GetImagesFolder();

	if (www_root.GetLength() > 0) {
		_server->set_mount_point("/", www_root.GetBuffer());
	}
	if (images_folder.GetLength() > 0) {
		_server->set_mount_point("/Images", images_folder.GetBuffer());
	}

	// Error handler: JSON 404 for API paths, SPA fallback for everything else
	CString index_path = www_root + "/index.html";
	_server->set_error_handler([index_path](const httplib::Request& req, httplib::Response& res) {
		if (res.status == 404) {
			if (req.path.find("/api/") == 0) {
				// API path: return JSON error
				res.set_content("{\"error\":\"Unknown API endpoint\"}", "application/json");
			} else {
				// SPA fallback: serve index.html
				std::ifstream f(index_path.GetBuffer(), std::ios::binary);
				if (f.is_open()) {
					std::string body((std::istreambuf_iterator<char>(f)),
									 std::istreambuf_iterator<char>());
					res.set_content(body, "text/html");
					res.status = 200;
				}
			}
		}
	});
}

void CDispatcher::RegisterRoutes()
{
	CWebHandler::RegisterRoutes(*_server);
}

void CDispatcher::Start()
{
	if (!_server) {
		LOG_ERROR("[Dispatcher] No HTTPS server -- web interface disabled.");
		return;
	}
	_listen_thread = std::thread([this]() {
		_server->listen("0.0.0.0", _port);
	});
}

void CDispatcher::Close()
{
	if (_server) {
		_server->stop();
	}
	if (_listen_thread.joinable()) {
		_listen_thread.join();
	}
}
