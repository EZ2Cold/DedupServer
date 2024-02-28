cmake_minimum_required(VERSION 3.16)

project(DedupServer)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(include)

find_package(Boost 1.36.0 REQUIRED COMPONENTS thread system serialization iostreams)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else ()
    message(FATAL_ERROR, "Cannot find Boost library")
endif ()
find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_directories(${OPENSSL_LIBRARY_DIR})
else ()
    message(FATAL_ERROR, "Cannot find OpenSSL library")
endif ()

add_executable(
    server
    src/app/server.cc
    src/comm/ThreadPool.cc
    src/comm/Config.cc
    src/server/DedupServer.cc
    src/server/ClientConn.cc
    src/comm/Logger.cc
    src/server/MysqlConnPool.cc
)
target_link_libraries(server OpenSSL::SSL mysqlclient)

add_executable(
    client
    src/client/DedupClient.cc
    src/app/client.cc
    src/comm/Config.cc
    src/comm/Logger.cc
)
target_link_libraries(client OpenSSL::SSL)

set(CMAKE_CXX_FLAGS -pthread)
add_executable(
    testIOMultiplex
    src/app/testIOMultiplex.cc
)

add_executable(
    testSingleton
    src/app/testSingleton.cc
)

add_executable(
    testPthread
    src/app/testPthread.cc
)

add_executable(
    testProducerConsumer
    src/app/testProducerConsumer.cc
)


add_executable(
    testThreadPool
    src/app/testThreadPool.cc
    src/comm/ThreadPool.cc
)

add_executable(
    testMysqlConnPool
    src/app/testMysqlConnPool.cc
    src/comm/Logger.cc
    src/server/MysqlConnPool.cc
)
target_link_libraries(testMysqlConnPool mysqlclient)

add_executable(
    testCPP11sync
    src/app/testCPP11sync.cc
)


add_executable(
    testThread
    src/app/testThread.cc
)
