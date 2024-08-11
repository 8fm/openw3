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


#ifdef AK_MOTION

#include "AkCritical.h"
#include "FeedbackNodeProxyLocal.h"
#include "AkAudioLib.h"
#include "AkFeedbackNode.h"

FeedbackNodeProxyLocal::FeedbackNodeProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkFeedbackNode::Create( in_id ) );
}

FeedbackNodeProxyLocal::~FeedbackNodeProxyLocal()
{
}

void FeedbackNodeProxyLocal::AddPluginSource( 
		AkUniqueID	in_srcID,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddPluginSource(in_srcID, in_idDeviceCompany, in_idDevicePlugin);
	}
}

void FeedbackNodeProxyLocal::RemoveAllSources()
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveAllSources();
	}
}

void FeedbackNodeProxyLocal::SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetSourceVolumeOffset(in_srcID, in_fOffset);
	}
}

#endif
#endif
#endif // #ifndef AK_OPTIMIZED
