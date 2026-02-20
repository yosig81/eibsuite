add_executable(EIBRelay
    src/EIBRelayServer.cpp
    src/Main.cpp
    src/RelayHandler.cpp
    src/RelayServerConfig.cpp
)

target_include_directories(EIBRelay PRIVATE include include/conf)
target_link_libraries(EIBRelay PRIVATE EIBStdLib)
