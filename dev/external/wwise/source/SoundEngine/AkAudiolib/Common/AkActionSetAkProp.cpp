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
#include "AkActionSetAkProp.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkModifiers.h"

CAkActionSetAkProp::CAkActionSetAkProp(AkActionType in_eActionType, AkUniqueID in_ulID, AkPropID in_ePropID) 
: CAkActionSetValue(in_eActionType, in_ulID)
, m_eValueMeaning( AkValueMeaning_Offset )
, m_ePropID( in_ePropID )
{
}

CAkActionSetAkProp::~CAkActionSetAkProp()
{

}

void CAkActionSetAkProp::ExecSetValue(CAkParameterNodeBase* in_pNode)
{
	ExecSetValueInternal(in_pNode, NULL, (AkValueMeaning)m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ));
}

void CAkActionSetAkProp::ExecSetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	ExecSetValueInternal(in_pNode, in_pGameObj, (AkValueMeaning)m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ));
}

void CAkActionSetAkProp::ExecResetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->ResetAkProp( (AkPropID) m_ePropID, CurveType(), GetTransitionTime() );
}

void CAkActionSetAkProp::ExecResetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	ExecSetValueInternal(in_pNode, in_pGameObj, AkValueMeaning_Default, 0.0f );
}

void CAkActionSetAkProp::ExecResetValueAll(CAkParameterNodeBase* in_pNode)
{
	ExecResetValue(in_pNode);
}

void CAkActionSetAkProp::ExecResetValueExcept(CAkParameterNodeBase* in_pNode)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		WwiseObjectID wwiseId( in_pNode->ID(), in_pNode->IsBusCategory() );
		if( (*iter) == wwiseId )
		{
			return;
		}
	}

	ExecResetValue(in_pNode);
}

void CAkActionSetAkProp::ExecResetValueExcept(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj )
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		WwiseObjectID wwiseId( in_pNode->ID(), in_pNode->IsBusCategory() );
		if( (*iter) == wwiseId )
		{
			return;
		}
	}
	ExecResetValue(in_pNode, in_pGameObj);
}

void CAkActionSetAkProp::ExecSetValueInternal(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj, AkValueMeaning in_eMeaning, AkReal32 in_fValue)
{
	in_pNode->SetAkProp( (AkPropID) m_ePropID, in_pGameObj, in_eMeaning, in_fValue, CurveType(), GetTransitionTime() );
}

CAkActionSetAkProp* CAkActionSetAkProp::Create(AkActionType in_eActionType, AkUniqueID in_ulID, AkPropID in_ePropID)
{
	CAkActionSetAkProp*	pAction = AkNew(g_DefaultPoolId,CAkActionSetAkProp(in_eActionType, in_ulID, in_ePropID));
	if( pAction )
	{
		if( pAction->Init() != AK_Success )
		{
			pAction->Release();
			pAction = NULL;
		}
	}

	return pAction;
}

void CAkActionSetAkProp::SetValue(const AkReal32 in_fValue, const AkValueMeaning in_eValueMeaning, const AkReal32 in_RangeMin/*=0*/, const AkReal32 in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TargetValue, in_fValue, in_RangeMin, in_RangeMax);
	m_eValueMeaning = in_eValueMeaning;
}

AKRESULT CAkActionSetAkProp::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 TargetValueMeaning = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	AkReal32 ValBase = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMin = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMax = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	RandomizerModifier::SetModValue( m_TargetValue, ValBase, ValMin, ValMax );
	m_eValueMeaning = (AkUInt8) TargetValueMeaning;

	return AK_Success;
}
