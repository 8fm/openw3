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
// AkTransition.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _TRANSITION_H_
#define _TRANSITION_H_

#include "ITransitionable.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"
#include "AudiolibDefs.h"
#include "AkSettings.h"

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
class CAkTransition
{
// the one that will get things moving
	friend	class CAkTransitionManager;

public:
	CAkTransition();
	~CAkTransition();

	// Getters.
	inline AkReal32 GetTargetValue() { return m_fTargetValue; }

	// cleans up before leaving
	void Term();

	// get it ready for re-use
	void Reset();

    // applies a time offset to transition (changes transition's current time)
    // non-reversible, apply after TransitionMgr's creation.
    void Offset(
        AkInt32 in_iOffset  // time offset (in audio frames -buffer ticks-)
        ) 
    { 
		if ( in_iOffset >= 0 
			||  m_uStartTimeInBufferTick > (AkUInt32) -in_iOffset )
		{
	        m_uStartTimeInBufferTick += in_iOffset; 
		}
		else 
		{
			m_uStartTimeInBufferTick = 0;
		}
    }

	// computes next value according to current time
	bool	ComputeTransition( AkUInt32 in_CurrentBufferTick );

	static AkForceInline AkUInt32 Convert( AkTimeMs in_ValueInTime )
	{
		/*
		AkUInt32 l_ValueInNumBuffer = in_ValueInTime/AK_MS_PER_BUFFER_TICK;
		l_ValueInNumBuffer += in_ValueInTime%AK_MS_PER_BUFFER_TICK ? 1 : 0;

		return l_ValueInNumBuffer;
		*/

		// Approximation is nearly as accurate and way faster.
		return ( in_ValueInTime + AK_MS_PER_BUFFER_TICK - 1 ) / AK_MS_PER_BUFFER_TICK;
	}

private:

	// fill it in
	AKRESULT InitParameters( const class TransitionParameters& in_Params, AkUInt32 in_CurrentBufferTick );

	// which one should be used
	AkIntPtr	m_eTarget;

	// Initial value.
	// Will be useful to calculate the interpolation of non-linear curves.{AkUInt32, AkReal32, etc}
	AkReal32	m_fStartValue;

	// Target Volume of the fade {AkUInt32, AkReal32, etc}
	AkReal32	m_fTargetValue;

	// Current value, used for parameter changing
	AkReal32	m_fCurrentValue;

	// Start time of the fade in Buffer ticks
	AkUInt32	m_uStartTimeInBufferTick;

	// Time it will take in Buffer ticks
	AkUInt32	m_uDurationInBufferTick;

	// how far it is in the duration
	AkReal32	m_fTimeRatio;

	// Last time it was processed, used for paused transitions
	AkUInt32	m_uLastBufferTickUpdated;

	// our transition's possible states
	enum State
	{
		Idle		= 0,	// created
		Running		= 1,	// transitioning some value
		ToPause		= 2,	// pause request
		Paused		= 3,	// transitioning suspended
		ToResume	= 4,	// resume request
		Done		= 5,	// transition ended
		ToRemove	= 6		// remove from active list request
	};

	// list of those we have to call
	typedef AkArray<ITransitionable*, ITransitionable*,ArrayPoolDefault, LIST_POOL_BLOCK_SIZE/sizeof(ITransitionable*)> AkTransitionUsersList;
	AkTransitionUsersList	m_UsersList;

	// Curve to use for the transition
	AkCurveInterpolation			m_eFadeCurve;

	// our transition's current state
	State		m_eState;

	// values are dBs
	bool		m_bdBs :1;

	// this one tells ChangeParameter that it is called before ProcessTransitionList had a chance to run
	bool		m_bCurrentValueSet :1;

	AkUInt8		m_iNumUsers;
};
//----------------------------------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------------------------------
class TransitionParameters
{
public:
	TransitionParameters( 
		ITransitionable*	in_pUser,
		AkIntPtr			in_eTarget,
		AkReal32			in_fStartValue,
		AkReal32			in_fTargetValue,
		AkTimeMs			in_lDuration,
		AkCurveInterpolation in_eFadeCurve,
		bool				in_bdBs,
		bool				in_bUseReciprocalCurve = true )
		: pUser( in_pUser )
		, eTarget( in_eTarget )
		, fStartValue( in_fStartValue )
		, fTargetValue( in_fTargetValue )
		, lDuration( in_lDuration )
		, eFadeCurve( in_eFadeCurve )
		, bdBs( in_bdBs )
		, bUseReciprocalCurve( in_bUseReciprocalCurve )
	{}
	~TransitionParameters() {}

	ITransitionable*	pUser;			// who will update the value
	AkIntPtr			eTarget;		// what's in the union
	AkReal32			fStartValue;	// where we are starting from
	AkReal32			fTargetValue;	// where we	are going to
	AkTimeMs			lDuration;		// how long it will take
	AkCurveInterpolation eFadeCurve;	// what shape to use
	bool				bdBs;			// start and target are dB values
	bool				bUseReciprocalCurve;	// use reciprocal curve when fading out

private:
	TransitionParameters();
};
#endif
