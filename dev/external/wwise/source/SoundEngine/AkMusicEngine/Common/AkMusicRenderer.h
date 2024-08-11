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
// AkMusicRenderer.h
//
// Base context class for all parent contexts.
// Propagates commands to its children. Implements child removal.
//
// NOTE: Only music contexts are parent contexts, so this class is
// defined here. Move to AkAudioEngine if other standard nodes use them.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_RENDERER_H_
#define _MUSIC_RENDERER_H_

#include "AkMusicPBI.h"
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkMatrixAwareCtx.h"
#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include "AkSegmentInfoRepository.h"

class CAkSoundBase;
class CAkSource;

class CAkMatrixSequencer;




//-------------------------------------------------------------------
// Class CAkMusicRenderer: Singleton. Manages top-level music contexts,
// routes game actions to music contexts, acts as an external Behavioral 
// Extension and State Handler, and as a music PBI factory.
//-------------------------------------------------------------------
class CAkMusicRenderer
{
public:

    ~CAkMusicRenderer();

	static CAkMusicRenderer * Create(
		AkMusicSettings *	in_pSettings
		);
    inline static CAkMusicRenderer * Get() { return m_pMusicRenderer; }
    void Destroy();

	// Access to global settings.
	inline static AkReal32 StreamingLookAheadRatio() { return m_musicSettings.fStreamingLookAheadRatio; }

    // Add/Remove Top-Level Music Contexts (happens when created from MusicNode::Play()).
    
    AKRESULT AddChild( 
        CAkMatrixAwareCtx * in_pMusicCtx,
        UserParams &    in_rUserparams,
        CAkRegisteredObj *  in_pGameObj
        );
    void RemoveChild( 
        CAkMatrixAwareCtx * in_pMusicCtx
        );
	
    // Behavioral Extension callback.
    static void PerformNextFrameBehavior( bool in_bLastCall );

	// Music objects queries.
	AKRESULT GetPlayingSegmentInfo(
		AkPlayingID		in_PlayingID,		// Playing ID returned by AK::SoundEngine::PostEvent()
		AkSegmentInfo &	out_segmentInfo,	// Position of the music segment (in ms) associated with that playing ID. The position is relative to the Entry Cue.
		bool			in_bExtrapolate		// Position is extrapolated based on time elapsed since last sound engine update.
		);

    
    // Similar to URenderer::Play().
    // Creates a Music PBI (a PBI that can be a child of a high-level context) and assigns a parent.
    // Returns it
    static AKRESULT Play( 
        CAkMusicCtx *		io_pParentCtx,
		CAkSoundBase*		in_pSound,
		CAkSource *			in_pSource,
        CAkRegisteredObj *	in_pGameObj,
        TransParams &		in_transParams,
        UserParams&			in_rUserparams,
		const AkTrackSrc *	in_pSrcInfo,		// Pointer to track's source playlist item.
        AkUInt32			in_uSourceOffset,   // Start position of source (in samples, at the native sample rate).
		AkUInt32			in_uFrameOffset,    // Frame offset for look-ahead and LEngine sample accuracy.
		CAkMusicPBI *&		out_pPBI            // TEMP: Created PBI is needed to set the transition from outside.
        );

    // Game triggered actions (stop/pause/resume).
    void Stop(	
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams,
		AkPlayingID			in_playingID = AK_INVALID_PLAYING_ID
        );
	void Pause(
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams,
		AkPlayingID			in_playingID = AK_INVALID_PLAYING_ID
        );
	void Resume(	
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams,
        bool                in_bIsMasterResume,  // REVIEW
		AkPlayingID			in_playingID = AK_INVALID_PLAYING_ID
        );
	void SeekTimeAbsolute(	
        CAkMusicNode *		in_pNode,
        CAkRegisteredObj *  in_pGameObj,
		AkTimeMs			in_iSeekTime,
		bool				in_bSnapToCue
        );
	void SeekPercent(	
        CAkMusicNode *		in_pNode,
        CAkRegisteredObj *  in_pGameObj,
		AkReal32			in_fSeekPercent,
		bool				in_bSnapToCue
        );


    // States management.
    // ---------------------------------------------

    // External State Handler callback.
    // Returns true if state change was handled (delayed) by the Music Renderer. False otherwise.
    static bool SetState( 
        AkStateGroupID      in_stateGroupID, 
        AkStateID           in_stateID
        );

    // Execute a StateChange music action.
    void PerformDelayedStateChange(
        void *              in_pCookie
        );
    // Notify Renderer whenever a StateChange music action is Flushed.
    void RescheduleDelayedStateChange(
        void *              in_pCookie
        );
    
#ifndef AK_OPTIMIZED
	// External handle for profiling interactive music specific information.
	static void HandleProfiling();

	// For live editing.
	void NotifyEditDirty() { m_bEditDirty = true; }
	
#define NOTIFY_EDIT_DIRTY()		if ( AkMonitor::IsMonitoring() ) CAkMusicRenderer::Get()->NotifyEditDirty()
#else
#define NOTIFY_EDIT_DIRTY()
#endif

	// U_NEXTITEM Policy for MatrixAwareCtxList (used herein). Needs to be public.
	struct AkListBareNextTopLevelCtxSibling
	{
		static AkForceInline CAkMatrixAwareCtx *& Get( CAkMatrixAwareCtx * in_pItem ) 
		{
			return in_pItem->pNextTopLevelSibling;
		}
	};

    // Stops all top-level contexts.
	static bool StopAll();

private:
    static void Play(	
        CAkMusicPBI *   in_pContext, 
        TransParams &   in_transParams
        );

    CAkMusicRenderer();
    AKRESULT Init(
		AkMusicSettings *	in_pSettings
		);

private:
    static CAkMusicRenderer * m_pMusicRenderer;
	static AkMusicSettings	m_musicSettings;

	static CAkSegmentInfoRepository m_segmentInfoRepository;

    // List of top-level music contexts.
	
	typedef AkListBare<CAkMatrixAwareCtx,AkListBareNextTopLevelCtxSibling> MatrixAwareCtxList;
    static MatrixAwareCtxList	m_listCtx;

    // States management.
    struct AkStateChangeRecord
    {
        AkStateGroupID  stateGroupID;
        AkStateID       stateID;
		AkUInt32		bWasPosted		:1;
		AkUInt32		bIsReferenced	:1;
    };
    // Note: Order is important. New pending state changes are added at the beginning of the list. Old ones are at the end.
    typedef CAkList2<AkStateChangeRecord,const AkStateChangeRecord&,AkAllocAndFree> PendingStateChanges;
    static PendingStateChanges          m_queuePendingStateChanges;

    // Helpers.
    // Query top-level context sequencers that need to handle state change by delaying it.
    // Returns the minimal absolute delay for state change. Value <= 0 means "immediate".
    static AkInt64 GetDelayedStateChangeData(
        AkStateGroupID          in_stateGroupID, 
        CAkMatrixAwareCtx *&    out_pChosenCtx,
        AkInt64 &               out_iChosenRelativeSyncTime,
        AkUInt32 &              out_uChosenSegmentLookAhead
        );
    typedef PendingStateChanges::Iterator   PendingStateChangeIter;
    typedef PendingStateChanges::IteratorEx PendingStateChangeIterEx;
    static void CancelDelayedStateChange( 
        AkStateGroupID     in_stateGroupID, 
        PendingStateChangeIterEx & in_itPendingStateChg
        );
    static void FindPendingStateChange( 
        void * in_pCookie,
        PendingStateChangeIterEx & out_iterator
        );
    static void InvalidateOlderPendingStateChanges( 
        PendingStateChangeIter & in_iterator,
        AkStateGroupID           in_stateGroupID
        );
    static void CleanPendingStateChanges();

	// Synchronization event for MusicEngine::Term():
	// We need to wait for the Lower Engine to stop/notify all music PBIs
	// before we can destroy our data structures.
	static AkEvent m_hTermEvent;

#ifndef AK_OPTIMIZED
	inline static bool IsEditDirty() { return m_bEditDirty; }
	inline static void ClearEditDirty() { m_bEditDirty = false; }
	// For live editing: list of nodes that have been set dirty because of live edit.
	static bool m_bEditDirty;
#endif
};

#endif
