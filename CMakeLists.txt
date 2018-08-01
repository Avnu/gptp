cmake_minimum_required (VERSION 3.4)
project (gptp)
include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

include_directories( "./common" )
file(GLOB GPTP_COMMON "./common/*.cpp" "./common/*.c")

if(UNIX)
  include_directories( include "./linux/src" )
  file(GLOB GPTP_OS 
  "./linux/src/daemon_cl.cpp"
  "./linux/src/linux_ipc.cpp"
  "./linux/src/platform.cpp"
  "./linux/src/linux_hal_persist_file.cpp"
  "./linux/src/linux_hal_generic.cpp"
  "./linux/src/linux_hal_generic_adj.cpp"
  "./linux/src/linux_hal_common.cpp")
  add_executable (gptp ${GPTP_COMMON} ${GPTP_OS})
  target_link_libraries(gptp pthread rt)
elseif(WIN32)
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    link_directories($ENV{WPCAP_DIR}/Lib/x64)
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    link_directories($ENV{WPCAP_DIR}/Lib)
  endif()

  # HAVE_REMOTE change pcap include options
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DHAVE_REMOTE)
  include_directories( include "./windows/daemon_cl" $ENV{WPCAP_DIR}/Include )
  file(GLOB GPTP_OS "./windows/daemon_cl/*.cpp" "./windows/daemon_cl/gptp.manifest")
  add_executable (gptp ${GPTP_COMMON} ${GPTP_OS})
  target_link_libraries(gptp wpcap Iphlpapi Ws2_32)

  add_subdirectory("windows/named_pipe_test")

endif()

