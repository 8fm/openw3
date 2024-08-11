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

#include "MusicRanSeqProxyLocal.h"
#include "AkMusicRanSeqCntr.h"
#include "AkAudioLib.h"
#include "AkCritical.h"

MusicRanSeqProxyLocal::MusicRanSeqProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicRanSeqCntr::Create( in_id ) );
}

MusicRanSeqProxyLocal::~MusicRanSeqProxyLocal()
{
}

void MusicRanSeqProxyLocal::SetPlayList( AkMusicRanSeqPlaylistItem* in_pArrayItems, AkUInt32 )
{
	CAkMusicRanSeqCntr* pMusicRanSeq = static_cast<CAkMusicRanSeqCntr*>( GetIndexable() );
	if( pMusicRanSeq )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pMusicRanSeq->SetPlayListChecked( in_pArrayItems );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
