cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (WIN32)
  set(CMAKE_CXX_FLAGS "/utf-8 /EHsc")
  set(CMAKE_C_FLAGS "/utf-8 /EHsc")
  string(REPLACE "/subsystem:console" "/subsystem:windows" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
else() 
  set(CMAKE_CXX_FLAGS "-fPIC")
endif()
add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
add_compile_definitions(_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

project(LIGHTNINGS VERSION 0.0.1 DESCRIPTION "lightnings algo" LANGUAGES CXX C)

# TARGET
set(LIGHTNINGS_SOURCE_FILES    
 "main.cpp"
)
add_executable(LIGHTNINGS ${LIGHTNINGS_SOURCE_FILES})
target_include_directories(LIGHTNINGS PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
if (WIN32)
    target_link_options(LIGHTNINGS PRIVATE "/SUBSYSTEM:WINDOWS")
    target_link_options(LIGHTNINGS PRIVATE "/ENTRY:mainCRTStartup")
endif()

# DEPENDENCIES
set(RAYLIB_VERSION 5.5.4)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  set(CMAKE_TLS_VERIFY FALSE)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/KraftJrIvo/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    URL_HASH SHA512=0f24c24108a75b5f4e82de38e814f17220823bd1e578369eee4122f323e491dc4a607d192c21935170035a22b23d0f4a5f197486758be434c6f80759f1356b80
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()
target_compile_options(raylib PUBLIC -DRAYMATH_DISABLE_CPP_OPERATORS)
target_compile_options(raylib PUBLIC -DGRAPHICS_API_OPENGL_33)

# LINK
target_link_libraries(LIGHTNINGS PUBLIC raylib)
