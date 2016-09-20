project(test-sdl)

find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)
find_package(OpenGLES REQUIRED)

if (NOT SDL2_FOUND)
	return()
endif (NOT SDL2_FOUND)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)

set(TEST_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} ${OPENGLES_INCLUDE_DIR})
set(TEST_LIBRARIES ${SDL2_LIBRARY} ${OPENGLES_LIBRARIES})

set(test_sdl_SOURCES
	testEgl.c
)

include_directories(${TEST_INCLUDE_DIRS})
add_executable(test-sdl ${test_sdl_SOURCES})
target_link_libraries(test-sdl ${TEST_LIBRARIES})

install(TARGETS test-sdl DESTINATION "bin")
