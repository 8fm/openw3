project(AkCompressorFX)

set(SRC_FILES
    "AkCompressorFXParams.cpp"
)
if (WIN32)
    list(APPEND SRC_FILES
        "Win32/AkCompressorFX.cpp"
    )
else()
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../Common/Win32"
    "../Common"
    "../../Common"
    "../../../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})
