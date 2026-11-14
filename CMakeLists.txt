cmake_minimum_required(VERSION 3.20)
project(SolarSystem LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)
endif()
find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)
endif()
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
add_executable(solar_system
    main.cpp glad.c Audio.cpp Benchmark.cpp Easter.cpp Input.cpp
    Render.cpp SaveLoad.cpp Scenario.cpp UI.cpp Weapons.cpp World.cpp)
target_include_directories(solar_system PRIVATE libraries/include)
target_link_libraries(solar_system PRIVATE glfw OpenGL::GL Threads::Threads ${CMAKE_DL_LIBS})
set_target_properties(solar_system PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(solar_system PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
if(MSVC)
    target_compile_definitions(solar_system PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(solar_system PRIVATE /utf-8)
endif()
add_custom_target(run
    COMMAND $<TARGET_FILE:solar_system>
    DEPENDS solar_system
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL)
