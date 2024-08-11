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
// AkAuxBus.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUXBUS_H_
#define _AUXBUS_H_

#include "AkBus.h"

class CAkAuxBus : public CAkBus
{
public:

	//Thread safe version of the constructor
	static CAkAuxBus* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();

protected:

	// Constructors
    CAkAuxBus(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkAuxBus();

	AKRESULT Init();
};
#endif //_AUXBUS_H_
