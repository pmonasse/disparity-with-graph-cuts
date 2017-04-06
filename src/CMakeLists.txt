SET(SRC_C io_tiff.c io_tiff.h
          io_png.c io_png.h)
 
SET(SRC cmdLine.h
        data.cpp
        image.cpp image.h
        kz2.cpp
        main.cpp
        match.cpp match.h
        nan.h
        statistics.cpp)
SET(SRC_ENERGY energy/energy.h)
SET(SRC_MAXFLOW maxflow/graph.cpp maxflow/graph.h
                maxflow/maxflow.cpp)

FIND_PACKAGE(PNG)
FIND_PACKAGE(TIFF)

IF(NOT PNG_FOUND)
    FIND_PACKAGE(ZLIB)
    IF(NOT ZLIB_FOUND)
        ADD_SUBDIRECTORY(third_party/zlib-1.2.11)
    ENDIF(NOT ZLIB_FOUND)
    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
    ADD_SUBDIRECTORY(third_party/libpng-1.6.29)
ENDIF(NOT PNG_FOUND)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})

IF(NOT TIFF_FOUND)
    FIND_PACKAGE(JPEG)
    IF(NOT JPEG_FOUND)
        ADD_SUBDIRECTORY(third_party/jpeg-9b)
    ENDIF(NOT JPEG_FOUND)
    INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
    ADD_SUBDIRECTORY(third_party/tiff-4.0.7)
ENDIF(NOT TIFF_FOUND)
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})

ADD_DEFINITIONS(${PNG_DEFINITIONS} -DHAS_PNG)
ADD_DEFINITIONS(${TIFF_DEFINITIONS} -DHAS_TIFF)

ADD_EXECUTABLE(KZ2 ${SRC} ${SRC_ENERGY} ${SRC_MAXFLOW} ${SRC_C})
INCLUDE_DIRECTORIES(energy maxflow)
TARGET_LINK_LIBRARIES(KZ2 ${TIFF_LIBRARIES} ${PNG_LIBRARIES})

IF(UNIX)
    SET_SOURCE_FILES_PROPERTIES(${SRC} PROPERTIES
                                COMPILE_FLAGS "-Wall -Wextra -Werror -std=c++98")
    SET_SOURCE_FILES_PROPERTIES(${SRC_C} PROPERTIES
                                COMPILE_FLAGS "-Wall -Wextra -Werror -std=c89")
ENDIF(UNIX)
IF(MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
ENDIF(MSVC)
