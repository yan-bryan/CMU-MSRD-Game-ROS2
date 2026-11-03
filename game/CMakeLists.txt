cmake_minimum_required(VERSION 3.8)
project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(game_interfaces REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)

# Bundled GLFW, built from source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/glfw)

include_directories(include)

add_executable(missiledefense
  src/startup.cpp
  src/Game.cpp
  src/glad.c
  src/gl_frontend.cpp
  src/glutils.cpp
  src/GLMesh.cpp
  src/Controller.cpp
)
target_link_libraries(missiledefense glfw ${CMAKE_DL_LIBS} ${OpenCV_LIBS})

ament_target_dependencies(missiledefense
  rclcpp sensor_msgs cv_bridge ament_index_cpp game_interfaces)

install(TARGETS
  missiledefense
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  shaders
  DESTINATION share/${PROJECT_NAME}
)

ament_package()