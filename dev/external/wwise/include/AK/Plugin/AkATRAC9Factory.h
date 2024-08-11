//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkATRAC9Factory.h

/// \file
/// Codec plug-in unique ID and creation functions (hooks) necessary to register the Sony ATRAC9 codec in the sound engine.
/// \warning This plug-in is only implemented on Sony Vita. Do not refer to these functions on other platforms.

#ifndef _AK_ATRAC9FACTORY_H_
#define _AK_ATRAC9FACTORY_H_

#include <AK/AkPlatforms.h>
#include <AK/SoundEngine/Common/AkSoundEngineExport.h>

class IAkSoftwareCodec;
/// Prototype of the ATRAC9 codec bank source creation function.
AK_FUNC( IAkSoftwareCodec*, CreateATRAC9BankPlugin )( 
	void* in_pCtx			///< Bank source decoder context
	);

/// Prototype of the ATRAC9 codec file source creation function.
AK_FUNC( IAkSoftwareCodec*, CreateATRAC9FilePlugin )( 
	void* in_pCtx 			///< File source decoder context
	);

#if defined( AK_VITA )

#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <audiodec.h>

#define AK_ATRAC9_DEFAULT_MAX_CHANNELS		SCE_AUDIODEC_AT9_MAX_CH_IN_LIBRARY

namespace AK
{
	namespace ATRAC9
	{
		struct InitSettings
		{
			/// Initialize the sceAudiodec for ATRAC9. Set to false only if you know this
			/// is done elsewhere in your game (must initialized before AK::ATRAC9::Init(),
			/// and terminated after AK::ATRAC9::Term()).
			bool				bInitAudiodec;

			/// Maximum number of channels to be decoded simultaneously (between 1 and
			/// SCE_AUDIODEC_AT9_MAX_CH_IN_LIBRARY). Relevant only if bInitAudiodec is true.
			AkUInt32			unMaxChannels;

			/// Number of decoding threads (between 1 and AK_ATRAC9_MAX_DECODING_THREADS)
			AkUInt32			unDecodingThreadCount;

			/// Properties of the AK ATRAC9 decoding threads
			AkThreadProperties  decodingThreadProperties;
		};

		/// Gets default initialization settings for AK::ATRAC9::Init().
		void GetDefaultInitSettings(
			InitSettings& out_settings ///< Default init settings
		);

		/// Initializes the ATRAC9 sound engine plug-in. This must be done 
		/// after initializing the sound engine.
		///
		/// \return See return codes for sceAudiodecInitLibrary in the
		/// Vita SDK documentation
		AKRESULT Init(
			InitSettings* in_pSettings = NULL ///< ATRAC9 settings. If NULL, default settings will be used.
		);

		/// Terminates the ATRAC9 sound engine plug-in. This must
		/// be done before terminating the sound engine.
		///
		/// \return See return codes for sceAudiodecTermLibrary in the
		/// Vita SDK documentation
		void Term();
	}
}

#endif // defined( AK_VITA )

/*
Use the following code to register this codec, after the sound engine has been initialized:

	AK::ATRAC9::Init(); // Or pass customized init settings

	AK::SoundEngine::RegisterCodec(
		AKCOMPANYID_AUDIOKINETIC,
		AKCODECID_ATRAC9,
		CreateATRAC9FilePlugin,
		CreateATRAC9BankPlugin );

Then before terminating the sound engine:

	AK::ATRAC9::Term();

*/

#endif // _AK_ATRAC9FACTORY_H_
