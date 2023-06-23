project(WorkshopTool VERSION 2.1.0)

option(TOOL_ENABLEGETCH "Enable user prompt on exit" OFF)
option(TOOL_USE_FMT "Use libfmt instead of stdc++ <format>" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)

if(TOOL_ENABLEGETCH)
	set(TOOL_ENABLEGETCH true)
else()
	set(TOOL_ENABLEGETCH false)
endif()

if(TOOL_USE_FMT)
	set(TOOL_USE_FMT true)
else()
	set(TOOL_USE_FMT false)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND (NOT((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13.0.0"))))
	set(TOOL_USE_FMT true)
endif()

if(TOOL_USE_FMT)
	find_package(fmt REQUIRED)
	target_link_libraries(tool PRIVATE fmt::fmt)
endif()

configure_file(version.h.in version.h)

add_executable(tool main.cpp HttpClient.cpp SteamWorkshop.cpp)
target_include_directories(tool PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tool PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
install(TARGETS tool DESTINATION bin)
