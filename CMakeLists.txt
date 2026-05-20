cmake_minimum_required(VERSION 3.12...3.31)
project(blablacar_service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

set(USERVER_FEATURE_POSTGRESQL ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_REDIS ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_RABBITMQ ON CACHE BOOL "" FORCE)
find_package(userver COMPONENTS core postgresql mongo redis rabbitmq REQUIRED)
find_package(userver COMPONENTS core postgresql QUIET)
if(NOT userver_FOUND)
  # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
  download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

# Force Clang to use GCC 11 toolchain for proper C++20 coroutine support
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=/usr")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --gcc-toolchain=/usr")
endif()


# Common sources
add_library(${PROJECT_NAME}_objs OBJECT
    src/handlers/api_handlers.cpp
    src/handlers/api_handlers.hpp
    src/storage_pg.cpp
    src/storage_pg.hpp
    src/storage_mongo.cpp
    src/storage_mongo.hpp
    src/redis_cache.cpp
    src/redis_cache.hpp
    src/rate_limiter.cpp
    src/rate_limiter.hpp
    src/events.cpp
    src/events.hpp
    src/models.hpp
    src/auth_middleware.hpp
    src/auth_middleware.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::postgresql userver::mongo userver::redis userver::rabbitmq)

# Force include GCC 11 headers for coroutine support with Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_include_directories(${PROJECT_NAME}_objs SYSTEM BEFORE PRIVATE
    /usr/include/c++/11
    /usr/include/x86_64-linux-gnu/c++/11
  )
endif()


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)



# Functional Tests
userver_testsuite_add_simple()


# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})
