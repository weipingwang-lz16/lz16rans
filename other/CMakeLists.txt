cmake_minimum_required(VERSION 3.16.1)
Project(lzOther)

enable_language(ASM)

	file(GLOB HEADER_LZFSE "lzfse/src/*.h")
	file(GLOB HEADER_LZ4 "lz4/lib/*.h")
	file(GLOB HEADER_ZSTD "zstd/lib/*.h" "zstd/lib/common/*.h" "zstd/lib/compress/*.h" "zstd/lib/decompress/*.h")
		
	aux_source_directory(lzfse/src  LZFSE)		
	aux_source_directory(lz4/lib  LZ4)
	aux_source_directory(zstd/lib ZSTD)
	aux_source_directory(zstd/lib/common ZSTD)
	aux_source_directory(zstd/lib/compress ZSTD)
	aux_source_directory(zstd/lib/decompress ZSTD)
        set(ZSTDSRC zstd/lib/decompress/huf_decompress_amd64.S ${ZSTD})

list(REMOVE_ITEM ${LZFSE} main.c)

	add_library(lzfse STATIC ${LZFSE} ${HEADER_LZFSE} )
	add_library(lz4 STATIC ${LZ4} ${HEADER_LZ4} )
	add_library(zstd STATIC ${ZSTDSRC} ${HEADER_ZSTD} )


	target_include_directories(lzfse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lzfse/src/lzfse.h)
	target_include_directories(lz4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/zstd/lib/zstd.h)
	target_include_directories(zstd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lz4/lib/lz4.h)
