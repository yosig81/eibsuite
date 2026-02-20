add_executable(EIBGenericTemplate
    src/Main.c
    src/GenericTemplate.c
    src/Private.cpp
)

target_include_directories(EIBGenericTemplate PRIVATE include)
target_link_libraries(EIBGenericTemplate PRIVATE EIBStdLib)
