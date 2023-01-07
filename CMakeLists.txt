cmake_minimum_required(VERSION 3.15)

project(WorkShopTool VERSION 2.0.0)

option(TOOL_HEADLESS "Compile Tool in headless mode" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(json)
find_package(CURL CONFIG REQUIRED)

if(TOOL_HEADLESS)
	set(TOOL_HEADLESS true)
else()
	set(TOOL_HEADLESS false)
endif()
configure_file(version.h.in version.h)

add_executable(tool main.cpp HttpClient.cpp)
target_include_directories(tool PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tool PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
install(TARGETS tool DESTINATION bin)
