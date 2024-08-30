
if (WIN32)
    set(BISON_BIN ${CMAKE_SOURCE_DIR}/dev/external/bison/bin/bison.exe)
    set(BISON_GENERATION_DIR ${CMAKE_SOURCE_DIR}/dev/external/bison/bin)
else()
    set(BISON_BIN bison)
    set(BISON_GENERATION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(OUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${GAME_PLATFORM}/${GAME_CONFIG})
foreach(BISON_FILE ${BISON_FILES})
    string(FIND "${BISON_FILE}" ":" pos)
    if (pos LESS 1)
        set(BISON_SRC ${BISON_FILE})
        set(BISON_ARGS "")
        #message("${BISON_FILE} has no arguments")
    else()
        string(SUBSTRING "${BISON_FILE}" 0 "${pos}" BISON_SRC)
        math(EXPR pos "${pos} + 1")  # Skip the separator
        string(SUBSTRING "${BISON_FILE}" "${pos}" -1 BISON_ARGS)
        #message("${BISON_SRC} has args ${BISON_ARGS}")
    endif()
    get_filename_component(OUT_NAME ${BISON_SRC} NAME_WE)
    if(NOT EXISTS "${OUT_PATH}/${OUT_NAME}_bison.cxx.h")
        set(OUT_CXX_H ${OUT_PATH}/${OUT_NAME}_bison.cxx.h)
        set(OUT_CXX ${OUT_PATH}/${OUT_NAME}_bison.cxx)
        message("Custom command: ${BISON_BIN} ${BISON_ARGS} --defines=${OUT_CXX_H} -o ${OUT_CXX} ${CMAKE_CURRENT_SOURCE_DIR}/${BISON_SRC}")
        add_custom_command(
            OUTPUT "${OUT_CXX_H}"
                   "${OUT_CXX}"
            COMMAND "${BISON_BIN}" ${BISON_ARGS} --defines="${OUT_CXX_H}" -o "${OUT_CXX}" "${CMAKE_CURRENT_SOURCE_DIR}/${BISON_SRC}"
            WORKING_DIRECTORY "${BISON_GENERATION_DIR}"
        )
        list(APPEND SRC_FILES "${OUT_CXX_H}")
    endif()
endforeach()
