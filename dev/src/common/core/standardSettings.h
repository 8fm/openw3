/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


/////////////////////////////////////////////////////////////////////////////
// Flag for DEBUG
/////////////////////////////////////////////////////////////////////////////

//#define NO_CUTSCENES		// Disable cutscenes
//#define NO_ITEMS			// Disable items
//#define NO_HEADS			// Disable heads
//#define NO_SLOT_ANIM		// Disable slot animations
//#define NO_MIMIC_ANIM		// Disable mimic animations
//#define NO_MULTI_ANIMS	// Disable multi animation update
//#define NO_MULTI_ASYNC_ANIMS
//#define AI_WIZARD_BACKWARD_COMPAT // Ai wizard temp code will go away soon
//#define DEBUG_CORRUPT_TRANSFORMS	// Debug corrupt transforms for RedQsTransforms ect.
//#define NO_TERRAIN_FOOTSTEP_DATA	// Disable using footstep data from terrain (dynamic decompression)
//#define SOUND_EXTRA_LOGGING		// Enables sounds extended logging

/////////////////////////////////////////////////////////////////////////////
// Global Configuration
/////////////////////////////////////////////////////////////////////////////

#ifdef RED_PLATFORM_WINPC
# define RED_MOD_SUPPORT
#endif

#define RED_ENABLE_STRIPE
#define NO_DEBUG_DATA_SERVICE		//Disable debug data service (QA department), To enable also  NO_TELEMETRY_DEBUG must be disabled
#define USE_EXT_ANIM_EVENTS			// Use animation events from external files
#define NO_ANIM_CACHE				// Do not use animation cache in editor
#define SYNCHRONOUS_MATERIAL_CACHE	// Enable synchronous material/shader cache queries
#define NO_DEFAULT_ANIM				// Disable default animations
#define USE_CAM_ASAP				// Use camera calc asap

#ifndef RED_NETWORK_ENABLED

	#define NO_DEBUG_SERVER

#endif

//#define DISABLE_ALL_PERFCOUNTERS_EXCEPT_FPS
//#define RED_FULL_DETERMINISM
//#define NO_TELEMETRY			// Disable the telemetry logging
//#define NO_TELEMETRY_DEBUG	// Disable telemetry debugging
//#define NO_DEBUG_SERVER		// disable debug server

/////////////////////////////////////////////////////////////////////////////
// Viewport Configuration
/////////////////////////////////////////////////////////////////////////////

#define RED_VIEWPORT_TURN_ON_CACHETS_16_9

/////////////////////////////////////////////////////////////////////////////
// Per Platform Configuration
/////////////////////////////////////////////////////////////////////////////

#ifdef RED_PLATFORM_WINPC

	// PS4 pad for window is now redistributable
	#define RED_INPUT_DEVICE_GAMEPAD_PS4FORPC

	#ifndef RED_FINAL_BUILD
		#ifndef RED_NETWORK_ENABLED
			#define NO_SECOND_SCREEN
		#endif
	#else
		#define NO_STRING_DB
		#define NO_SECOND_SCREEN
	#endif

#define USE_RAW_INPUT_FOR_KEYBOARD_DEVICE	// Use RawInput instead of DirectInput, so we support more keyboard layouts (like UK).
											// We can't use both DirectInput and RawInput for keyboard at once.
											// Note: Editor does not use RawInput. It's safer to use DirectInput there.
	
#endif

#ifdef RED_PLATFORM_CONSOLE
	
	#define NO_HEIGHTMAP_EDIT
	#define NO_TEXTURECACHE_COOKER
	#define NO_NAVMESH_GENERATION
	#define NO_CUBEMAP_GENERATION
	#define NO_TEXTURE_EDITING
	#define NO_MARKER_SYSTEMS
	#define NO_STRING_DB
	#define NO_DEBUG_DATA_SERVICE
	#define NO_TABLET_INPUT_SUPPORT

#endif

#ifdef RED_PLATFORM_DURANGO

	#define RED_KINECT

#endif

#ifdef RED_PLATFORM_ORBIS

	#define NO_TELEMETRY
	#define NO_WINDOWS_INPUT // need a better name, but code needs refactoring
	#define NO_SECOND_SCREEN

#endif

/////////////////////////////////////////////////////////////////////////////
// Per Target Configuration
/////////////////////////////////////////////////////////////////////////////

#ifdef RED_FINAL_BUILD

	#define STATIC_GAME_ONLY static
	#define NO_EDITOR_FRAGMENTS	// No editor fragments generation

	#define GPUAPI_DEBUG 0						// Gpu api debugging OFF
	#define NO_DATA_ASSERTS 					// No Data Asserts
	#define NO_DEBUG_PAGES						// Disable debug pages
	#define NO_DEBUG_WINDOWS					// Disable debug windows
	#define NO_RED_GUI							// Disable redgui
	#define NO_RUNTIME_MATERIAL_COMPILATION		// Disable material compilation
	#define NO_ASYNCHRONOUS_MATERIALS			// Disable spawning jobs to collect materials
	#define NO_FILE_LOADING_STATS				// Disable file loading stats
	#define NO_MARKER_SYSTEMS					// Disable review system
	#define NO_ERROR_STATE						// Disable error state reporting
	//#define NO_PERFCOUNTERS						// Perfcounters disabling
	#define NO_MEMORY_STATS
	#define DISABLE_LOW_MEMORY_WARNINGS			// Low memory warnings - always enabled for now
	#define NO_TELEMETRY_DEBUG 					// Disable telemetry debug
	#define NO_SCRIPT_FUNCTION_CALL_VALIDATION	// Disable runtime checking of calls to script functions (from native code)
	#define NO_UMBRA_DATA_GENERATION 			// Disable UMBRA data building
	#define NO_OBSTACLE_MESH_DATA				// For dynamic generation of static mesh obstacles
	#define NO_CUBEMAP_GENERATION		
	#define NO_TEXTURECACHE_COOKER	
	#define NO_TEXTURE_EDITING					// required for cubemap building right now. Disable only in final.
	#define NO_SCRIPT_DEBUG

	#if !defined( RED_PROFILE_BUILD )
		#define NO_TEST_FRAMEWORK
	#endif

	#if !defined( RED_PROFILE_BUILD ) && !defined( LOG_IN_FINAL )
		#define NO_DEBUG_SERVER					// No game debugger
		#define NO_FREE_CAMERA					// Disable free camera in final but keep it for FinalWithLogging and Profiling builds
	#endif	

#else

	#define STATIC_GAME_ONLY
	#define SOUND_DEBUG		// Enables sounds debugging (debug page, visual debugger)
	#define GPUAPI_DEBUG 1  // Gpu api debugging ON
	#define ENABLE_RESOURCE_MONITORING			// Resource monitor

#endif

/////////////////////////////////////////////////////////////////////////////
// No Editor Configuration
/////////////////////////////////////////////////////////////////////////////

#ifdef NO_EDITOR

	#define NO_TABLET_INPUT_SUPPORT
	#define NO_RESOURCE_IMPORT					
	#define NO_HEIGHTMAP_EDIT		
	#define NO_COMPONENT_GRAPH				
	#define NO_DATA_VALIDATION
	#define NO_EDITOR_WORLD_SUPPORT
	#define NO_EDITOR_GRAPH_SUPPORT
	#define NO_EDITOR_STEERING_SUPPORT
	#define NO_EDITOR_EVENT_SYSTEM
	#define NO_EDITOR_DEBUG_QUEST_SCENES
	#define NO_EDITOR_PROPERTY_SUPPORT
	#define NO_FILE_SOURCE_CONTROL_SUPPORT		
	#define NO_EDITOR_ENTITY_VALIDATION
	#define NO_EDITOR_RESOURCE_SAVE
	#define NO_TERRAIN_EDITOR_STUFF	
	#define NO_UMBRA_DATA_GENERATION				// just in case data is not there
	#define NO_RESOURCE_COOKING	
	#define NO_RUNTIME_WAYPOINT_COOKING

	#ifndef RED_FINAL_BUILD
		#define EDITOR_AI_DEBUG						// Editor AI debugging
	#endif
#else
	
	#define EDITOR_AI_DEBUG						// Editor AI debugging
	#define TERRAIN_TOOL_HAS_OWN_MEMORY_POOL	// Terrain tool has a dedicated memory pool in the editor
	#define SOUND_EDITOR_STUFF					// Enables editor related stuff
	#define DEBUG_CAM_ASAP

#endif
