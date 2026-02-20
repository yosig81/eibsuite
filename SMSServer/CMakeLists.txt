add_executable(SMSServer
    src/Main.cpp
    src/SMSServer.cpp
    src/SMSListener.cpp
    src/EIBAgent.cpp
    src/MsgTable.cpp
    src/SMSServerDB.cpp
    src/SMSServerConfig.cpp
)

target_include_directories(SMSServer PRIVATE include)
target_compile_definitions(SMSServer PRIVATE HAVE_CONFIG_H)
target_link_libraries(SMSServer PRIVATE EIBStdLib GSM)
