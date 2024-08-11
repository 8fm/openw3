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
// AkState.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _STATE_H_
#define _STATE_H_

#include "AkParameters.h"
#include "AkPropBundle.h"
#include "AkIndexable.h"
#include "ITransitionable.h"

class CAkParameterNodeBase;

class CAkState : public CAkIndexable
{
public:

	//Destructor
	virtual ~CAkState();

	//Thread safe version of the constructor
	static CAkState* Create( AkUniqueID in_ulID );

protected:
	//Constructor
	CAkState( AkUniqueID in_ulID );

	AKRESULT Init(){ AddToIndex(); return AK_Success; }

public:

	void AddToIndex();
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// INTERNAL USE ONLY, not to be exported in the SDK
	// Gives the CAkState the opportunity to notify the parent it it was modified by the Wwise
	//
	// Return  - void - 
	void InitNotificationSystem(
		CAkParameterNodeBase * in_pNode //NULL means the parent is the StateMgr, otherwise it must be the parent node
		);

	// INTERNAL USE ONLY, not to be exported in the SDK
	// Gives the CAkState the opportunity to notify the parent it it was modified by the Wwise
	//
	// Return  - void - 
	void TermNotificationSystem();

	void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue );
	AkForceInline const AkPropBundle<AkReal32> & Props() { return m_props; }

	AKRESULT SetInitialValues(AkUInt8* pData, AkUInt32 ulDataSize);

private:

	void NotifyParent();
	
	AkPropBundle<AkReal32> m_props; // State properties need to be float due to transitions

	CAkParameterNodeBase* m_pParentToNotify;
};

#endif
