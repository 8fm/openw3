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
// AkDynamicSequencePBI.h
//
//////////////////////////////////////////////////////////////////////
#ifndef AKDYNAMICSEQUENCEPBI_H
#define AKDYNAMICSEQUENCEPBI_H

#include "AkContinuousPBI.h"
#include <AK/SoundEngine/Common/AkDynamicSequence.h> // for DynamicSequenceType in the SDK

class CAkDynamicSequence;

class CAkDynamicSequencePBI : public CAkContinuousPBI
{
public:
	CAkDynamicSequencePBI(
		CAkSoundBase*				in_pSound,			// Sound associated to the PBI (NULL if none).	
		CAkSource*					in_pSource,
		CAkRegisteredObj *			in_pGameObj,		// Game object.
		ContParams&					in_rCparameters,	// Continuation parameters.
		UserParams&					in_rUserParams,
		PlayHistory&				in_rPlayHistory,	// History stuff.
		bool						in_bIsFirst,		// Is it the first play of a series.
		AkUniqueID					in_SeqID,			// Sample accurate sequence id.	 
        CAkPBIAware*				in_pInstigator,
		const PriorityInfoCurrent&	in_rPriority,
		AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType,
		CAkLimiter*				in_pAMLimiter,
		CAkLimiter*				in_pBusLimiter);

	virtual ~CAkDynamicSequencePBI();

	virtual void Term( bool in_bFailedToInit );

	// Get the information about the next sound to play in a continuous system
	virtual void PrepareNextToPlay( bool in_bIsPreliminary );

private:

	bool IsPlaybackCompleted();

	AKRESULT PlayNextElement( AkUniqueID in_nextElementID, AkTimeMs in_delay );

	bool m_bRequestNextFromDynSeq;
	AK::SoundEngine::DynamicSequence::DynamicSequenceType m_eDynamicSequenceType;

	AkUniqueID m_startingNode;
	void* m_pDynSecCustomInfo;
	/*
	AkUniqueID LastNodeIDSelected() const { return m_queuedItem.audioNodeID; }
	void* LastCustomInfo() const { return m_queuedItem.pCustomInfo; }
	*/
};

#endif // AKDYNAMICSEQUENCEPBI_H
