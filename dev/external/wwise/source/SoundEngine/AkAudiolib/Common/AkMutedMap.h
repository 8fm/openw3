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
// AkMutedMap.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUTED_MAP_H_
#define _MUTED_MAP_H_

#include "AkKeyArray.h"

struct AkMutedMapItem
{
	void* m_Identifier;
    AkUInt32 m_bIsGlobal        :1;
    AkUInt32 m_bIsPersistent    :1; // True if it comes from a higher level context: do not remove on PBI::RefreshParameters()

	bool operator<(const AkMutedMapItem& in_sCompareTo) const
	{
		if( m_Identifier == in_sCompareTo.m_Identifier )
		{
			return m_bIsGlobal < in_sCompareTo.m_bIsGlobal;
		}
		return m_Identifier < in_sCompareTo.m_Identifier;
	}

	bool operator==( const AkMutedMapItem& in_sCompareTo ) const
	{
		return( m_Identifier == in_sCompareTo.m_Identifier && m_bIsGlobal == in_sCompareTo.m_bIsGlobal );
	}
};

typedef CAkKeyArray<AkMutedMapItem, AkReal32> AkMutedMap;
#endif
