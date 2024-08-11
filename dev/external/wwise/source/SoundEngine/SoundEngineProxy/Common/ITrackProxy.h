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

#include "IParameterNodeProxy.h"
#include "AkMusicStructs.h"

struct AkTrackSrcInfo;

class ITrackProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:

	virtual AKRESULT AddSource(
		AkUniqueID      in_srcID,
        AkPluginID      in_pluginID,
		const AkOSChar* in_pszFilename,
		AkFileID		in_uCacheID
        ) = 0;

	virtual AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID 
		) = 0;

	virtual void SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		) = 0;

	virtual void AddClipAutomation(
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eAutomationType,
		AkRTPCGraphPoint		in_arPoints[], 
		AkUInt32				in_uNumPoints 
		) = 0;

    virtual void RemoveAllSources() = 0;

	virtual void IsStreaming( bool in_bIsStreaming ) = 0;

	virtual void IsZeroLatency( bool in_bIsZeroLatency ) = 0;

	virtual void LookAheadTime( AkTimeMs in_LookAheadTime ) = 0;

	virtual void SetMusicTrackRanSeqType( AkUInt32 /*AkMusicTrackRanSeqType*/ in_eRSType ) = 0;

	virtual void SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue ) = 0;

	enum MethodIDs
	{
		MethodAddSource = __base::LastMethodID,
		MethodAddPluginSource,
		MethodSetPlayList,
		MethodAddClipAutomation,
		MethodRemoveAllSources,
		MethodIsStreaming,
		MethodIsZeroLatency,
		MethodLookAheadTime,
		MethodSetMusicTrackRanSeqType,
		MethodSetEnvelope,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
