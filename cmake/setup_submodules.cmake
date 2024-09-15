set(STATUS)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule init
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE STATUS)
if(STATUS)
	message(FATAL_ERROR "Failed to init submodules: ${STATUS}")
endif()
set(STATUS)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE STATUS)
if(STATUS)
	message(FATAL_ERROR "Failed to update submodules: ${STATUS}")
endif()
