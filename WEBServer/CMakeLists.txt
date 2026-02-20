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
add_custom_command(TARGET WEBServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/www
        $<TARGET_FILE_DIR:WEBServer>/www
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Images
        $<TARGET_FILE_DIR:WEBServer>/Images
)
