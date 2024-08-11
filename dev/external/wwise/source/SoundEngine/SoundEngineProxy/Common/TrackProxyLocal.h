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
#pragma once

#ifndef AK_OPTIMIZED
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterNodeProxyLocal.h"
#include "ITrackProxy.h"

class CAkMusicTrack;

class TrackProxyLocal : public ParameterNodeProxyLocal
						, virtual public ITrackProxy
{
public:
	TrackProxyLocal( AkUniqueID in_id );
	virtual ~TrackProxyLocal();

	// ITrackProxy members
	virtual AKRESULT AddSource(
		AkUniqueID      in_srcID,
        AkPluginID      in_pluginID,
		const AkOSChar* in_pszFilename,
        AkFileID		in_uCacheID
        );

	virtual AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID 
		);

	virtual void SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		);

	virtual void AddClipAutomation(
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eAutomationType,
		AkRTPCGraphPoint		in_arPoints[], 
		AkUInt32				in_uNumPoints 
		);

    virtual void RemoveAllSources();

	virtual void IsStreaming( bool in_bIsStreaming );

	virtual void IsZeroLatency( bool in_bIsZeroLatency );

	virtual void LookAheadTime( AkTimeMs in_LookAheadTime );

	virtual void SetMusicTrackRanSeqType( AkUInt32 /*AkMusicTrackRanSeqType*/ in_eRSType );

	virtual void SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue );
};
#endif
#endif // #ifndef AK_OPTIMIZED
