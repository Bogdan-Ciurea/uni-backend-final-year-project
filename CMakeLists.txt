cmake_minimum_required (VERSION 3.10)
project(api_server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the headers in the /include directory
include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# -------------------

# -------------------
# Libraries
# Drogon
# for more details visit https://drogon.docsforge.com
add_subdirectory(modules/drogon)
include_directories({PROJECT_NAME} PUBLIC modules/drogon/lib/inc/drogon)
include_directories({PROJECT_NAME} PUBLIC modules/drogon/trantor/trantor/utils)
link_libraries(drogon)

# Google Test
add_subdirectory(modules/googletest)
include_directories({PROJECT_NAME} PUBLIC modules/googletest/googletest/include)
link_libraries(gtest_main)

# Cassandra cpp-driver
# for more details visit https://docs.datastax.com/en/developer/cpp-driver/2.16/
if(APPLE)
    set(LIBUV_INCLUDE_DIR /opt/homebrew/Cellar/libuv/1.44.2/include)
endif()
add_subdirectory(modules/cassandra)
include_directories({PROJECT_NAME} PUBLIC modules/cassandra/include)
link_libraries(cassandra)

# SMTPMail-drogon - for sending the emails
include_directories({PROJECT_NAME} PUBLIC modules/SMTPMail-drogon)

# bcrypt - for hashing the passwords
include_directories({PROJECT_NAME} PUBLIC modules/bcrypt/include)

# cpp-jwt - for generating the JWT tokens
add_subdirectory(modules/jwt-cpp)
include_directories({PROJECT_NAME} PUBLIC modules/jwt-cpp/include)

# -------------------

# Add the main project
set(PROJECT_LIBS project_libs)

# Copy the assets
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
#      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
# -------------------

# -------------------

# Add tests for the project
option(PACKAGE_TESTS "Build the tests" OFF)
if(PACKAGE_TESTS)
    enable_testing()
    set(TEST_EXEC_NAME unit_tests)
    add_subdirectory(tests)
endif()
# -------------------