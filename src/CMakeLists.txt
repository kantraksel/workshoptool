project(WorkshopTool VERSION 2.0.0)

option(TOOL_ENABLEGETCH "Enable user prompt on exit" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)

if(TOOL_ENABLEGETCH)
	set(TOOL_ENABLEGETCH true)
else()
	set(TOOL_ENABLEGETCH false)
endif()

configure_file(version.h.in version.h)

add_executable(tool main.cpp HttpClient.cpp SteamWorkshop.cpp)
target_include_directories(tool PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tool PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
install(TARGETS tool DESTINATION bin)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	find_package(fmt REQUIRED)
	target_link_libraries(tool PRIVATE fmt::fmt)
endif()
