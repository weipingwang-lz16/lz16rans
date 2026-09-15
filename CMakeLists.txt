cmake_minimum_required(VERSION 3.10.1)
Project(lz16rans LANGUAGES C)

add_subdirectory(other)

        file(GLOB HEADER_MY  "*.h" "lib/c/*.h" "lib/TPdynamic/*.h")
        file(GLOB HEADER_MY  "lib/rans/*.h")

	aux_source_directory(lib/c  MY_LIB)
	aux_source_directory(lib/TPdynamic  MY_TP)		
	aux_source_directory(lib/rans  RANS) 

	set(MY_SRC ${MY_LIB} ${MY_TP} ${RANS})		
	add_library(lz16rans STATIC ${MY_SRC}  ${HEADER_MY} )
	target_include_directories(lz16rans PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/c/lz16.h)

file(GLOB HEADER_EXE  "tests/*.h")
set(EXE_SRC tests/main.c  tests/common.c)
add_executable(mylz16  ${EXE_SRC})

target_link_libraries(mylz16 
		PUBLIC lz16rans
		PRIVATE zstd
		PRIVATE lz4
		PRIVATE lzfse
)
