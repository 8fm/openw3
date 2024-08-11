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

#include "SoundProxyConnected.h"
#include "AkSound.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

SoundProxyConnected::SoundProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkSound::Create( in_id );

	SetIndexable( pIndexable );
}

SoundProxyConnected::~SoundProxyConnected()
{
}

void SoundProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkSound * pSound = static_cast<CAkSound *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case ISoundProxy::MethodSetSource:
		{
			SoundProxyCommandData::SetSource setSource;
			if( in_rSerializer.Get( setSource ) )
					SetSource( setSource.m_pszSourceName );
			break;
		}

	case ISoundProxy::MethodSetSource_Plugin:
		{
			SoundProxyCommandData::SetSource_Plugin setSource_Plugin;
			if( in_rSerializer.Get( setSource_Plugin ) )
					pSound->SetSource( setSource_Plugin.m_param1 );
			break;
		}

	case ISoundProxy::MethodIsZeroLatency:
		{
			SoundProxyCommandData::IsZeroLatency isZeroLatency;
			if( in_rSerializer.Get( isZeroLatency ) )
					pSound->IsZeroLatency( isZeroLatency.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

void SoundProxyConnected::SetSource( char* /*in_pszSource*/ )
{
//	static_cast<SoundProxyLocal&>( GetLocalProxy() ).SetSource( CString( "w:\\TestSounds\\" ) + in_pszSource );
}
#endif // #ifndef AK_OPTIMIZED
