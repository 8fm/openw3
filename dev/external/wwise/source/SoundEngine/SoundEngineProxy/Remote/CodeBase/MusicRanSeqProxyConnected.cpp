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

#include "MusicRanSeqProxyConnected.h"
#include "AkMusicRanSeqCntr.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IMusicRanSeqProxy.h"

MusicRanSeqProxyConnected::MusicRanSeqProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkMusicRanSeqCntr::Create( in_id );

	SetIndexable( pIndexable );
}

MusicRanSeqProxyConnected::~MusicRanSeqProxyConnected()
{
}

void MusicRanSeqProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicRanSeqCntr* pMusicRanSeq = static_cast<CAkMusicRanSeqCntr*>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IMusicRanSeqProxy::MethodSetPlayList:
		{
			MusicRanSeqProxyCommandData::SetPlayList setPlayList;
			if (in_rSerializer.Get( setPlayList ))
				pMusicRanSeq->SetPlayListChecked( setPlayList.m_pArrayItems );

			break;
		}
	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
