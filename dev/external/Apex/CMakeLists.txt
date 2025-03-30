# For all Apex

# In physicsIncludes.h Apex and PhysX3 has 4 build types:
# PHYSICS_PROFILE - disabled,
# PHYSICS_DEBUG - used when #ifdef _DEBUG
# PHYSICS_RELEASE - used when #ifdef RED_PROFILE_BUILD or #ifdef NO_EDITOR
# PHYSICS_CHECKED - used in all other cases
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DPX_DEBUG -DPX_CHECKED -DPX_SUPPORT_VISUAL_DEBUGGER -DPHYSX_PROFILE_SDK -DPX_PROFILE)
elseif (${GAME_CONFIG} STREQUAL "Release" OR
        ${GAME_CONFIG} STREQUAL "No opts"
    )
    add_definitions(-DPX_CHECKED -DPX_SUPPORT_VISUAL_DEBUGGER)
else()
    # "Final", "FinalWithLogging", "Profiling". "ReleaseGame"
    add_definitions(-DAPEX_SHIPPING)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

if (MSVC)
else()
    add_compile_options(-ggdb)
    #add_compile_options(-MMD)
    add_compile_options(-msse2 -mfpmath=sse -fpic -ffast-math -fno-exceptions -fno-rtti -Wno-invalid-offsetof -Wno-unknown-pragmas -std=c++0x)
    if (${GAME_CONFIG} STREQUAL "Debug")
        add_compile_options(-O0)
    else()
        add_compile_options(-O3 -fno-strict-aliasing)
    endif()

    # Verbose output
    add_compile_options(-v)
    add_link_options(-v)
endif()

set(APEX_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(common)
add_subdirectory(framework)
add_subdirectory(module/clothing)
add_subdirectory(module/destructible)
add_subdirectory(module/legacy)
add_subdirectory(shared)
