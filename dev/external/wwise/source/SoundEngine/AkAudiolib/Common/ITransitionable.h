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
// ITransitionable.h
//
// Interface that allows being updated by the transition manager
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _ITRANSITIONABLE_H_
#define _ITRANSITIONABLE_H_

// Interface that allows being updated by the transition manager
//
// Author:  alessard

class ITransitionable
{
public:

    virtual void TransUpdateValue(AkIntPtr in_eTargetType, AkReal32 in_unionValue, bool in_bIsTerminated) = 0;
};
#endif
