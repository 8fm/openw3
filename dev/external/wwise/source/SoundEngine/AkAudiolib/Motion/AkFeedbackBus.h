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
// AkFeedbackBus.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _FEEDBACK_BUS_H_
#define _FEEDBACK_BUS_H_

#include "AkBus.h"

class CAkFeedbackBus : public CAkBus
{
public:
	
	//Thread safe version of the constructor
	static CAkFeedbackBus* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();	

	virtual AKRESULT AddChildInternal( CAkParameterNodeBase* pAudioNode );

	virtual void RemoveChild(
        CAkParameterNodeBase* in_pChild
		);
	using CAkParameterNodeBase::RemoveChild; //This will remove warnings on 3DS, WiiU and Vita compilers

	virtual AKRESULT CanAddChild( 
		CAkParameterNodeBase * in_pAudioNode 
		);

	virtual void Notification(
		AkRTPC_ParameterID in_ParamID, 
		AkReal32 in_fValue,						// Param variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	virtual void ParamNotification( NotifParams& in_rParams );

	static CAkFeedbackBus* GetMasterMotionBusAndAddRef();

	// Manage the master bus when loading
	static void ResetMasterBus(CAkFeedbackBus* in_pBus);
	static CAkFeedbackBus* ClearTempMasterBus();

	// Get the compounded feedback parameters.  There is currenly only the volume.
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetFeedbackParameters( 
		class AkFeedbackParams &io_Params,			// Parameters
		CAkSource *in_pSource,
		CAkRegisteredObj * in_GameObjPtr,		// Game object associated to the query
		bool in_bDoBusCheck = true );

private:
	CAkFeedbackBus(AkUniqueID in_ulID);
	~CAkFeedbackBus();
	
	//Master bus management
	static CAkFeedbackBus* s_pMasterMotionBus;
};

#endif
