#ifndef __DISPATCHER_HEADER__
#define __DISPATCHER_HEADER__

#include <httplib.h>
#include <thread>
#include <memory>
#include "CString.h"

class CDispatcher {
public:
	CDispatcher();
	~CDispatcher();

	void Init();   // creates SSLServer, registers all routes & mount points
	void Start();  // launches listen() in background thread
	void Close();  // calls server->stop(), joins thread

	int GetServerPort() const { return _port; }

private:
	void RegisterRoutes();

	std::unique_ptr<httplib::SSLServer> _server;
	std::thread _listen_thread;
	int _port;
};

#endif
