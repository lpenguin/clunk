project(clunk)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_USE_RELATIVE_PATHS TRUE)

# modules
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
option(WITH_SSE "Use highly optimized SSE FFT/MDCT routines" false)

if ( NOT SDL2_FOUND )
	message ( FATAL_ERROR "SDL2 not found!" )
endif ( NOT SDL2_FOUND )

set(clunk_VERSION_MAJOR 1)
set(clunk_VERSION_MINOR 0)

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINDOWS)
endif(WIN32)

add_definitions(-DCLUNKAPI=DLLEXPORT)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
include_directories(${SDL2_INCLUDE_DIR})

set(SOURCES 
	buffer.cpp
	clunk_ex.cpp
	context.cpp
	distance_model.cpp
	kemar.c
	logger.cpp
	object.cpp
	sample.cpp
	sdl_ex.cpp
	source.cpp
	stream.cpp
	clunk_c.cpp
)
set(PUBLIC_HEADERS
	buffer.h
	clunk.h
	clunk_assert.h
	context.h
	distance_model.h
	export_clunk.h
	fft_context.h
	locker.h
	logger.h
	mdct_context.h
	object.h
	sample.h
	source.h
	sse_fft_context.h
	stream.h
	v3.h
	clunk_c.h
)

if (WITH_SSE)
	set(SOURCES ${SOURCES} sse_fft_context.cpp)
	add_definitions(-DCLUNK_USES_SSE)
	add_definitions(-msse2)
endif(WITH_SSE)

add_library(clunk SHARED
	${SOURCES}
)

install(TARGETS clunk DESTINATION lib)
install(FILES ${PUBLIC_HEADERS} DESTINATION include/clunk)

target_link_libraries(clunk ${SDL2_LIBRARY})
