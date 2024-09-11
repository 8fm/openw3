# Common

add_definitions(-DSPEEDTREE_EXPOSE_REGISTRY_FUNCTIONS -D_MBCS)
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DNDEBUG)
else()
    add_definitions(-D_DEBUG)
endif()

if (WIN32)
    add_definitions(-DWIN32 -D_WINDOWS)
endif()

if (MSVC)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        add_compile_options(/GS /W4 /Gy /Zc:wchar_t /Zi /Gm- /O2 /Ob2 /Zc:inline /fp:precise /errorReport:prompt /GF /WX- /Zc:forScope /Gd /MT /openmp /FC /EHsc /nologo /diagnostics:column /MP)
    else()
        add_compile_options(/GS /W4 /Zc:wchar_t /Zi /Gm- /Od /Zc:inline /fp:precise /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MTd /FC /EHsc /nologo /diagnostics:column /MP)
    endif()
else()
    add_compile_options(-std=c++14)

    add_compile_options(-fPIC -ffast-math -finput-charset=UTF-8 -fshort-wchar)
    add_compile_options(-ffunction-sections -fdata-sections)
    add_compile_options(-mfpmath=sse -msse3)

    # Verbose output
    add_compile_options(-v)
    add_link_options(-v)
endif()

add_subdirectory(Core/cmake)
add_subdirectory(Forest/cmake)
