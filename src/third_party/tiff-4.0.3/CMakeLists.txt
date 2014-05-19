project(TIFF)

set(TIFF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libtiff PARENT_SCOPE)
set(TIFF_LIBRARIES tiff ${JPEG_LIBRARIES} PARENT_SCOPE)

# library object files common to compression and decompression
set(common_SRCS
  libtiff/tif_aux.c
  libtiff/tif_close.c
  libtiff/tif_codec.c
  libtiff/tif_color.c
  libtiff/tif_compress.c
  libtiff/tif_dir.c
  libtiff/tif_dirinfo.c
  libtiff/tif_dirread.c
  libtiff/tif_dirwrite.c
  libtiff/tif_dumpmode.c
  libtiff/tif_error.c
  libtiff/tif_extension.c
  libtiff/tif_fax3.c
  libtiff/tif_flush.c
  libtiff/tif_getimage.c
  libtiff/tif_jbig.c
  libtiff/tif_jpeg_12.c
  libtiff/tif_jpeg.c
  libtiff/tif_luv.c
  libtiff/tif_lzma.c
  libtiff/tif_lzw.c
  libtiff/tif_next.c
  libtiff/tif_ojpeg.c
  libtiff/tif_open.c
  libtiff/tif_packbits.c
  libtiff/tif_pixarlog.c
  libtiff/tif_predict.c
  libtiff/tif_print.c
  libtiff/tif_read.c
  libtiff/tif_strip.c
  libtiff/tif_swab.c
  libtiff/tif_tile.c
  libtiff/tif_thunder.c
  libtiff/tif_version.c
  libtiff/tif_warning.c
  libtiff/tif_write.c
  libtiff/tif_zip.c
  )

add_definitions(-DJPEG_SUPPORT -DZIP_SUPPORT -D_HPUX_SOURCE)

if(WIN32)
  if(BUILD_SHARED_LIBS)
    set(TIFFDLL 1)
  else(BUILD_SHARED_LIBS)
    set(TIFFSTATIC 1)
  endif(BUILD_SHARED_LIBS)
endif(WIN32)

if(BORLAND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-8004 -w-8057")
endif(BORLAND)

include(TestBigEndian)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)

# Check if header file exists and add it to the list.
CHECK_INCLUDE_FILES("assert.h" HAVE_ASSERT_H)
CHECK_INCLUDE_FILES("dlfcn.h" HAVE_DLFCN_H)
CHECK_INCLUDE_FILES("fcntl.h" HAVE_FCNTL_H)
CHECK_INCLUDE_FILES("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES("limits.h" HAVE_LIMITS_H)
CHECK_INCLUDE_FILES("malloc.h" HAVE_MALLOC_H)
CHECK_INCLUDE_FILES("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILES("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILES("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILES("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILES("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILES("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILES("windows.h" HAVE_WINDOWS_H)
CHECK_INCLUDE_FILES("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILES("ieeefp.h" HAVE_IEEEFP_H)

CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(getopt HAVE_GETOPT)
CHECK_FUNCTION_EXISTS(isascii HAVE_ISASCII)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(pow HAVE_POW)
CHECK_FUNCTION_EXISTS(sqrt HAVE_SQRT)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(strtoul HAVE_STRTOUL)

include(CheckTypeSize)

CHECK_TYPE_SIZE("signed int" SIZEOF_SIGNED_INT)
CHECK_TYPE_SIZE("signed long" SIZEOF_SIGNED_LONG)
CHECK_TYPE_SIZE("signed long long" SIZEOF_SIGNED_LONG_LONG)
CHECK_TYPE_SIZE("unsigned char*" SIZEOF_POINTER)

if(SIZEOF_SIGNED_INT STREQUAL "4")
  set(INT32_T "signed int")
  set(UINT32_T "unsigned int")
elseif(SIZEOF_SIGNED_LONG STREQUAL "4")
  set(INT32_T "signed long")
  set(UINT32_T "unsigned long")
endif(SIZEOF_SIGNED_INT STREQUAL "4")

if(SIZEOF_SIGNED_LONG STREQUAL "8")
  set(INT64_T "signed long")
  set(UINT64_T "unsigned long")
  set(UINT64_FORMAT "%lu")
elseif(SIZEOF_SIGNED_LONG_LONG STREQUAL "8")
  set(INT64_T "signed long long")
  set(UINT64_T "unsigned long long")
  set(UINT64_FORMAT "%llu")
endif(SIZEOF_SIGNED_LONG STREQUAL "8")

if(SIZEOF_SIGNED_LONG STREQUAL SIZEOF_POINTER)
  set(SSIZE_T "signed long")
elseif(SIZEOF_SIGNED_LONG_LONG STREQUAL SIZEOF_POINTER)
  set(SSIZE_T "signed long long")
endif(SIZEOF_SIGNED_LONG STREQUAL SIZEOF_POINTER)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(i.86.*)|x86_64")
    set(FILLORDER "FILLORDER_LSB2MSB")
else(CMAKE_SYSTEM_PROCESSOR MATCHES "(i.86.*)|x86_64")
    set(FILLORDER "FILLORDER_MSB2LSB")
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "(i.86.*)|x86_64")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tif_config.h.cmakein
               ${CMAKE_CURRENT_SOURCE_DIR}/libtiff/tif_config.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tiffconf.h.cmakein
               ${CMAKE_CURRENT_SOURCE_DIR}/libtiff/tiffconf.h @ONLY)

if(NOT UNIX)
  add_definitions( -DTIF_PLATFORM_CONSOLE )
  set(common_SRCS ${common_SRCS} libtiff/tif_win32.c libtiff/tif_fax3sm.c)
else(NOT UNIX)
  set(TIFF_RIGHT_VERSION 1)
  set(common_SRCS ${common_SRCS} libtiff/tif_unix.c libtiff/tif_fax3sm.c)
  set_source_files_properties(tif_fax3sm.c GENERATED)
endif(NOT UNIX)

add_library(tiff ${common_SRCS})
target_link_libraries(tiff ${JPEG_LIBRARIES})

if(UNIX)
  target_link_libraries(tiff -lm)
endif(UNIX)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)
