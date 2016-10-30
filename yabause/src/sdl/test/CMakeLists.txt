project(test-sdl)

find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)
find_package(OpenGLES REQUIRED)
find_package(EGL REQUIRED)

if (NOT SDL2_FOUND)
	return()
endif (NOT SDL2_FOUND)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)

if (NOT EGL_FOUND)
	return()
endif (NOT EGL_FOUND)

set(TEST_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} ${OPENGLES_INCLUDE_DIR} ${EGL_INCLUDE_DIR})
set(TEST_LIBRARIES ${SDL2_LIBRARY} ${OPENGLES_LIBRARIES} ${EGL_LIBRARIES})

set(test_sdl_SOURCES
	testEgl.c
	../../patternManager.c
)

include_directories(${TEST_INCLUDE_DIRS})
add_executable(test-sdl ${test_sdl_SOURCES})
target_link_libraries(test-sdl ${TEST_LIBRARIES})

install(TARGETS test-sdl DESTINATION "bin")
