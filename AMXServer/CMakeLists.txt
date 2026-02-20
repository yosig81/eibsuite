add_executable(AMXServer
    src/AMXHandler.cpp
    src/AMXServerConfig.cpp
    src/AMXServer.cpp
    src/CRC.cpp
    src/EIBListener.cpp
    src/Main.cpp
    src/MsgTable.cpp
)

target_include_directories(AMXServer PRIVATE include include/conf)
target_link_libraries(AMXServer PRIVATE EIBStdLib)
