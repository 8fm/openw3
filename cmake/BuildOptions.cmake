#Shared2022.props

add_definitions(-D_UNICODE -DUNICODE -DPROJECT_CONFIGURATION=${GAME_CONFIG} -DPROJECT_PLATFORM=${GAME_PLATFORM})

if (${GAME_CONFIG} STREQUAL "Final")
    add_definitions(-DRED_FINAL_BUILD -DRED_CONFIGURATION_FINAL -DNDEBUG -DNO_SECUROM -DNO_EDITOR -DRELEASE)
elseif (${GAME_CONFIG} STREQUAL "Profiling")
    add_definitions(-DRED_FINAL_BUILD -DRED_CONFIGURATION_RELEASE -DNDEBUG -DNO_SECUROM -DNO_EDITOR -DRELEASE -DRED_PROFILE_BUILD)
elseif (${GAME_CONFIG} STREQUAL "FinalWithLogging")
    add_definitions(-DRED_FINAL_BUILD -DRED_CONFIGURATION_FINAL -DNDEBUG -DNO_SECUROM -DNO_EDITOR -DRELEASE -DLOG_IN_FINAL)
elseif (${GAME_CONFIG} STREQUAL "Release")
    add_definitions(-DRED_CONFIGURATION_RELEASE -DNDEBUG -DNO_SECUROM -DRELEASE)
elseif (${GAME_CONFIG} STREQUAL "No opts")
    add_definitions(-DRED_CONFIGURATION_NOPTS -DNDEBUG -DNO_SECUROM)
elseif (${GAME_CONFIG} STREQUAL "Debug")
    add_definitions(-DRED_CONFIGURATION_DEBUG -D_DEBUG -DNO_EDITOR)
elseif (${GAME_CONFIG} STREQUAL "ReleaseGame")
    add_definitions(-DRED_CONFIGURATION_RELEASEGAME -DNDEBUG -DNO_SECUROM -DNO_EDITOR -DRELEASE)
endif()

if (${GAME_PLATFORM} STREQUAL "x64")
    add_definitions(-D_WIN64 -D_WINDOWS -DWIN32_LEAN_AND_MEAN)
endif()

if (${TARGET_TYPE} STREQUAL "StaticLibrary")
    add_definitions(-D_LIB)
endif()

#SharedItemGroups2022.props
if (MSVC)
    add_compile_options(/W3) # WarningLevel = 3
    add_compile_options(/WX-) # TreatWarningAsError = false
    add_compile_options(/Gm-) # MinimalRebuild = false

    if (${GAME_CONFIG} STREQUAL "Debug")
        add_compile_options(/Od) # Optimization = Disabled
        add_compile_options(/RTC1) # BasicRuntimeChecks = EnableFastChecks
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug") # instead of add_compile_options(/MTd)
        add_compile_options(/fp:fast) # FloatingPointModel = Fast
        add_compile_options(/Zi) # DebugInformationFormat = ProgramDatabase (separate PDB file)
        if (${TARGET_TYPE} STREQUAL "Application")
            add_link_options(/NODEFAULTLIB:msvcrtd) # IgnoreSpecificDefaultLibraries
            add_link_options(/NODEFAULTLIB:libcmt) # IgnoreSpecificDefaultLibraries
            add_link_options(/STACK:33554432) # StackReserveSize
        endif()
    elseif (${GAME_CONFIG} STREQUAL "No opts")
        #TODO add options
    else()
        add_compile_options(/Ox) # Optimization = Full
        add_compile_options(/Ob2) # InlineFunctionExpansion  = AnySuitable
        add_compile_options(/Oi) # IntrinsicFunctions = true
        add_compile_options(/Ot) # FavorSizeOrSpeed = Speed
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded") # instead of add_compile_options(/MT)
        add_compile_options(/fp:fast) # FloatingPointModel = Fast
        add_compile_options(/Zi) # DebugInformationFormat = ProgramDatabase (separate PDB file)
        add_compile_options(/openmp) # OpenMPSupport = true
        # BasicRuntimeChecks = Default (without /RTC)
        add_compile_options(/GS-) # BufferSecurityCheck = false
        if (${TARGET_TYPE} STREQUAL "Application")
            add_link_options(/OPT:REF) # OptimizeReferences = true
            add_link_options(/OPT:ICF) # EnableCOMDATFolding = true
            add_link_options(/NODEFAULTLIB:msvcrt) # IgnoreSpecificDefaultLibraries
            add_link_options(/STACK:33554432) # StackReserveSize
        endif()
    endif()
endif()

#Shared.targets
#TODO add RTTIVerify
if (MSVC)
    add_compile_options(/Gd) # CallingConvention = Cdecl
    add_compile_options(/errorReport:prompt) # ErrorReporting = Prompt
    add_compile_options(/Zc:forScope) # ForceConformanceInForLoopScope = true
    add_compile_options(/diagnostics:column) # DiagnosticsFormat = Column
    add_compile_options(/Zc:inline) # RemoveUnreferencedCodeData = true
    add_compile_options(/Zc:wchar_t) # TreatWChar_tAsBuiltInType = true
    add_compile_options(/FC) # UseFullPaths = true
endif()
