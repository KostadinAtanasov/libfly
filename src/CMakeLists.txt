project(fly)
cmake_minimum_required(VERSION 2.6)

# Project source files...
set(fly_SRCS
	fly.c
	fly_job.c
	fly_sched.c
	fly_thread.c
	fly_worker.c
	)

# Public headers for installation
FILE(GLOB_RECURSE fly_pub_HEADERS "include/*")

# Compilation flags
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "-Wall -Werror")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Global defines
add_definitions(-DFLY_LINUX -DPOSIX -D_GNU_SOURCE)

# Global build flags
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS_DEBUG "-g3 -DDEBUG -DFLY_DEBUG -DFLY_MEMDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLY_ENABLE_ASSERT")

	set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -DFLY_RELEASE")

	set(CMAKE_C_FLAGS_DEBUGTESTS "-g3 -DDEBUG -DFLY_DEBUG")
	set(CMAKE_C_FLAGS_DEBUGTESTS
		"${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_DEBUGTESTS "${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_MEMDEBUG")
	set(CMAKE_C_FLAGS_DEBUGTESTS
		"${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_ENABLE_ASSERT")

	set(CMAKE_C_FLAGS_RELEASETESTS "-O2 -DNDEBUG -DFLY_RELEASE")
	set(CMAKE_C_FLAGS_RELEASETESTS
		"${CMAKE_C_FLAGS_RELEASETESTS} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_RELEASETESTS
		"${CMAKE_C_FLAGS_RELEASETESTS} -DFLY_MEMDEBUG")
endif()

if(BUILD_STATICLIB)
	add_library(fly STATIC ${fly_SRCS})
else()
	add_library(fly SHARED ${fly_SRCS})
	set_target_properties(fly PROPERTIES VERSION 0.3.0 SOVERSION 0)
	install(FILES ${fly_pub_HEADERS}
			DESTINATION include/libfly COMPONENT Devel)
	install(TARGETS fly LIBRARY DESTINATION lib COMPONENT Runtime)
endif()
target_link_libraries(fly pthread)

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()
