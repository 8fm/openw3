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
// AkIDStringMap.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ID_STRING_MAP_H_
#define _ID_STRING_MAP_H_

#include "AkHashList.h"

class AkIDStringHash
{
public:
	typedef AkHashList< AkUIntPtr, char, AK_SMALL_HASH_SIZE > AkStringHash;

	void Init( AkMemPoolId in_MemPoolId );

	void Term();

	AkStringHash::Item * Preallocate( AkUIntPtr in_ID, const char* in_pszString );
	void FreePreallocatedString( AkStringHash::Item * in_pItem );

	AKRESULT Set( AkStringHash::Item * in_pItem );

	AKRESULT Set( AkUIntPtr in_ID, const char* in_pszString );

	void Unset( AkUIntPtr in_ID );

	char * GetStr( AkUIntPtr in_ID );

//Private, but set as public to allowing to iterate in the list.
	AkStringHash m_list;
};


#endif //_ID_STRING_MAP_H_
