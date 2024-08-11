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
// AkContainerBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONTAINER_BASE_H_
#define _CONTAINER_BASE_H_

#include "AkParameterNode.h"
#include "AkActiveParent.h"

class CAkContainerBaseInfo;

typedef CAkContainerBaseInfo* CAkContainerBaseInfoPtr;

// class corresponding to a Container
//
// Author:  alessard
class CAkContainerBase : public CAkActiveParent<CAkParameterNode>
{
public:

	// Check if the specified child can be connected
    //
    // Return - bool -	AK_NotCompatible
	//					AK_Succcess
	//					AK_MaxReached
    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode // Audio node ID to connect on
        );


protected:

	// Constructors
    CAkContainerBase(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkContainerBase();
};
#endif
