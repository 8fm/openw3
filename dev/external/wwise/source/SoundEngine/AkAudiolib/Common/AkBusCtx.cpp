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
// AkBusCtx.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBusCtx.h"
#include "AkBus.h"
#include "AkRTPCMgr.h"
#include "AkFxBase.h"
#include "AkLEngine.h"

using namespace AK;

CAkBusCtx CAkBusCtx::GetParentCtx() const
{
	CAkBusCtx ctx;

	if ( m_pBus )
	{
		CAkParameterNodeBase * pParent = m_pBus->ParentBus();
		if ( pParent )
		{
			CAkBus * pBus = pParent->GetMixingBus();
			if ( pBus )
				ctx.SetBus( pBus );
		}
	}
	
	return ctx;
}

// Sound parameters.
AkUniqueID CAkBusCtx::ID() const
{
	if( m_pBus )
	{
		return m_pBus->ID();
	}
	else
	{
		return AK_INVALID_UNIQUE_ID;
	}
}

AkVolumeValue CAkBusCtx::GetVolume( BusVolumeType in_VolumeType ) const
{
	if( m_pBus )
	{
		return m_pBus->GetBusEffectiveVolume( in_VolumeType, AkPropID_BusVolume );
	}
	else if( in_VolumeType == BusVolumeType_IncludeEntireBusTree && GetPrimaryMasterBus() )
	{
		// A context has an empty bus if the output is the master bus.
		// If the user requested for the full volume, make sure we return this volume too.
		return GetPrimaryMasterBus()->GetBusEffectiveVolume( in_VolumeType, AkPropID_BusVolume );
	}
	else
	{
		return 0;
	}
}

bool CAkBusCtx::IsTopBusCtx() const
{
	if ( m_pBus )
		return m_pBus->IsTopBus();
	else
		return false;
}

AkUInt32 CAkBusCtx::GetChannelConfig() const
{
	// if m_pBus->ChannelConfig() == AK_CHANNEL_MASK_PARENT (0), the bus has the same config as its parent.
	if ( m_pBus )
		return m_pBus->ChannelConfig();

	// No m_pBus
	return AK_CHANNEL_MASK_PARENT;
}

// Effects access.
void CAkBusCtx::GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo ) const
{
	if( m_pBus )
	{
		m_pBus->GetFX( in_uFXIndex, out_rFXInfo );
	}
	else
	{
		out_rFXInfo.pFx = NULL;
		out_rFXInfo.bIsBypassed = false;
	}
}

void CAkBusCtx::GetFXDataID( AkUInt32 in_uFXIndex, AkUInt32 in_uDataIndex, AkUniqueID& out_rDataID) const
{
	if( m_pBus )
	{
		m_pBus->GetFXDataID( in_uFXIndex, in_uDataIndex, out_rDataID );
	}
	else
	{
		out_rDataID = AK_INVALID_SOURCE_ID;
	}
}

bool CAkBusCtx::GetBypassAllFX() const
{
	if ( m_pBus )
		return m_pBus->GetBypassAllFX();

	return false;
}

bool CAkBusCtx::IsAuxBus() const
{
	return m_pBus ? m_pBus->NodeCategory() ==  AkNodeCategory_AuxBus: false;
}
