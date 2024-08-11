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

#include "ParameterNodeProxyConnected.h"
#include "AkParameterNode.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

ParameterNodeProxyConnected::ParameterNodeProxyConnected()
{
}

ParameterNodeProxyConnected::~ParameterNodeProxyConnected()
{
}

void ParameterNodeProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkParameterNode * pNode = static_cast<CAkParameterNode *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IParameterNodeProxy::MethodPosSetSpatializationEnabled:
		{
			ParameterNodeProxyCommandData::PosSetSpatializationEnabled SetSpatializationEnabledParams;
			if( in_rSerializer.Get( SetSpatializationEnabledParams ) )
					pNode->PosSetSpatializationEnabled( SetSpatializationEnabledParams.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetAttenuationID:
		{
			ParameterNodeProxyCommandData::PosSetAttenuationID SetAttenuationIDParams;
			if( in_rSerializer.Get( SetAttenuationIDParams ) )
					pNode->PosSetAttenuationID( SetAttenuationIDParams.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetIsPositionDynamic:
		{
			ParameterNodeProxyCommandData::PosSetIsPositionDynamic posSetIsPositionDynamic;
			if( in_rSerializer.Get( posSetIsPositionDynamic ) )
					pNode->PosSetIsPositionDynamic( posSetIsPositionDynamic.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetFollowOrientation:
		{
			ParameterNodeProxyCommandData::PosSetFollowOrientation posSetFollowOrientation;
			if( in_rSerializer.Get( posSetFollowOrientation ) )
					pNode->PosSetFollowOrientation( posSetFollowOrientation.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPathMode:
		{
			ParameterNodeProxyCommandData::PosSetPathMode posSetPathMode;
			if( in_rSerializer.Get( posSetPathMode ) )
					pNode->PosSetPathMode( (AkPathMode) posSetPathMode.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetIsLooping:
		{
			ParameterNodeProxyCommandData::PosSetIsLooping posSetIsLooping;
			if( in_rSerializer.Get( posSetIsLooping ) )
					pNode->PosSetIsLooping( posSetIsLooping.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetTransition:
		{
			ParameterNodeProxyCommandData::PosSetTransition posSetTransition;
			if( in_rSerializer.Get( posSetTransition ) )
					pNode->PosSetTransition( posSetTransition.m_param1 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPath:
		{
			ParameterNodeProxyCommandData::PosSetPath posSetPath;
			if( in_rSerializer.Get( posSetPath ) )
					pNode->PosSetPath( posSetPath.m_pArrayVertex, posSetPath.m_ulNumVertices, posSetPath.m_pArrayPlaylist, posSetPath.m_ulNumPlaylistItem );

			break;
		}

	case IParameterNodeProxy::MethodPosUpdatePathPoint:
		{
			ParameterNodeProxyCommandData::PosUpdatePathPoint posUpdatePathPoint;
			if( in_rSerializer.Get( posUpdatePathPoint ) )
					pNode->PosUpdatePathPoint( posUpdatePathPoint.m_param1, posUpdatePathPoint.m_param2, posUpdatePathPoint.m_param3, posUpdatePathPoint.m_param4 );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPathRange:
		{
			ParameterNodeProxyCommandData::PosSetPathRange posRange;
			if( in_rSerializer.Get( posRange ) )
					pNode->PosSetPathRange( posRange.m_param1, posRange.m_param2, posRange.m_param3 );

			break;
		}

	case IParameterNodeProxy::MethodOverrideFXParent:
		{
			ParameterNodeProxyCommandData::OverrideFXParent overrideFXParent;
			if( in_rSerializer.Get( overrideFXParent ) )
					pNode->OverrideFXParent( overrideFXParent.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetBelowThresholdBehavior:
		{
			ParameterNodeProxyCommandData::SetBelowThresholdBehavior setBelowThresholdBehavior;
			if( in_rSerializer.Get( setBelowThresholdBehavior ) )
					pNode->SetBelowThresholdBehavior( (AkBelowThresholdBehavior) setBelowThresholdBehavior.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetMaxNumInstancesOverrideParent:
		{
			ParameterNodeProxyCommandData::SetMaxNumInstancesOverrideParent setMaxNumInstancesOverrideParent;
			if( in_rSerializer.Get( setMaxNumInstancesOverrideParent ) )
					pNode->SetMaxNumInstOverrideParent( setMaxNumInstancesOverrideParent.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetVVoicesOptOverrideParent:
		{
			ParameterNodeProxyCommandData::SetVVoicesOptOverrideParent setVVoicesOptOverrideParent;
			if( in_rSerializer.Get( setVVoicesOptOverrideParent ) )
					pNode->SetVVoicesOptOverrideParent( setVVoicesOptOverrideParent.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetMaxNumInstances:
		{
			ParameterNodeProxyCommandData::SetMaxNumInstances setMaxNumInstances;
			if( in_rSerializer.Get( setMaxNumInstances ) )
					pNode->SetMaxNumInstances( setMaxNumInstances.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetMaxReachedBehavior:
		{
			ParameterNodeProxyCommandData::SetMaxReachedBehavior setMaxReachedBehavior;
			if( in_rSerializer.Get( setMaxReachedBehavior ) )
					pNode->SetMaxReachedBehavior( setMaxReachedBehavior.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetIsGlobalLimit:
		{
			ParameterNodeProxyCommandData::SetIsGlobalLimit setIsGlobalLimit;
			if( in_rSerializer.Get( setIsGlobalLimit ) )
					pNode->SetIsGlobalLimit( setIsGlobalLimit.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetOverLimitBehavior:
		{
			ParameterNodeProxyCommandData::SetOverLimitBehavior setOverLimitBehavior;
			if( in_rSerializer.Get( setOverLimitBehavior ) )
					pNode->SetOverLimitBehavior( setOverLimitBehavior.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetVirtualQueueBehavior:
		{
			ParameterNodeProxyCommandData::SetVirtualQueueBehavior setVirtualQueueBehavior;
			if( in_rSerializer.Get( setVirtualQueueBehavior ) )
					pNode->SetVirtualQueueBehavior( (AkVirtualQueueBehavior) setVirtualQueueBehavior.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetAuxBusSend:
		{
			ParameterNodeProxyCommandData::SetAuxBusSend setAuxBusSend;
			if( in_rSerializer.Get( setAuxBusSend ) )
				pNode->SetAuxBusSend( (AkUniqueID) setAuxBusSend.m_param1, (AkUInt32) setAuxBusSend.m_param2 );

			break;
		}
	case IParameterNodeProxy::MethodSetOverrideGameAuxSends:
		{
			ParameterNodeProxyCommandData::SetOverrideGameAuxSends setOverrideGameAuxSends;
			if( in_rSerializer.Get( setOverrideGameAuxSends ) )
				pNode->SetOverrideGameAuxSends( (bool) setOverrideGameAuxSends.m_param1 );

			break;
		}	
	case IParameterNodeProxy::MethodSetUseGameAuxSends:
		{
			ParameterNodeProxyCommandData::SetUseGameAuxSends setUseGameAuxSends;
			if( in_rSerializer.Get( setUseGameAuxSends ) )
				pNode->SetUseGameAuxSends( (bool) setUseGameAuxSends.m_param1 );

			break;
		}	
	case IParameterNodeProxy::MethodSetOverrideUserAuxSends:
		{
			ParameterNodeProxyCommandData::SetOverrideUserAuxSends setOverrideUserAuxSends;
			if( in_rSerializer.Get( setOverrideUserAuxSends ) )
				pNode->SetOverrideUserAuxSends( (bool) setOverrideUserAuxSends.m_param1 );

			break;
		}	
	case IParameterNodeProxy::MethodSetOverrideHdrEnvelope:
		{
			ParameterNodeProxyCommandData::SetOverrideHdrEnvelope setOverrideHdrEnvelope;
			if( in_rSerializer.Get( setOverrideHdrEnvelope ) )
				pNode->SetOverrideHdrEnvelope( (bool) setOverrideHdrEnvelope.m_param1 );

			break;
		}	
	case IParameterNodeProxy::MethodSetOverrideAnalysis:
		{
			ParameterNodeProxyCommandData::SetOverrideAnalysis setOverrideAnalysis;
			if( in_rSerializer.Get( setOverrideAnalysis ) )
				pNode->SetOverrideAnalysis( (bool) setOverrideAnalysis.m_param1 );

			break;
		}	
	case IParameterNodeProxy::MethodSetNormalizeLoudness:
		{
			ParameterNodeProxyCommandData::SetNormalizeLoudness setNormalizeLoudness;
			if( in_rSerializer.Get( setNormalizeLoudness ) )
				pNode->SetNormalizeLoudness( (bool) setNormalizeLoudness.m_param1 );

			break;
		}
	case IParameterNodeProxy::MethodSetEnableEnvelope:
		{
			ParameterNodeProxyCommandData::SetEnableEnvelope setEnableEnvelope;
			if( in_rSerializer.Get( setEnableEnvelope ) )
				pNode->SetEnableEnvelope( (bool) setEnableEnvelope.m_param1 );

			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
