cmake_minimum_required(VERSION 3.4.1)


project(mcu_test)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11")

#include_directories(${INCLUDE_FILE_LIST})

#LINK_DIRECTORIES(${RM_LIB}/lib)
#LINK_DIRECTORIES(${THIRD_PARTY_PATH}/axera_sdk_ax620_v0310/lib)

add_executable(sdk_tcp ./sdk_tcp.cpp)
target_link_libraries(sdk_tcp
	pthread
)

add_executable(ad_tcp ./ad_tcp.cpp)
target_link_libraries(ad_tcp
	pthread
)

add_executable(sdk_test ./protocol.cpp ./connect_tcp.cpp ./async_message.cpp ./sdk_main.cpp ./app.cpp)
target_link_libraries(sdk_test
	pthread
)

add_executable(ad_test ./protocol.cpp ./connect_tcp.cpp ./async_message.cpp ./ad_main.cpp ./app.cpp)
target_link_libraries(ad_test
	pthread
)

#set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)
#install(TARGETS mcu_test DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)
