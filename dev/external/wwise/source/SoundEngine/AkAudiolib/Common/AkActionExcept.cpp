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
// AkActionExcept.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionExcept.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

CAkActionExcept::CAkActionExcept(AkActionType in_eActionType, AkUniqueID in_ulID)
: CAkAction(in_eActionType, in_ulID)
{
}

CAkActionExcept::~CAkActionExcept()
{
	m_listElementException.Term();
}

AKRESULT CAkActionExcept::AddException(const WwiseObjectIDext in_ulElementID)
{
	if ( m_listElementException.Exists( in_ulElementID ) )
		return AK_Success;

	return m_listElementException.AddLast( in_ulElementID ) ? AK_Success : AK_Fail;
}

void CAkActionExcept::RemoveException( const WwiseObjectIDext in_ulElementID )
{
	m_listElementException.Remove( in_ulElementID );
}

void CAkActionExcept::ClearExceptions()
{
	m_listElementException.RemoveAll();
}

AKRESULT CAkActionExcept::SetExceptParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 ulExceptionListSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
	if ( m_listElementException.Reserve( ulExceptionListSize ) != AK_Success )
		return AK_Fail;

	for (AkUInt32 i = 0; i < ulExceptionListSize; ++i)
	{
		WwiseObjectIDext idExt;
		idExt.id = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
		idExt.bIsBus = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize) != 0;

		m_listElementException.AddLast( idExt );
	}

	return AK_Success;
}
