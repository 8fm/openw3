//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Main Sound Engine interface, XboxOne-specific.

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

struct IXAudio2;

///< API used for audio output.
enum AkSinkType
{
	AkSink_Main	= 0,		///< Main device.  When initializing, the proper sink is instantiated based on system's capabilities (Wasapi first, then XAudio2).
	AkSink_Main_Wasapi,		///< Main device WASAPI sink.  When initializing, pass this value to force Wwise to use a WASAPI sink.
	AkSink_Main_XAudio2,	///< Main device XAudio2 sink.  When initializing, pass this value to force Wwise to use an XAudio2 sink.
	AkSink_BGM,				///< Background music channel.  Not recorded by DVR.  This sink is currently only compatible with AkSink_Main_XAudio2 main output.
	AkSink_Communication,	///< Communication channel.  Not recorded by DVR.
	AkSink_Dummy,			///< No output.  Used internally.
	AkSink_MergeToMain,		///< Secondary output.  This sink will output to the main sink.
	AkSink_NumSinkTypes
};

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
/// - \ref soundengine_initialization_advanced_soundengine_using_memory_threshold
struct AkPlatformInitSettings
{
    // Threading model.
    AkThreadProperties  threadLEngine;			///< Lower engine threading properties
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)
	AkThreadProperties  threadMonitor;			///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL). This parameter is not used in Release build.

    // Memory.
    AkUInt32            uLEngineDefaultPoolSize;///< Lower Engine default memory pool size
	AkReal32            fLEngineDefaultPoolRatioThreshold;	///< 0.0f to 1.0f value: The percentage of occupied memory where the sound engine should enter in Low memory mode. \ref soundengine_initialization_advanced_soundengine_using_memory_threshold
	AkUInt32            uShapeDefaultPoolSize;	///< SHAPE pool size. When 0 (default), the sound engine estimates the required pool size according to the maximum nomber of XMA voices (uMaxXMAVoices) below.

	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.
	AkUInt16			uMaxXMAVoices;			///< Maximum number of hardware-accelerated XMA voices used at run-time. 

	IXAudio2*			pXAudio2;				///< XAudio2 instance to use for the Wwise sound engine.  If NULL (default) Wwise will initialize its own instance.  Used only if the sink type is XAudio2 in AkInitSettings.eSinkType.
};

namespace AK
{
	//=================================================
	//  Sharing IXAudio2 with the outside world.
	//=================================================

	/// Returns the current, NON-ADDREF'd, XAudio2 interface currently being used by the Wwise SoundEngine.
	/// This should be called only once the SoundEngine has been successfully initialized, otherwise
	/// the function will return NULL.
	///
	/// It is the responsibility of the caller to AddRef (and Release) the interface if there is a risk the 
	/// sound engine be terminated before the caller is done with the interface.
	/// This function is not thread safe with AK::SoundEngine::Init() nor AK::SoundEngine::Term().
	///
	/// XAudio2 allows creating multiple instances of XAudio2 objects, so systems are not forced to 
	/// share the same IXAudio2 object, but can be useful to share some resources.
	///
	/// There can not be more than XAUDIO2_MAX_INSTANCES (8 at the time of writing this) 
	/// simultaneous instances of XAudio2.
	///
	/// \return Non-addref'd pointer to XAudio2 interface. NULL if sound engine is not initialized.
	extern IXAudio2 * GetWwiseXAudio2Interface();
};
