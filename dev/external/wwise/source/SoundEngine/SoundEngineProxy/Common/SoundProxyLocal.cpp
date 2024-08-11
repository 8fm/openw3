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

#include "SoundProxyLocal.h"
#include "AkSound.h"
#include "AkAudioLib.h"
#include "AkCritical.h"

namespace AK
{
	namespace WWISESOUNDENGINE_DLL
	{
		extern GlobalAnalysisSet g_setAnalysis;
	}
}


SoundProxyLocal::SoundProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkSound::Create( in_id ) );
}

SoundProxyLocal::~SoundProxyLocal()
{
}

void SoundProxyLocal::SetSource( AkUniqueID in_sourceID, AkUInt32 in_PluginID, const AkOSChar* in_szFileName, AkFileID in_uCacheID )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
#ifdef AK_WIN
		pIndexable->SetSource( in_sourceID, in_PluginID, in_szFileName, in_uCacheID );
#endif
	}
}

void SoundProxyLocal::SetSource( AkUniqueID in_sourceID )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetSource( in_sourceID );
	}
}

void SoundProxyLocal::IsZeroLatency( bool in_bIsZeroLatency )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IsZeroLatency(in_bIsZeroLatency);
	}
}

void SoundProxyLocal::SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		AK::WWISESOUNDENGINE_DLL::GlobalAnalysisSet::iterator it = AK::WWISESOUNDENGINE_DLL::g_setAnalysis.find( in_sourceID );
		if ( it != AK::WWISESOUNDENGINE_DLL::g_setAnalysis.end() )
		{
			// Already there. Update and notify sources.
			(*it).second = AK::WWISESOUNDENGINE_DLL::AnalysisInfo( 
				in_pEnvelope, 
				in_uNumPoints, 
				in_fMaxEnvValue );
			(*it).second.NotifyObservers();
		}
		else
		{
			// New envelope. 
			AK::WWISESOUNDENGINE_DLL::g_setAnalysis[ in_sourceID ] = AK::WWISESOUNDENGINE_DLL::AnalysisInfo( 
				in_pEnvelope, 
				in_uNumPoints, 
				in_fMaxEnvValue );
		}
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
