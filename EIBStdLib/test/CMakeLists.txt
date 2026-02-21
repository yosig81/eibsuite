add_executable(eibstdlib_tests
    unit/Base64Test.cpp
    unit/BugFixTests.cpp
    unit/CemiLBusMonFrameTest.cpp
    unit/CemiLDataFrameTest.cpp
    unit/ConfigFileTest.cpp
    unit/ConnectDisconnectRequestEdgeTest.cpp
    unit/ConnectionResponsesTest.cpp
    unit/CStringTest.cpp
    unit/CTimeTest.cpp
    unit/DataBufferTest.cpp
    unit/DescriptionSearchResponseTest.cpp
    unit/DiffieHellmanTest.cpp
    unit/DigestMd5Test.cpp
    unit/DirectoryTest.cpp
    unit/EIBAddressTest.cpp
    unit/GenericDBTest.cpp
    unit/GenericServerTest.cpp
    unit/HttpParserTest.cpp
    unit/HttpRequestReplyTest.cpp
    unit/LogFileTest.cpp
    unit/MulticastBindTest.cpp
    unit/ProtocolPacketRoundTripTest.cpp
    unit/RoutingIndicationDescriptionRequestTest.cpp
    unit/SocketNetworkTest.cpp
    unit/StringTokenizerTest.cpp
    unit/TunnelRequestTest.cpp
    unit/URLEncodingTest.cpp
    unit/UtilsTest.cpp
    unit/XGetoptTest.cpp
    unit/XmlParserTest.cpp
)

set_target_properties(eibstdlib_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(eibstdlib_tests PRIVATE fixtures)
target_link_libraries(eibstdlib_tests PRIVATE EIBStdLib GTest::gtest GTest::gtest_main)

gtest_discover_tests(eibstdlib_tests)
