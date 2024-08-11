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
// AkContinuousPBI.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONTINUOUS_PBI_H_
#define _CONTINUOUS_PBI_H_

#include "AkPBI.h"
#include "AkRanSeqCntr.h"

class CAkContinuousPBI :public CAkPBI
{
public:
	// Constructor
	CAkContinuousPBI(
		CAkSoundBase*				in_pSound,			// Sound associated to the PBI (NULL if none).	
		CAkSource*					in_pSource,
		CAkRegisteredObj *			in_pGameObj,		// Game object.
		ContParams&					in_rCparameters,	// Continuation parameters.
		UserParams&					in_rUserparams,		// User parameters.
		PlayHistory&				in_rPlayHistory,	// History stuff.
		bool						in_bIsFirst,		// Is it the first play of a series.
		AkUniqueID					in_SeqID,			// Sample accurate sequence id.	 
        CAkPBIAware*				in_pInstigator,
		const PriorityInfoCurrent&	in_rPriority
#ifdef AK_MOTION
		, bool						in_bTargetFeedback
#endif // AK_MOTION
		, CAkLimiter*				in_pAMLimiter
		, CAkLimiter*				in_pBusLimiter
		);

	//Destructor
	virtual ~CAkContinuousPBI(void);

	virtual AKRESULT Init( AkPathInfo* in_pPathInfo );

	virtual void Term( bool in_bFailedToInit );

	//Seeking
	// Disabled with transitions that involve playing more than one PBI at a time.
	virtual void SeekTimeAbsolute( AkTimeMs in_iPosition, bool in_bSnapToMarker );
	virtual void SeekPercent( AkReal32 in_fPercent, bool in_bSnapToMarker );

	// Get the information about the next sound to play in a continuous system
	virtual void PrepareNextToPlay( bool in_bIsPreliminary );

	// Get the information about the next sound to play in a continuous system
	void PrepareNextPlayHistory( PlayHistory& in_rPlayHistory );

	// Estimated lenght must be zero if unknown, that ensure that there is no crossfade when the length is unknown.
	// This function should be called as near as possible from the play of a sound.
	virtual void SetEstimatedLength( AkReal32 in_fEstimatedLength ); 

	// Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual void _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false, bool in_bHasNotStarted = false );

#ifndef AK_OPTIMIZED
	virtual void _StopAndContinue(
		AkUniqueID				in_ItemToPlay,
		AkUInt16					in_PlaylistPosition,
		CAkContinueListItem*	in_pContinueListItem
		);
#endif

	virtual void PlayToEnd( CAkParameterNodeBase * in_pNode );

	virtual void SetPauseStateForContinuous(bool in_bIsPaused);

	// Prepare the Sample accurate next sound if possible and available.
	virtual void PrepareSampleAccurateTransition();
	static AkUniqueID GetNewSequenceID(){ return m_CalSeqID++; }

	AkForceInline AkTransitionMode GetTransitionMode() { return m_eTransitionMode; }

protected:

	bool CanSeek();

	bool HasNextToPlay()
	{
		return m_ulNextElementToPlay != AK_INVALID_UNIQUE_ID; 
	}

#ifndef AK_OPTIMIZED
	virtual void Monitor(
		AkMonitorData::NotificationReason in_Reason,			// Reason for the notification
		bool in_bUpdateCount = true
		);
#endif

	CAkSmartPtr<CAkContinuationList>	m_spContList;				// Continuation list, information of the following behavior	
	PlayHistory							m_PlayHistoryForNextToPlay;
    CAkPBIAware*						m_pInstigator;
	AkReal32			m_fTransitionTime;									// Transition time(delay) to next sound
	AkUniqueID			m_ulNextElementToPlay;								// Next element to be played
	AkTransitionMode	m_eTransitionMode :TRANSITION_MODE_NUM_STORAGE_BIT;	// Transition mode to next sound
	bool				m_bIsFirstPlay :1;									// Is this PBI first of a continuous sequence?
	bool				m_bIsContinuousPaused :1;
	bool				m_bIsNextPrepared :1;//True if the continuous sound has been launched

	static AkUniqueID	m_CalSeqID;
};
#endif
