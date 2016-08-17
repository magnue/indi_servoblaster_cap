cmake_minimum_required(VERSION 2.6)
PROJECT(indi_servoblaster_cap CXX)

cmake_policy(SET CMP0003 NEW)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

##################  Paths  ###################################

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")

##################  Includes  ################################

include (MacroBoolTo01)

FIND_PACKAGE(INDI REQUIRED)
FIND_PACKAGE(INDIDriver REQUIRED)

macro_bool_to_01(INDI_DRIVER_LIBRARIES HAVE_INDIDRIVER_LIB)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_SOURCE_DIR}/servoblaster_cap)

################ ServoBlaster Cap ##############################

set(servoblaster_cap_SRCS
        ${CMAKE_SOURCE_DIR}/servoblaster_cap/servoblaster_cap.cpp
        ${CMAKE_SOURCE_DIR}/servoblaster_cap/parkdata.cpp
   )

add_executable(indi_servoblaster_cap ${servoblaster_cap_SRCS})
target_link_libraries(indi_servoblaster_cap ${INDI_LIBRARIES} ${INDI_DRIVER_LIBRARIES})
install(TARGETS indi_servoblaster_cap RUNTIME DESTINATION bin )
install( FILES ${CMAKE_SOURCE_DIR}/servoblaster_cap/indi_servoblaster_cap.xml DESTINATION ${DATA_INSTALL_DIR})

###########################################################
