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
IF(PNG_FOUND)
    ADD_DEFINITIONS(${PNG_DEFINITIONS} -DHAS_PNG)
    INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
    SET(SRC ${SRC} io_png.c io_png.h)
ENDIF(PNG_FOUND)

FIND_PACKAGE(TIFF)
IF(TIFF_FOUND)
    ADD_DEFINITIONS(${TIFF_DEFINITIONS} -DHAS_TIFF)
    INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
    SET(SRC ${SRC} io_tiff.c io_tiff.h)
ENDIF(TIFF_FOUND)

ADD_EXECUTABLE(KZ2 ${SRC} ${SRC_ENERGY} ${SRC_MAXFLOW})
INCLUDE_DIRECTORIES(energy maxflow)
TARGET_LINK_LIBRARIES(KZ2 ${PNG_LIBRARIES} ${TIFF_LIBRARIES})

IF(UNIX)
    SET_TARGET_PROPERTIES(KZ2 PROPERTIES COMPILE_FLAGS
                          "-ansi -Wall -Wextra -Werror")
ENDIF(UNIX)
IF(MSVC)
     ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
ENDIF(MSVC)

INSTALL(FILES ${SRC} CMakeLists.txt DESTINATION src)
INSTALL(FILES ${SRC_ENERGY} energy/README.TXT DESTINATION src/energy)
INSTALL(FILES ${SRC_MAXFLOW} maxflow/README.TXT DESTINATION src/maxflow)
