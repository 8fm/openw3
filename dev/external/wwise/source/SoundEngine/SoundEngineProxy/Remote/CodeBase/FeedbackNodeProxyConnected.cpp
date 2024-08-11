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
#ifndef AK_OPTIMIZED

#include "FeedbackNodeProxyConnected.h"
#include "CommandDataSerializer.h"
#include "CommandData.h"

#ifdef AK_MOTION
#include "AkFeedbackNode.h"
#endif

FeedbackNodeProxyConnected::FeedbackNodeProxyConnected( AkUniqueID in_id )
{
#ifdef AK_MOTION
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkFeedbackNode::Create( in_id );

	SetIndexable( pIndexable );
#endif
}

FeedbackNodeProxyConnected::~FeedbackNodeProxyConnected()
{
}

void FeedbackNodeProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
#ifdef AK_MOTION
	CAkFeedbackNode * pFeedbackNode = static_cast<CAkFeedbackNode *>( GetIndexable() );
#endif

	switch( in_uMethodID )
	{	
		case IFeedbackNodeProxy::MethodAddPluginSource:
		{
			FeedbackNodeProxyCommandData::AddPluginSource methodData;
			if (in_rSerializer.Get( methodData ))
			{
#ifdef AK_MOTION
				pFeedbackNode->AddPluginSource(methodData.m_param1, methodData.m_param2, methodData.m_param3);
#endif
			}
			break;
		}

		case IFeedbackNodeProxy::MethodSetSourceVolumeOffset:
		{
			FeedbackNodeProxyCommandData::SetSourceVolumeOffset methodData;
			if (in_rSerializer.Get( methodData ))
			{
#ifdef AK_MOTION
				pFeedbackNode->SetSourceVolumeOffset(methodData.m_param1, methodData.m_param2);
#endif
			}
			break;
		}

		default:
			__base::HandleExecute(in_uMethodID, in_rSerializer, out_rReturnSerializer);
			break;
	}
}
#endif // #ifndef AK_OPTIMIZED
