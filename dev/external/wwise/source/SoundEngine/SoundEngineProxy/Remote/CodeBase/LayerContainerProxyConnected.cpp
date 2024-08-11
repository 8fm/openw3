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

#include "LayerContainerProxyConnected.h"
#include "AkLayerCntr.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "ILayerContainerProxy.h"

LayerContainerProxyConnected::LayerContainerProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkLayerCntr::Create( in_id );

	SetIndexable( pIndexable );
}

void LayerContainerProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkLayerCntr * pLayerCntr = static_cast<CAkLayerCntr *>( GetIndexable() );

	switch( in_uMethodID )
	{
		case ILayerContainerProxy::MethodAddLayer:
			{
				LayerContainerProxyCommandData::AddLayer addLayer;
				if (in_rSerializer.Get( addLayer ))
					pLayerCntr->AddLayer( addLayer.m_LayerID );

				break;
			}

		case ILayerContainerProxy::MethodRemoveLayer:
			{
				LayerContainerProxyCommandData::RemoveLayer removeLayer;
				if (in_rSerializer.Get( removeLayer ))
					pLayerCntr->RemoveLayer( removeLayer.m_LayerID );

				break;
			}

		default:
			__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
