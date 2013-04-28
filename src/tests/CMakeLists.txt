project(fly_tests)
cmake_minimum_required(VERSION 2.6)

# Project source files...
set(fly_tests_SRCS
	test_init.c
	test_parallel_for.c
	test_push_task.c
	)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Global defines
add_definitions(-DFLY_LINUX -DPOSIX -D_GNU_SOURCE)

# Global build flags
if(CMAKE_COMPILER_IS_GNUC)
	set(CMAKE_C_FLAGS_DEBUG "-g3 -DDEBUG -DFLY_DEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLY_MEMDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLY_ENABLE_ASSERT")

	set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -DFLY_RELEASE")

	set(CMAKE_C_FLAGS_DEBUGTESTS "-g3 -DDEBUG -DFLY_DEBUG")
	set(CMAKE_C_FLAGS_DEBUGTESTS
		"${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_DEBUGTESTS
		"${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_MEMDEBUG")
	set(CMAKE_C_FLAGS_DEBUGTESTS
		"${CMAKE_C_FLAGS_DEBUGTESTS} -DFLY_ENABLE_ASSERT")

	set(CMAKE_C_FLAGS_RELEASETESTS "-O2 -DNDEBUG -DFLY_RELEASE")
	set(CMAKE_C_FLAGS_RELEASETESTS
		"${CMAKE_C_FLAGS_RELEASETESTS} -DFLY_ENABLE_LOG")
	set(CMAKE_C_FLAGS_RELEASETESTS
		"${CMAKE_C_FLAGS_RELEASETESTS} -DFLY_MEMDEBUG")
endif()

add_executable(test_init test_init.c)
target_link_libraries(test_init fly)

add_executable(test_parallel_for test_parallel_for.c)
target_link_libraries(test_parallel_for fly m)


add_executable(test_push_task test_push_task.c)
target_link_libraries(test_push_task fly m)