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
// AkMusicTrack.h
//
// Class for music track node.
// The music track is not a music node. It cannot be played, and does
// not implement a context creation method. However it is an active
// parent because it has children sounds, and propagates notifications
// to them. Only MusicSegment can be a parent of MusicTrack.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_TRACK_H_
#define _MUSIC_TRACK_H_

#include "AkSoundBase.h"
#include "AkMusicStructs.h"
#include "AkSource.h"

// -------------------------------------------------------------------
// Structures.
// -------------------------------------------------------------------
struct AkTrackSrc
{
	AkUInt32	uSubTrackIndex;		// Subtrack index.
	AkUniqueID	srcID;				// Source ID.
    AkUInt32    uClipStartPosition;	// Clip start position, relative to beginning of the track.
    AkUInt32    uClipDuration;		// Clip duration.
	AkUInt32    uSrcDuration;		// Source's original duration.
    AkInt32     iSourceTrimOffset;	// Source offset at the beginning of the clip (positive, < OriginalSourceDuration).
};

// Clip automation objects, stored in an array in tracks.
class CAkClipAutomation
{
public:
	// IMPORTANT: Do NOT free conversion table in destructor. CAkClipAutomation objects are stored in an 
	// array, which moves items efficiently by performing shallow copy. Users are responsible to free the 
	// table (with UnsetCurve()) when removing/destroying this object.
	CAkClipAutomation() {}
	CAkClipAutomation( const CAkClipAutomation & in_other )
	{
		// Shallow copy.
		m_uClipIndex		= in_other.m_uClipIndex;
		m_eAutoType			= in_other.m_eAutoType;
		m_tableAutomation	= in_other.m_tableAutomation;
	}
	~CAkClipAutomation() {}

	AKRESULT Set( 
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eAutoType,
		AkRTPCGraphPoint		in_arPoints[], 
		AkUInt32				in_uNumPoints 
		)
	{
		ClearCurve();

		m_uClipIndex	= in_uClipIndex;
		m_eAutoType		= in_eAutoType;

		if ( in_uNumPoints > 0 )
		{
			if ( m_tableAutomation.Set( in_arPoints, in_uNumPoints, AkCurveScaling_None ) == AK_Success )
			{
				// Iterate through table to convert point time (x) in samples.
				for ( AkUInt32 uPoint = 0; uPoint<in_uNumPoints; uPoint++ )
					m_tableAutomation.m_pArrayGraphPoints[uPoint].From = (AkReal32)AkTimeConv::SecondsToSamples( m_tableAutomation.m_pArrayGraphPoints[uPoint].From );
			}
		}
		return AK_Success;
	}

	inline void ClearCurve()
	{
		m_tableAutomation.Unset();
	}

	// Get the automation value at the given time.
	inline AkReal32 GetValue( 
		AkInt32 iTime 	// Current time relative to clip start.
		)
	{
		return m_tableAutomation.Convert( (AkReal32)iTime );
	}

	inline AkUInt32	ClipIndex() { return m_uClipIndex; }
	inline AkClipAutomationType	Type() { return m_eAutoType; }

protected:
	
	// Key.
	AkUInt32				m_uClipIndex;
	AkClipAutomationType	m_eAutoType;

	// Automation table.
	CAkConversionTable<AkRTPCGraphPoint,AkReal32> m_tableAutomation;
};

// -------------------------------------------------------------------
// Extension to the Source object containing music specific information
// -------------------------------------------------------------------
class CAkMusicSource : public CAkSource
{
public:

    CAkMusicSource()
        :m_uStreamingLookAhead(0)
    {
    }

    AkUInt32 StreamingLookAhead()
    {
        return m_uStreamingLookAhead;
    }

    void StreamingLookAhead( AkUInt32 in_uStmLookAhead )
    {
        m_uStreamingLookAhead = in_uStmLookAhead;
    }

private:
    AkUInt32				m_uStreamingLookAhead;	// Streaming look-ahead (in samples, at the native frame rate).
};

// -------------------------------------------------------------------
// Name: CAkMusicTrack
// Desc: Track audio node.
// -------------------------------------------------------------------
class CAkMusicTrack : public CAkSoundBase
{
public:
    // Thread safe version of the constructor.
	static CAkMusicTrack * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly );

	// Return the node category.
	virtual AkNodeCategory NodeCategory();

    // Play the specified node
    // NOT IMPLEMENTED.
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

    // Wwise specific interface.
    // -----------------------------------------
	AKRESULT AddPlaylistItem(
		AkTrackSrcInfo &in_srcInfo
		);

	AKRESULT SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		);

	AKRESULT AddClipAutomation(
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eAutomationType,
		AkRTPCGraphPoint		in_arPoints[], 
		AkUInt32				in_uNumPoints 
		);

    AKRESULT AddSource(
		AkUniqueID      in_srcID,
		AkPluginID      in_pluginID,
        const AkOSChar* in_pszFilename,
		AkFileID		in_uCacheID
        );

	AKRESULT AddSource( 
		AkUniqueID in_srcID, 
		AkUInt32 in_pluginID, 
		AkMediaInformation in_MediaInfo
		);

	AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID
		);

	bool HasBankSource();

	bool SourceLoaded(){ return !m_arSrcInfo.IsEmpty(); }

	// WAL-only 
    void RemoveAllSources();

	void IsZeroLatency( bool in_bIsZeroLatency );

	void LookAheadTime( AkTimeMs in_LookAheadTime );

	virtual AkObjectCategory Category();

	// Like ParameterNodeBase's, but does not check parent.
	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

    // Interface for Contexts
    // ----------------------

	CAkMusicSource* GetSourcePtr( AkUniqueID in_SourceID );

	// Note: AkTrackSrcInfo is the internal representation of audio source positions in tracks: in samples.
    // Loop count is stored in the Sound object.
	typedef AkArray<AkTrackSrc, const AkTrackSrc&, ArrayPoolDefault> TrackPlaylist;
	const TrackPlaylist & GetTrackPlaylist(){ return m_arTrackPlaylist; }

	void SetMusicTrackRanSeqType( AkMusicTrackRanSeqType in_eType ){ m_eRSType = in_eType; };

	AkUInt16 GetNextRS();

	CAkClipAutomation * GetClipAutomation(
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eAutomationType
		)
	{
		ClipAutomationArray::Iterator it = m_arClipAutomation.FindByKey( in_uClipIndex, in_eAutomationType );
		if ( it != m_arClipAutomation.End() )
			return &(*it);
		return NULL;
	}		

	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

protected:
    CAkMusicTrack( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicTrack();

    AKRESULT Init() { return CAkSoundBase::Init(); }

	// Helper: remove all sources without checking if the play count is above 0.
	void RemoveAllSourcesNoCheck();

    // Array of source descriptors.
    typedef CAkKeyArray<AkUniqueID, CAkMusicSource*> SrcInfoArray;
    SrcInfoArray    m_arSrcInfo;

	// Array of clip automation.
	class ClipAutomationArray : public AkArray<CAkClipAutomation, const CAkClipAutomation&,ArrayPoolDefault>
	{
	public:
		Iterator FindByKey( AkUInt32 in_uClipIndex, AkClipAutomationType in_eAutomationType )
		{
			Iterator it = Begin();
			while ( it != End() )
			{
				if ( (*it).ClipIndex() == in_uClipIndex && (*it).Type() == in_eAutomationType )
					break;
				++it;
			}
			return it;
		}

		// Override Term(): Free curve tables before destroying (see note in CAkClipAutomation).
		void Term()
		{
			if ( m_pItems )
			{
				for ( Iterator it = Begin(), itEnd = End(); it != itEnd; ++it )
				{
					(*it).ClearCurve();
					(*it).~CAkClipAutomation();
				}
				AkFree( ArrayPoolDefault::Get(), m_pItems );
				m_pItems = 0;
				m_uLength = 0;
				m_ulReserved = 0;
			}
		}
	protected:
		void RemoveAll();	// Undefined.
	};
	ClipAutomationArray m_arClipAutomation;

	AkUInt32		m_uNumSubTrack;

	// Classified by index of the sub track
	
	TrackPlaylist			m_arTrackPlaylist;
	AkInt32					m_iLookAheadTime;
	AkMusicTrackRanSeqType	m_eRSType;
	AkUInt16				m_SequenceIndex;
};

#endif
