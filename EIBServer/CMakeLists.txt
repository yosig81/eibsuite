add_executable(EIBServer
    src/BusMonConnection.cpp
    src/Client.cpp
    src/ClientsMgr.cpp
    src/CommandScheduler.cpp
    src/Dispatcher.cpp
    src/EIBHandler.cpp
    src/EIBInterface.cpp
    src/EIBServer.cpp
    src/Main.cpp
    src/PacketFilter.cpp
    src/RoutingConnection.cpp
    src/ServerConfig.cpp
    src/TunnelConnection.cpp
    src/UsersDB.cpp
    src/WebHandler.cpp
    src/XmlJsonUtil.cpp
    src/conf/EIBBusMonConf.cpp
    src/conf/EIBInterfaceConf.cpp
    src/conf/EIBServerUsersConf.cpp
)

target_include_directories(EIBServer PRIVATE include include/conf)
target_link_libraries(EIBServer PRIVATE EIBStdLib)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
