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

# Copy static assets (www/, Images/) next to the executable so the
# default relative paths ("./www", "./Images") work out of the box.
# Uses a separate target so assets are synced on every build, even
# when no C++ sources change.
add_custom_target(EIBServer-assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/www
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/www
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Images
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Images
)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
