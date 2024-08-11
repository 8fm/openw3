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

#include "stdafx.h"

#include "AkDialogueEvent.h"

#include <AK/Tools/Common/AkAssert.h>
#include "AkAudioLibIndex.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkMonitor.h"
#include "AkAudioLib.h"

CAkDialogueEvent* CAkDialogueEvent::Create( AkUniqueID in_ulID )
{
	CAkDialogueEvent* pObj = AkNew( g_DefaultPoolId, CAkDialogueEvent( in_ulID ) );

	if( pObj && pObj->Init() != AK_Success )
	{
		pObj->Release();
		pObj = NULL;
	}
	
	return pObj;
}

AkUInt32 CAkDialogueEvent::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDialogueEvents.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkDialogueEvent::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDialogueEvents.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		AKASSERT(g_pIndex);
		g_pIndex->m_idxDialogueEvents.RemoveID( ID() );
		AkDelete( g_DefaultPoolId, this ); 
	} 
    return lRef; 
}

AKRESULT CAkDialogueEvent::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = AK_Success;

	// Skip DynamicSequenceItem ID
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUInt8 uProbability = READBANKDATA(AkUInt8, in_pData, in_ulDataSize);

	AkUInt32 uTreeDepth = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	// Skip arguments -- unused for now

	AkUInt32 uArgumentsSize = uTreeDepth * ( sizeof( AkUniqueID ) + sizeof(AkUInt8) );
	SKIPBANKBYTES( uArgumentsSize, in_pData, in_ulDataSize );

	// Read decision tree

	AkUInt32 uTreeDataSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	m_decisionTree.SetProbability( uProbability );
	m_decisionTree.SetMode( READBANKDATA(AkUInt8, in_pData, in_ulDataSize) );

	eResult = m_decisionTree.SetTree( in_pData, uTreeDataSize, uTreeDepth );
	if ( eResult != AK_Success )
		return eResult;

	SKIPBANKBYTES( uTreeDataSize, in_pData, in_ulDataSize );

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

void CAkDialogueEvent::SetAkProp( AkPropID /*in_eProp*/, AkReal32 /*in_fValue*/, AkReal32 /*in_fMin*/, AkReal32 /*in_fMax*/ )
{
	AKASSERT( false ); // no float properties yet
}

void CAkDialogueEvent::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 /*in_iMin*/, AkInt32 /*in_iMax*/ )
{
	if ( in_eProp == AkPropID_Probability )
		m_decisionTree.SetProbability( (AkUInt16) in_iValue );
	else if ( in_eProp == AkPropID_DialogueMode )
		m_decisionTree.SetMode( (AkUInt8) in_iValue );
}

AKRESULT CAkDialogueEvent::SetDecisionTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth )
{
#ifndef AK_OPTIMIZED
	// Use index lock during resolution in case game thread is also reading this (outside of main audio lock).
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDialogueEvents.GetLock() ); 
#endif

	return m_decisionTree.SetTree( in_pData, in_uSize, in_uDepth );
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT CAkDialogueEvent::ResolveArgumentValueNames( const wchar_t* * in_aNames, AkArgumentValueID * out_pPath, AkUInt32 in_cPath )
{
	if ( in_cPath != m_decisionTree.Depth() )
		return AK_Fail;

	for ( AkUInt32 i = 0; i < in_cPath; ++ i ) 
	{
		AkArgumentValueID valueID( AK_FALLBACK_ARGUMENTVALUE_ID );

		if ( in_aNames[ i ][ 0 ] != '\0' ) // Empty string == default argument value
		{
			valueID = AK::SoundEngine::GetIDFromString( in_aNames[ i ] );
			
			if ( valueID == AK_INVALID_UNIQUE_ID  )
			{
				valueID = AK_FALLBACK_ARGUMENTVALUE_ID;
				MONITOR_ERRORMSG2( L"Unknown Argument Value name: ", in_aNames[ i ] );
			}
		}

		out_pPath[ i ] = valueID;
	}

	return AK_Success;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT CAkDialogueEvent::ResolveArgumentValueNames( const char** in_aNames, AkArgumentValueID * out_pPath, AkUInt32 in_cPath )
{
	if ( in_cPath != m_decisionTree.Depth() )
		return AK_Fail;

	for ( AkUInt32 i = 0; i < in_cPath; ++ i ) 
	{
		AkArgumentValueID valueID( AK_FALLBACK_ARGUMENTVALUE_ID );

		if ( in_aNames[ i ][ 0 ] != '\0' ) // Empty string == default argument value
		{
			valueID = AK::SoundEngine::GetIDFromString( in_aNames[ i ] );
			
			if ( valueID == AK_INVALID_UNIQUE_ID  )
			{
				valueID = AK_FALLBACK_ARGUMENTVALUE_ID;
				MONITOR_ERRORMSG2( "Unknown Argument Value name: ", in_aNames[ i ] );
			}
		}

		out_pPath[ i ] = valueID;
	}

	return AK_Success;
}

CAkDialogueEvent::CAkDialogueEvent( AkUniqueID in_ulID )
	: CAkIndexable( in_ulID )
{
}

CAkDialogueEvent::~CAkDialogueEvent()
{
}

AKRESULT CAkDialogueEvent::Init()
{
	AKASSERT( g_pIndex );
	g_pIndex->m_idxDialogueEvents.SetIDToPtr( this );

	return AK_Success;
}
