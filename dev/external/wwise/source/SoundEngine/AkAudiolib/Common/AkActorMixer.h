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
// AkActorMixer.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTOR_MIXER_H_
#define _ACTOR_MIXER_H_

#include "AkParameters.h"
#include "AkActiveParent.h"

// class corresponding to an Actor Mixer
//
// Author:  alessard
class CAkActorMixer : public CAkActiveParent<CAkParameterNode>
{
public:
	//Thread safe version of the constructor
	static CAkActorMixer* Create(AkUniqueID in_ulID = 0);

	// Check if the specified child can be connected
    //
    // Return - bool -	AK_NotCompatible
	//					AK_Succcess
	//					AK_MaxReached
    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode // Audio node ID to connect on
        );

	virtual AkNodeCategory NodeCategory();		

	// NOT IMPLEMENTED IN THE ACTOR MIXER
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

	AKRESULT SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize);

protected:

	// Constructors
    CAkActorMixer(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkActorMixer(void);

	AKRESULT Init(){ return CAkActiveParent<CAkParameterNode>::Init(); }
};
#endif
