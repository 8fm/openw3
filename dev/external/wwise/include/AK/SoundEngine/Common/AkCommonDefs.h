//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkCommonDefs.h

/// \file 
/// AudioLib common defines, enums, and structs.


#ifndef _AK_COMMON_DEFS_H_
#define _AK_COMMON_DEFS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSpeakerConfig.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

//-----------------------------------------------------------------------------
// AUDIO DATA FORMAT
//-----------------------------------------------------------------------------

// Maximum number of channel supported for a voice.
#if defined(AK_71AUDIO)
#define AK_VOICE_MAX_NUM_CHANNELS	(8) ///< Platform supports 7.1
#define AK_SUPPORTED_SPEAKER_SETUP	(AK_SPEAKER_SETUP_7POINT1)	///< Most complete speaker configuration supported on this platform.
#elif defined(AK_LFECENTER) && defined(AK_REARCHANNELS)
#define AK_VOICE_MAX_NUM_CHANNELS	(6) ///< Platform supports 5.1
#define AK_SUPPORTED_SPEAKER_SETUP	(AK_SPEAKER_SETUP_5POINT1)	///< Most complete speaker configuration supported on this platform.
#elif defined(AK_REARCHANNELS)
	#ifdef AK_WII
		#define AK_VOICE_MAX_NUM_CHANNELS	(2) ///< Platform supports stereo input
		#define AK_SUPPORTED_SPEAKER_SETUP	(AK_SPEAKER_SETUP_DPL2)	///< Most complete speaker configuration supported on this platform.
	#else
		#define AK_VOICE_MAX_NUM_CHANNELS	(4) ///< Platform supports 4.0
		#define AK_SUPPORTED_SPEAKER_SETUP	(AK_SPEAKER_SETUP_4)	///< Most complete speaker configuration supported on this platform.
	#endif
#else 
#define AK_VOICE_MAX_NUM_CHANNELS	(2) ///< Platform is stereo
#define AK_SUPPORTED_SPEAKER_SETUP	(AK_SPEAKER_SETUP_STEREO)	///< Most complete speaker configuration supported on this platform.
#endif

// Channel mask helpers.
namespace AK
{
	/// Returns the number of channels of a given channel configuration.
	/// \return Number of channels.
	AkForceInline AkUInt32 GetNumChannels( AkChannelMask in_uChannelMask )
	{
		AkUInt32 num = 0;
		while( in_uChannelMask ){ ++num; in_uChannelMask &= in_uChannelMask-1; } // iterate max once per channel.
		return num;
	}

	/// Returns true when the LFE channel is present in a given channel configuration.
	/// \return True if the LFE channel is present.
	AkForceInline bool HasLFE( AkChannelMask in_uChannelMask )
	{ 
		return ( in_uChannelMask & AK_SPEAKER_LOW_FREQUENCY ) > 0; 
	}

	/// Returns true when the center channel is present in a given channel configuration.
	/// Note that mono configurations have one channel which is arbitrary set to AK_SPEAKER_FRONT_CENTER,
	/// so HasCenter() returns true for mono signals.
	/// \return True if the center channel is present.
	AkForceInline bool HasCenter( AkChannelMask in_uChannelMask )
	{ 
		// All supported non-mono configurations have an AK_SPEAKER_FRONT_LEFT.
		return ( in_uChannelMask & AK_SPEAKER_FRONT_CENTER ) > 0; 
	}

	/// Returns the number of angle values required to represent the given channel configuration.
	/// \sa AK::SoundEngine::SetSpeakerAngles().
	AkForceInline AkUInt32 GetNumberOfAnglesForConfig( AkChannelMask in_uChannelMask )
	{
		// LFE is irrelevant.
		in_uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;
		// Center speaker is always in the center and thus does not require an angle.
		in_uChannelMask &= ~AK_SPEAKER_FRONT_CENTER;
		// We should have complete pairs at this point, unless there is a speaker at 180 degrees, 
		// in which case we need one more angle to specify it.
		AKASSERT( ( in_uChannelMask & AK_SPEAKER_BACK_CENTER ) || ( ( GetNumChannels( in_uChannelMask ) % 2 ) == 0 ) );
		return GetNumChannels( in_uChannelMask ) >> 1;
	}
}

// Audio data format.
// ------------------------------------------------

const AkDataTypeID		AK_INT				= 0;		///< Integer data type (uchar, short, and so on)
const AkDataTypeID		AK_FLOAT			= 1;		///< Float data type

const AkDataInterleaveID AK_INTERLEAVED		= 0;		///< Interleaved data
const AkDataInterleaveID AK_NONINTERLEAVED	= 1;		///< Non-interleaved data

// Native format currently the same on all supported platforms, may become platform specific in the future
const AkUInt32 AK_LE_NATIVE_BITSPERSAMPLE  = 32;					///< Native number of bits per sample.
const AkUInt32 AK_LE_NATIVE_SAMPLETYPE = AK_FLOAT;					///< Native data type.
const AkUInt32 AK_LE_NATIVE_INTERLEAVE = AK_NONINTERLEAVED;			///< Native interleaved setting.

/// Defines the parameters of an audio buffer format.
struct AkAudioFormat
{
    AkUInt32	uSampleRate;		///< Number of samples per second

	AkUInt32	uChannelMask	:18;///< Channel mask (configuration). 
	AkUInt32	uBitsPerSample	:6; ///< Number of bits per sample.
	AkUInt32	uBlockAlign		:5; ///< Number of bytes per sample frame. 
	AkUInt32	uTypeID			:2; ///< Data type ID (AkDataTypeID). 
	AkUInt32	uInterleaveID	:1; ///< Interleave ID (AkDataInterleaveID). 

	/// Get the channel mask.
	/// \return The number of channels
	AkForceInline AkChannelMask GetChannelMask() const		
	{
		return uChannelMask;
	}
	
	/// Get the number of channels.
	/// \return The number of channels
	AkForceInline AkUInt32 GetNumChannels() const
	{
		return AK::GetNumChannels( uChannelMask );
	}

	/// Query if LFE channel is present.
	/// \return True when LFE channel is present
	AkForceInline bool HasLFE() const
	{ 
		return AK::HasLFE( uChannelMask ); 
	}

	/// Query if center channel is present.
	/// Note that mono configurations have one channel which is arbitrary set to AK_SPEAKER_FRONT_CENTER,
	/// so HasCenter() returns true for mono signals.
	/// \return True when center channel is present and configuration has more than 2 channels.
	AkForceInline bool HasCenter() const
	{ 
		return AK::HasCenter( uChannelMask ); 
	}

	/// Get the number of bits per sample.
	/// \return The number of bits per sample
	AkForceInline AkUInt32 GetBitsPerSample()	const						
	{ 
		return uBitsPerSample;
	}

	/// Get the block alignment.
	/// \return The block alignment
	AkForceInline AkUInt32 GetBlockAlign() const
	{
		return uBlockAlign;
	}

	/// Get the data sample format (Float or Integer).
	/// \return The data sample format
	AkForceInline AkUInt32 GetTypeID() const
	{
		return uTypeID;
	}

	/// Get the interleaved type.
	/// \return The interleaved type
	AkForceInline AkUInt32 GetInterleaveID() const
	{
		return uInterleaveID;
	}

	/// Set all parameters of the audio format structure.
	void SetAll(
		AkUInt32    in_uSampleRate,		///< Number of samples per second
		AkUInt32	in_uChannelMask,	///< Channel configuration
		AkUInt32    in_uBitsPerSample,	///< Number of bits per sample
		AkUInt32    in_uBlockAlign,		///< Block alignment
		AkUInt32    in_uTypeID,			///< Data sample format (Float or Integer)
		AkUInt32    in_uInterleaveID	///< Interleaved type
		)
	{
		uSampleRate		= in_uSampleRate;
		uChannelMask	= in_uChannelMask;
		uBitsPerSample	= in_uBitsPerSample;
		uBlockAlign		= in_uBlockAlign;
		uTypeID			= in_uTypeID;
		uInterleaveID	= in_uInterleaveID;
	}

	/// Checks if the channel configuration is supported by the source pipeline.
	/// \return The interleaved type
	AkForceInline bool IsChannelConfigSupported() const
	{
		bool bIsSupported = true;
		switch ( uChannelMask )
		{
		case AK_SPEAKER_SETUP_MONO:
		case AK_SPEAKER_SETUP_STEREO:
#if AK_VOICE_MAX_NUM_CHANNELS > 2
		case AK_SPEAKER_SETUP_0POINT1:
		case AK_SPEAKER_SETUP_1POINT1:
		case AK_SPEAKER_SETUP_2POINT1:
		case AK_SPEAKER_SETUP_3STEREO:
		case AK_SPEAKER_SETUP_3POINT1:
		case AK_SPEAKER_SETUP_4:
		case AK_SPEAKER_SETUP_4POINT1:
		case AK_SPEAKER_SETUP_5:
		case AK_SPEAKER_SETUP_5POINT1:
#ifdef AK_71AUDIO
		case AK_SPEAKER_SETUP_7:
		case AK_SPEAKER_SETUP_7POINT1:
#endif // AK_71AUDIO
#endif // AK_VOICE_MAX_NUM_CHANNELS > 2
			break;
		default:
			bIsSupported = false;
		}
		return bIsSupported;
	}

};

// Channel indices.
// ------------------------------------------------

#define AK_IDX_SETUP_0_LFE			(0)	///< Index of low-frequency channel in 0.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_1_CENTER		(0)	///< Index of center channel in 1.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_1_LFE			(1)	///< Index of low-frequency channel in 1.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_2_LEFT			(0)	///< Index of left channel in 2.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_2_RIGHT		(1)	///< Index of right channel in 2.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_2_LFE			(2) ///< Index of low-frequency channel in 2.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_3_LEFT			(0)	///< Index of left channel in 3.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_3_RIGHT		(1)	///< Index of right channel in 3.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_3_CENTER		(2)	///< Index of center channel in 3.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_3_LFE			(3)	///< Index of low-frequency channel in 3.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_4_FRONTLEFT	(0)	///< Index of front left channel in 4.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_4_FRONTRIGHT	(1)	///< Index of front right channel in 4.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_4_REARLEFT		(2)	///< Index of rear left channel in 4.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_4_REARRIGHT	(3)	///< Index of rear right channel in 4.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_4_LFE			(4)	///< Index of low-frequency channel in 4.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_5_FRONTLEFT	(0)	///< Index of front left channel in 5.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_5_FRONTRIGHT	(1)	///< Index of front right channel in 5.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_5_CENTER		(2)	///< Index of center channel in 5.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_5_REARLEFT		(3)	///< Index of rear left channel in 5.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_5_REARRIGHT	(4)	///< Index of rear right channel in 5.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_5_LFE			(5)	///< Index of low-frequency channel in 5.1 setup (use with AkAudioBuffer::GetChannel())

#ifdef AK_71AUDIO
#define AK_IDX_SETUP_6_FRONTLEFT	(0)	///< Index of front left channel in 6.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_FRONTRIGHT	(1)	///< Index of fornt right channel in 6x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_REARLEFT		(2)	///< Index of rear left channel in 6.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_REARRIGHT	(3)	///< Index of rear right channel in 6.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_SIDELEFT		(4)	///< Index of side left channel in 6.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_SIDERIGHT	(5)	///< Index of side right channel in 6.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_6_LFE			(6)	///< Index of low-frequency channel in 6.1 setup (use with AkAudioBuffer::GetChannel())

#define AK_IDX_SETUP_7_FRONTLEFT	(0)	///< Index of front left channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_FRONTRIGHT	(1)	///< Index of fornt right channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_CENTER		(2)	///< Index of center channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_REARLEFT		(3)	///< Index of rear left channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_REARRIGHT	(4)	///< Index of rear right channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_SIDELEFT		(5)	///< Index of side left channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_SIDERIGHT	(6)	///< Index of side right channel in 7.x setups (use with AkAudioBuffer::GetChannel())
#define AK_IDX_SETUP_7_LFE			(7)	///< Index of low-frequency channel in 7.1 setup (use with AkAudioBuffer::GetChannel())
#endif

// Audio buffer.
// ------------------------------------------------

/// Native sample type. 
/// \remarks Sample values produced by insert effects must use this type.
/// \remarks Source plug-ins can produce samples of other types (specified through 
/// according fields of AkAudioFormat, at initial handshaking), but these will be 
/// format converted internally into the native format.
/// \sa
/// - \ref iaksourceeffect_init
/// - \ref iakmonadiceffect_init
#if defined AK_WII_FAMILY_HW || defined(AK_3DS)
typedef AkInt16 AkSampleType;	///< Audio sample data type (Wii-specific: 16 bit signed integer)
#else
typedef AkReal32 AkSampleType;	///< Audio sample data type (32 bit floating point)
#endif

/// Audio buffer structure including the address of an audio buffer, the number of valid frames inside, 
/// and the maximum number of frames the audio buffer can hold.
/// \sa
/// - \ref fx_audiobuffer_struct
class AkAudioBuffer
{
public:
	
	/// \name Channel queries.
	//@{
	/// Get the number of channels.
	inline AkUInt32 NumChannels()
	{
		return AK::GetNumChannels( uChannelMask );
	}

	/// Returns true if there is an LFE channel present.
	inline bool HasLFE()
	{ 
		return AK::HasLFE( uChannelMask ); 
	}

	/// Returns the channel mask (channel values defined in AkCommonDefs.h)
	AkForceInline AkChannelMask GetChannelMask() { return uChannelMask; }
	//@}

	/// \name Interleaved interface
	//@{
	/// Get address of data: to be used with interleaved buffers only.
	/// \remarks Only source plugins can output interleaved data. This is determined at 
	/// initial handshaking.
	/// \sa 
	/// - \ref fx_audiobuffer_struct
#if !defined(AK_3DS) && !defined(AK_WII_FAMILY_HW)
	AkForceInline void * GetInterleavedData()
	{ 
		return pData; 
	}

	// Interleaved. Allocation is performed outside.
	inline void AttachInterleavedData( void * in_pData, AkUInt16 in_uMaxFrames, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask )
	{ 
		pData = in_pData; 
		uMaxFrames = in_uMaxFrames; 
		uValidFrames = in_uValidFrames; 
		uChannelMask = in_uChannelMask; 
	}

	// Contiguous deinterleaved. Allocation is performed outside.
	AkForceInline void AttachContiguousDeinterleavedData( void * in_pData, AkUInt16 in_uMaxFrames, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask )
	{ 
		AttachInterleavedData( in_pData, in_uMaxFrames, in_uValidFrames, in_uChannelMask );
	}
#endif
	//@}
	
	/// \name Deinterleaved interface
	//@{
	/// Get the buffer of the ith channel. 
	/// Access to channel data is most optimal through this method. Use whenever the
	/// speaker configuration is known, or when an operation must be made independently
	/// for each channel.
	/// \remarks Use the AK_IDX_SETUP_* defines to access channels (defined in AkCommonDefs.h).
	/// \return Address of the buffer of the ith channel.
	/// \sa
	/// - \ref fx_audiobuffer_struct
	/// - \ref fx_audiobuffer_struct_channels
	inline AkSampleType * GetChannel(
		AkUInt32 in_uIndex		///< Channel index [0,NumChannels()-1]
		)
	{
		AKASSERT( in_uIndex < NumChannels() );
#if defined (AK_WII_FAMILY_HW) || defined(AK_3DS)
		return (AkSampleType*)arData[in_uIndex];
#else
		return (AkSampleType*)((AkUInt8*)(pData) + ( in_uIndex * sizeof(AkSampleType) * MaxFrames() ));
#endif
	}

	/// Get the buffer of the LFE.
	/// \return Address of the buffer of the LFE. Null if there is no LFE channel.
	/// \sa
	/// - \ref fx_audiobuffer_struct_channels
	inline AkSampleType * GetLFE()
	{
		if ( uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
			return GetChannel( NumChannels()-1 );
		
		return (AkSampleType*)0;
	}

	/// Can be used to transform an incomplete into a complete buffer with valid data.
	/// The invalid frames are made valid (zeroed out) for all channels and the validFrames count will be made equal to uMaxFrames.
	void ZeroPadToMaxFrames()
	{
		// Zero out all channels.
		const AkUInt32 uNumChannels = NumChannels();
		const AkUInt32 uNumZeroFrames = MaxFrames()-uValidFrames;
		if ( uNumZeroFrames )
		{
			for ( AkUInt32 i = 0; i < uNumChannels; ++i )
			{
				AkZeroMemLarge( GetChannel(i) + uValidFrames, uNumZeroFrames * sizeof(AkSampleType) );
			}
			uValidFrames = MaxFrames();
		}
	}

#if defined(_DEBUG) && !defined(AK_WII_FAMILY_HW)
	bool CheckValidSamples()
	{
		// Zero out all channels.
		const AkUInt32 uNumChannels = NumChannels();
		for ( AkUInt32 i = 0; i < uNumChannels; ++i )
		{
			AkSampleType * AK_RESTRICT pfChan = GetChannel(i);
			if ( pfChan )
			{
				for ( AkUInt32 j = 0; j < uValidFrames; j++ )
				{
					AkSampleType fSample = *pfChan++;
					if ( fSample > 4.f )
						return false;
					else if ( fSample < -4.f )
						return false;
				}
			}
		}

		return true;
	}
#endif

#ifdef AK_PS3
	/// Access to contiguous channels for DMA transfers on SPUs (PS3 specific).
	/// \remarks On the PS3, deinterleaved channels are guaranteed to be contiguous
	/// in memory to allow one-shot DMA transfers.
	AkForceInline void * GetDataStartDMA()
	{
		return pData;
	}
#endif

#ifdef __SPU__
	/// Construct AkAudioBuffer on SPU from data obtained through explicit DMAs.
	/// \remarks Address provided should point to a contiguous memory space for all deinterleaved channels.
	AkForceInline void CreateFromDMA( void * in_pData, AkUInt16 in_uMaxFrames, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask, AKRESULT in_eState )
	{ 
		pData = in_pData; 
		uMaxFrames = in_uMaxFrames; 
		uValidFrames = in_uValidFrames; 
		uChannelMask = in_uChannelMask; 
		eState = in_eState;
	}
#endif
	//@}

#if !defined(AK_3DS) && !defined(AK_WII_FAMILY_HW)
	void RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia )
	{
		AkUIntPtr uMemoryOffset = (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;
		pData = (void*) (((AkUIntPtr)pData) + uMemoryOffset);
	}
#endif

protected:
#if defined (AK_WII_FAMILY_HW) || defined(AK_3DS)
	void *			arData[AK_VOICE_MAX_NUM_CHANNELS];	///< Array of audio buffers for each channel (Wii-specific implementation).
#else
	void *			pData;				///< Start of the audio buffer.
#endif
	AkChannelMask	uChannelMask;		///< Channel mask.
public:	
	AKRESULT		eState;				///< Execution status	
protected:	
	AkUInt16		uMaxFrames;			///< Number of sample frames the buffer can hold. Access through AkAudioBuffer::MaxFrames().

public:
	/// Access to the number of sample frames the buffer can hold.
	/// \return Number of sample frames the buffer can hold.
	AkForceInline AkUInt16 MaxFrames() { return uMaxFrames; }
	
	AkUInt16		uValidFrames;		///< Number of valid sample frames in the audio buffer
} AK_ALIGN_DMA;

#endif // _AK_COMMON_DEFS_H_

