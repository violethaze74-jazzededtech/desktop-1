project(QtHttpServer)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network Concurrent)

# Build QtSslServer
add_library(QtSslServer SHARED ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/sslserver/qsslserver.cpp)

target_include_directories(QtSslServer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/QtSslServer
)

target_link_libraries(QtSslServer PUBLIC Qt5::Core Qt5::Network Qt5::CorePrivate)

set_target_properties(QtSslServer PROPERTIES DEFINE_SYMBOL "QT_BUILD_SSLSERVER_LIB")

set_target_properties(QtSslServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY})

file(GLOB QtSslServer_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/sslserver/*.h")
list(FILTER QtSslServer_HEADERS EXCLUDE REGEX "_p.h")

file(GLOB QtSslServer_private_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/sslserver/*_p.h")

file(COPY ${QtSslServer_HEADERS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtSslServer/")

file(COPY ${QtSslServer_private_HEADERS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtSslServer/private/")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/headers/QtSslServer" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtSslServer/")

# Build QtHttpServer
add_library(QtHttpServer SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qabstracthttpserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverliterals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverrequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverresponder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverresponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverfutureresponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverrouter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/qhttpserverrouterrule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/3rdparty/http-parser/http_parser.c
)

target_include_directories(QtHttpServer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/3rdparty/http-parser
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QtHttpServer
)

target_link_libraries(QtHttpServer PUBLIC Qt5::Core Qt5::Concurrent Qt5::Network Qt5::CorePrivate QtSslServer)

set_target_properties(QtHttpServer PROPERTIES DEFINE_SYMBOL "QT_BUILD_HTTPSERVER_LIB")

set_target_properties(QtHttpServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY})

file(GLOB QtHttpServer_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/*.h")
list(FILTER QtHttpServer_HEADERS EXCLUDE REGEX "_p.h")

file(GLOB QtHttpServer_private_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/qthttpserver/src/httpserver/*_p.h")

file(COPY ${QtHttpServer_HEADERS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtHttpServer/")

file(COPY ${QtHttpServer_private_HEADERS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtHttpServer/private/")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/headers/QtHttpServer/" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/QtHttpServer/")
