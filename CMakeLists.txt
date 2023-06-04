cmake_minimum_required(VERSION 3.5)

project(KarenIntegration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required dependencies
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)


# Add the Boost macro definition
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

# Add the header and source files to your project
set(HEADER_FILES
    dafa.h
)

set(SOURCE_FILES
    dafa.cpp
)

find_package(RapidJSON REQUIRED)

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(KarenIntegration main.cpp ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(KarenIntegration CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)

# Install target
install(TARGETS KarenIntegration
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

