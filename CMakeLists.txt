cmake_minimum_required(VERSION 3.1)
project(boostExample C CXX)

set(BOOST_COMPONENTS date_time chrono filesystem iostreams program_options regex system thread unit_test_framework)
find_package(Boost 1.54 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(Threads REQUIRED)
set(BOOST_BASE_LIBRARIES
   ${Boost_DATE_TIME_LIBRARY}
   ${Boost_CHRONO_LIBRARY}
   ${Boost_FILESYSTEM_LIBRARY}
   ${Boost_IOSTREAMS_LIBRARY}
   ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${Boost_LOG_LIBRARY}
   ${Boost_LOG_SETUP_LIBRARY}
   ${Boost_ATOMIC_LIBRARY})


add_executable(example-shm src/shm.cpp)
add_executable(example-sync src/sync.cpp)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
if(WIN32)
	target_link_libraries(example-shm ${BOOST_BASE_LIBRARIES})
	target_link_libraries(example-sync ${BOOST_BASE_LIBRARIES})
else()
	target_link_libraries(example-shm ${BOOST_BASE_LIBRARIES} rt ${CMAKE_THREAD_LIBS_INIT})
	target_link_libraries(example-sync ${BOOST_BASE_LIBRARIES} rt ${CMAKE_THREAD_LIBS_INIT})
endif()
