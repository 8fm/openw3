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

///////////////////////////////////////////////////////////////////////
//
// AkIndexable.h
//
// Declaration of audionodes items that are indexable
//
///////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkIndexable.h"
#include "AkAudioLibIndex.h"

CAkIndexable::CAkIndexable(AkUniqueID in_ulID)
	: m_lRef( 1 )
{
	key = in_ulID;
}

CAkIndexable::~CAkIndexable()
{
	AKASSERT(!m_lRef);
}

AkObjectCategory CAkIndexable::Category()
{
	return ObjCategory_Undefined;
}
