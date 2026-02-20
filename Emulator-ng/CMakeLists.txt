add_executable(Emulator-ng
    src/EmulatorConfig.cpp
    src/EmulatorHandler.cpp
    src/Emulator-ng.cpp
    src/EmulatorDB.cpp
    src/EmulatorCmd.cpp
    src/Main.cpp
)

target_include_directories(Emulator-ng PRIVATE include include/conf)
target_link_libraries(Emulator-ng PRIVATE EIBStdLib)
