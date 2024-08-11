/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkCommon.h
//
// AudioLib common defines, enums, and structs.
//
//////////////////////////////////////////////////////////////////////

#ifndef _COMMON_H_
#define _COMMON_H_

// Additional "AkCommon" definition made public.
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#ifndef __SPU__
#include "AkLEngineDefs.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkListBareLight.h>
#endif

#include "AkSIMDSpeakerVolumes.h"
#include "AkKeyArray.h"
#include "AkPrivateTypes.h"

struct AkSoundParams;
class  CAkPBI;

#define EPSILON_CONTROL_VALUE			(0.001f)

//-----------------------------------------------------------------------------
// AUDIO FILES AND MARKERS
//-----------------------------------------------------------------------------

/// Defines the header of a block of markers.
struct AkMarkersHeader
{
    AkUInt32        uNumMarkers;		    ///< Number of markers
};

/// Defines the parameters of a marker.
struct AkAudioMarker
{     
    AkUInt32        dwIdentifier;           ///< Identifier.
    AkUInt32        dwPosition;             ///< Position in the audio data in sample frames.
	char*			strLabel;				///< Label of the marker taken from the file.
};

// Structure contained in the buffer to carry markers
struct AkBufferMarker
{
	CAkPBI*			pContext;
	AkUInt32		dwPositionInBuffer;
	AkAudioMarker	marker;
};

//-----------------------------------------------------------------------------
// Name: struct AkAudioMix
// Desc: Defines the parameters of a buffer to be used by mixers.
//-----------------------------------------------------------------------------
struct AkAudioMix
{
	// all values : [0, 1]
	AkSIMDSpeakerVolumes Next;
	AkSIMDSpeakerVolumes Previous;
} AK_ALIGN_DMA;
//-----------------------------------------------------------------------------
// Name: struct AkListenerData
// Desc: Defines the parameters of a listener.
//-----------------------------------------------------------------------------
struct AkListenerData : public AkListener
{
	AkSIMDSpeakerVolumes customSpeakerGain;	// user-specified per-speaker volume offset
	AkReal32			Matrix[3][3];		// world to listener coordinates transformation
	AkOutputDeviceID	uDeviceID;			// Device ID to which this listener is routed
	bool				bPositionDirty;		// True when position (or scaling factor) is changed, reset after game objects have been notified.
};

//-----------------------------------------------------------------------------
// Name: struct AkBufferPosInformation
// Desc: Defines the position info for a source used by GetSourcePlayPosition()
//-----------------------------------------------------------------------------
struct AkBufferPosInformation
{
	AkUInt32	uStartPos;		//start position for data contained in the buffer
	AkReal32	fLastRate;		//last known pitch rate
	AkUInt32	uFileEnd;		//file end position
	AkUInt32	uSampleRate;	//file sample rate
	
	inline void Clear()
	{
		uStartPos	= 0xFFFFFFFF;
		fLastRate	= 1.0f;
		uFileEnd	= 0xFFFFFFFF;
		uSampleRate	= 1;
	}
};

//-----------------------------------------------------------------------------
// Name: struct AKSourceToIndex
// Desc: Struct commonly used to represent Index to media ID association.
//-----------------------------------------------------------------------------
struct AKSourceToIndex
{
	AkUInt32 index;
	AkUInt32 sourceID;
};

//-----------------------------------------------------------------------------
// Name: class AkPipelineBufferBase
//-----------------------------------------------------------------------------
class AkPipelineBufferBase : public AkAudioBuffer 
{
public:
	AkForceInline AkPipelineBufferBase() { ClearData(); }

	AkForceInline void ClearData()
	{
#if defined AK_WII_FAMILY_HW || defined(AK_3DS)
		arData[0] = arData[1] = NULL;
#else
		pData				= NULL;
#endif
	}
	inline void Clear()
	{
		ClearData();
		uValidFrames		= 0;
		uMaxFrames			= 0;
		eState				= AK_DataNeeded;
	}

#if defined AK_WII_FAMILY_HW || defined(AK_3DS)

	inline void AttachData( void * in_pDataL, void * in_pDataR, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask ) 
	{ 
		arData[0] = in_pDataL;
		arData[1] = in_pDataR;
		uValidFrames = in_uValidFrames; 
		uChannelMask = in_uChannelMask; 
	}
	
	// Logic.
	inline void SetRequestSize( AkUInt16 in_uMaxFrames ) 
	{
		AKASSERT( !arData[0] );
		uMaxFrames = in_uMaxFrames; 
	}
	
	inline bool HasData() { return ( NULL != arData[0] ); }

#else

	AkForceInline bool HasData() { return ( NULL != pData ); }

	// Buffer management.
	// ----------------------------

	// Logical.
	inline void SetRequestSize( AkUInt16 in_uMaxFrames ) 
	{
		AKASSERT( !pData );
		uMaxFrames = in_uMaxFrames; 
	}

	// Detach data from buffer, after it has been freed from managing instance.
	inline void DetachData()
	{
		AKASSERT( pData && uMaxFrames > 0 && uChannelMask > 0 );
		pData = NULL;
		uMaxFrames = 0;
		uValidFrames = 0;
	}

	AkForceInline void * GetContiguousDeinterleavedData()
	{
		return pData;
	}

	// Deinterleaved (pipeline API). Allocation is performed inside.
	AKRESULT GetCachedBuffer( 
		AkUInt16		in_uMaxFrames, 
		AkChannelMask	in_uChannelMask );
	void ReleaseCachedBuffer();

#endif

	AkForceInline void SetChannelMask( AkChannelMask in_uChannelMask ) { uChannelMask = in_uChannelMask; }

#ifdef AK_PS3
	AkPipelineBufferBase *	pNextItemMix;			// for chained mix items
	AkAudioMix *			pNextItemVolumes;		// for chained mix items
	AkVolumeOffset *		pNextVolumeAttenuation;	// for chained mix items
#endif
};

class AkPipelineBuffer
	: public AkPipelineBufferBase
{
public:
	AkPipelineBuffer()
		: uNumMarkers( 0 )
		, pMarkers(NULL)
	{}

	inline void ResetMarkerPointer()
	{
		uNumMarkers			= 0;
		pMarkers			= NULL;
	}

	AkForceInline void ClearPosInfo()
	{
		posInfo.Clear();
	}

	inline void Clear()
	{
		AkPipelineBufferBase::Clear();
		ResetMarkerPointer();
		posInfo.Clear();
	}

	void FreeMarkers();

	AkForceInline void ClearAndFreeMarkers()
	{
		FreeMarkers();
		AkPipelineBufferBase::Clear();
		posInfo.Clear();
	}

	// Markers.
	AkUInt16        uNumMarkers;        // Number of markers present in this buffer
	AkBufferMarker* pMarkers;           // List of markers present in this buffer
	AkBufferPosInformation posInfo;		// Position information for GetSourcePlayPosition
};

namespace AkMonitorData
{
	enum BusMeterDataType
	{
		BusMeterDataType_Peak		= 1 << 0,
		BusMeterDataType_TruePeak	= 1 << 1,
		BusMeterDataType_RMS		= 1 << 2,
		BusMeterDataType_HdrPeak	= 1 << 3,
		BusMeterDataType_KPower		= 1 << 4,

		BusMeterMask_RequireContext = ~BusMeterDataType_HdrPeak
	};
}

#ifndef AK_3DS

// Output buffers of mix busses.
class AkAudioBufferBus
	: public AkPipelineBufferBase
{
public:
	AkReal32		m_fNextVolume;			// Next bus volume.
	AkReal32		m_fPreviousVolume;		// Previous bus volume.

#ifndef AK_OPTIMIZED
	class AkMeterCtx *	pMeterCtx;			// Metering data (optional).
#endif
};

enum VPLBufferType
{
	BufferType_Cbx = 0,
	BufferType_Bus = 1
};

#endif

enum AkAuxType
{
	AkAuxType_GameDef,
	AkAuxType_UserDef
};


class AkVPL;
class CAkVPLSrcCbxNodeBase;
typedef CAkKeyArray<AkOutputDeviceID, AkVPL*, 1> AkDeviceVPLArray;
struct AkAuxSendValueEx: public AkAuxSendValue
{
	AkForceInline ~AkAuxSendValueEx()
	{
		PerDeviceAuxBusses.Term();
	}

	AkAuxType eAuxType;
	AkDeviceVPLArray PerDeviceAuxBusses;
};


struct AkMergedEnvironmentValue
{
	AkForceInline AkMergedEnvironmentValue() 
		: fControlValue( 0.0f )
		, fLastControlValue( 0.0f )
		, auxBusID( AK_INVALID_AUX_ID )
#if defined AK_WII || defined AK_3DS
		, HWAuxBusID( AK_INVALID_AUX_ID )
#endif
		, eAuxType( AkAuxType_GameDef )
	{}

	AkForceInline ~AkMergedEnvironmentValue()
	{
		PerDeviceAuxBusses.Term();
	}

	AkForceInline AkMergedEnvironmentValue& operator=( AkMergedEnvironmentValue& in_b)
	{
		if (&in_b == this)
			return *this;
		
		fControlValue = in_b.fControlValue;
		fLastControlValue = in_b.fLastControlValue;
		auxBusID = in_b.auxBusID;
#if defined AK_WII || defined AK_3DS
		HWAuxBusID = in_b.HWAuxBusID;
#endif
		eAuxType = in_b.eAuxType;
		PerDeviceAuxBusses.Transfer(in_b.PerDeviceAuxBusses);
		return *this;
	}

	AkForceInline AkVPL* FirstDeviceBus() const {return (*PerDeviceAuxBusses.Begin()).item;}

	AkReal32 fControlValue;
	AkReal32 fLastControlValue;
	AkAuxBusID auxBusID;
#if defined AK_WII || defined AK_3DS
	AkUInt32 HWAuxBusID;
#endif
	AkAuxType eAuxType;
	AkDeviceVPLArray PerDeviceAuxBusses;
};

void MergeLastAndCurrentValues(
	const AkAuxSendValueEx *in_pNewValues,
	AkMergedEnvironmentValue* io_paMergedValues,
	bool in_bFirstBufferConsumed,
	AkUInt8 & out_uNumSends
#if !defined AK_WII && !defined AK_3DS
	,CAkVPLSrcCbxNodeBase * in_pCbx
#endif
	);

#ifndef __SPU__

struct AkVolumeAttenuations
{
	AK_ALIGN_SIMD( AkVolumeOffset dry );
	AK_ALIGN_SIMD( AkVolumeOffset userDef );
	AK_ALIGN_SIMD( AkVolumeOffset gameDef );
};

class AkDeviceInfo
{
public:
	AkDeviceInfo( AkVPL * in_pOutputBus, AkOutputDeviceID in_uDeviceID, bool in_bCrossDeviceSend);
	virtual ~AkDeviceInfo();

	void ZeroAll()
	{
		memset( mxDirect, 0, sizeof( AkAudioMix ) * AK_VOICE_MAX_NUM_CHANNELS );
		memset( &mxAttenuations, 0, sizeof( mxAttenuations ));
	}

	AkChannelMask GetOutputConfig();
	AkForceInline AkOutputDeviceID GetDeviceID() const { return uDeviceID; }

	// Prepare for new frame: "next" volumes become "previous". LPF values remain the same.
	AkForceInline void Reset( AkUInt32 in_uNumChannels )
	{
		// Move old "Next" volumes to "Previous".
		/// TODO: Move pointers instead of copying data. Careful with PS3.
		unsigned int uChannel=0;
		do
		{
			mxDirect[uChannel].Previous = mxDirect[uChannel].Next;
		}
		while ( ++uChannel < in_uNumChannels );
		mxAttenuations.dry.fPrev		= mxAttenuations.dry.fNext;
		mxAttenuations.userDef.fPrev	= mxAttenuations.userDef.fNext;
		mxAttenuations.gameDef.fPrev	= mxAttenuations.gameDef.fNext;
	}

	AkForceInline void ClearNext( AkUInt32 in_uNumChannels )
	{
		unsigned int uChannel=0;
		do
		{
			mxDirect[uChannel].Next.Zero();
		}
		while ( ++uChannel < in_uNumChannels );
		mxAttenuations.dry.fNext = 0;
		mxAttenuations.userDef.fNext = 0;
		mxAttenuations.gameDef.fNext = 0;
	}

	AkForceInline void ClearPrev( AkUInt32 in_uNumChannels )
	{
		unsigned int iChannel=0;
		do
		{
			mxDirect[iChannel].Previous.Zero();
		}
		while ( ++iChannel < in_uNumChannels );
		mxAttenuations.dry.fPrev		= 0;
		mxAttenuations.gameDef.fPrev	= 0;
		mxAttenuations.userDef.fPrev	= 0;
	}

	AkForceInline void CopyTo( AkDeviceInfo & in_dest, AkUInt32 in_uNumChannels )
	{
		unsigned int uChannel=0;
		do
		{
			mxDirect[uChannel].Previous.CopyTo( in_dest.mxDirect[uChannel].Previous );
			mxDirect[uChannel].Next.CopyTo( in_dest.mxDirect[uChannel].Next );
		}
		while ( ++uChannel < in_uNumChannels );
		mxAttenuations = in_dest.mxAttenuations;
		
		in_dest.fLPF = fLPF;
		in_dest.fObsLPF = fObsLPF;
		in_dest.bCrossDeviceSend = bCrossDeviceSend;
	}

	/// TODO Dynamic number of channels.

	// IMPORTANT: mxDirect must remain on top.
	AkAudioMix	mxDirect[AK_VOICE_MAX_NUM_CHANNELS];
	AK_ALIGN_SIMD( AkVolumeAttenuations mxAttenuations );
	AkReal32	fLPF;		// Common LPF (dry+wet): actor-mixer and listener-dependent LPF, except obstruction/dry bus.
	AkReal32	fObsLPF;	// Obstruction LPF. /// NOTE For multi-device, need to add bus LPF at the proper time.
	AkDeviceInfo * pNextLightItem;		// For list bare light.
	AkVPL*		pMixBus;
	AkOutputDeviceID uDeviceID;
	AkReal32	fMaxVolume;	//Maximum volume in all the paths (dry + wet).  Used to determine HDR window and audibility
	bool		bCrossDeviceSend;
};

typedef AkListBareLight<AkDeviceInfo> AkDeviceInfoList;

// Device volumes manages a list of volume matrices per-device. 
class CAkOutputDevices
{
public:
	CAkOutputDevices()
		: m_uNumDevices( 0 )
	{}

	~CAkOutputDevices()
	{
		while ( m_listDeviceVolumes.First() )
		{
			AkDeviceInfo * pVolumes = m_listDeviceVolumes.First();
			m_listDeviceVolumes.RemoveFirst();
			AkDeleteAligned( g_LEngineDefaultPoolId, pVolumes );
		}
		m_listDeviceVolumes.Term();
	}

	// Create a device and its speaker volume matrix. Creates it if necessary.
	AkForceInline AkDeviceInfo * CreateDevice(
		AkVPL *	in_pOutputBus,
		AkOutputDeviceID in_uDeviceID,
		bool in_bCrossDeviceSend
		)
	{
		AkDeviceInfo * pVolumeMx = AkNewAligned( g_LEngineDefaultPoolId, AkDeviceInfo( in_pOutputBus, in_uDeviceID, in_bCrossDeviceSend ), AK_SIMD_ALIGNMENT );
		if ( pVolumeMx )
		{
			m_listDeviceVolumes.AddFirst( pVolumeMx );
			++m_uNumDevices;
		}
		return pVolumeMx;
	}

	AkForceInline void RemoveVPL(AkVPL * in_pOutputBus)
	{
		AkDeviceInfoList::IteratorEx it = m_listDeviceVolumes.BeginEx();
		while ( it != m_listDeviceVolumes.End() )
		{
			if ((*it)->pMixBus == in_pOutputBus)
			{
				Remove(it);
				break;
			}
			++it;
		}
	}

	AkForceInline AkDeviceInfoList::IteratorEx Remove(AkDeviceInfoList::IteratorEx it)
	{
		AkDeviceInfo *pToDelete = *it;
		it = m_listDeviceVolumes.Erase(it);
		AkDeleteAligned(g_LEngineDefaultPoolId, pToDelete);				
		m_uNumDevices--;
		return it;
	}

	// Get a pointer to volumes matrix for given device. 
	// Note: Needs to exist.
	AkForceInline AkDeviceInfo * GetVolumesByID( AkOutputDeviceID in_uDeviceID )
	{
		// Search for this device.
		AkDeviceInfoList::Iterator it = m_listDeviceVolumes.Begin();
		while ( it != m_listDeviceVolumes.End() )
		{
			if ( (*it)->uDeviceID == in_uDeviceID )
				return (*it);
			++it;
		}
		return NULL;
	}

	AkForceInline void Reset( AkUInt32 in_uNumChannels )
	{
		AkDeviceInfoList::Iterator it = m_listDeviceVolumes.Begin();
		while ( it != m_listDeviceVolumes.End() )
		{
			(*it)->Reset( in_uNumChannels );
			++it;
		}
	}

	AkForceInline void ClearNext( AkUInt32 in_uNumChannels )
	{
		AkDeviceInfoList::Iterator it = m_listDeviceVolumes.Begin();
		while ( it != m_listDeviceVolumes.End() )
		{
			(*it)->ClearNext( in_uNumChannels );
			++it;
		}
	}

	// Returns number of devices. Returns 0 if sound has not yet been audible through any device.
	AkForceInline AkUInt32 GetNumDevices() const
	{
		return m_uNumDevices;
	}

	AkForceInline bool HasOutputDevice() {return m_uNumDevices > 0;}

	AkForceInline AkDeviceInfoList::Iterator Begin() {return m_listDeviceVolumes.Begin();}
	AkForceInline AkDeviceInfoList::IteratorEx BeginEx() {return m_listDeviceVolumes.BeginEx();}
	AkForceInline AkDeviceInfoList::Iterator End() {return m_listDeviceVolumes.End();}

protected:
	AkDeviceInfoList m_listDeviceVolumes;
	AkUInt32			m_uNumDevices;
};


//-----------------------------------------------------------------------------
// Looping constants.
//-----------------------------------------------------------------------------
const AkInt16 LOOPING_INFINITE  = 0;
const AkInt16 LOOPING_ONE_SHOT	= 1;

#define IS_LOOPING( in_Value ) ( in_Value == LOOPING_INFINITE || in_Value > LOOPING_ONE_SHOT )

inline void ZeroPadBuffer( AkAudioBuffer * io_pAudioBuffer )
{ 
	// Extra frames should be padded with 0's
	AkUInt32 uPadFrames = io_pAudioBuffer->MaxFrames() - io_pAudioBuffer->uValidFrames;
	if ( uPadFrames )
	{
		// Do all channels
		AkUInt32 uNumChannels = io_pAudioBuffer->NumChannels();
		for ( unsigned int uChanIter = 0; uChanIter < uNumChannels; ++uChanIter )
		{
			AkSampleType * pPadStart = io_pAudioBuffer->GetChannel(uChanIter) + io_pAudioBuffer->uValidFrames;
			for ( unsigned int uFrameIter = 0; uFrameIter < uPadFrames;  ++uFrameIter )
			{
				pPadStart[uFrameIter] = 0.f;
			}
		}
	}
}

inline void ZeroPrePadBuffer( AkAudioBuffer * io_pAudioBuffer, AkUInt32 in_ulNumFramestoZero )
{ 
	if ( in_ulNumFramestoZero )
	{
		// Do all channels
		AkUInt32 uNumChannels = io_pAudioBuffer->NumChannels();
		for ( unsigned int uChanIter = 0; uChanIter < uNumChannels; ++uChanIter )
		{
			AkSampleType * pPadStart = io_pAudioBuffer->GetChannel(uChanIter);
			for ( unsigned int uFrameIter = 0; uFrameIter < in_ulNumFramestoZero;  ++uFrameIter )
			{
				pPadStart[uFrameIter] = 0.f;
			}
		}
	}
}

// Helper: Converts an absolute source position (which takes looping region into account) into 
// a value that is relative to the beginning of the file, and the number of loops remaining.
inline void AbsoluteToRelativeSourceOffset( 
	AkUInt32 in_uAbsoluteSourcePosition, 
    AkUInt32 in_uLoopStart,
    AkUInt32 in_uLoopEnd,
    AkUInt16 in_usLoopCount,
	AkUInt32 & out_uRelativeSourceOffset,
	AkUInt16 & out_uRemainingLoops 
	)
{
    out_uRemainingLoops = in_usLoopCount;

	if ( IS_LOOPING( in_usLoopCount ) 
		&& in_uAbsoluteSourcePosition > in_uLoopEnd // Strictly greater than, because loop end is _always inclusive_
		&& in_uLoopEnd > in_uLoopStart )
	{
        AkUInt32 uTotalMinusStart = in_uAbsoluteSourcePosition - in_uLoopStart;
		AkUInt32 uLoopRegionLength = ( in_uLoopEnd - in_uLoopStart + 1 );
		AkUInt32 uLoopCount = uTotalMinusStart / uLoopRegionLength;

		if ( uLoopCount < out_uRemainingLoops || out_uRemainingLoops == 0 )
		{
			out_uRemainingLoops = ( out_uRemainingLoops == 0 ) ? 0 : out_uRemainingLoops - (AkUInt16)uLoopCount;
			out_uRelativeSourceOffset = (uTotalMinusStart % uLoopRegionLength) + in_uLoopStart;
		}
		else
		{
			// Passed loop region: clamp to total loop count and re-express postion according to it.
			AKASSERT( in_uAbsoluteSourcePosition >= ( out_uRemainingLoops - 1 ) * uLoopRegionLength );
			out_uRelativeSourceOffset = in_uAbsoluteSourcePosition - ( out_uRemainingLoops - 1 )* uLoopRegionLength;
            out_uRemainingLoops = 1;
		}
	}
    else
    {
        out_uRelativeSourceOffset = in_uAbsoluteSourcePosition;
    }
}

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
inline void ZeroPrePadBufferInterleaved( AkAudioBuffer * io_pAudioBuffer, AkUInt32 in_ulNumFramestoZero )
{ 
	if ( in_ulNumFramestoZero )
	{
		memset( io_pAudioBuffer->GetInterleavedData(), 0, in_ulNumFramestoZero * io_pAudioBuffer->NumChannels() * sizeof( AkSampleType ) );
	}
}
#endif // AK_WII

//-----------------------------------------------------------------------------
/// Helpers for AkAudioFormat bitfield stored in banks.
//-----------------------------------------------------------------------------

#define AKAUDIOFORMAT_CHANNELMASK_MASK		0x0003FFFF ///< Mask for ChannelMask
#define AKAUDIOFORMAT_BITSPERSAMPLE_MASK	0x0000003F ///< Mask for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_MASK		0x0000001F ///< Mask for BlockAlign
#define AKAUDIOFORMAT_TYPEID_MASK			0x00000003 ///< Mask for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_MASK		0x00000001 ///< Mask for InterleaveID

#define AKAUDIOFORMAT_CHANNELMASK_SHIFT		0  ///< Shift for ChannelMask
#define AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT	18 ///< Shift for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_SHIFT		24 ///< Shift for BlockAlign
#define AKAUDIOFORMAT_TYPEID_SHIFT			29 ///< Shift for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_SHIFT	31 ///< Shift for InterleaveID

#define BANK_SOURCE_LANGUAGE_BITSHIFT			0
#define BANK_SOURCE_ISSTREAMEDFROMRSX_BITSHIFT	1

//Formats copied from Microsoft headers.  They are already defined in windows.
#if defined AK_WIN || defined AK_XBOXONE
#include "mmreg.h"
#elif defined AK_XBOX360
#include "audiodefs.h"
#else
#define		WAVE_FORMAT_PCM     1
#define		WAVE_FORMAT_ADPCM	2
#define		WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif

#endif

#define AK_MASK_TO_INDEX(__mask) (((__mask & 0xFFFF0000) != 0) << 4) | \
	(((__mask & 0xFF00FF00) != 0) << 3) | \
	(((__mask & 0xF0F0F0F0) != 0) << 2) | \
	(((__mask & 0xCCCCCCCC) != 0) << 1)

#endif // _COMMON_H_


