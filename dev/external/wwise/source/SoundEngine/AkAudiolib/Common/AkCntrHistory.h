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
// AkCntrHistory.h
//
// Audiokinetic Data Type Definition (internal)
//
//////////////////////////////////////////////////////////////////////

#ifndef _CNTR_HISTORY_H_
#define _CNTR_HISTORY_H_

#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkBitArray.h"

#define AK_CONT_HISTORY_SIZE AK_MAX_HIERARCHY_DEEP

struct AkCntrHistArray
{
	inline void Init()
	{
		uiArraySize = 0;
	}

	AkUInt32	uiArraySize;
	AkUInt16	aCntrHist[ AK_CONT_HISTORY_SIZE ];
};

//This class was created to automate the call to init function of its only member.
//because AkCntrHistArray cannot have a constructor since it is contained in a union.
class CAkCntrHist
	: public AkCntrHistArray
{
public:
	CAkCntrHist(){Init();}

	AkCntrHistArray& operator = ( const AkCntrHistArray& in_from )
	{
		AkCntrHistArray* pSelf = this;// Cast down from CAkCntrHist to AkCntrHistArray otherwise it will refuse to copy.
		*pSelf = in_from;

		return *this;
	}
};

struct PlayHistory
{
	AkCntrHistArray			HistArray;

	inline void Init()
	{
		HistArray.Init();
		AKASSERT( AK_CONT_HISTORY_SIZE <= sizeof(AkUInt64)*8 ); // if AK_CONT_HISTORY_SIZE cannot be over 32 when using CAkBitArrayMax32
	}

	inline void Add( AkUInt16 in_uNewPosToAdd, bool in_bIsContinuous )
	{
		if( HistArray.uiArraySize < AK_CONT_HISTORY_SIZE )
		{
			arrayIsContinuous.Set( HistArray.uiArraySize, in_bIsContinuous );
			HistArray.aCntrHist[HistArray.uiArraySize++] = in_uNewPosToAdd;
		}
		else
		{
			++HistArray.uiArraySize;
		}
	}

	inline void RemoveLast()
	{
		AKASSERT( HistArray.uiArraySize );
		--HistArray.uiArraySize;
	}

	inline bool IsContinuous( AkUInt32 in_index )
	{
		if( in_index < AK_CONT_HISTORY_SIZE )
			return arrayIsContinuous.IsSet( in_index );
		else
			return false;
	}
	
private:

	CAkBitArray<AkUInt32> 	arrayIsContinuous;// 32 bits array, replacing the previous 32 bool array.
};

#endif //_CNTR_HISTORY_H_
