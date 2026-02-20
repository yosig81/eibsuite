add_executable(WEBServer
    src/ConsoleClient.cpp
    src/ContentGenerator.cpp
    src/Dispatcher.cpp
    src/Main.cpp
    src/UsersDB.cpp
    src/WEBCollector.cpp
    src/WEBServer.cpp
    src/WEBServerConfig.cpp
    src/WebHandler.cpp
    src/XmlJsonUtil.cpp
)

target_include_directories(WEBServer PRIVATE include include/conf)
target_link_libraries(WEBServer PRIVATE EIBStdLib)
