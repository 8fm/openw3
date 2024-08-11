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
// AkTransitionManager.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _TRANSITION_MANAGER_H_
#define _TRANSITION_MANAGER_H_

#include "AkParameters.h"
#include "AkTransition.h"
#include <AK/Tools/Common/AkArray.h>

enum TransitionTargets
{
	TransTarget_Play			= 0x01000000,
	TransTarget_Stop			= 0x02000000,
	TransTarget_Pause			= 0x04000000,
	TransTarget_Resume			= 0x08000000,
	TransTarget_RTPC			= 0x10000000,
	TransTarget_Undefined		= 0x20000000
};

class CAkTransitionManager
{
public:

	enum AkTransitionCategory
	{
		TC_Fade,
		TC_State
	};

	CAkTransitionManager();
	~CAkTransitionManager();

	AKRESULT Init( AkUInt32 in_uMaxNumTransitions );
	void Term();

	// will return NULL if none is available
	CAkTransition* AddTransitionToList( 
		const TransitionParameters& Params, 
		bool in_bStart = true, 
		AkTransitionCategory in_eTransitionCategory = TC_Fade 
		);

	// adds an ITransitionable to the list
	AKRESULT AddTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser);

	// removes an ITransitionable from the list
	AKRESULT RemoveTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser);

	// returns if the transition is terminated
	bool IsTerminated( CAkTransition* in_pTransition );

	// make'em move as long as they should then get them outta the list
	void ProcessTransitionsList( AkUInt32 in_CurrentBufferTick );

	// move it to the paused list
	void Pause(CAkTransition* in_pTransition);

	// move it back to the active list
	void Resume(CAkTransition* in_pTransition);

	// PhM : will need to change more parameters
	// change the route the target parameter will take
	void ChangeParameter(CAkTransition* ThisTransition,
						AkIntPtr in_eTarget,
						AkReal32 NewTarget,
						AkTimeMs NewDuration,
						AkCurveInterpolation in_eCurveType,
						AkValueMeaning in_TargetMeaning);

	// gets the current and maximum number of transitions
	void GetTransitionsUsage( 
		AkUInt16& out_ulNumFadeTransitionsUsed, 
		AkUInt16& out_ulMaxFadeNumTransitions,
		AkUInt16& out_ulNumStateTransitionsUsed, 
		AkUInt16& out_ulMaxStateFadeNumTransitions
		);

private:

	typedef AkArray<CAkTransition*, CAkTransition*, ArrayPoolDefault, 0> AkTransitionList;

	// Helper, called for each transition list
	void ProcessTransitionsList( AkUInt32 in_CurrentBufferTick, AkTransitionList& in_rTransitionList );

	void TermList( AkTransitionList& in_rTransitionList );
	void RemoveTransitionFromList(CAkTransition* in_pTransition);

	// the transitions we manage

	AkUInt32 m_uMaxNumTransitions;
	
	AkTransitionList			m_ActiveTransitionsList_Fade;
	AkTransitionList			m_ActiveTransitionsList_State;
};

extern CAkTransitionManager*	g_pTransitionManager;

#endif
