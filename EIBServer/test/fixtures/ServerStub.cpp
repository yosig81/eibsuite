// ServerStub.cpp -- Minimal stubs for CEIBServer and related classes.
//
// When compiling EIBServer source files for unit testing we deliberately
// exclude Main.cpp and EIBServer.cpp to avoid pulling in the full
// dependency graph.  However, several .cpp files reference
// CEIBServer::GetInstance() (through LOG macros or direct calls), the
// conf helper classes, and CEIBInterface / CEIBHandler.  This file
// provides just enough linker symbols to satisfy those references.
//
// IMPORTANT: None of the methods below are expected to execute during
// normal test runs.  If a test accidentally hits a code path that calls
// GetInstance(), it will throw so the failure is obvious.

#include "EIBServer.h"
#include "conf/EIBServerUsersConf.h"
#include "conf/EIBInterfaceConf.h"
#include "conf/EIBBusMonConf.h"

// ---------------------------------------------------------------------------
// CEIBServer stubs
// ---------------------------------------------------------------------------

CEIBServer* CEIBServer::_instance = NULL;

CEIBServer::CEIBServer():
    CSingletonProcess(EIB_SERVER_PROCESS_NAME),
    _clients_mgr(NULL),
    _dispatcher(NULL),
    _scheduler(NULL),
    _interface(NULL)
{
}

CEIBServer::~CEIBServer()
{
    // _interface is NULL in the stub, so nothing to delete.
}

CEIBServer& CEIBServer::GetInstance()
{
    if (_instance == NULL) {
        throw CEIBException(GeneralError,
            "CEIBServer::GetInstance() called in test context -- "
            "this code path should not be reached by unit tests");
    }
    return *_instance;
}

void CEIBServer::Create() {}

bool CEIBServer::Init() { return false; }

void CEIBServer::Close() {}

void CEIBServer::Start() {}

void CEIBServer::ReloadConfiguration() {}

void CEIBServer::InteractiveConf() {}

// ---------------------------------------------------------------------------
// CEIBInterface stubs
// ---------------------------------------------------------------------------

CEIBInterface::CEIBInterface():
    _mode(UNDEFINED_MODE),
    _connection(NULL)
{
}

CEIBInterface::~CEIBInterface() {}

void CEIBInterface::Init() {}
void CEIBInterface::Close() {}
void CEIBInterface::Start() {}
bool CEIBInterface::Read(CCemi_L_Data_Frame&) { return false; }
void CEIBInterface::Write(const KnxElementQueue&) {}
void CEIBInterface::SetDefaultPacketFields(CCemi_L_Data_Frame&) {}
IConnection* CEIBInterface::GetConnection() { return NULL; }
CString CEIBInterface::GetModeString() { return "STUB"; }

// ---------------------------------------------------------------------------
// CEIBHandler stubs
// ---------------------------------------------------------------------------

CEIBHandler::CEIBHandler(HANDLER_TYPE type):
    _type(type),
    _stop(false),
    _pause(false)
{
}

CEIBHandler::~CEIBHandler() {}

void CEIBHandler::Write(const CCemi_L_Data_Frame&, BlockingMode, JTCMonitor*) {}
void CEIBHandler::run() {}
void CEIBHandler::Close() {}
void CEIBHandler::Suspend() {}
void CEIBHandler::Resume() {}

// ---------------------------------------------------------------------------
// Conf class stubs (used by WebHandler API methods)
// ---------------------------------------------------------------------------

CEIBServerUsersConf::CEIBServerUsersConf() {}
CEIBServerUsersConf::~CEIBServerUsersConf() {}
void CEIBServerUsersConf::ToXml(CDataBuffer&) {}
void CEIBServerUsersConf::FromXml(const CDataBuffer&) {}
void CEIBServerUsersConf::GetConnectedClients() {}
void CEIBServerUsersConf::SetConnectedClients() {}

CEIBInterfaceConf::CEIBInterfaceConf():
    _interface_port(0),
    _device_mode(UNDEFINED_MODE),
    _auto_detect(false),
    _total_sent(0),
    _total_received(0)
{
}
CEIBInterfaceConf::~CEIBInterfaceConf() {}
void CEIBInterfaceConf::ToXml(CDataBuffer&) {}
void CEIBInterfaceConf::FromXml(const CDataBuffer&) {}
bool CEIBInterfaceConf::StartInterface(CDataBuffer&) { return false; }
bool CEIBInterfaceConf::StopInterface(CDataBuffer&) { return false; }

CEIBBusMonAddrListConf::CEIBBusMonAddrListConf() {}
CEIBBusMonAddrListConf::~CEIBBusMonAddrListConf() {}
void CEIBBusMonAddrListConf::ToXml(CDataBuffer&) {}
void CEIBBusMonAddrListConf::FromXml(const CDataBuffer&) {}
bool CEIBBusMonAddrListConf::SendCmdToAddr(const CHttpRequest&) { return false; }
