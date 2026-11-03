cmake_minimum_required(VERSION 3.8)
project(game_ai)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(game_interfaces REQUIRED)

add_executable(controller_full_state src/controller_full_state.cpp)
ament_target_dependencies(controller_full_state rclcpp sensor_msgs game_interfaces)

install(TARGETS
  controller_full_state

  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
