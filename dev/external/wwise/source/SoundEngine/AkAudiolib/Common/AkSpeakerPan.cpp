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
// AkSpeakerPan.cpp
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"

#include "AkSpeakerPan.h"
#include "AkSIMDSpeakerVolumes.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkLEngine.h"
#include "AkOutputMgr.h"

#include "AkMath.h"

//#include <tchar.h> //test traces

//----------------------------------------------------------------------------------------------------
// static members
//----------------------------------------------------------------------------------------------------
AkReal32 CAkSpeakerPan::m_fSin2[PAN_SIN2_TABLE_SIZE];

//====================================================================================================
//====================================================================================================
void CAkSpeakerPan::Init()
{
	for(int i=0; i < PAN_SIN2_TABLE_SIZE; ++i )
	{
		AkReal64 dblSin = sin((double) i * (PI/(PAN_CIRCLE/2)));
		m_fSin2[i] = (AkReal32) ( dblSin * dblSin );
	}
}

void CAkSpeakerPan::CreatePanCache( 
	AkChannelMask		in_uOutputConfig,	// config of bus to which this signal is routed.
	AkUInt32			in_uSpeakerAngles[],// Speaker angles.
	PanPair *			io_pPanPairs		// Allocated array of pan gain pairs, returned filled with values.
	)
{
	AkUInt32 uNumArcs = AK::GetNumberOfAnglesForConfig( in_uOutputConfig ) + 1;

#ifdef AK_LFECENTER
	bool bHasCenter = AK::HasCenter( in_uOutputConfig );
#endif

	/*
	AkInt64 start;
	AKPLATFORM::PerformanceCounter( &start );
	*/
	
	PanPair * pPanPair = io_pPanPairs;
	AkInt32 iAngle = 0;

	for ( AkUInt32 uArc = 0; uArc < uNumArcs; uArc++ )
	{
		AkReal32 fPhi0;
		AkInt32 iAngleArcStart;
		AkInt32 iAngleArcEnd;
		AkReal32 fOrigin;	// middle of arc (rad)

		if ( uArc == 0 )
		{
			// First arc.
#ifdef AK_LFECENTER
			if ( bHasCenter )
			{
				// With center.
				fPhi0 = IntAngleToRad( in_uSpeakerAngles[0] ) / 2.f;// "center" configs: arc goes from 0 to angle[0].
				fOrigin = fPhi0;
			}
			else
#endif
			{
				// No center.
				if ( in_uSpeakerAngles[0] < PAN_CIRCLE/4 )
				{
					fPhi0 = IntAngleToRad( in_uSpeakerAngles[0] );	// "no center" configs: arc goes to the other side.
					fOrigin = 0;
				}
				else
				{
					// Special case. Front half circle has no speakers.
					// Not supported (see SetSpeakerAngle).
					AKASSERT( !"Channel angles not supported" );
				}
			}

			iAngleArcStart = 0;
			iAngleArcEnd = in_uSpeakerAngles[0];
		}
		else 
		{
			iAngleArcStart = in_uSpeakerAngles[uArc-1];
			if ( uArc < uNumArcs-1 )
			{
				// Middle arcs.
				fPhi0 = IntAngleToRad( ( in_uSpeakerAngles[uArc] - iAngleArcStart ) ) / 2.f;
				iAngleArcEnd = in_uSpeakerAngles[uArc];
				fOrigin = IntAngleToRad( iAngleArcEnd - iAngleArcStart ) / 2.f + IntAngleToRad( iAngleArcStart );
			}
			else
			{
				// Last arc.
				if ( iAngleArcStart > PAN_CIRCLE/4 )
				{
					AKASSERT( iAngleArcStart < PAN_CIRCLE );

					// both arc end and origin equal pi.
					fPhi0 = IntAngleToRad( (PAN_CIRCLE/2) - iAngleArcStart );	// around PI.
					iAngleArcEnd = (PAN_CIRCLE/2);
					fOrigin = PI;
				}
				else
				{
					// Special case: clamp the remaining arc to 1 (gain computation would exceed the [0,1] range),
					// and fill the table up to PI/2. Then store the mirror of this data up to PI.
					AkInt32 uMirroredChannel = (PAN_CIRCLE/2) - iAngleArcStart;
					while ( iAngle < uMirroredChannel )
					{
						pPanPair[iAngle].fGain_i_minus_1 = 1.f;
						pPanPair[iAngle].fGain_i = 0.f;
						iAngle++;
					}
					while ( iAngle <= PAN_CIRCLE/2 )
					{
						AKASSERT( iAngle < PAN_TABLE_SIZE );
						pPanPair[iAngle].fGain_i_minus_1 = pPanPair[(PAN_CIRCLE/2)-iAngle].fGain_i;
						pPanPair[iAngle].fGain_i = pPanPair[(PAN_CIRCLE/2)-iAngle].fGain_i_minus_1;
						iAngle++;
					}
					break;
				}
			}
		}

		AkReal32 fSinPhi0 = sin( fPhi0 );
		AkReal32 fCosPhi0 = cos( fPhi0 );
		

		// Setup for Goertzel algorithm for fast computation of sin/cos
		const AkReal32 b = TWOPI / PAN_CIRCLE;	// step size
		AkReal32 a = IntAngleToRad( iAngle ) - fOrigin - 3*b;	// 3 steps back
		AkReal32 cb = 2 * cos(b);
		AkReal32 s2 = sin(a + b);
		AkReal32 s1 = sin(a + 2*b);
		AkReal32 c2 = cos(a + b);
		AkReal32 c1 = cos(a + 2*b);
		
		do
		{
			/**
			AkReal32 fAngle = IntAngleToRad( iAngle ) - fOrigin;
			AkReal32 fCosAngle = cos( fAngle );
			AkReal32 fSinAngle = sin( fAngle );
			**/

			AkReal32 fSinAngle = cb * s1 - s2;
			AkReal32 fCosAngle = cb * c1 - c2;
			s2 = s1;
			c2 = c1;
			s1 = fSinAngle;
			c1 = fCosAngle;


			AkReal32 gi_minus_1 = ( fCosAngle * fSinPhi0 - fSinAngle * fCosPhi0 );
			AkReal32 gi = ( fCosAngle * fSinPhi0 + fSinAngle * fCosPhi0 );
			// Can be slightly below 0.
			AKASSERT( gi_minus_1 >= -0.001f );
			AKASSERT( gi >= -0.001f );

			// Transform to power and normalize.
			gi_minus_1 *= gi_minus_1;
			gi *= gi;
			AKASSERT( ( gi_minus_1 + gi ) > 0 );
			AkReal32 fNormFactor = 1.f / ( gi_minus_1 + gi );
			pPanPair[iAngle].fGain_i_minus_1 = gi_minus_1 * fNormFactor;
			pPanPair[iAngle].fGain_i = gi * fNormFactor;
		}
		while ( ++iAngle <= iAngleArcEnd );	// inclusive.
	}

	/*
	AkInt64 end;
	AKPLATFORM::PerformanceCounter( &end );
	AkReal32 fTime = AKPLATFORM::Elapsed( end, start );
	{
		char msg[64];
		sprintf( msg, "Gen table t=%f\n", fTime );
		AKPLATFORM::OutputDebugMsg( msg );
	}
	*/
}

#ifdef AK_71AUDIO
static AkUInt16 ChannelIndicesForSpread[7][7] = 
#else
static AkUInt16 ChannelIndicesForSpread[5][5] = 
#endif
{
	{ AK_IDX_SETUP_1_CENTER },
	{ AK_IDX_SETUP_2_LEFT, AK_IDX_SETUP_2_RIGHT },
	{ AK_IDX_SETUP_3_LEFT, AK_IDX_SETUP_3_CENTER, AK_IDX_SETUP_3_RIGHT },
	{ AK_IDX_SETUP_4_REARLEFT, AK_IDX_SETUP_4_FRONTLEFT, AK_IDX_SETUP_4_FRONTRIGHT, AK_IDX_SETUP_4_REARRIGHT },
	{ AK_IDX_SETUP_5_REARLEFT, AK_IDX_SETUP_5_FRONTLEFT, AK_IDX_SETUP_5_CENTER, AK_IDX_SETUP_5_FRONTRIGHT, AK_IDX_SETUP_5_REARRIGHT }
#ifdef AK_71AUDIO
	,{ AK_IDX_SETUP_7_REARLEFT, AK_IDX_SETUP_7_SIDELEFT, AK_IDX_SETUP_7_FRONTLEFT, AK_IDX_SETUP_7_FRONTRIGHT, AK_IDX_SETUP_7_SIDERIGHT, AK_IDX_SETUP_7_REARRIGHT },
	{ AK_IDX_SETUP_7_REARLEFT, AK_IDX_SETUP_7_SIDELEFT, AK_IDX_SETUP_7_FRONTLEFT, AK_IDX_SETUP_7_CENTER, AK_IDX_SETUP_7_FRONTRIGHT, AK_IDX_SETUP_7_SIDERIGHT, AK_IDX_SETUP_7_REARRIGHT }
#endif
};

// Speaker assignation on the plane.
void CAkSpeakerPan::GetSpeakerVolumesPlane( 
	AkReal32			in_fAngle,
	AkReal32			in_fDivergenceCenter,
	AkReal32			in_fSpread,
	AkSIMDSpeakerVolumes* out_pVolumes,
	AkUInt32			in_uNumFullBandChannels,
	AkChannelMask		in_uOutputConfig,	// config of bus to which this signal is routed.
	AkDevice *			in_pDevice
	)
{
	AKASSERT( in_uNumFullBandChannels );

	// Ignore LFE.
#ifdef AK_LFECENTER
	in_uOutputConfig = in_uOutputConfig & ~AK_SPEAKER_LOW_FREQUENCY;
#endif

	// Handle mono output.
	if ( in_uOutputConfig == AK_SPEAKER_SETUP_MONO )
	{
		AkUInt32 iChannel=0;
		do
		{
			out_pVolumes[iChannel].Zero();
#ifdef AK_LFECENTER
			out_pVolumes[iChannel].volumes.fCenter = 1.f;
#else
			out_pVolumes[iChannel].volumes.fFrontLeft = ONE_OVER_SQRT_OF_TWO;
			out_pVolumes[iChannel].volumes.fFrontRight = ONE_OVER_SQRT_OF_TWO;
#endif // AK_LFECENTER
		}
		while ( ++iChannel<in_uNumFullBandChannels );
		return;
	}
	
	// Get pan table according to output config (without center).
#ifdef AK_LFECENTER
	AkChannelMask uOutputConfigNoCenter = in_uOutputConfig & ~AK_SPEAKER_FRONT_CENTER;
#else
	AkChannelMask uOutputConfigNoCenter = in_uOutputConfig;
#endif
	CAkSpeakerPan::PanPair * pPanTableNoCenter = in_pDevice->GetPanTable( uOutputConfigNoCenter );
	
	//Spread is [0,100], we need to convert to [0,PAN_CIRCLE]
	AkReal32 fHalfSpreadAngle = in_fSpread * ( PAN_CIRCLE / (2.f*100.0f) );
	AkReal32 fOneOverNumChannels = 1.f / in_uNumFullBandChannels;
	// Note: Convert in_fAngle (clockwise) to "PAN_CIRCLE" angle units (counterclockwise).
	AkReal32 fCurrChannelAngle = -in_fAngle/TWOPI*PAN_CIRCLE + fHalfSpreadAngle * ( 1.f - fOneOverNumChannels );
	AkReal32 fHalfChannelSpreadAngle = fHalfSpreadAngle * fOneOverNumChannels;

	//Calculate the number of virtual points needed for each input channel (so the channels have a width to fill all speakers)
	AkReal32 fNbVirtualPoints = ( fHalfChannelSpreadAngle * in_pDevice->fOneOverMinAngleBetweenSpeakers ); //min 2 points
	AkUInt32 uNbVirtualPoints = (AkUInt32)( fNbVirtualPoints + 1.f ) + 1; //min 2 points
	AkReal32 fTotalAttenuationFactor = 1.0f / ( uNbVirtualPoints * in_uNumFullBandChannels );
	AkReal32 fHalfVirtPtSpreadAngle = (fHalfChannelSpreadAngle / uNbVirtualPoints);
	if( !( uNbVirtualPoints & 1 ) ) //if we have an even number of virtual points (2,4,etc)
		fCurrChannelAngle -= fHalfVirtPtSpreadAngle;

	AkReal32 fVirtPtSpreadAngle = 2.f * fHalfVirtPtSpreadAngle;
	AkReal32 fChannelSpreadAngle = 2.f * fHalfChannelSpreadAngle;

	AkUInt32 iChannel=0;
	do
	{
		AkSIMDSpeakerVolumes & volume = out_pVolumes[ ChannelIndicesForSpread[ in_uNumFullBandChannels-1 ][ iChannel ] ];

		// Ensure it is >= 0 so that we can use RoundU, which is more efficient. AddSpeakerVolumesPower() supports all ranges.
		AkReal32 fCurrVirtPtAngle = fCurrChannelAngle + ((uNbVirtualPoints/2) * fVirtPtSpreadAngle) + (AkReal32)PAN_CIRCLE;		

		volume.Zero(); //initialize output values
		AkUInt32 uVirtualPoint=0;
		do
		{
			AKASSERT( ( fCurrVirtPtAngle ) >= 0 );
			AkInt32 iAngle = AkMath::RoundU( fCurrVirtPtAngle );
			AddSpeakerVolumesPower( iAngle, in_fDivergenceCenter, in_uOutputConfig, pPanTableNoCenter, in_pDevice, &volume.volumes );
			fCurrVirtPtAngle -= fVirtPtSpreadAngle;
		}
		while( ++uVirtualPoint<uNbVirtualPoints );

		volume.Mul( fTotalAttenuationFactor );
		volume.Sqrt(); //convert power back to a gain

		fCurrChannelAngle -= fChannelSpreadAngle;
	}
	while ( ++iChannel<in_uNumFullBandChannels);
}

void CAkSpeakerPan::AddSpeakerVolumesPower(
	AkInt32				in_iAngle,
	AkReal32			in_fDivergenceCenter,
	AkChannelMask		in_uOutputConfig,
	PanPair *			in_pPanTableNoCenter,	// Beginning of pan table (no center).
	AkDevice *			in_pDevice,
	AkSpeakerVolumes *	out_pVolumes )
{
	AKASSERT( ( in_uOutputConfig & AK_SPEAKER_LOW_FREQUENCY ) == 0 );
	AKASSERT( in_uOutputConfig != AK_SPEAKER_SETUP_MONO );

#ifdef AK_REARCHANNELS
	if ( ( in_uOutputConfig & ~AK_SPEAKER_SETUP_STEREO ) == 0 )
#else
	// No rear or center channels; we obviously go through this code path.
#endif // AK_REARCHANNELS
	{
		// Headphone panning for stereo case. 
		// Notes about headphone panning:
		// - Only relevant for stereo output config (note that the output config is that of the destination
		// _bus_, not the device output).
		// - One can set the panning rule independently for stereo/front-only busses, even in multichannel setups.
		// - It minimizes the width of the ambiguity of objects positioned to the sides, but 
		//	-- with loudspeakers, it results in (theoretically) incorrect/skewed localization when played back within 
		//	the speakers' arc;
		//	-- with headphones, intensity-based panning is only capable of in-head localization, so sound objects
		//	are placed between the left and right ears with "optimal" precision. Proper externalization would require 
		//	binaural processing and HRTF.
		if( in_pDevice->ePanningRule == AkPanningRule_Headphones )
		{
			// Rotate by 90 degrees counterclockwise.
			in_iAngle = in_iAngle + PAN_CIRCLE/4;
			in_iAngle &= ( PAN_CIRCLE-1 );		// Wrap around full circle; angle is [0,PAN_CIRCLE-1].
			
			// Back is a mirror of front:
			if ( in_iAngle >= (PAN_CIRCLE/2) )
				in_iAngle = PAN_CIRCLE - in_iAngle;
			
			// We are now between 0 (Right) and (PAN_CIRCLE/2) (Left)
			AKASSERT( in_iAngle >= 0 && in_iAngle <= (PAN_CIRCLE/2) );

			AkReal32 sin2x = m_fSin2[( in_iAngle >> 1 )];

			// sin^2(x) + cos^2(x) = 1, so:
			const AkReal32 cos2x = 1.0f - sin2x;

			out_pVolumes->fFrontLeft += sin2x;
			out_pVolumes->fFrontRight += cos2x;

			return;
		}
	}

	// Wrap in_iAngle into interval [-pi,pi].
	in_iAngle &= ( PAN_CIRCLE-1 );		// Wrap around full circle; angle is [0,PAN_CIRCLE-1].
	AkUInt32 uAbsAngle;
	bool bLeft;
	if ( in_iAngle <= PAN_CIRCLE/2 )
	{
		uAbsAngle = in_iAngle;
		bLeft = true;
	}
	else
	{
		uAbsAngle = (AkUInt32)( PAN_CIRCLE - in_iAngle );
		bLeft = false;
	}
	
	// Get cached gains without center.
#ifdef AK_LFECENTER
	AkChannelMask uOutputConfigNoCenter = in_uOutputConfig & ~AK_SPEAKER_FRONT_CENTER;
	bool bRouteToCenter = false;
#else
	AkChannelMask uOutputConfigNoCenter = in_uOutputConfig;
#endif

	// Select speaker pair.
	const AkUInt32 * uSpeakerAngles = in_pDevice->puSpeakerAngles;
	AkReal32 *pfSpeaker_i, *pfSpeaker_i_minus_1;

#ifdef AK_REARCHANNELS
	if (
#ifdef AK_71AUDIO
		( ( uOutputConfigNoCenter == AK_SPEAKER_SETUP_6_0 )
			&& ( uAbsAngle > uSpeakerAngles[2] ) )
		|| 
#endif	// AK_71AUDIO
		( ( uOutputConfigNoCenter == AK_SPEAKER_SETUP_4_0 )
			&& ( uAbsAngle > uSpeakerAngles[1] ) ) )
	{
		// Between back ("rear" == 5.1 surround or 7.1 back) channels and PI.
		if ( bLeft )
		{
			pfSpeaker_i_minus_1 = &out_pVolumes->fRearLeft;
			pfSpeaker_i = &out_pVolumes->fRearRight;
		}
		else
		{
			pfSpeaker_i_minus_1 = &out_pVolumes->fRearRight;
			pfSpeaker_i = &out_pVolumes->fRearLeft;
		}
	}
	else 
#ifdef AK_71AUDIO
	if ( ( uOutputConfigNoCenter == AK_SPEAKER_SETUP_6_0 )
			&& ( uAbsAngle > uSpeakerAngles[1] ) )
	{
		// 7.1 only: Between side and back channels.
		if ( bLeft )
		{
			// Left.
			pfSpeaker_i_minus_1 = &out_pVolumes->fSideLeft;
			pfSpeaker_i = &out_pVolumes->fRearLeft;
		}
		else
		{
			// Right.
			pfSpeaker_i_minus_1 = &out_pVolumes->fSideRight;
			pfSpeaker_i = &out_pVolumes->fRearRight;
		}
	}
	else if ( ( uOutputConfigNoCenter == AK_SPEAKER_SETUP_6_0 )
			&& ( uAbsAngle > uSpeakerAngles[0] ) )
	{
		// Between front and side.
		if ( bLeft )
		{
			// Left.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontLeft;
			pfSpeaker_i = &out_pVolumes->fSideLeft;
		}
		else
		{
			// Right.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontRight;
			pfSpeaker_i = &out_pVolumes->fSideRight;
		}
	}
	else
#endif	// AK_71AUDIO
	if ( ( uOutputConfigNoCenter == AK_SPEAKER_SETUP_4_0 )
			&& ( uAbsAngle > uSpeakerAngles[0] ) )
	{
		// Between front and surround.
		if ( bLeft )
		{
			// Left.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontLeft;
			pfSpeaker_i = &out_pVolumes->fRearLeft;
		}
		else
		{
			// Right.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontRight;
			pfSpeaker_i = &out_pVolumes->fRearRight;
		}
	}
	else
#endif	// AK_REARCHANNELS
	{
		// Either stereo, or 4/5.x between front speakers.
		if ( ( uAbsAngle > uSpeakerAngles[0] ) ^ ( bLeft ) )
		{
			// Front-left or back-right.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontRight;
			pfSpeaker_i = &out_pVolumes->fFrontLeft;
		}
		else
		{
			// Back-left or front-right.
			pfSpeaker_i_minus_1 = &out_pVolumes->fFrontLeft;
			pfSpeaker_i = &out_pVolumes->fFrontRight;
		}
	}

	// Apply gains.
#ifdef AK_LFECENTER
	if ( in_uOutputConfig & AK_SPEAKER_FRONT_CENTER
		&& uAbsAngle < uSpeakerAngles[0] 
		&& in_fDivergenceCenter > 0.f )
	{
		// Get pan gains with and without center.
		PanPair & panPair = in_pPanTableNoCenter[ uAbsAngle ];
		PanPair & panPairCenter = in_pDevice->GetPanTable( in_uOutputConfig )[ uAbsAngle ];

		AkReal32 fCenterVolume = out_pVolumes->fCenter;
		AkReal32 fVolume_minus_i = *pfSpeaker_i_minus_1;
		AkReal32 fVolume_i = *pfSpeaker_i;

		fCenterVolume += in_fDivergenceCenter * panPairCenter.fGain_i_minus_1;
		fVolume_i += ( 1.0f - in_fDivergenceCenter ) * panPair.fGain_i + in_fDivergenceCenter * panPairCenter.fGain_i;
		fVolume_minus_i += ( 1.0f - in_fDivergenceCenter ) * panPair.fGain_i_minus_1;
			
		out_pVolumes->fCenter = fCenterVolume;
		*pfSpeaker_i_minus_1 = fVolume_minus_i;
		*pfSpeaker_i = fVolume_i;
	}
	else
#endif
	{
		// Get pan gains without center.
		PanPair & panPair = in_pPanTableNoCenter[ uAbsAngle ];

		AkReal32 fVolume_minus_i = *pfSpeaker_i_minus_1;
		AkReal32 fVolume_i = *pfSpeaker_i;		
		
		fVolume_minus_i += panPair.fGain_i_minus_1;
		fVolume_i += panPair.fGain_i;
		
		*pfSpeaker_i_minus_1 = fVolume_minus_i;
		*pfSpeaker_i = fVolume_i;
	}
}

//
// Panning functions.
// 

// 1-position fade.
void _GetSpeakerVolumes2DPan1(
	AkReal32			in_fX,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( 1.0f - in_fX );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( in_fX );

#if defined( AK_LFECENTER )
	out_pVolumes->fCenter		= 0.f;
	out_pVolumes->fLfe			= 0.0f;
#endif // AK_LFECENTER

#if defined( AK_REARCHANNELS ) 
	// Apply to 2x2 panning only.
	out_pVolumes->fRearLeft		= 0.f;
	out_pVolumes->fRearRight	= 0.f;
#endif
#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif
}

// 1-position fade with routing of a portion of the signal (L-R) to center channel.
void _GetSpeakerVolumes2DPan1RouteToCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
#if defined( AK_LFECENTER )
	AkReal32 fCenter;
	AkReal32 fRight;
	if ( in_fX <= 0.5f )
	{
		fCenter = 2.f * in_fCenterPct * in_fX;
		fRight = in_fX * ( 1.f - in_fCenterPct );
	}
	else
	{
		fCenter = 2.f * in_fCenterPct * ( 1.f - in_fX );
		fRight = in_fX + in_fCenterPct * ( in_fX - 1.f );
	}
	AkReal32 fLeft = 1.f - fRight - fCenter;

	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fLeft );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fRight );
	out_pVolumes->fCenter		= AkSqrtEstimate( fCenter );
	out_pVolumes->fLfe			= 0.0f;
#else
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( 1.0f - in_fX );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( in_fX );
#endif // AK_LFECENTER

#if defined( AK_REARCHANNELS ) 
	// Apply to 2x2 panning only.
	out_pVolumes->fRearLeft		= 0.f;
	out_pVolumes->fRearRight	= 0.f;
#endif
#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif
}

#if defined( AK_LFECENTER )
// 1-position fade of source that has a center channel (3-stereo). 
void _GetSpeakerVolumes2DPan1HasCenter(
	AkReal32			in_fX,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	// Balance left-right.
	AkReal32 fLeft		= ( 2 - 2*in_fX ) / 3.f;
	AkReal32 fCenter	= 1/3.f;
	AkReal32 fRight		= ( 2 * in_fX ) / 3.f;

	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fLeft );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fRight );
	out_pVolumes->fCenter		= AkSqrtEstimate( fCenter );
	out_pVolumes->fLfe			= 0.0f;

#if defined( AK_REARCHANNELS ) 
	// Apply to 2x2 panning only.
	out_pVolumes->fRearLeft		= 0.f;
	out_pVolumes->fRearRight	= 0.f;
#endif
#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif
}
#endif // AK_LFECENTER


// 2-position fade family.
#ifdef AK_REARCHANNELS

// 2-position fade.
void _GetSpeakerVolumes2DPan2(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
#if defined( AK_LFECENTER )
	AkReal32 fRight = in_fX;
	AkReal32 fLeft = 1.f - fRight;
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fLeft * in_fY );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fRight * in_fY );
	out_pVolumes->fCenter		= 0.f;
	out_pVolumes->fLfe			= 0.f;
#else
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( 1.0f - in_fX );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( in_fX );
#endif // AK_LFECENTER

	AkReal32 fRearBalance = ( 1.0f - in_fY );
	out_pVolumes->fRearLeft		= AkSqrtEstimate( ( 1.0f - in_fX ) * fRearBalance );
	out_pVolumes->fRearRight	= AkSqrtEstimate( in_fX * fRearBalance );

#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif
}

#if defined( AK_LFECENTER )
// 2-position fade with routing of a portion of the signal (FL-FR) to center channel.
void _GetSpeakerVolumes2DPan2RouteToCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	AkReal32 fCenter;
	AkReal32 fRight;
	if ( in_fX <= 0.5f )
	{
		fCenter = 2.f * in_fCenterPct * in_fX;
		fRight = in_fX * ( 1.f - in_fCenterPct );
	}
	else
	{
		fCenter = 2.f * in_fCenterPct * ( 1.f - in_fX );
		fRight = in_fX + in_fCenterPct * ( in_fX - 1.f );
	}
	AkReal32 fLeft = 1.f - fRight - fCenter;

	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fLeft * in_fY );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fRight * in_fY );
	out_pVolumes->fCenter		= AkSqrtEstimate( fCenter * in_fY );
	out_pVolumes->fLfe			= 0.0f;

#if defined( AK_REARCHANNELS ) 
	AkReal32 fRearBalance = ( 1.0f - in_fY );
	out_pVolumes->fRearLeft		= AkSqrtEstimate( ( 1.0f - in_fX ) * fRearBalance );
	out_pVolumes->fRearRight	= AkSqrtEstimate( in_fX * fRearBalance );
#endif

#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif
}

// 2-position fade of source that has a center channel (3-stereo). 
void _GetSpeakerVolumes2DPan2HasCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	// Balance left-right.
	AkReal32 fFrontLeft = ( 2 - 2*in_fX ) / 3.f;
	AkReal32 fFrontCenter = 1/3.f;
	AkReal32 fFrontRight = ( 2 * in_fX ) / 3.f;

#if defined( AK_REARCHANNELS ) 
	AkReal32 fRearRight = in_fX;
	AkReal32 fRearLeft = 1 - in_fX;

	// Balance front-rear.
	// Note: Because our pad is square but the channels are not symmetric, the linear interpolation
	// has a different slope whether it is in the top or bottom half. At y = 0.5, power is evenly distributed.
	AkReal32 fRearBalance = ( in_fY >= 0.5 ) ? ( ( 4 - 4 * in_fY ) / 5.f ) : ( 1 - 6 * in_fY / 5.f );
	AkReal32 fFrontBalance = 1 - fRearBalance;

	fFrontLeft *= fFrontBalance;
	fFrontCenter *= fFrontBalance;
	fFrontRight *= fFrontBalance;	

	fRearLeft *= fRearBalance;
	fRearRight *= fRearBalance;

	AKASSERT( fFrontLeft + fFrontCenter + fFrontRight + fRearLeft + fRearRight > 1 - 0.00001 
			&& fFrontLeft + fFrontCenter + fFrontRight + fRearLeft + fRearRight < 1 + 0.00001 );

	out_pVolumes->fLfe	= 0.0f;

	//convert power to speaker gains
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fFrontLeft );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fFrontRight );
	out_pVolumes->fCenter		= AkSqrtEstimate( fFrontCenter );
	out_pVolumes->fRearLeft		= AkSqrtEstimate( fRearLeft );
	out_pVolumes->fRearRight	= AkSqrtEstimate( fRearRight );
#ifdef AK_71AUDIO
	// Apply to 3x2 panning only.
	out_pVolumes->fSideLeft		= 0.0f;
	out_pVolumes->fSideRight	= 0.0f;
#endif

#else	// !AK_REARCHANNELS
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fFrontLeft );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fFrontRight );
	out_pVolumes->fCenter		= AkSqrtEstimate( fFrontCenter );
#endif	// AK_REARCHANNELS
}
#endif // AK_LFECENTER // End of HasCenter/RouteToCenter family

#endif	// AK_REARCHANNELS	// End of 2-position fade family.

// 2D panning rules extended for 6/7.1 (sides and backs) (3-position fade). 
#ifdef AK_71AUDIO
// 3-position fade.
void _GetSpeakerVolumes2DPan3(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	// Balance front channels: center + left-right
	AkReal32 fFrontRight = in_fX;
	AkReal32 fFrontLeft = 1.f - fFrontRight;

	// Balance surround channels (side and back pairs).
	AkReal32 fSurroundRight = in_fX;
	AkReal32 fSurroundLeft = 1.f - in_fX;

	// Balance front-rear.
	AkReal32 fFrontBalance = ( 4.f * in_fY - 1.f ) / 3.f;
	if ( fFrontBalance < 0.f ) fFrontBalance = 0.f;
	AkReal32 fBackBalance = ( 3.f - 4.f * in_fY ) / 3.f;
	if ( fBackBalance < 0.f ) fBackBalance = 0.f;
	AkReal32 fSideBalance = 1 - ( fFrontBalance + fBackBalance );

	AKASSERT( fFrontLeft * fFrontBalance + fFrontRight * fFrontBalance + fSurroundLeft * fBackBalance + fSurroundLeft * fSideBalance + fSurroundRight * fBackBalance + fSurroundRight * fSideBalance > 1 - 0.00001 
			&& fFrontLeft * fFrontBalance + fFrontRight * fFrontBalance + fSurroundLeft * fBackBalance + fSurroundLeft * fSideBalance + fSurroundRight * fBackBalance + fSurroundRight * fSideBalance < 1 + 0.00001 );

	//convert power to speaker gains
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fFrontLeft * fFrontBalance );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fFrontRight * fFrontBalance );
	out_pVolumes->fCenter		= 0.f;
	out_pVolumes->fRearLeft		= AkSqrtEstimate( fSurroundLeft * fBackBalance );
	out_pVolumes->fRearRight	= AkSqrtEstimate( fSurroundRight * fBackBalance );
	out_pVolumes->fSideLeft		= AkSqrtEstimate( fSurroundLeft * fSideBalance );
	out_pVolumes->fSideRight	= AkSqrtEstimate( fSurroundRight * fSideBalance );
	out_pVolumes->fLfe			= 0.0f;
}

// 3-position fade with routing of a portion of the signal (FL-FR) to center channel.
void _GetSpeakerVolumes2DPan3RouteToCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	// Balance front channels: center + left-right
	AkReal32 fFrontCenter;
	AkReal32 fFrontRight;
	if ( in_fX <= 0.5f )
	{
		fFrontCenter = 2.f * in_fCenterPct * in_fX;
		fFrontRight = in_fX * ( 1.f - in_fCenterPct );
	}
	else
	{
		fFrontCenter = 2.f * in_fCenterPct * ( 1.f - in_fX );
		fFrontRight = in_fX + in_fCenterPct * ( in_fX - 1.f );
	}
	AkReal32 fFrontLeft = 1.f - fFrontRight - fFrontCenter;

	// Balance surround channels (side and back pairs).
	AkReal32 fSurroundRight = in_fX;
	AkReal32 fSurroundLeft = 1.f - in_fX;

	// Balance front-rear.
	AkReal32 fFrontBalance = ( 4.f * in_fY - 1.f ) / 3.f;
	if ( fFrontBalance < 0.f ) fFrontBalance = 0.f;
	AkReal32 fBackBalance = ( 3.f - 4.f * in_fY ) / 3.f;
	if ( fBackBalance < 0.f ) fBackBalance = 0.f;
	AkReal32 fSideBalance = 1 - ( fFrontBalance + fBackBalance );

	AKASSERT( fFrontLeft * fFrontBalance + fFrontRight * fFrontBalance + fFrontCenter * fFrontBalance + fSurroundLeft * fBackBalance + fSurroundLeft * fSideBalance + fSurroundRight * fBackBalance + fSurroundRight * fSideBalance > 1 - 0.00001 
			&& fFrontLeft * fFrontBalance + fFrontRight * fFrontBalance + fFrontCenter * fFrontBalance + fSurroundLeft * fBackBalance + fSurroundLeft * fSideBalance + fSurroundRight * fBackBalance + fSurroundRight * fSideBalance < 1 + 0.00001 );

	//convert power to speaker gains
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fFrontLeft * fFrontBalance );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fFrontRight * fFrontBalance );
	out_pVolumes->fCenter		= AkSqrtEstimate( fFrontCenter * fFrontBalance );
	out_pVolumes->fRearLeft		= AkSqrtEstimate( fSurroundLeft * fBackBalance );
	out_pVolumes->fRearRight	= AkSqrtEstimate( fSurroundRight * fBackBalance );
	out_pVolumes->fSideLeft		= AkSqrtEstimate( fSurroundLeft * fSideBalance );
	out_pVolumes->fSideRight	= AkSqrtEstimate( fSurroundRight * fSideBalance );
	out_pVolumes->fLfe			= 0.0f;
}

// 3-position fade of source that has a center channel (3-stereo). 
void _GetSpeakerVolumes2DPan3HasCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
	// Balance left-right.
	AkReal32 fFrontLeft = ( 2 - 2*in_fX )/3.f;
	AkReal32 fFrontCenter = 1/3.f;
	AkReal32 fFrontRight = 1 - ( fFrontLeft + fFrontCenter );

	AkReal32 fRearRight = in_fX;
	AkReal32 fRearLeft = 1 - in_fX;

	// Balance front-rear.
	// Note: Because our pad is square but the number of channels is not symmetric in the front and in the back, 
	// the linear interpolation has a different slope whether it is in the top or bottom half. 
	// At y = 0.5, power is evenly distributed.
	AkReal32 fFrontBalance = ( 8.f * in_fY - 1.f ) / 7.f;
	if ( fFrontBalance < 0.f ) fFrontBalance = 0.f;
	AkReal32 fBackBalance = ( 7.f - 10.f * in_fY ) / 7.f;
	if ( fBackBalance < 0.f ) fBackBalance = 0.f;
	AkReal32 fSideBalance = 1 - ( fFrontBalance + fBackBalance );

	fFrontLeft *= fFrontBalance;
	fFrontCenter *= fFrontBalance;
	fFrontRight *= fFrontBalance;	

	AKASSERT( fFrontLeft + fFrontCenter + fFrontRight + fRearLeft * fBackBalance + fRearLeft * fSideBalance + fRearRight * fBackBalance + fRearRight * fSideBalance > 1 - 0.00001 
			&& fFrontLeft + fFrontCenter + fFrontRight + fRearLeft * fBackBalance + fRearLeft * fSideBalance + fRearRight * fBackBalance + fRearRight * fSideBalance < 1 + 0.00001 );

	//convert power to speaker gains
	out_pVolumes->fFrontLeft	= AkSqrtEstimate( fFrontLeft );
	out_pVolumes->fFrontRight	= AkSqrtEstimate( fFrontRight );
	out_pVolumes->fCenter		= AkSqrtEstimate( fFrontCenter );
	out_pVolumes->fRearLeft		= AkSqrtEstimate( fRearLeft * fBackBalance );
	out_pVolumes->fRearRight	= AkSqrtEstimate( fRearRight * fBackBalance );
	out_pVolumes->fSideLeft		= AkSqrtEstimate( fRearLeft * fSideBalance );
	out_pVolumes->fSideRight	= AkSqrtEstimate( fRearRight * fSideBalance );
	out_pVolumes->fLfe			= 0.0f;
}
#endif // AK_71AUDIO	// End of 3-position fade family

void CAkSpeakerPan::GetSpeakerVolumes2DPan(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	bool				in_bIsPannerEnabled,
	AkChannelMask		in_uInputConfig,
	AkChannelMask		in_uOutputConfig,
	AkSIMDSpeakerVolumes*	out_pVolumes
	)
{
	// We care only about fullband channels.
	// Handling of LFE is always done outside.
#ifdef AK_LFECENTER
	in_uInputConfig = ( in_uInputConfig & ~AK_SPEAKER_LOW_FREQUENCY );
	in_uOutputConfig = ( in_uOutputConfig & ~AK_SPEAKER_LOW_FREQUENCY );
#endif
	switch ( in_uInputConfig )
	{
	case 0:	// No full band channel: bail out.
		break;
	case AK_SPEAKER_SETUP_MONO:		// Mono

		if ( !in_bIsPannerEnabled )
		{
			// Direct speaker assignment:
#ifdef AK_LFECENTER
			// Ignore center% if output config has no center channel or if it is mono.
			if ( ( in_uOutputConfig & AK_SPEAKER_FRONT_CENTER )
				&& ( in_uOutputConfig & ~AK_SPEAKER_FRONT_CENTER ) )
			{
				// Handle mono case with center %.
				// Note: Mono->mono case does not need to be handled separately. "To mono" downmix is such that 
				// we may freely swap fCenter = 1 with constant power distribution across the front channels.
				AkReal32 fSides = AkSqrtEstimate( (1.f - in_fCenterPct) * 0.5f );
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fFrontLeft	= fSides;
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fFrontRight	= fSides;
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fCenter		= AkSqrtEstimate( in_fCenterPct );
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fLfe		= 0.f;
			}
			else
#endif
			{
				// No center; we express mono signal paths by distributing power equally onto left and right
				// channels. This is perfectly invertible with "to mono" AC3 downmix recipe with -3dB normalization.
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fFrontLeft	= ONE_OVER_SQRT_OF_TWO;
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fFrontRight	= ONE_OVER_SQRT_OF_TWO;
#ifdef AK_LFECENTER
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fCenter		= 0.f;
				out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fLfe		= 0.f;
#endif
			}

#ifdef AK_REARCHANNELS
			out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fRearLeft = 0.f;
			out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fRearRight = 0.f;
#endif
#ifdef AK_71AUDIO
			out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fSideLeft = 0.f;
			out_pVolumes[AK_IDX_SETUP_1_CENTER].volumes.fSideRight = 0.f;
#endif
		}
		else
		{
			// 2D panning:
			// Result of _GetSpeakerVolumes2DPan() is used directly as the distribution of our mono input to each output channel.
#ifdef AK_71AUDIO
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_7 )
				_GetSpeakerVolumes2DPan3RouteToCenter( in_fX, in_fY, in_fCenterPct, (AkSpeakerVolumes*)out_pVolumes );
			else if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 )
				_GetSpeakerVolumes2DPan3( in_fX, in_fY, (AkSpeakerVolumes*)out_pVolumes );
			else 
#endif
#ifdef AK_REARCHANNELS
#ifdef AK_LFECENTER
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_5 )
				_GetSpeakerVolumes2DPan2RouteToCenter( in_fX, in_fY, in_fCenterPct, (AkSpeakerVolumes*)out_pVolumes );
			else 
#endif
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_4 )
				_GetSpeakerVolumes2DPan2( in_fX, in_fY, (AkSpeakerVolumes*)out_pVolumes );
			else 
#endif
#ifdef AK_LFECENTER
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_3STEREO )
				_GetSpeakerVolumes2DPan1RouteToCenter( in_fX, in_fCenterPct, (AkSpeakerVolumes*)out_pVolumes );
			else
#endif
				_GetSpeakerVolumes2DPan1( in_fX, (AkSpeakerVolumes*)out_pVolumes );
		}
		break;

	case AK_SPEAKER_SETUP_STEREO:	// Stereo

		// Direct speaker assignment:
#if !defined(AK_LFECENTER) && !defined(AK_REARCHANNELS)
		// Optimized 2-channel platform case:
		out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fFrontRight = 0.f;
		out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fFrontLeft = 0.f;
		out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fFrontRight = 1.f;
#else
		memset( out_pVolumes, 0, 2 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fFrontRight = 1.f;
#endif

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{				
			// Compute recipe for even number of channels: use standard 2D pan with center% = 0.
			AkSIMDSpeakerVolumes tempAudioMix;
#ifdef AK_71AUDIO
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
			{
				// Bleed to rear and side channels.
				out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fSideLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fRearRight	= 1.0f;
				out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fSideRight	= 1.0f;
				_GetSpeakerVolumes2DPan3( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else 
#endif
#ifdef AK_REARCHANNELS
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_4 || in_uOutputConfig == AK_SPEAKER_SETUP_5 )
			{
				// Bleed to rear channels.
				out_pVolumes[AK_IDX_SETUP_2_LEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_2_RIGHT].volumes.fRearRight	= 1.0f;
				_GetSpeakerVolumes2DPan2( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else
#endif
			{
				_GetSpeakerVolumes2DPan1( in_fX, &tempAudioMix.volumes );
			}

			out_pVolumes[AK_IDX_SETUP_2_LEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_2_RIGHT].Mul( tempAudioMix );
		}
		break;

#if defined( AK_LFECENTER )
	case AK_SPEAKER_SETUP_3STEREO: 

		// Direct speaker assignment:
		memset( out_pVolumes, 0, 3 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fFrontRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fCenter = 1.f;

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{
			AkSIMDSpeakerVolumes tempAudioMix;
#ifdef AK_71AUDIO
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
			{
				// Bleed front and center to rear and side channels.
				out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fSideLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fRearRight	= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fSideRight	= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearLeft	= 0.5f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fSideLeft	= 0.5f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearRight	= 0.5f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fSideRight	= 0.5f;
				_GetSpeakerVolumes2DPan3HasCenter( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else 
#endif
#ifdef AK_REARCHANNELS
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_4 || in_uOutputConfig == AK_SPEAKER_SETUP_5 )
			{
				// Bleed front and center to rear channels.
				out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fRearRight	= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearLeft	= 0.5f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearRight	= 0.5f;
				_GetSpeakerVolumes2DPan2HasCenter( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else
#endif
			{
				_GetSpeakerVolumes2DPan1HasCenter( in_fX, &tempAudioMix.volumes );
			}

			out_pVolumes[AK_IDX_SETUP_3_LEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_3_RIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_3_CENTER].Mul( tempAudioMix );
		}
		break;
#endif // AK_LFECENTER
 
#if defined( AK_REARCHANNELS ) 
	case AK_SPEAKER_SETUP_4: 

		// Direct speaker assignment:
		memset( out_pVolumes, 0, 4 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_4_FRONTLEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_4_FRONTRIGHT].volumes.fFrontRight = 1.f;
#ifdef AK_71AUDIO
		if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
		{
			// With 5.1 sounds in 7.1 setup, 5.1 "rear channels" need to be routed to side.
			out_pVolumes[AK_IDX_SETUP_4_REARLEFT].volumes.fSideLeft = 1.f;
			out_pVolumes[AK_IDX_SETUP_4_REARRIGHT].volumes.fSideRight = 1.f;
		}
		else
#endif
		{
			out_pVolumes[AK_IDX_SETUP_4_REARLEFT].volumes.fRearLeft = 1.f;
			out_pVolumes[AK_IDX_SETUP_4_REARRIGHT].volumes.fRearRight = 1.f;
		}

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{
			// Compute recipe for even number of channels: use standard 2D pan with center% = 0.
			AkSIMDSpeakerVolumes tempAudioMix;
#ifdef AK_71AUDIO
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
			{
				// surround channels go to rear and side. 
				out_pVolumes[AK_IDX_SETUP_4_REARLEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_4_REARRIGHT].volumes.fRearRight	= 1.0f;
				_GetSpeakerVolumes2DPan3( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else 
#endif
			// Relying on downmix. if ( in_uOutputConfig == AK_SPEAKER_SETUP_4 || in_uOutputConfig == AK_SPEAKER_SETUP_5 )
			{
				_GetSpeakerVolumes2DPan2( in_fX, in_fY, &tempAudioMix.volumes );
			}

			out_pVolumes[AK_IDX_SETUP_4_FRONTLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_4_FRONTRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_4_REARLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_4_REARRIGHT].Mul( tempAudioMix );
		}
		break;
#endif // defined( AK_REARCHANNELS )

#if defined( AK_REARCHANNELS ) && defined( AK_LFECENTER )
	case AK_SPEAKER_SETUP_5:

		// Direct speaker assignment:
		memset( out_pVolumes, 0, 5 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_5_FRONTLEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_5_FRONTRIGHT].volumes.fFrontRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_5_CENTER].volumes.fCenter = 1.f;
#ifdef AK_71AUDIO
		if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
		{
			// With 5.1 sounds in 7.1 setup, 5.1 "rear channels" need to be routed to side.
			out_pVolumes[AK_IDX_SETUP_5_REARLEFT].volumes.fSideLeft = 1.f;
			out_pVolumes[AK_IDX_SETUP_5_REARRIGHT].volumes.fSideRight = 1.f;
		}
		else
#endif
		{
			out_pVolumes[AK_IDX_SETUP_5_REARLEFT].volumes.fRearLeft = 1.f;
			out_pVolumes[AK_IDX_SETUP_5_REARRIGHT].volumes.fRearRight = 1.f;
		}

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{
			// Compute recipe for odd number of channels.
			AkSIMDSpeakerVolumes tempAudioMix;
#ifdef AK_71AUDIO
			if ( in_uOutputConfig == AK_SPEAKER_SETUP_6 || in_uOutputConfig == AK_SPEAKER_SETUP_7 )
			{
				// Bleed surround channels to back channels
				out_pVolumes[AK_IDX_SETUP_5_REARLEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_5_REARRIGHT].volumes.fRearRight	= 1.0f;
				_GetSpeakerVolumes2DPan3HasCenter( in_fX, in_fY, &tempAudioMix.volumes );
			}
			else 
#endif
			// Relying on downmix. if ( in_uOutputConfig == AK_SPEAKER_SETUP_4 || in_uOutputConfig == AK_SPEAKER_SETUP_5 )
			{
				_GetSpeakerVolumes2DPan2HasCenter( in_fX, in_fY, &tempAudioMix.volumes );
			}
			
			out_pVolumes[AK_IDX_SETUP_5_FRONTLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_5_FRONTRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_5_CENTER].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_5_REARLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_5_REARRIGHT].Mul( tempAudioMix );
		}
		break;
#endif // defined( AK_REARCHANNELS ) && defined( AK_LFECENTER )

#if defined( AK_71AUDIO )
	case AK_SPEAKER_SETUP_6:

		// Direct speaker assignment:
		memset( out_pVolumes, 0, 6 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_6_FRONTLEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_6_FRONTRIGHT].volumes.fFrontRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_6_REARLEFT].volumes.fRearLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_6_REARRIGHT].volumes.fRearRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_6_SIDELEFT].volumes.fSideLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_6_SIDERIGHT].volumes.fSideRight = 1.f;

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{
			// Perform 3x2 panning. If the output config is smaller, we rely on standard mixdown rules as usual.
			AkSIMDSpeakerVolumes tempAudioMix;
			_GetSpeakerVolumes2DPan3( in_fX, in_fY, &( tempAudioMix.volumes ) );
			out_pVolumes[AK_IDX_SETUP_6_FRONTLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_6_FRONTRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_6_REARLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_6_REARRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_6_SIDELEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_6_SIDERIGHT].Mul( tempAudioMix );
		}
		break;
	case AK_SPEAKER_SETUP_7:
		
		// Direct speaker assignment:
		memset( out_pVolumes, 0, 7 * sizeof(AkSIMDSpeakerVolumes) );
		out_pVolumes[AK_IDX_SETUP_7_FRONTLEFT].volumes.fFrontLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_FRONTRIGHT].volumes.fFrontRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_CENTER].volumes.fCenter = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_REARLEFT].volumes.fRearLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_REARRIGHT].volumes.fRearRight = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_SIDELEFT].volumes.fSideLeft = 1.f;
		out_pVolumes[AK_IDX_SETUP_7_SIDERIGHT].volumes.fSideRight = 1.f;

		// 2D panning:
		if ( in_bIsPannerEnabled )
		{
			// Perform 3x2 panning. If the output config is smaller, we rely on standard mixdown rules as usual.
			AkSIMDSpeakerVolumes tempAudioMix;
			_GetSpeakerVolumes2DPan3HasCenter( in_fX, in_fY, &( tempAudioMix.volumes ) );
			out_pVolumes[AK_IDX_SETUP_7_FRONTLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_FRONTRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_CENTER].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_REARLEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_REARRIGHT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_SIDELEFT].Mul( tempAudioMix );
			out_pVolumes[AK_IDX_SETUP_7_SIDERIGHT].Mul( tempAudioMix );
		}
		break;
#endif

	default:
		AKASSERT( !"Channel config not supported" );
		break;
	}
}

void CAkSpeakerPan::GetDefaultSpeakerAngles( 
	AkChannelMask	in_channelMask,		// Desired channel config.
	AkReal32 		out_angles[]		// Returned angles for given channel config. Must be preallocated (use AK::GetNumberOfAnglesForConfig()).
	)
{
	// Defaults.
#if !defined(AK_REARCHANNELS)
	out_angles[0] = 45.f;	// Stereo-only platform: widen angle (use 45 degrees, like before).
#else
	out_angles[0] = 30.f;	// Front channels of surround setup.
	out_angles[1] = 110.f;	// Surround channels of 5.1 setup
#ifdef AK_71AUDIO
	out_angles[2] = 142.5f;	// Rear channels of 7.1 setup	

	// According to end point configuration, defaults may change slightly.
	if ( in_channelMask & AK_SPEAKER_SETUP_SIDE )
	{
		// 7.1 config: Move side speakers closer to sides.
		out_angles[1] = 100.f;	// Side channels of 7.1 setup == surround channels of 5.1 setup
	}
#endif
	if ( ( in_channelMask & ~AK_SPEAKER_SETUP_FRONT ) == 0 )
	{
		// Just front channels. Widen stereo pair angle.
		out_angles[0] = 45.f;
	}		
#endif
}

AKRESULT CAkSpeakerPan::SetSpeakerAngles(
	const AkReal32 *	in_pfSpeakerAngles,			// Array of loudspeaker pair angles, expressed in degrees relative to azimuth ([0,180]).
	AkUInt32			in_uNumAngles,				// Number of loudspeaker pair angles.
	AkUInt32 *			out_uSpeakerAngles,			// Pointer to buffer filled with speaker angles (in internal integer angle units).
	AkUInt32 &			out_uMinAngleBetweenSpeakers// Returned minimum angle between speakers (in internal integer angle units).
	)
{
	AKASSERT( in_uNumAngles > 0 );

	// Convert speaker angles to uint 0-PAN_CIRCLE. 
	for ( AkUInt32 uAngle = 0; uAngle < in_uNumAngles; uAngle++ )
	{
		out_uSpeakerAngles[uAngle] = AkMath::RoundU( PAN_CIRCLE * in_pfSpeakerAngles[uAngle] / 360.f );
		if ( out_uSpeakerAngles[uAngle] >= PAN_CIRCLE/2 )
		{
			AKASSERT( !"Angle out of range" );
			return AK_Fail;
		}
	}

	// Find and store the minimum angle. 
	// At the same time, ensure that they respect the following constraints: 
	// - Values [0,180]
	// - Any angle smaller than 180 degrees (speaker 0 must be smaller than 90 degrees).
	// - Increasing order
	AkUInt32 uMinAngleBetweenSpeakers = out_uSpeakerAngles[0];
	if ( uMinAngleBetweenSpeakers >= PAN_CIRCLE/4 )
	{
		AKASSERT( !"uSpeakerAngles[0] must be smaller than 90 degrees" );
		return AK_Fail;
	}
	AkUInt32 uSpeaker = 1;
	while ( uSpeaker < in_uNumAngles )
	{
		// Check each interval.
		if ( out_uSpeakerAngles[uSpeaker] < out_uSpeakerAngles[uSpeaker-1] )
		{
			AKASSERT( !"Angles need to be in increasing order" );
			return AK_Fail;
		}
		AkUInt32 uInterval = out_uSpeakerAngles[uSpeaker] - out_uSpeakerAngles[uSpeaker-1];
		if ( uInterval == 0 || uInterval >= PAN_CIRCLE/2 )
		{
			AKASSERT( !"Speaker interval out of range ]0,180[" );
			return AK_Fail;
		}
		if ( uInterval < uMinAngleBetweenSpeakers )
			uMinAngleBetweenSpeakers = out_uSpeakerAngles[uSpeaker] - out_uSpeakerAngles[uSpeaker-1];
		++uSpeaker;
	}
	// Check interval between last speaker and its image on the other side, unless there is only 2 channels.
	if ( uSpeaker > 1 )	// not stereo.
	{
		AkUInt32 uInterval = PAN_CIRCLE - 2 * out_uSpeakerAngles[uSpeaker-1];
		if ( uInterval < uMinAngleBetweenSpeakers )
			uMinAngleBetweenSpeakers = uInterval;		
	}
	
	out_uMinAngleBetweenSpeakers = uMinAngleBetweenSpeakers;

	return AK_Success;
}

// Convert angles stored into device (as integers) to SDK-friendly speaker angles (in radians)
void CAkSpeakerPan::ComputePlanarVBAPGains( 
	AkDevice *		in_pDevice,				// Output device.
	AkReal32		in_fAngle,				// Incident angle, in radians [-pi,pi], where 0 is the azimuth (positive values are clockwise)
	AkChannelMask	in_uOutputConfig,		// Desired output configuration. 
	AkSpeakerVolumes & out_volumes			// Returned volumes.
	)
{
	AkSIMDSpeakerVolumes volumesSIMD;
	volumesSIMD.Set( 0.0f ); //initialize output values

	in_fAngle = -in_fAngle;
	AkInt32 iAngle = AkMath::Round( in_fAngle * PAN_CIRCLE * ONEOVERTWOPI );

	// Ignore LFE.
	in_uOutputConfig = in_uOutputConfig & ~AK_SPEAKER_LOW_FREQUENCY;

	// Handle mono output.
	if ( in_uOutputConfig == AK_SPEAKER_SETUP_MONO )
	{
#ifdef AK_LFECENTER
		volumesSIMD.volumes.fCenter = 1.f;
#else
		volumesSIMD.volumes.fFrontLeft = ONE_OVER_SQRT_OF_TWO;
		volumesSIMD.volumes.fFrontRight = ONE_OVER_SQRT_OF_TWO;
#endif
	}
	else
	{
		// Get pan table according to output config (without center).
#ifdef AK_LFECENTER
		AkChannelMask uOutputConfigNoCenter = in_uOutputConfig & ~AK_SPEAKER_FRONT_CENTER;
#else
		AkChannelMask uOutputConfigNoCenter = in_uOutputConfig;
#endif

		/// LX TODO API should return an error code.
		AKVERIFY( in_pDevice->EnsurePanCacheExists( uOutputConfigNoCenter ) == AK_Success );
#ifdef AK_LFECENTER
		if ( in_uOutputConfig != uOutputConfigNoCenter )
			AKVERIFY( in_pDevice->EnsurePanCacheExists( in_uOutputConfig ) == AK_Success );
#endif

		CAkSpeakerPan::PanPair * pPanTableNoCenter = in_pDevice->GetPanTable( uOutputConfigNoCenter );

		CAkSpeakerPan::AddSpeakerVolumesPower( iAngle, 1.f, in_uOutputConfig, pPanTableNoCenter, in_pDevice, &volumesSIMD.volumes );
		volumesSIMD.Sqrt(); //convert power back to a gain

		volumesSIMD.CopyTo( out_volumes );
	}
}

// Convert angles stored into device (as integers) to SDK-friendly speaker angles (in radians)
void CAkSpeakerPan::ConvertSpeakerAngles(
	AkUInt32		in_uDeviceSpeakerAngles[],	// Speaker angles stored in device.
	AkUInt32		in_uNumAngles,				// Number of angle values.
	AkReal32		out_arAngles[]				// Returned speaker angles, in degrees (see AkOutputSettings::fSpeakerAngles). Pass in an array of size in_uNumAngles.
	)
{
	for ( AkUInt32 uAngle = 0; uAngle < in_uNumAngles; uAngle++ )
	{
		out_arAngles[uAngle] = 360.f * in_uDeviceSpeakerAngles[uAngle] / (AkReal32)PAN_CIRCLE;
	}
}

