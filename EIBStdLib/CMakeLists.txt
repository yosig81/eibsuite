add_library(EIBStdLib SHARED
    src/Base64.cpp
    src/CException.cpp
    src/CMutex.cpp
    src/CRI_CRD.cpp
    src/CString.cpp
    src/CTime.cpp
    src/CCemi_L_Data_Frame.cpp
    src/CCemi_L_BusMon_Frame.cpp
    src/ConfigFile.cpp
    src/ConnectRequest.cpp
    src/ConnectResponse.cpp
    src/ConnectionStateRequest.cpp
    src/ConnectionStateResponse.cpp
    src/DataBuffer.cpp
    src/DiffieHellman.cpp
    src/Digest.cpp
    src/Directory.cpp
    src/DisconnectRequest.cpp
    src/DisconnectResponse.cpp
    src/EIBAddress.cpp
    src/EIBNetIP.cpp
    src/GenericServer.cpp
    src/Globals.cpp
    src/HPAI.cpp
    src/HttpHeader.cpp
    src/HttpParameter.cpp
    src/HttpParser.cpp
    src/HttpReply.cpp
    src/HttpRequest.cpp
    src/HttpSession.cpp
    src/IConnection.cpp
    src/LogFile.cpp
    src/MD5.cpp
    src/RoutingIndication.cpp
    src/SearchRequest.cpp
    src/SearchResponse.cpp
    src/DescriptionRequest.cpp
    src/DescriptionResponse.cpp
    src/ServiceBase.cpp
    src/SharedMemory.cpp
    src/SingletonValidation.cpp
    src/Socket.cpp
    src/StatsDB.cpp
    src/StringTokenizer.cpp
    src/TunnelAck.cpp
    src/TunnelRequest.cpp
    src/URLEncoding.cpp
    src/Utils.cpp
    src/XGetopt.cpp
    src/cli.cpp
    src/xml/Xml.cpp
    src/xml/action_store.cpp
    src/xml/htmlutil.cpp
    src/xml/lex_util.cpp
    src/xml/node_set.cpp
    src/xml/tinystr.cpp
    src/xml/tinyxml.cpp
    src/xml/tinyxmlerror.cpp
    src/xml/tinyxmlparser.cpp
    src/xml/tokenlist.cpp
    src/xml/xml_util.cpp
    src/xml/xpath_expression.cpp
    src/xml/xpath_processor.cpp
    src/xml/xpath_stack.cpp
    src/xml/xpath_static.cpp
    src/xml/xpath_stream.cpp
    src/xml/xpath_syntax.cpp
)

target_include_directories(EIBStdLib
    PUBLIC  include
    PRIVATE include/xml
)

# Transitive: anything linking EIBStdLib automatically gets jtc + pthreads
target_link_libraries(EIBStdLib PUBLIC jtc)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
