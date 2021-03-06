# Copyright 2016 Silent Circle, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.



cmake_minimum_required(VERSION 3.0)

set(AUTO_BUILD $ENV{AUTOMATED_BUILD})

# If running auto build then we assume that we build it for Android, thus we
# don't need a CC or CXX compiler on the build system. It's all done in Android's
# cross-compiler environment when calling ndk-build. Ignore some Cmake messages
# in this case.
if (AUTO_BUILD EQUAL 1)
    PROJECT(libsalamander++ NONE)
else()
    PROJECT(libsalamander++)
endif()

SET(CPACK_PACKAGE_VERSION_MAJOR 1)
SET(CPACK_PACKAGE_VERSION_MINOR 0)
SET(CPACK_PACKAGE_VERSION_PATCH 0)

set (VERSION 1.0.0)
set (SOVERSION 1)

### Configuration section
# Where to find the modules that we share with ZRTP, i.e. the ZRTP root directory
if (AUTO_BUILD EQUAL 1)
    set (ZRTP_BASE_DIR $ENV{WORKSPACE})
else()
    set (ZRTP_BASE_DIR /home/werner/silentC/zrtpcpp)
endif()

option(STANDALONE "Build standlone shared library, otherwise a static lib without ZRTP crypto modules." OFF)
option(UNITTESTS "Build unit tests, implies STANDALONE true." OFF)
option(ANDROID "Compile and build for Android." OFF)

set(LIBRARY_BUILD_TYPE SHARED)
set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (UNITTESTS)
    set(STANDALONE true)
endif()

if (NOT STANDALONE)
    set(LIBRARY_BUILD_TYPE STATIC)
endif()

set(salLibName salamander++)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

### No more configuration below this line (usually ;-) )
if (EXISTS "${ZRTP_BASE_DIR}/zrtp/crypto/hmac256.cpp")
    message(STATUS "Using ${ZRTP_BASE_DIR} for common modules")
else()
    message(FATAL_ERROR "Cannot locate ${ZRTP_BASE_DIR}. Please set correct path")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLOG_MAX_LEVEL=VERBOSE")

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if ("debug" STREQUAL build_type)
    set (LOG_MAX_LEVEL "-DLOG_MAX_LEVEL=VERBOSE")
endif()


# use protobuf_lite.pc
# include most of the fine stuff we need
include(FindPkgConfig)
include(CheckIncludeFiles)

if (NOT ANDROID)
    pkg_check_modules(PROTO protobuf-lite>=2.6.1)
    if (PROTO_FOUND)
        include_directories(${PROTO_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${PROTO_LDFLAGS})
    else()
        message(FATAL_ERROR "ProtocolBuffer library not found")
    endif()

    pkg_check_modules(SQLCIPHER sqlcipher>=3.7)
    if (SQLCIPHER_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SQLCIPHER_INCLUDE_DIRS})
        check_include_files(sqlite3.h HAVE_SQLCIPHER_H)
        set(LIBS ${LIBS} -lsqlcipher)
        MESSAGE(STATUS "Using SQlCipher based ZRTP cache")
        add_definitions(-DSQL_CIPHER -DSQLITE_HAS_CODEC)
    else()
        message(FATAL_ERROR "SQLCipher library not found")
    endif()
endif()

# pkg_check_modules(SQLITE3 sqlite3>=3.7)
# if (SQLITE3_FOUND)
# #    check_include_files(sqlite3.h HAVE_SQLITE_H)
#     set(LIBS ${LIBS} -lsqlite3)
#     MESSAGE(STATUS "Using SQLite based ZRTP cache")
# else()
#     message(FATAL_ERROR "SQLite3 library not found")
# endif()

if (UNITTESTS)
    add_definitions(-DUNITTESTS)
else()
    add_definitions(-DEMBEDDED)
endif()

add_subdirectory(salamander/crypto)

set (protocol_src
    salamander/SalZrtpConnector.cpp
    salamander/SalPreKeyConnector.cpp
    salamander/ratchet/SalRatchet.cpp
    salamander/state/SalConversation.cpp
)
set (interface_src 
    interfaceApp/AppInterfaceImpl.cpp
    interfaceApp/MessageEnvelope.pb.cc
    interfaceApp/java/JavaNativeImpl.cpp
    interfaceTransport/sip/SipTransport.cpp
)

set (storage_src
    storage/sqlite/SQLiteStoreConv.cpp
)

set (key_mngmnt_src
    keymanagment/PreKeys.cpp
)

set (provisioning_src
    provisioning/ScProvisioning.cpp
)

set (util_src
    util/cJSON.c
    util/b64helper.cpp
    util/UUID.cpp
)

set (app_repo_src
    appRepository/AppRepository.cpp
)

if (STANDALONE)
    set (external_src
        ${ZRTP_BASE_DIR}/common/osSpecifics.c)
endif()

add_subdirectory(attachments)

set (sal_src
    ${protocol_src}
    ${storage_src}
    ${interface_src}
    ${key_mngmnt_src}
    ${provisioning_src}
    ${util_src}
    ${external_src}
    ${app_repo_src}
)

if (NOT ANDROID)
    add_library(${salLibName} ${LIBRARY_BUILD_TYPE} ${sal_src} $<TARGET_OBJECTS:crypto_lib> $<TARGET_OBJECTS:attachment_lib>)
    set_target_properties(${salLibName} PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
    TARGET_INCLUDE_DIRECTORIES(${salLibName} PUBLIC ${ZRTP_BASE_DIR} ${ZRTP_BASE_DIR}/zrtp)
    target_link_libraries(${salLibName} ${LIBS})

    if (UNITTESTS)
        add_subdirectory(gtest-1.7.0)
        add_subdirectory(unittests)
    endif()
endif()

add_custom_target(protobuf COMMAND ${CMAKE_SOURCE_DIR}/protobuf/android/runndk.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/protobuf/android)

add_custom_target(android ${CMAKE_SOURCE_DIR}/android/runndk.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/android)
add_dependencies(android protobuf)


# add_subdirectory(protobuf/android)
#
# Prepare Android's Android.mk and Application.mk files
string(REPLACE ";" " " crypto_src_spc "${crypto_src_global}")
string(REPLACE ";" " " attachment_src_spc "${attchment_src_global}")
if (STANDALONE)
    set (sal_src "${sal_src}" ${CMAKE_SOURCE_DIR}/android/jni/sqlite3/sqlite3.c)
else()
    set (EMBEDDED -DEMBEDDED)
endif()
if (UNITTESTS)
    set (EMBEDDED -DUNITTESTS)
endif()

string(REPLACE ";" " " sal_src_spc "${sal_src}")
set(local_cpp_features "exceptions")

configure_file(${CMAKE_SOURCE_DIR}/android/jni/Android.mk
               ${CMAKE_BINARY_DIR}/android/jni/Android.mk @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/android/jni/Application.mk
               ${CMAKE_BINARY_DIR}/android/jni/Application.mk @ONLY)

#add_subdirectory(salamander/android)
