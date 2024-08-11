//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Main Sound Engine interface, PS4 specific.

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
/// - \ref soundengine_initialization_advanced_soundengine_using_memory_threshold
struct AkPlatformInitSettings
{
    // Threading model.
    AkThreadProperties  threadLEngine;			///< Lower engine threading properties
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)

    // Memory.
    AkUInt32            uLEngineDefaultPoolSize;///< Lower Engine default memory pool size
	AkReal32            fLEngineDefaultPoolRatioThreshold;	///< 0.0f to 1.0f value: The percentage of occupied memory where the sound engine should enter in Low memory mode. \ref soundengine_initialization_advanced_soundengine_using_memory_threshold
	
	// (SCE_AJM_JOB_INITIALIZE_SIZE*MAX_INIT_SOUND_PER_FRAME) + (SCE_AJM_JOB_RUN_SPLIT_SIZE(4)*MAX_BANK_SRC + (SCE_AJM_JOB_RUN_SPLIT_SIZE(5)*MAX_FILE_SRC
	AkUInt32            uLEngineAcpBatchBufferSize; ///< Lower Engine default memory pool size
	
	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.	
	
	AkThreadProperties  threadMonitor;			///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL). This parameter is not used in Release build.
};

enum AkSinkType
{
	AkSink_Main	= 0,			///< Use the main audio output.
	AkSink_Main_NonRecordable,	///< Use the main audio output, non recordable content.
	AkSink_Voice,				///< Use the PS4 voice channel.
	AkSink_Personal,			///< Use the Personal channel (headset).
	AkSink_PAD,					///< Use the controller speaker channel.
	AkSink_BGM,					///< Output to background music port.
	AkSink_BGM_NonRecordable,	///< Output to background music port, non recordable content.
	AkSink_Dummy,				///< No output.  Used internally.
	AkSink_NumSinkTypes
};
