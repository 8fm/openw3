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
#ifndef PROXYCENTRAL_CONNECTED

#include "SwitchContainerProxyLocal.h"

#include "AkSwitchCntr.h"
#include "AkAudioLib.h"
#include "AkCritical.h"


SwitchContainerProxyLocal::SwitchContainerProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkSwitchCntr::Create( in_id ) );
}

SwitchContainerProxyLocal::~SwitchContainerProxyLocal()
{
}

void SwitchContainerProxyLocal::SetSwitchGroup( AkUInt32 in_ulGroup, AkGroupType in_eGroupType )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetSwitchGroup( in_ulGroup, in_eGroupType );
	}
}

void SwitchContainerProxyLocal::SetDefaultSwitch( AkUInt32 in_switch )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetDefaultSwitch( in_switch );
	}
}

void SwitchContainerProxyLocal::ClearSwitches()
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->ClearSwitches();
	}
}

void SwitchContainerProxyLocal::AddSwitch( AkSwitchStateID in_switch )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddSwitch( in_switch );
	}
}

void SwitchContainerProxyLocal::RemoveSwitch( AkSwitchStateID in_switch )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveSwitch( in_switch );
	}
}

void SwitchContainerProxyLocal::AddNodeInSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		)
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddNodeInSwitch( in_switch, in_nodeID );
	}
}

void SwitchContainerProxyLocal::RemoveNodeFromSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		)
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveNodeFromSwitch( in_switch, in_nodeID );
	}
}

void SwitchContainerProxyLocal::SetContinuousValidation( bool in_bIsContinuousCheck )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetContinuousValidation( in_bIsContinuousCheck );
	}
}

void SwitchContainerProxyLocal::SetContinuePlayback( AkUniqueID in_NodeID, bool in_bContinuePlayback )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetContinuePlayback( in_NodeID, in_bContinuePlayback );
	}
}

void SwitchContainerProxyLocal::SetFadeInTime( AkUniqueID in_NodeID, AkTimeMs in_time )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetFadeInTime( in_NodeID, in_time );
	}
}

void SwitchContainerProxyLocal::SetFadeOutTime( AkUniqueID in_NodeID, AkTimeMs in_time )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetFadeOutTime( in_NodeID, in_time );
	}
}

void SwitchContainerProxyLocal::SetOnSwitchMode( AkUniqueID in_NodeID, AkOnSwitchMode in_bSwitchMode )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetOnSwitchMode( in_NodeID, in_bSwitchMode );
	}
}

void SwitchContainerProxyLocal::SetIsFirstOnly( AkUniqueID in_NodeID, bool in_bIsFirstOnly )
{
	CAkSwitchCntr* pIndexable = static_cast<CAkSwitchCntr*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetIsFirstOnly( in_NodeID, in_bIsFirstOnly );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
