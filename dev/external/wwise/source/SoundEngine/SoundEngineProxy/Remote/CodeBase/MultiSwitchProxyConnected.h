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

#pragma once

#ifndef AK_OPTIMIZED

#include "ObjectProxyConnected.h"
#include "MultiSwitchProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IMultiSwitchProxy.h"

//Note: tBase must have an implemented HandleExecute() method.
template< typename tBase, typename tSoundEngineObj, AkUInt32 eAkIdxType >
class MultiSwitchProxyConnected : public tBase 
{
public:
	MultiSwitchProxyConnected( AkUniqueID in_id ): tBase()
	{
		CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, (AkIndexType)eAkIdxType );
		if ( !pIndexable )
			pIndexable = tSoundEngineObj::Create( in_id );

		this->SetIndexable( pIndexable );
	}

	virtual ~MultiSwitchProxyConnected(){};

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
	{
		tSoundEngineObj * pDialogueEvent = static_cast<tSoundEngineObj *>( this->GetIndexable() );

		switch( in_uMethodID )
		{
		case IMultiSwitchProxy::MethodSetAkPropF:
			{
				MultiSwitchProxyCommandData::SetAkPropF setAkProp;
				if( in_rSerializer.Get( setAkProp ) )
					pDialogueEvent->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			}
			break;

		case IMultiSwitchProxy::MethodSetAkPropI:
			{
				MultiSwitchProxyCommandData::SetAkPropI setAkProp;
				if( in_rSerializer.Get( setAkProp ) )
					pDialogueEvent->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			}
			break;

		case IMultiSwitchProxy::MethodSetDecisionTree:
			{
				MultiSwitchProxyCommandData::SetDecisionTree setDecisionTree;
				if( in_rSerializer.Get( setDecisionTree ) )
					pDialogueEvent->SetDecisionTree( setDecisionTree.m_pData, setDecisionTree.m_uSize, setDecisionTree.m_uDepth );
			}
			break;

		case IMultiSwitchProxy::MethodSetArguments:
			{
				MultiSwitchProxyCommandData::SetArguments setArguments;
				if( in_rSerializer.Get( setArguments ) )
					pDialogueEvent->SetArguments( setArguments.m_pArgs, setArguments.m_pGroupTypes, setArguments.m_uNumArgs );
			}
			break;

		default:
			tBase::HandleExecute(in_uMethodID, in_rSerializer, out_rReturnSerializer);
		}
	}
};

#endif // #ifndef AK_OPTIMIZED
