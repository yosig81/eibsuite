#include "EIBServer.h"
#include "Globals.h"
#include "cli.h"

CEIBServer* CEIBServer::_instance = NULL;

CEIBServer::CEIBServer():
CSingletonProcess(EIB_SERVER_PROCESS_NAME),
_clients_mgr(NULL),
_dispatcher(NULL),
_scheduler(NULL),
_interface(NULL)
{
	_interface = new CEIBInterface();
	_clients_mgr = new CClientsMgr();
	_dispatcher = new CDispatcher();
	_scheduler = new CCommandScheduler();
}

// Note: _dispatcher is now a raw pointer (not JTC handle) since
// CDispatcher no longer inherits from JTCThread.

CEIBServer::~CEIBServer()
{
	delete _dispatcher;
	delete _interface;
}

void CEIBServer::Close()
{
	if(_conf.GetLoadOK()) {
		LOG_INFO("Saving Configuration file...");
		_conf.Save(DEFAULT_CONF_FILE_NAME);
	}

	LOG_INFO("Saving Users database...");
	_users_db.Save();

	//close web interface
	LOG_INFO("Closing WEB Interface...");
	_dispatcher->Close();

	//close command scheduler
	LOG_INFO("Closing Command Scheduler...");
	_scheduler->Close();
	_scheduler->join();

	//close clients manager
	LOG_INFO("Closing Clients manager...");
	_clients_mgr->Close();
	_clients_mgr->join();

	LOG_INFO("Closing EIBNet/IP Device...");
	_interface->Close();

	CTime t;
	//indicate user
	LOG_INFO("EIB Server closed on %s",t.Format().GetBuffer());
}

CEIBServer& CEIBServer::GetInstance()
{
	return *CEIBServer::_instance;
}

void CEIBServer::Create()
{
	static bool created = false;
	if(!created){
		_instance = new CEIBServer();
		created = true;
	}
}

void CEIBServer::Start()
{
	_interface->Start();

	//start the clients manager
	_clients_mgr->start();
	//start web dispatcher
	_dispatcher->Start();
	//start command scheduler
	_scheduler->start();

	CTime t;
	CString time_str = t.Format();

	//indicate user
	LOG_INFO("EIB Server started on %s",time_str.GetBuffer());
}

bool CEIBServer::Init()
{
	bool res = true;
	START_TRY
		//initialize log file
		_log.SetPrinterMethod(printf);
		_log.Init(CURRENT_LOGS_FOLDER + CString(DEFAULT_LOG_FILE_NAME));
		LOG_INFO("Initializing Log manager...Successful.");
	END_TRY_START_CATCH(e)
		cerr << "Initializing Log manager...Failed: " << e.what() << endl;
		return false;
	END_CATCH

	START_TRY
		//load configuration from file
		_conf.Load(DEFAULT_CONF_FILE_NAME);
		_log.SetLogLevel((LogLevel)_conf.GetLogLevel());
		LOG_INFO("Reading Configuration file...Successful.");
	END_TRY_START_CATCH(e)
		LOG_ERROR("Reading Configuration file... Failed: %s", e.what());
		return false;
	END_CATCH

	START_TRY
		//initialize EIBNet/IP device connection
		_interface->Init();
		LOG_INFO("Initializing EIBNet/IP Device...Successful. (Mode: %s)", _interface->GetModeString().GetBuffer());
	END_TRY_START_CATCH(e)
		LOG_ERROR("Initializing EIBNet/IP Device...Failed: %s",e.what());
		return false;
	END_TRY_START_CATCH_SOCKET(ex)
		LOG_ERROR("Initializing EIBNet/IP Device...Failed: %s",ex.what());
		return false;
	END_CATCH

	START_TRY
		//initializing users db
		CString file_name(CURRENT_CONF_FOLDER);
		file_name += DEFAULT_USERS_DB_FILE;
		_users_db.Init(file_name);
		_users_db.Load();
		LOG_INFO("Loading Users database...Successful.");
	END_TRY_START_CATCH(e)
		LOG_ERROR("Loading Users database...Failed: %s",e.what());
		return false;
	END_CATCH

	START_TRY
		_users_db.Validate();
	END_TRY_START_CATCH(e)
		_log.SetConsoleColor(YELLOW);
		LOG_INFO("Warning: %s",e.what());
		_log.SetConsoleColor(WHITE);
	END_CATCH

	START_TRY
		//initialize clients manager module
		_clients_mgr->Init();
		LOG_INFO("Initializing Clients manager...Successful.");
		LOG_INFO("Listening for new clients on [%s:%d]", _clients_mgr->GetListeningAddress().GetBuffer(),_conf.GetListeningPort());
	END_TRY_START_CATCH(e)
		LOG_ERROR("Initializing Clients manager...Failed: %s",e.what());
		return false;
	END_TRY_START_CATCH_SOCKET(ex)
		LOG_ERROR("Initializing Clients manager...Failed: %s",ex.what());
		return false;
	END_CATCH

	START_TRY
		_dispatcher->Init();
		LOG_INFO("Initializing WEB Interface...Successful.");
		LOG_INFO("WEB Server: https://localhost:%d", _conf.GetWEBServerPort());
	END_TRY_START_CATCH(e)
		LOG_ERROR("Initializing WEB Interface...Failed: %s", e.what());
		// Non-fatal: EIBServer can run without web UI
	END_CATCH

	if (!CDirectory::IsExist(_conf.GetWwwRoot())) {
		_log.SetConsoleColor(YELLOW);
		LOG_INFO("Warning: WWW root directory \"%s\" not found. Web UI will not be available.", _conf.GetWwwRoot().GetBuffer());
		_log.SetConsoleColor(WHITE);
	}
	if (!CDirectory::IsExist(_conf.GetImagesFolder())) {
		_log.SetConsoleColor(YELLOW);
		LOG_INFO("Warning: Images directory \"%s\" not found.", _conf.GetImagesFolder().GetBuffer());
		_log.SetConsoleColor(WHITE);
	}

	return res;
}

void CEIBServer::ReloadConfiguration()
{
	START_TRY
		//load configuration from file
		_conf.Load(DEFAULT_CONF_FILE_NAME);
		_log.SetLogLevel((LogLevel)_conf.GetLogLevel());
		LOG_INFO("Reloading Configuration file...Successful.");
	END_TRY_START_CATCH(e)
		LOG_ERROR("Reloading Configuration file...Failed: %s",e.what());
	END_CATCH

	START_TRY
		//initializing users db
		CString file_name(CURRENT_CONF_FOLDER);
		file_name += DEFAULT_USERS_DB_FILE;
		_users_db.Init(file_name);
		_users_db.Clear();
		_users_db.Load();
		_users_db.Validate();
		LOG_INFO("Reloading Users database...Successful.");
	END_TRY_START_CATCH(e)
		LOG_ERROR("Reloading Users database...Failed: %s",e.what());
	END_CATCH
}

void CEIBServer::InteractiveConf()
{
	START_TRY
		_conf.Load(DEFAULT_CONF_FILE_NAME);
	END_TRY_START_CATCH_ANY
		_conf.Init();
	END_CATCH

	LOG_SCREEN("************************************\n");
	LOG_SCREEN("EIBServer Interactive configuration:\n");
	LOG_SCREEN("************************************\n");

	CString sval;
	int ival;
	bool bval;
	if(ConsoleCLI::GetCString("Initial Encryption/Decryption key?",sval, _conf.GetInitialKey())){
		_conf.SetInitialKey(sval);
	}
	if(ConsoleCLI::Getint("EIB Server listening port?",ival, _conf.GetListeningPort())){
		_conf.SetListeningPort(ival);
	}
	if(ConsoleCLI::Getint("Max concurrent clients connected to EIBServer?",ival, _conf.GetMaxConcurrentClients())){
		_conf.SetMaxConcurrentClients(ival);
	}
	map<int,CString> map1;
	map1.insert(map1.end(),pair<int,CString>(LOG_LEVEL_ERROR,"ERROR"));
	map1.insert(map1.end(),pair<int,CString>(LOG_LEVEL_INFO,"INFO"));
	map1.insert(map1.end(),pair<int,CString>(LOG_LEVEL_DEBUG,"DEBUG"));
	if(ConsoleCLI::GetStrOption("Program Logging Level?", map1, ival, _conf.GetLogLevel())){
		_conf.SetLogLevel(ival);
	}

	map<CString,CString> map2;
	map2.insert(map2.end(),pair<CString,CString>("MODE_ROUTING","MODE_ROUTING"));
	map2.insert(map2.end(),pair<CString,CString>("MODE_TUNNELING","MODE_TUNNELING"));
	map2.insert(map2.end(),pair<CString,CString>("MODE_BUSMONITOR","MODE_BUSMONITOR"));
	if(ConsoleCLI::GetStrOption("EIBNetIP Device working mode?", map2, sval, _conf.GetEibDeviceMode())){
		_conf.SetEibDeviceMode(sval);
	}

	if(ConsoleCLI::Getbool("Auto detect EIBNet/IP on local network?",bval,_conf.GetAutoDetectEibDeviceAddress())){
		_conf.SetAutoDetectEibDeviceAddress(bval);
		if(!bval){
			if(ConsoleCLI::GetCString("EIBNet/IP Device IP Address?",sval,_conf.GetEibDeviceAddress())){
				_conf.SetEibDeviceAddress(sval);
			}
		}
	}

#ifdef WIN32
	if(CUtils::EnumNics(map2) && ConsoleCLI::GetStrOption("Choose Interface to listen for new clients", map2, sval, CString(_conf.GetClientsListenInterface()))){
		_conf.SetClientsListenInterface(sval.ToInt());
	}
	if(ConsoleCLI::GetStrOption("Choose Interface to connect EIBNet/IP device", map2, sval, CString(_conf.GetEibLocalInterface()))){
		_conf.SetEibLocalInterface(sval.ToInt());
	}
#else
	if(CUtils::EnumNics(map2) && ConsoleCLI::GetStrOption("Choose Interface to listen for new clients", map2, sval, _conf.GetClientsListenInterface())){
		_conf.SetClientsListenInterface(sval);
	}
	if(ConsoleCLI::GetStrOption("Choose Interface to connect EIBNet/IP device", map2, sval, _conf.GetEibLocalInterface())){
		_conf.SetEibLocalInterface(sval);
	}
#endif

	// Web server configuration
	if(ConsoleCLI::Getint("WEB Server listening port?",ival, _conf.GetWEBServerPort())){
		_conf.SetWEBServerPort(ival);
	}
#ifdef WIN32
	if(ConsoleCLI::GetStrOption("Choose Interface to listen for web requests (Browser)", map2, sval, CString(_conf.GetWEBListenInterface()))){
		_conf.SetWEBListenInterface(sval.ToInt());
	}
#else
	if(ConsoleCLI::GetStrOption("Choose Interface to listen for web requests (Browser)", map2, sval, _conf.GetWEBListenInterface())){
		_conf.SetWEBListenInterface(sval);
	}
#endif
	if(ConsoleCLI::GetCString("WWW root directory?",sval, _conf.GetWwwRoot())){
		_conf.SetWwwRoot(sval);
	}
	if(ConsoleCLI::GetCString("Images folder?",sval, _conf.GetImagesFolder())){
		_conf.SetImagesFolder(sval);
	}
	// TLS certificate setup
	{
		map<CString,CString> tls_opts;
		tls_opts.insert(tls_opts.end(), pair<CString,CString>("auto","Auto-generate self-signed certificate"));
		tls_opts.insert(tls_opts.end(), pair<CString,CString>("manual","Provide certificate and key file paths"));
		CString tls_choice;
		// Default to "auto" unless the user has set custom (non-default) paths
		CString tls_def = (_conf.GetTLSCertFile() == "./conf/server.crt") ? "auto" : "manual";
		if(ConsoleCLI::GetStrOption("TLS certificate setup?", tls_opts, tls_choice, tls_def)){
			if(tls_choice == "auto"){
				CString cert_path = CURRENT_CONF_FOLDER + "server.crt";
				CString key_path  = CURRENT_CONF_FOLDER + "server.key";
				CString cmd = "openssl req -x509 -newkey rsa:2048 -keyout ";
				cmd += key_path;
				cmd += " -out ";
				cmd += cert_path;
				cmd += " -days 3650 -nodes -subj \"/CN=EIBServer\" 2>&1";
				LOG_SCREEN("Generating self-signed certificate...\n");
				int ret = system(cmd.GetBuffer());
				if(ret != 0){
					LOG_SCREEN("ERROR: openssl command failed (is openssl installed?)\n");
				}else{
					_conf.SetTLSCertFile(cert_path);
					_conf.SetTLSKeyFile(key_path);
					LOG_SCREEN("Certificate: %s\n", cert_path.GetBuffer());
					LOG_SCREEN("Private key: %s\n", key_path.GetBuffer());
				}
			}else{
				if(ConsoleCLI::GetCString("TLS certificate file (PEM)?",sval, _conf.GetTLSCertFile())){
					_conf.SetTLSCertFile(sval);
				}
				if(ConsoleCLI::GetCString("TLS private key file (PEM)?",sval, _conf.GetTLSKeyFile())){
					_conf.SetTLSKeyFile(sval);
				}
			}
		}
	}

	_conf.SetLoadOK(true);
	LOG_SCREEN("Saving configuration to %s...", DEFAULT_CONF_FILE_NAME);
	if(!_conf.Save(DEFAULT_CONF_FILE_NAME)){
		throw CEIBException(FileError, "Cannot save configuration to file \"%s\"",DEFAULT_CONF_FILE_NAME);
	}
	LOG_SCREEN(" [OK]\n");
	_log.SetConsoleColor(GREEN);
	LOG_SCREEN("\nNow you can run EIB Server. the new file will be loaded automatically.");
	_log.SetConsoleColor(WHITE);
	LOG_SCREEN("\n\n");
}
