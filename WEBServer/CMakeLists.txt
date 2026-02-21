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

# Copy static assets (www/, Images/) next to the executable so the
# default relative paths ("./www", "./Images") work out of the box.
# Uses a separate target so assets are synced on every build, even
# when no C++ sources change.
add_custom_target(WEBServer-assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/www
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/www
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Images
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Images
)
