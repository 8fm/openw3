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
// AkSpeakerPan.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SPEAKER_PAN_H_
#define _SPEAKER_PAN_H_

#include "AkMath.h"
#include "AkKeyArray.h"
#include <AK/SoundEngine/Common/AkSpeakerConfig.h>

// Number of 'degrees' in a Speaker Pan circle (360 was an unsufficient precision)
#define PAN_CIRCLE 512 // PAN_CRCLE MUST be a power of 2 since we do &= (PAN_CIRCLE-1) to allow wrapping around after exceding 511.

struct AkSIMDSpeakerVolumes;
class AkDevice;

//====================================================================================================
// speaker pan
//====================================================================================================
class CAkSpeakerPan
{
public:
	static void Init();

	static void GetSpeakerVolumesPlane( 
		AkReal32			in_fAngle,
		AkReal32			in_fDivergenceCenter,
		AkReal32			in_fSpread,
		AkSIMDSpeakerVolumes* out_pVolumes,
		AkUInt32			in_uNumFullBandChannels,
		AkChannelMask		in_uOutputConfig,	// config of bus to which this signal is routed.
		AkDevice *			in_pDevice
		);

	static void GetSpeakerVolumes2DPan(
		AkReal32			in_fX,			// [0..1] // 0 = full left, 1 = full right
		AkReal32			in_fY,			// [0..1] // 0 = full rear, 1 = full front
		AkReal32			in_fCenterPct,	// [0..1]
		bool				in_bIsPannerEnabled,
		AkUInt32			in_uInputConfig,
		AkUInt32			in_uOutputConfig,
		AkSIMDSpeakerVolumes*	out_pVolumes
		);

	static void GetDefaultSpeakerAngles( 
		AkChannelMask	in_channelMask,		// Desired channel config.
		AkReal32 		out_angles[]		// Returned angles for given channel config. Must be preallocated (use AK::GetNumberOfAnglesForConfig()).
		);

	static AKRESULT SetSpeakerAngles(
		const AkReal32 *	in_pfSpeakerAngles,			// Array of loudspeaker pair angles, expressed in degrees relative to azimuth ([0,180]).
		AkUInt32			in_uNumAngles,				// Number of loudspeaker pair angles.
		AkUInt32 *			out_uSpeakerAngles,			// Pointer to buffer filled with speaker angles (in internal integer angle units).
		AkUInt32 &			out_uMinAngleBetweenSpeakers// Returned minimum angle between speakers (in internal integer angle units).
		);

	// Convert angles stored into device (as integers) to SDK-friendly speaker angles (in radians)
	static void ConvertSpeakerAngles(
		AkUInt32			in_uDeviceSpeakerAngles[],	// Speaker angles stored in device (as internal integer angle units).
		AkUInt32			in_uNumAngles,				// Number of angle values.
		AkReal32			out_arAngles[]				// Returned speaker angles, in degrees (see AkOutputSettings::fSpeakerAngles). Pass in an array of size in_uNumAngles.
		);

	//Look up tables for trigonometric functions
	#define PAN_TABLE_SIZE (PAN_CIRCLE/2+1)	// 1/2 cycle
	struct PanPair
	{
		AkReal32 fGain_i_minus_1;	// Squared gain of previous speaker.
		AkReal32 fGain_i;			// Squared gain of next speaker.
	};
	typedef CAkKeyArray<AkChannelMask, PanPair*, 1> MapConfig2PanPlane;

	static void CreatePanCache( 
		AkChannelMask		in_uOutputConfig,	// config of bus to which this signal is routed.
		AkUInt32			in_uSpeakerAngles[],// Speaker angles.
		PanPair *			io_pPanPairs		// Allocated array of pan gain pairs, returned filled with values.
		);

	// Compute vector-based amplitude gains on the plane.
	static void ComputePlanarVBAPGains( 
		AkDevice *			in_pDevice,					// Output device.
		AkReal32			in_fAngle,					// Incident angle, in radians [-pi,pi], where 0 is the azimuth (positive values are clockwise)
		AkChannelMask		in_uOutputConfig,			// Desired output configuration. 
		AkSpeakerVolumes &	out_volumes					// Returned volumes.
		);

	/// Not used.
	//Returns an angle in range [-PI, PI]
	static AkForceInline AkReal32 CartesianToPolar( AkReal32 in_fX, AkReal32 in_fY )
	{
		AkReal32 fAngle;
		//convert to polar coordinates
		if( in_fX == 0.0f )
		{
			if( in_fY > 0.0f )
				fAngle = PIOVERTWO;
			else if ( in_fY == 0.0f )
				fAngle = 0; // Speaker-coordinate extension: we want x=y=0 case to be like a source straight ahead.
			else
				fAngle = -PIOVERTWO;
		}
		else
		{
			fAngle = atan2f( in_fY, in_fX );
		}
		return fAngle;
	}

	// Compute angles from left-handed rectangular coordinates, where xz is the plane (phi=0) and y is the elevation axis.
	// Distance should have been precomputed.
	static AkForceInline void CartesianToSpherical( 
		AkReal32 x, 
		AkReal32 z, 
		AkReal32 y, 
		AkReal32 fDistance,
		AkReal32 & out_fAzimuth,
		AkReal32 & out_fElevation )
	{
		/// TODO PERF Approx for atan and asin.

		//convert to polar coordinates
		if( z == 0.0f )
		{
			if ( x == 0.0f )
			{
				out_fAzimuth = 0; // Speaker-coordinate extension: we want x=y=0 case to be like a source straight ahead.
				out_fElevation = 0;
			}
			else
			{
				if( x > 0.0f )
					out_fAzimuth = PIOVERTWO;
				else
					out_fAzimuth = -PIOVERTWO;
				out_fElevation = asinf( y / fDistance );
			}
		}
		else
		{
			out_fAzimuth = atan2f( x, z );
			out_fElevation = asinf( y / fDistance );
		}
	}

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
private:

	static void AddSpeakerVolumesPower(
		AkInt32				in_iAngle,
		AkReal32			in_fDivergenceCenter, // [0..1]
		AkChannelMask		in_uOutputConfig,
		PanPair *			in_pPanTableNoCenter,	// Beginning of pan table (no center).
		AkDevice *			in_pDevice,
		AkSpeakerVolumes*	out_pVolumes
		);

	static inline AkReal32 IntAngleToRad( AkInt32 in_iAngle )
	{
		return TWOPI * in_iAngle / (AkReal32)PAN_CIRCLE;
	}

	#define PAN_SIN2_TABLE_SIZE (PAN_CIRCLE/4+1)	// 1/4 cycle; used only for headphones panning.
	static AkReal32 m_fSin2[PAN_SIN2_TABLE_SIZE];	// sin^2(x), for quarter cycle
};

#endif
