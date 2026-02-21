# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------
add_executable(eibserver_tests
    unit/CommandSchedulerTest.cpp
    unit/PacketFilterTest.cpp
    unit/UsersDBTest.cpp
    unit/UserTest.cpp
    unit/WebHandlerUtilTest.cpp
    unit/XmlJsonUtilTest.cpp
    # Server sources under test (no Main.cpp, no EIBServer.cpp)
    ../src/XmlJsonUtil.cpp
    ../src/UsersDB.cpp
    ../src/PacketFilter.cpp
    ../src/WebHandler.cpp
    ../src/CommandScheduler.cpp
    ../src/ServerConfig.cpp
    # Stub for linker resolution
    fixtures/ServerStub.cpp
)

target_include_directories(eibserver_tests PRIVATE
    ../include
    ../include/conf
    fixtures
)

set_target_properties(eibserver_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(eibserver_tests PRIVATE EIBStdLib GTest::gtest GTest::gtest_main)

gtest_discover_tests(eibserver_tests)

# ---------------------------------------------------------------------------
# Integration tests
# ---------------------------------------------------------------------------
if(BUILD_EMULATOR)
    add_executable(eibserver_integration_tests
        integration/BusMonitorTest.cpp
        integration/DispatcherNullGuardTest.cpp
        integration/EibCommunicationTest.cpp
        integration/GenerateIndicationsTest.cpp
        integration/IntegrationMain.cpp
        integration/PacketFilterIntegrationTest.cpp
        integration/ServerLifecycleTest.cpp
        integration/WebApiAdminTest.cpp
        integration/WebApiDataTest.cpp
        integration/WebApiSessionTest.cpp
        # All server sources (except Main.cpp)
        ../src/BusMonConnection.cpp
        ../src/Client.cpp
        ../src/ClientsMgr.cpp
        ../src/CommandScheduler.cpp
        ../src/Dispatcher.cpp
        ../src/EIBHandler.cpp
        ../src/EIBInterface.cpp
        ../src/EIBServer.cpp
        ../src/PacketFilter.cpp
        ../src/RoutingConnection.cpp
        ../src/ServerConfig.cpp
        ../src/TunnelConnection.cpp
        ../src/UsersDB.cpp
        ../src/WebHandler.cpp
        ../src/XmlJsonUtil.cpp
        ../src/conf/EIBBusMonConf.cpp
        ../src/conf/EIBInterfaceConf.cpp
        ../src/conf/EIBServerUsersConf.cpp
        # Emulator sources (core only)
        ../../Emulator-ng/src/Emulator-ng.cpp
        ../../Emulator-ng/src/EmulatorHandler.cpp
        ../../Emulator-ng/src/EmulatorConfig.cpp
        ../../Emulator-ng/src/EmulatorDB.cpp
        # Fixture
        fixtures/EmulatorWrapper.cpp
    )

    target_include_directories(eibserver_integration_tests PRIVATE
        ../include
        ../include/conf
        ../../Emulator-ng/include
        fixtures
    )

    set_target_properties(eibserver_integration_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(eibserver_integration_tests PRIVATE EIBStdLib GTest::gtest)

    # -------------------------------------------------------------------
    # Working directory setup for integration tests
    # -------------------------------------------------------------------
    set(INTEG_WORKDIR /tmp/eibtest)

    # Detect loopback interface name
    if(APPLE)
        set(LOOPBACK_IF lo0)
    else()
        set(LOOPBACK_IF lo)
    endif()

    # Generate config files at configure time
    file(MAKE_DIRECTORY ${INTEG_WORKDIR}/conf)
    file(MAKE_DIRECTORY ${INTEG_WORKDIR}/logs)
    file(MAKE_DIRECTORY ${INTEG_WORKDIR}/www)
    file(MAKE_DIRECTORY ${INTEG_WORKDIR}/Images)

    file(WRITE ${INTEG_WORKDIR}/conf/EIB.conf
"[EIB-GENERAL]
EIB_INITIAL_KEY = EIBKEY
LISTENING_PORT = 15000
MAX_CONCURRENT_CLIENTS = 10
LOG_LEVEL = 3
LOG_FILE_MAX_SIZE = 512
MAX_NUM_OBJECTS_HISTORY = 100
EIB_DEVICE_MODE = MODE_TUNNELING
EIB_IP_ADDRESS = 127.0.0.1
AUTO_DETECT_EIB_DEVICE_ADDRESS = false
EIB_LOCAL_INTERFACE = ${LOOPBACK_IF}
CLIENTS_LISTEN_INTERFACE = ${LOOPBACK_IF}
WEB_SERVER_PORT = 18080
WWW_ROOT = ./www
IMAGES_FOLDER = ./Images
WEB_LISTEN_INTERFACE = ${LOOPBACK_IF}
")

    file(WRITE ${INTEG_WORKDIR}/conf/Users.db
"[admin]
PASSWORD = admin123
PRIVILIGES = 15
ALLOWED_SOURCE_MASK = 0xFFFF
ALLOWED_DEST_MASK = 0xFFFF

[readonly]
PASSWORD = readonly123
PRIVILIGES = 5
ALLOWED_SOURCE_MASK = 0xFFFF
ALLOWED_DEST_MASK = 0xFFFF

[noaccess]
PASSWORD = noaccess123
PRIVILIGES = 0
ALLOWED_SOURCE_MASK = 0x0000
ALLOWED_DEST_MASK = 0x0000

[restricted_dest]
PASSWORD = restdest123
PRIVILIGES = 7
ALLOWED_SOURCE_MASK = 0xFFFF
ALLOWED_DEST_MASK = 0xFF00

[restricted_src]
PASSWORD = restsrc123
PRIVILIGES = 7
ALLOWED_SOURCE_MASK = 0xFF00
ALLOWED_DEST_MASK = 0xFFFF

[restricted_both]
PASSWORD = restboth123
PRIVILIGES = 7
ALLOWED_SOURCE_MASK = 0xFF00
ALLOWED_DEST_MASK = 0xFF00
")

    file(WRITE ${INTEG_WORKDIR}/conf/Emulator.conf
"[EMULAOTR-GENERAL]
EIB_PORT = 3671
LOG_LEVEL = 3
LISTEN_INTERFACE = ${LOOPBACK_IF}
")

    file(WRITE ${INTEG_WORKDIR}/conf/Emulator.db
"[0/0/1]
PHY = 1.1.1
VALUE = 0x01

[0/0/2]
PHY = 1.1.2
VALUE = 0x0080

[1/2/3]
PHY = 1.2.1
VALUE = 0x00
")

    file(WRITE ${INTEG_WORKDIR}/www/index.html
"<html><body>EIBServer Test</body></html>
")

    # Run all integration tests in a single process so the emulator +
    # server start up once instead of once per test case (~23s -> ~1s).
    add_test(NAME eibserver_integration_tests
        COMMAND eibserver_integration_tests
        WORKING_DIRECTORY ${INTEG_WORKDIR}
    )
endif()
