cmake_minimum_required(VERSION 3.15)

project(WorkShopTool VERSION 1.0.0)

option(TOOL_HEADLESS "Compile Tool in headless mode" OFF)
option(ONEBINTOOL "Compile with static linked runtime library" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(json)
add_subdirectory(cpp-httplib)

if(TOOL_HEADLESS)
	set(_TOOL_HEADLESS TRUE)
else()
	set(_TOOL_HEADLESS FALSE)
endif()

configure_file(version.h.in version.h)
add_executable(tool main.cpp)
if(ONEBINTOOL)
	set_property(TARGET tool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_include_directories(tool PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tool PRIVATE httplib::httplib nlohmann_json::nlohmann_json)
install(TARGETS tool DESTINATION bin)
