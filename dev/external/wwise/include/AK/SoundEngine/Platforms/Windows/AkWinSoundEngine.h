//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkWinSoundEngine.h

/// \file 
/// Main Sound Engine interface, specific WIN32.

#ifndef _AK_WIN_SOUND_ENGINE_H_
#define _AK_WIN_SOUND_ENGINE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

/// Sound quality option
/// (available in the PC version only)
enum AkSoundQuality
{
	AkSoundQuality_High,	///< High quality (48 kHz sampling rate, default value)
	AkSoundQuality_Low,		///< Reduced quality (24 kHz sampling rate)
};

///< API used for audio output (PC only).
enum AkSinkType
{
	AkSink_Main	= 0,		///< Main device.  When initializing, the proper sink is instantiated based on system's capabilities (XAudio2 first, then DirectSound).
	AkSink_Main_XAudio2,	///< Main device XAudio2 sink (better performance on Windows post-Vista).  When initializing, pass this value to force Wwise to use an XAudio2 sink.
	AkSink_Main_DirectSound,///< Main device DirectSound sink (for backward compatibility).  When initializing, pass this value to force Wwise to use a DirectSound sink.
	AkSink_Dummy,			///< No output.  Used internally.
	AkSink_MergeToMain,		///< Secondary output.  This sink will output to the main sink.
	AkSink_NumSinkTypes
};

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
/// - \ref soundengine_initialization_advanced_soundengine_using_memory_threshold

struct IXAudio2;
struct AkPlatformInitSettings
{
    // Direct sound.
    HWND			    hWnd;					///< Handle to the window associated to the audio.
												///< Each game must specify the HWND that will be passed to DirectSound initialization.
												///< The value returned by GetDefaultPlatformInitSettings is the foreground HWND at 
												///< the moment of the initialization of the sound engine and may not be the correct one for your game.
												///< It is required that each game provides the correct HWND to be used.
									

    // Threading model.
    AkThreadProperties  threadLEngine;			///< Lower engine threading properties
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)
	AkThreadProperties  threadMonitor;			///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL). This parameter is not used in Release build.

    // Memory.
    AkUInt32            uLEngineDefaultPoolSize;///< Lower Engine default memory pool size
	AkReal32            fLEngineDefaultPoolRatioThreshold;	///< 0.0f to 1.0f value: The percentage of occupied memory where the sound engine should enter in Low memory mode. \ref soundengine_initialization_advanced_soundengine_using_memory_threshold

	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.
	AkSoundQuality		eAudioQuality;			///< Quality of audio processing, default = AkSoundQuality_High.
	
	bool				bGlobalFocus;			///< Corresponding to DSBCAPS_GLOBALFOCUS. If using the DirectSound sink type, sounds will be muted if set to false when the game loses the focus.
												///< This setting is ignored when using other sink types.

	IXAudio2*			pXAudio2;				///< XAudio2 instance to use for the Wwise sound engine.  If NULL (default) Wwise will initialize its own instance.  Used only if the sink type is XAudio2 in AkInitSettings.eSinkType.
};

struct IDirectSound8;
struct IXAudio2;

namespace AK
{
	/// Get instance of XAudio2 created by the sound engine at initialization.
	/// \return Non-addref'd pointer to XAudio2 interface. NULL if sound engine is not initialized or XAudio2 is not used.
	AK_EXTERNAPIFUNC( IXAudio2 *, GetWwiseXAudio2Interface )();

	/// Get instance of DirectSound created by the sound engine at initialization.
	/// \return Non-addref'd pointer to DirectSound interface. NULL if sound engine is not initialized or DirectSound is not used.
	AK_EXTERNAPIFUNC( IDirectSound8 *, GetDirectSoundInstance )();
};

#endif //_AK_WIN_SOUND_ENGINE_H_
