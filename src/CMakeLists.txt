set(SRC_C io_tiff.c io_tiff.h
          io_png.c io_png.h)
 
set(SRC cmdLine.h
        data.cpp
        image.cpp image.h
        kz2.cpp
        main.cpp
        match.cpp match.h
        nan.h
        statistics.cpp)
set(SRC_ENERGY energy/energy.h)
set(SRC_MAXFLOW maxflow/graph.cpp maxflow/graph.h
                maxflow/maxflow.cpp)

find_package(PNG)
find_package(TIFF)

if(NOT PNG_FOUND OR NOT TIFF_FOUND)
    find_package(ZLIB) # zlib is needed by libPNG and by libTIFF
    if(NOT ZLIB_FOUND)
        add_subdirectory(third_party/zlib-1.2.13)
        set(ZLIB_FOUND TRUE)
    endif()
    include_directories(${ZLIB_INCLUDE_DIRS})

    if(NOT PNG_FOUND)
        set(SKIP_INSTALL_ALL 1)
        add_subdirectory(third_party/libpng-1.6.39)
    endif()

    if(NOT TIFF_FOUND)
        find_package(JPEG)
        if(NOT JPEG_FOUND)
            add_subdirectory(third_party/jpeg-9e)
            set(JPEG_FOUND TRUE)
        endif()
        include_directories(${JPEG_INCLUDE_DIR})
        add_subdirectory(third_party/tiff-4.0.10)
    endif()
endif()

include_directories(${PNG_INCLUDE_DIRS})
include_directories(${TIFF_INCLUDE_DIR})

add_definitions(${PNG_DEFINITIONS} -DHAS_PNG)
add_definitions(${TIFF_DEFINITIONS} -DHAS_TIFF)

add_executable(KZ2 ${SRC} ${SRC_ENERGY} ${SRC_MAXFLOW} ${SRC_C})
include_directories(energy maxflow)
target_link_libraries(KZ2 ${TIFF_LIBRARIES} ${PNG_LIBRARIES})

if(UNIX)
    set_source_files_properties(${SRC} PROPERTIES
                                COMPILE_FLAGS "-Wall -Wextra -std=c++98")
    set_source_files_properties(${SRC_C} PROPERTIES
                                COMPILE_FLAGS "-Wall -Wextra -std=c89")
endif()
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()
