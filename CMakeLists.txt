cmake_minimum_required(VERSION 3.22)
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/Install)

set(MACOSX_RPATH ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)

set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "@loader_path/../lib/")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(deps/glfw-3.3.8)

find_package(Vulkan REQUIRED FATAL_ERROR)

add_executable(${PROJECT_NAME} "Game.cpp" UserInputs.hpp Plane.hpp Package.hpp UserModelPool.hpp DataStructs.hpp)

if (VULKAN_FOUND)
    message(STATUS "Found Vulkan. Including and linking...")
    include_directories(${GLFW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw ${GLFW_LIBRARIES})
endif(VULKAN_FOUND)

target_link_libraries(${PROJECT_NAME} glfw)

target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include)
target_include_directories(${PROJECT_NAME} PUBLIC /Users/$ENV{USER}/VulkanSDK/1.3.239.0/macOS/include)
target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/lib)
target_include_directories(${PROJECT_NAME} PUBLIC /Users/$ENV{USER}/VulkanSDK/1.3.239.0/macOS/lib)

target_include_directories(${PROJECT_NAME} PUBLIC headers)

target_link_libraries(${PROJECT_NAME} /usr/local/lib/libglfw.3.3.dylib)
target_link_libraries(${PROJECT_NAME} /Users/$ENV{USER}/VulkanSDK/1.3.239.0/macOS/lib/libvulkan.1.dylib)
target_link_libraries(${PROJECT_NAME} /Users/$ENV{USER}/VulkanSDK/1.3.239.0/macOS/lib/libvulkan.1.3.239.dylib)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES /usr/local/lib/libvulkan.1.3.211.dylib DESTINATION lib RENAME libvulkan.1.dylib)

