cmake_minimum_required(VERSION 3.4.1)


project(mcu_test)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11")

#include_directories(${INCLUDE_FILE_LIST})

#LINK_DIRECTORIES(${RM_LIB}/lib)
#LINK_DIRECTORIES(${THIRD_PARTY_PATH}/axera_sdk_ax620_v0310/lib)

add_executable(can_tool ./can_tool.cpp)
target_link_libraries(can_tool
	pthread
)


#set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)
#install(TARGETS mcu_test DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)
