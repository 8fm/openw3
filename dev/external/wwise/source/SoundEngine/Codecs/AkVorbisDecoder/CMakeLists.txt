project(AkVorbisDecoder)

if (MSVC)
    add_compile_options(/wd4244 /wd4305 /wd4267)
endif()

set(SRC_FILES
    "Common/AkSrcBankVorbis.cpp"
    "Common/AkSrcFileVorbis.cpp"
    "Common/AkVorbisCodebookMgr.cpp"
    "Tremor/FloatingPoint/floor1.cpp"
    "Tremor/FloatingPoint/floor_lookup.cpp"
    "Tremor/FloatingPoint/mdct.cpp"
    "Tremor/bitwise.cpp"
    "Tremor/info.cpp"
    "Tremor/mapping0.cpp"
    "Tremor/packed_codebooks.cpp"
    "Tremor/res012.cpp"
)
if (WIN32)
    set(SYSTEM_INC "Win32")
    list(APPEND SRC_FILES
        "Win32/AkVorbisCodec.cpp"
        "Tremor/Win32/codebook.cpp"
        "Tremor/Win32/dsp.cpp"
    )
else()
    list(APPEND SRC_FILES
        "Tremor/FloatingPoint/AkVorbisCodec.cpp"
        "Tremor/FloatingPoint/codebook.cpp"
    )
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SYSTEM_INC}
    "Common"
    "Tremor"
    "Tremor/Win32"
    "Tremor/FloatingPoint"
    "../../AkAudiolib/Win32"
    "../../AkAudiolib/SoftwarePipeline"
    "../../AkAudiolib/Common"
    "../../../../include"
    "../../Plugins/Effects/Common"
)

target_link_libraries(${PROJECT_NAME})
