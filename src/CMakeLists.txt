SET(SRC cmdLine.h
        data.cpp
        image.cpp image.h
        io_png.c io_png.h
        io_tiff.c io_tiff.h
        kz2.cpp
        main.cpp
        match.cpp match.h
        nan.h
        statistics.cpp)
SET(SRC_ENERGY energy/energy.h)
SET(SRC_MAXFLOW maxflow/block.h
                maxflow/graph.cpp maxflow/graph.h
                maxflow/maxflow.cpp)

FIND_PACKAGE(PNG REQUIRED)
ADD_DEFINITIONS(${PNG_DEFINITIONS})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})

FIND_PACKAGE(TIFF REQUIRED)
ADD_DEFINITIONS(${TIFF_DEFINITIONS})
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})

ADD_EXECUTABLE(KZ2 ${SRC} ${SRC_ENERGY} ${SRC_MAXFLOW})
INCLUDE_DIRECTORIES(energy maxflow)
TARGET_LINK_LIBRARIES(KZ2 ${PNG_LIBRARIES} ${TIFF_LIBRARIES})

IF(UNIX)
    SET_TARGET_PROPERTIES(KZ2 PROPERTIES
                          COMPILE_FLAGS "-ansi -Wall -Wextra -Werror")
ENDIF(UNIX)

INSTALL(FILES ${SRC} CMakeLists.txt DESTINATION src)
INSTALL(FILES ${SRC_ENERGY} energy/README.TXT DESTINATION src/energy)
INSTALL(FILES ${SRC_MAXFLOW}
              maxflow/CHANGES.TXT maxflow/LICENSE.TXT maxflow/README.TXT
        DESTINATION src/maxflow)
