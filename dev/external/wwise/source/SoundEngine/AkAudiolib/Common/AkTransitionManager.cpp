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
// AkTransitionManager.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkTransitionManager.h"
#include "AkList2.h"
#include "AkMath.h"
#include "AudiolibDefs.h"
#include "AkTransition.h"
#include "AkInterpolation.h"
#include "AkAudioMgr.h"

//====================================================================================================
//====================================================================================================
CAkTransitionManager::CAkTransitionManager()
{
	m_uMaxNumTransitions = 0;
}
//====================================================================================================
//====================================================================================================
CAkTransitionManager::~CAkTransitionManager()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkTransitionManager::Init( AkUInt32 in_uMaxNumTransitions )
{
	m_uMaxNumTransitions = in_uMaxNumTransitions ? in_uMaxNumTransitions : DEFAULT_MAX_NUM_TRANSITIONS;

	AKRESULT eResult = m_ActiveTransitionsList_Fade.Reserve( m_uMaxNumTransitions );
	if( eResult == AK_Success )
		eResult = m_ActiveTransitionsList_State.Reserve( m_uMaxNumTransitions );

	return eResult;
}
//====================================================================================================
//====================================================================================================
void CAkTransitionManager::Term()
{
	// anyone got lost in here ?
	TermList( m_ActiveTransitionsList_Fade );
	TermList( m_ActiveTransitionsList_State );
}

void CAkTransitionManager::TermList( AkTransitionList& in_rTransitionList )
{
	if(!in_rTransitionList.IsEmpty())
	{
		for( AkTransitionList::Iterator iter = in_rTransitionList.Begin(); iter != in_rTransitionList.End(); ++iter )
		{
			CAkTransition* pTransition = *iter;
			// force transition to end to ensure clients are detached (WG-19176)
			pTransition->ComputeTransition( pTransition->m_uStartTimeInBufferTick + pTransition->m_uDurationInBufferTick );
			// get rid of the transition
			pTransition->Term();
			AkDelete( g_DefaultPoolId, pTransition );
		}
	}
	in_rTransitionList.Term();
}
//====================================================================================================
// add a new one to the list of those to be processed
//====================================================================================================
CAkTransition* CAkTransitionManager::AddTransitionToList(const TransitionParameters& in_Params,bool in_bStart,AkTransitionCategory in_eTransitionCategory)
{	
	AkTransitionList* pTransitionList = &m_ActiveTransitionsList_Fade;
	if( in_eTransitionCategory == TC_State )
	{
		pTransitionList = &m_ActiveTransitionsList_State;
	}
	else
	{
		pTransitionList = &m_ActiveTransitionsList_Fade;
	}

	CAkTransition* pThisTransition = NULL;

	// can we add a new one ?
	if(pTransitionList->Length() < m_uMaxNumTransitions)
	{
		// get a new one
		pThisTransition = AkNew( g_DefaultPoolId, CAkTransition );
	}
	// find a transition that can be stopped
	if( !pThisTransition )
	{
		// look for the one closest to completion
		AkReal32 fBiggestTimeRatio = -1.0f;
		AkTransitionList::Iterator iter = pTransitionList->Begin();
		while(iter != pTransitionList->End())
		{
			CAkTransition* pTransition = *iter;
			// is this one closer to completion ?
			if(pTransition->m_fTimeRatio > fBiggestTimeRatio)
			{
				pThisTransition = pTransition;
				fBiggestTimeRatio = pTransition->m_fTimeRatio;
			}
			++iter;
		}
		// if we've got one then stop it and clean it up
		if(pThisTransition != NULL)
		{
			// force transition to end
			pThisTransition->ComputeTransition( pThisTransition->m_uStartTimeInBufferTick + pThisTransition->m_uDurationInBufferTick );

			// clean it up to be re-used
			pThisTransition->Reset();
			pTransitionList->RemoveSwap( pThisTransition );
		}
	}

	// have we got one ?
	if(pThisTransition != NULL)
	{
		// fill it in and check that it was ok
		if(pThisTransition->InitParameters( in_Params, g_pAudioMgr->GetBufferTick() ) != AK_Fail)
		{
			// add it to the active list
			if( pTransitionList->AddLast( pThisTransition ) )
			{
				if(in_bStart)
				{
					// start it
					pThisTransition->m_eState = CAkTransition::Running;
				}
			}
			else
			{
				pThisTransition->Term();
				AkDelete( g_DefaultPoolId, pThisTransition );
				pThisTransition = NULL;
			}
		}
		// couldn't fill it in for some reason, get rid of it
		else
		{
			pThisTransition->Term();
			AkDelete( g_DefaultPoolId, pThisTransition );
			pThisTransition = NULL;
		}
	}

	if( pThisTransition == NULL )
	{
		AKASSERT( in_Params.pUser );
		// We ended up in a situation where a memory allocation failure most likely occured
		// That caused an impossibility to Create a requested transition.
		// Fallback on faking the end of the transition to avoit the command being lost.(WG-23935)
		in_Params.pUser->TransUpdateValue( 
			in_Params.eTarget, 
			in_Params.fTargetValue, 
			true //Completed/bDone
			);
	}
	return pThisTransition;
}
//====================================================================================================
// adds a given user to a given multi user transition
//====================================================================================================
AKRESULT CAkTransitionManager::AddTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// assume already in list
	AKRESULT eResult = AK_UserAlreadyInList;

	if(!in_pTransition->m_UsersList.Exists(in_pUser))
	{
		eResult = AK_UsersListFull;

		// can we add a new one ?
		if(in_pTransition->m_iNumUsers < AK_TRANSITION_USERS_LIST_SIZE
			&& in_pTransition->m_UsersList.AddLast(in_pUser) )
		{
			eResult = AK_Success;

			// we have one more
			++in_pTransition->m_iNumUsers;
		}
	}

	return(eResult);
}

bool CAkTransitionManager::IsTerminated( CAkTransition* in_pTransition )
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	return ( in_pTransition->m_eState == CAkTransition::Done );
}

//====================================================================================================
// removes a given user from a given multi user transition
//====================================================================================================
AKRESULT CAkTransitionManager::RemoveTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// assume we don't have this user
	AKRESULT eResult = AK_UserNotInList;

	// look for our user in the list
	CAkTransition::AkTransitionUsersList::Iterator iter = in_pTransition->m_UsersList.Begin();
	while( iter != in_pTransition->m_UsersList.End() )
	{
		ITransitionable* pITransitionable = *iter;
		// got it ?
		if(pITransitionable == in_pUser)
		{
		eResult = AK_Success;

		// remove it
			iter = in_pTransition->m_UsersList.EraseSwap( iter );

			// we have one less
			--in_pTransition->m_iNumUsers;
			
			if( in_pTransition->m_iNumUsers == 0 )
		{
			RemoveTransitionFromList( in_pTransition );
			}

			break;
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}
//====================================================================================================
// flags a transition for removal
//====================================================================================================
void CAkTransitionManager::RemoveTransitionFromList(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// then remove it
	in_pTransition->m_eState = CAkTransition::ToRemove;
}
//====================================================================================================
// flags a transition for pause
//====================================================================================================
void CAkTransitionManager::Pause(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	AKASSERT(in_pTransition != NULL);

	// is it pause-able ?
	if(in_pTransition->m_eState == CAkTransition::Running)
	{
		// then pause it
		in_pTransition->m_eState = CAkTransition::ToPause;
	}
	else if (in_pTransition->m_eState == CAkTransition::ToResume)
	{
		// we were already RESUMED within this frame. We can revert it.
		in_pTransition->m_eState = CAkTransition::Paused;
	}
}
//====================================================================================================
// flags a transition for un-pause
//====================================================================================================
void CAkTransitionManager::Resume(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	AKASSERT(in_pTransition != NULL);

	// is it resume-able
	if(in_pTransition->m_eState == CAkTransition::Paused)
	{
		// then resume it
		in_pTransition->m_eState = CAkTransition::ToResume;
	}
	else if (in_pTransition->m_eState == CAkTransition::ToPause)
	{
		// we were already PAUSED within this frame. We can revert it.
		in_pTransition->m_eState = CAkTransition::Running;
	}
}
//====================================================================================================
// changes the target and duration values
//====================================================================================================
void CAkTransitionManager::ChangeParameter(CAkTransition*		in_pTransition,
											AkIntPtr			in_eTarget,
											AkReal32			in_NewTarget,
											AkTimeMs			in_NewDuration,
											AkCurveInterpolation in_eCurveType,
											AkValueMeaning		in_eValueMeaning)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	AKASSERT(in_pTransition != NULL);
	
	AkIntPtr l_oldTarget = in_pTransition->m_eTarget;

	// set the new target type
	in_pTransition->m_eTarget = in_eTarget;

//----------------------------------------------------------------------------------------------------
// process dB values
//----------------------------------------------------------------------------------------------------
	if(in_pTransition->m_bdBs)
	{
		// did it get processed at least once ?
		if(in_pTransition->m_bCurrentValueSet)
		{
			// this is our new starting point
			in_pTransition->m_fStartValue = in_pTransition->m_fCurrentValue;

			// convert dBs to linear
			in_pTransition->m_fStartValue = AkMath::dBToLin( in_pTransition->m_fStartValue );
			in_NewTarget = AkMath::dBToLin( in_NewTarget );
		}
		// nope, keep the same start value
		else
		{
			// convert dBs to linear
			in_NewTarget = AkMath::dBToLin( in_NewTarget );
		}

		// is our new ending point an offset ?
		if(in_eValueMeaning == AkValueMeaning_Offset)
		{
			in_pTransition->m_fTargetValue *= in_NewTarget;
		}
		// nope it's not, fill in the union with whatever we got
		else
		{
			in_pTransition->m_fTargetValue = in_NewTarget;
		}
	}
//----------------------------------------------------------------------------------------------------
// process linear values
//----------------------------------------------------------------------------------------------------
	else
	{
		// did it get processed at least once ?
		if(in_pTransition->m_bCurrentValueSet)
		{
			// this is our new starting point
			in_pTransition->m_fStartValue = in_pTransition->m_fCurrentValue;
		}

		// is our new ending point an offset ?
		if(in_eValueMeaning == AkValueMeaning_Offset)
		{
			in_pTransition->m_fTargetValue += in_NewTarget;
		}
		// nope it's not, fill in the union with whatever we got
		else
		{
			in_pTransition->m_fTargetValue = in_NewTarget;
		}
	}

	// WG-13494: want to use the reciprocal curve when fading out. Except for S-Curves.
	bool bFadeIn = ( in_pTransition->m_fStartValue < in_pTransition->m_fTargetValue );
	in_pTransition->m_eFadeCurve = ( bFadeIn || ( in_eCurveType == AkCurveInterpolation_InvSCurve ) || ( in_eCurveType == AkCurveInterpolation_SCurve ) ) ?
		in_eCurveType : (AkCurveInterpolation) ( AkCurveInterpolation_LastFadeCurve - in_eCurveType );

	// Update time settings.
	AkUInt32 uLastTickUpdated = g_pAudioMgr->GetBufferTick();
	AkUInt32 uNewDurationTicks = CAkTransition::Convert( in_NewDuration );

	// Same target in a Play/Stop/Pause/Resume transition ? 
	if ( ( l_oldTarget == in_pTransition->m_eTarget )
		&& ( in_eTarget & ( TransTarget_Play | TransTarget_Stop | TransTarget_Pause | TransTarget_Resume ) ) )
	{
		AkUInt32 uElapsedTicks = uLastTickUpdated - in_pTransition->m_uStartTimeInBufferTick;
		AkUInt32 uRemainingDurationTicks = in_pTransition->m_uDurationInBufferTick - uElapsedTicks;
		in_pTransition->m_uDurationInBufferTick = AkMin( uNewDurationTicks, uRemainingDurationTicks );
	}
	else
	{
		in_pTransition->m_uDurationInBufferTick = uNewDurationTicks;
	}

	// this is our new start time
	in_pTransition->m_uStartTimeInBufferTick = uLastTickUpdated;
	in_pTransition->m_uLastBufferTickUpdated = uLastTickUpdated;
}
//====================================================================================================
// does what it takes to get things moving
//====================================================================================================
void CAkTransitionManager::ProcessTransitionsList( AkUInt32 in_CurrentBufferTick )
{
	ProcessTransitionsList( in_CurrentBufferTick, m_ActiveTransitionsList_Fade );
	ProcessTransitionsList( in_CurrentBufferTick, m_ActiveTransitionsList_State );
}
//====================================================================================================
// Helper, called for each transition list
//====================================================================================================
void CAkTransitionManager::ProcessTransitionsList( AkUInt32 in_CurrentBufferTick, AkTransitionList& in_rTransitionList )
{
	CAkTransition*	pThisTransition;
//----------------------------------------------------------------------------------------------------
// process the active transitions
//----------------------------------------------------------------------------------------------------

	AkTransitionList::Iterator iter = in_rTransitionList.Begin();
	while( iter != in_rTransitionList.End() )
	{
		pThisTransition = *iter;

//----------------------------------------------------------------------------------------------------
// should be removed ? if yes then it should not be processed
//----------------------------------------------------------------------------------------------------
		if(pThisTransition->m_eState == CAkTransition::ToRemove)
		{
			// get rid of the transition
			pThisTransition->Term();
			AkDelete( g_DefaultPoolId, pThisTransition );

			// remove the list entry
			iter = in_rTransitionList.EraseSwap( iter );
		}
		else
		{
			switch(pThisTransition->m_eState)
			{
//----------------------------------------------------------------------------------------------------
// this one needs to be paused
//----------------------------------------------------------------------------------------------------
			case CAkTransition::ToPause:
				// remember when this happened
				pThisTransition->m_uLastBufferTickUpdated = in_CurrentBufferTick;
				pThisTransition->m_eState = CAkTransition::Paused;
			break;
//----------------------------------------------------------------------------------------------------
// This one needs to be resumed
//----------------------------------------------------------------------------------------------------
			case CAkTransition::ToResume:
				// erase the time spent in pause mode
				pThisTransition->m_uStartTimeInBufferTick += in_CurrentBufferTick - pThisTransition->m_uLastBufferTickUpdated;

				pThisTransition->m_eState = CAkTransition::Running;
			break;
			}
//----------------------------------------------------------------------------------------------------
// now let's update those who need to
//----------------------------------------------------------------------------------------------------
			if(pThisTransition->m_eState == CAkTransition::Running)
			{
				// step it and let me know what to do
				if(pThisTransition->ComputeTransition( in_CurrentBufferTick )) // TRUE == transition completed 
				{
					// this one is done
					pThisTransition->Term();
					iter = in_rTransitionList.EraseSwap( iter );
					AkDelete( g_DefaultPoolId, pThisTransition );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}
}

// gets the current and maximum number of transitions
void CAkTransitionManager::GetTransitionsUsage( 
		AkUInt16& out_ulNumFadeTransitionsUsed, 
		AkUInt16& out_ulMaxFadeNumTransitions,
		AkUInt16& out_ulNumStateTransitionsUsed, 
		AkUInt16& out_ulMaxStateFadeNumTransitions 
		)
{
	out_ulNumFadeTransitionsUsed = (AkUInt16)m_ActiveTransitionsList_Fade.Length();
	out_ulMaxFadeNumTransitions = (AkUInt16)m_uMaxNumTransitions;
	out_ulNumStateTransitionsUsed = (AkUInt16)m_ActiveTransitionsList_State.Length();
	out_ulMaxStateFadeNumTransitions = (AkUInt16)m_uMaxNumTransitions;
}
