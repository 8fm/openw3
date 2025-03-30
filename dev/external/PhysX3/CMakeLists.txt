# For all PhysX3

# In physicsIncludes.h Apex and PhysX3 has 4 build types:
# PHYSICS_PROFILE - disabled,
# PHYSICS_DEBUG - used when #ifdef _DEBUG
# PHYSICS_RELEASE - used when #ifdef RED_PROFILE_BUILD or #ifdef NO_EDITOR
# PHYSICS_CHECKED - used in all other cases
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DPX_DEBUG -DPX_CHECKED -DPX_SUPPORT_VISUAL_DEBUGGER)
elseif (${GAME_CONFIG} STREQUAL "Release" OR
        ${GAME_CONFIG} STREQUAL "No opts"
    )
    add_definitions(-DPX_CHECKED -DPX_SUPPORT_VISUAL_DEBUGGER)
else()
    # "Final", "FinalWithLogging", "Profiling". "ReleaseGame"
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

if (MSVC)
else()
    add_compile_options(-std=c++14)

    add_compile_options(-ggdb)
    #add_compile_options(-MMD -Werror)
    add_compile_options(-fPIC -msse2 -mfpmath=sse -ffast-math -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden)
    add_compile_options(-Wall -Wextra -Wstrict-aliasing=2 -fdiagnostics-show-option)
    add_compile_options(-Wno-long-long)
    add_compile_options(-Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-uninitialized -Wno-attributes -Wno-unused-local-typedefs)
    add_compile_options(-Wno-unused-parameter -Wno-missing-field-initializers -Wno-ignored-qualifiers)
    if (${GAME_CONFIG} STREQUAL "Debug")
        add_compile_options(-O0)
    else()
        add_compile_options(-O3 -fno-strict-aliasing)
    endif()

    # Verbose output
    add_compile_options(-v)
    add_link_options(-v)
endif()

add_subdirectory(Source/Common)
add_subdirectory(Source/LowLevel)
add_subdirectory(Source/LowLevelCloth)
add_subdirectory(Source/PhysX)
add_subdirectory(Source/PhysXCharacterKinematic)
add_subdirectory(Source/PhysXCooking)
add_subdirectory(Source/PhysXExtensions)
add_subdirectory(Source/PhysXProfileSDK)
#add_subdirectory(Source/PhysXVehicle)
add_subdirectory(Source/PhysXVisualDebuggerSDK)
add_subdirectory(Source/PvdRuntime)
add_subdirectory(Source/pxtask)
add_subdirectory(Source/SceneQuery)
add_subdirectory(Source/SimulationController)
