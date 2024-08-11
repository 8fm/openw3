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
// AkMusicSegment.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkMusicSegment.h"
#include "AkSegmentCtx.h"
#include "AkMusicRenderer.h"
#include "AkMusicTrack.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkPBI.h"
#include "AkMonitor.h"
#include "AkRandom.h"
#include "AkPlayingMgr.h"

#include "AkMatrixSequencer.h"
#include "AkSequencableSegmentCtx.h"

#define NUM_MIN_MARKERS     (2)

CAkMusicSegment::CAkMusicSegment( 
    AkUniqueID in_ulID
    )
:CAkMusicNode( in_ulID )
,m_uDuration(0)
#ifndef AK_OPTIMIZED
,m_uStartPos(0)
#endif
{
}

CAkMusicSegment::~CAkMusicSegment()
{
    Term();
}

CAkMusicSegment * CAkMusicSegment::Create(
    AkUniqueID in_ulID 
    )
{
	CAkMusicSegment * pSegment = AkNew( g_DefaultPoolId, CAkMusicSegment( in_ulID ) );
    if( pSegment )
	{
		if( pSegment->Init() != AK_Success )
		{
			pSegment->Release();
			pSegment = NULL;
		}
	}
    return pSegment;
}

AKRESULT CAkMusicSegment::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = SetMusicNodeParams( in_pData, in_ulDataSize, false );
	if ( eResult != AK_Success )
		return eResult;

	Duration( READBANKDATA( AkReal64, in_pData, in_ulDataSize ) );

	AkUInt32 ulNumMarkers = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( ulNumMarkers )
	{
		AkMusicMarkerWwise* pArrayMarkers = (AkMusicMarkerWwise*)AkAlloc( g_DefaultPoolId, ulNumMarkers*sizeof( AkMusicMarkerWwise ) );
		if( !pArrayMarkers )
			return AK_InsufficientMemory;

		for( AkUInt32 i = 0; i < ulNumMarkers; ++i )
		{
			pArrayMarkers[i].id = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pArrayMarkers[i].fPosition = READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			AkUInt32 uStringSize;
			char * pMarkerName = READBANKSTRING_UTF8( in_pData, in_ulDataSize, uStringSize );
			if ( uStringSize > 0 )
			{
				pArrayMarkers[i].pszName = (char*)AkAlloc( g_DefaultPoolId, uStringSize + 1 );
				if ( pArrayMarkers[i].pszName )
				{
					memcpy( pArrayMarkers[i].pszName, pMarkerName, uStringSize );
					pArrayMarkers[i].pszName[uStringSize] = 0;
				}
				else
				{
					eResult = AK_Fail;
					break;
				}
			}
			else
				pArrayMarkers[i].pszName = NULL;
		}
		if( eResult == AK_Success )
		{
			eResult = SetMarkers( pArrayMarkers, ulNumMarkers );
		}
		AkFree( g_DefaultPoolId, pArrayMarkers );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkMusicSegment::Init()
{  
    return CAkMusicNode::Init();
}

void CAkMusicSegment::Term()
{
    m_markers.Term();
}

AkNodeCategory CAkMusicSegment::NodeCategory()
{
	return AkNodeCategory_MusicSegment;
}

// Override MusicObject::ExecuteAction() to catch Seek actions.
AKRESULT CAkMusicSegment::ExecuteAction( ActionParams& in_rAction )
{
    if ( ActionParamType_Seek == in_rAction.eType )
	{
		// No need to propagate this action on children; only segments can handle this.
		SeekActionParams & rSeekActionParams = (SeekActionParams&)in_rAction;
		if ( rSeekActionParams.bIsSeekRelativeToDuration )
		{
			AkReal32 fSeekPercent = rSeekActionParams.fSeekPercent;
			
			// Clamp to [0,1].
			if ( fSeekPercent < 0 )
				fSeekPercent = 0;
			else if ( fSeekPercent > 1 )
				fSeekPercent = 1;

			CAkMusicRenderer::Get()->SeekPercent( this, rSeekActionParams.pGameObj, fSeekPercent, rSeekActionParams.bSnapToNearestMarker );
		}
		else
		{
			// Note: Negative seeking is allowed for single segments.
			CAkMusicRenderer::Get()->SeekTimeAbsolute( this, rSeekActionParams.pGameObj, rSeekActionParams.iSeekTime, rSeekActionParams.bSnapToNearestMarker );
		}
		return AK_Success;
	}
	return CAkMusicNode::ExecuteAction( in_rAction );
}	

AKRESULT CAkMusicSegment::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	if ( ActionParamType_Seek == in_rAction.eType )
	{
		// No need to propagate this action on children; only segments can handle this.
		SeekActionParamsExcept & rSeekActionParams = (SeekActionParamsExcept&)in_rAction;
		if ( rSeekActionParams.bIsSeekRelativeToDuration )
		{
			AkReal32 fSeekPercent = rSeekActionParams.fSeekPercent;
			
			// Clamp to [0,1].
			if ( fSeekPercent < 0 )
				fSeekPercent = 0;
			else if ( fSeekPercent > 1 )
				fSeekPercent = 1;

			CAkMusicRenderer::Get()->SeekPercent( this, rSeekActionParams.pGameObj, fSeekPercent, rSeekActionParams.bSnapToNearestMarker );
		}
		else
		{
			// Note: Negative seeking is allowed for single segments.
			CAkMusicRenderer::Get()->SeekTimeAbsolute( this, rSeekActionParams.pGameObj, rSeekActionParams.iSeekTime, rSeekActionParams.bSnapToNearestMarker );
		}
		return AK_Success;
	}
	return CAkMusicNode::ExecuteActionExcept( in_rAction );
}


// Context factory. 
// Creates a sequencable segment context, usable by switch containers or as a top-level instance.
CAkMatrixAwareCtx * CAkMusicSegment::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
    return CreateSegmentContext( in_pParentCtx, in_GameObject, in_rUserparams );
}

CAkSequencableSegmentCtx * CAkMusicSegment::CreateSegmentContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
	if ( m_markers.IsEmpty() )
		return NULL;

    CAkSequencableSegmentCtx * pCtx = AkNew( g_DefaultPoolId, CAkSequencableSegmentCtx( 
        this,
        in_pParentCtx ) );
    if ( pCtx )
    {
		pCtx->AddRef();
        if ( pCtx->Init( in_GameObject, in_rUserparams ) == AK_Success )
		{
			pCtx->Release();
		}
		else
        {
			pCtx->_Cancel();
			pCtx->Release();
            pCtx = NULL;
        }
    }
    return pCtx;
}


CAkSegmentCtx * CAkMusicSegment::CreateLowLevelSegmentCtxAndAddRef( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
	if ( m_markers.IsEmpty() )
		return NULL;

    CAkSegmentCtx * pSegmentCtx = AkNew( g_DefaultPoolId, CAkSegmentCtx( 
        this,
        in_pParentCtx ) );
    if ( pSegmentCtx )
    {
		pSegmentCtx->AddRef();
        if ( pSegmentCtx->Init( in_GameObject, in_rUserparams ) != AK_Success )
        {
            pSegmentCtx->_Cancel();
			pSegmentCtx->Release();
            pSegmentCtx = NULL;
        }
    }
    return pSegmentCtx;
}

AKRESULT CAkMusicSegment::CanAddChild( CAkParameterNodeBase * in_pAudioNode )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		MONITOR_ERRORMSG( AKTEXT("Too many children in one single container.") );
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicTrack)
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

AKRESULT CAkMusicSegment::PlayInternal( AkPBIParams& in_rPBIParams )
{
    // Create a Context as a top-level (that is, attached to the Music Renderer).

    // OPTIM. Could avoid virtual call.
    CAkChainCtx * pCtx = CreateSegmentContext( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams );
    if ( pCtx )
    {
		AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
        fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
        // Set fade offset to segment context's silent duration.
        fadeParams.iFadeOffset      = pCtx->GetSilentDuration();
        pCtx->_Play( fadeParams );
		return AK_Success;
    }
    return AK_Fail;
}

void CAkMusicSegment::Duration(
    AkReal64 in_fDuration               // Duration in milliseconds.
    )
{
    AKASSERT( in_fDuration >= 0 );
    m_uDuration = AkTimeConv::MillisecondsToSamples( in_fDuration );
}

#ifndef AK_OPTIMIZED
void CAkMusicSegment::StartPos(
		AkReal64 in_fStartPos            // StartPosition in milliseconds.
        )
{
    m_uStartPos = AkTimeConv::MillisecondsToSamples( in_fStartPos );
}
#endif

AKRESULT CAkMusicSegment::SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                 in_ulNumMarkers
		)
{
	m_markers.Term();
	if ( m_markers.Reserve( in_ulNumMarkers ) != AK_Success )
		return AK_Fail;

	for( AkUInt32 i = 0; i < in_ulNumMarkers; ++i )
	{
		AkMusicMarker * pNewMarker = m_markers.AddLast();
		AKASSERT( pNewMarker );	// reserved.
		pNewMarker->id = in_pArrayMarkers[i].id;
		pNewMarker->uPosition = AkTimeConv::MillisecondsToSamples( in_pArrayMarkers[i].fPosition );
		pNewMarker->SetNameAndTakeOwnership( in_pArrayMarkers[i].pszName );
		
	}
	return AK_Success;
}

// Interface for Contexts
// ----------------------
CAkMusicTrack * CAkMusicSegment::Track(
    AkUInt16 in_uIndex
    )
{
    if ( in_uIndex < m_mapChildId.Length( ) )
        return static_cast<CAkMusicTrack*>( m_mapChildId[in_uIndex] );
    return NULL;
}

AKRESULT CAkMusicSegment::GetExitSyncPos(
    AkUInt32		in_uSrcMinTime,         // Minimal time-to-sync constraint related to source, relative to the Entry Marker (always >= 0).
    AkSyncType		in_eSyncType,           // Sync type.
	AkUniqueID &	io_uCueFilterHash,		// Cue filter name hash. Returned as filter hash if sync point was found on a cue, AK_INVALID_UNIQUE_ID otherwise.
    bool			in_bSkipEntryCue,		// If true, will not consider Entry cue.
    AkUInt32 &		out_uExitSyncPos        // Returned Exit Sync position (always >= 0), relative to the Entry Marker.
    )
{
    AKRESULT eResult = AK_Fail;

    // Leave now if uMinSyncPosition is passed the Exit marker.
    if ( in_uSrcMinTime <= ExitMarkerPosition() )
    {
        switch ( in_eSyncType )
        {
        case SyncTypeImmediate:
			io_uCueFilterHash = AK_INVALID_UNIQUE_ID;
            out_uExitSyncPos = in_uSrcMinTime;
            eResult = AK_Success;
            break;
        case SyncTypeNextGrid:
            {
                const AkMusicGrid & grid = GetMusicGrid();
                eResult = GetNextMusicGridValue(
                    in_uSrcMinTime,
                    grid.uGridDuration,
                    grid.uGridOffset,
					in_bSkipEntryCue,
                    out_uExitSyncPos ); // Returns position relative to Entry Cue.
				io_uCueFilterHash = AK_INVALID_UNIQUE_ID;
            }
            break;
        case SyncTypeNextBeat:
            eResult = GetNextMusicGridValue(
                in_uSrcMinTime,
                GetMusicGrid().uBeatDuration,
                0,
				in_bSkipEntryCue,
                out_uExitSyncPos ); // Returns position relative to Entry Cue.
			io_uCueFilterHash = AK_INVALID_UNIQUE_ID;
            break;
        case SyncTypeNextBar:
            eResult = GetNextMusicGridValue(
                in_uSrcMinTime,
                GetMusicGrid().uBarDuration,
                0,
				in_bSkipEntryCue,
                out_uExitSyncPos ); // Returns position relative to Entry Cue.
			io_uCueFilterHash = AK_INVALID_UNIQUE_ID;
            break;
        case SyncTypeNextMarker:
            out_uExitSyncPos = GetNextMarkerPosition( in_uSrcMinTime, io_uCueFilterHash, in_bSkipEntryCue );
            eResult = AK_Success;   // Must succeed because we already made sure that uMinSyncPosition was not passed Exit Cue.
            break;
        case SyncTypeNextUserMarker:
            eResult = GetNextUserMarkerPosition( in_uSrcMinTime, io_uCueFilterHash, out_uExitSyncPos );
            break;
        case SyncTypeExitMarker:
            AKASSERT( ExitMarkerPosition() >= in_uSrcMinTime );
            out_uExitSyncPos = ExitMarkerPosition();
			io_uCueFilterHash = m_markers.Last().id;
            eResult = AK_Success;   // Must succeed because we already made sure that uMinSyncPosition was not passed Exit Cue.
            break;
        case SyncTypeEntryMarker:
            if ( in_uSrcMinTime == 0 && !in_bSkipEntryCue )
            {
				io_uCueFilterHash = m_markers[0].id;
                out_uExitSyncPos = 0;
                eResult = AK_Success;
            }
            break;
        default:
            AKASSERT( !"Invalid source transition type" );
            eResult = AK_Fail;
        }
    }

    return eResult;
}

// Returns the position of the entry Sync point defined by the rule. Positions are relative to Entry Cue.
void CAkMusicSegment::GetEntrySyncPos(
    const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
	AkUInt32 in_uMinimumEntryPosition,		// Minimum entry position. Always >= 0.
	AkUniqueID in_uCueHashForMatchSrc,		// Hash of the cue which was used in the source segment. Used only if rule is bDestMatchCueName.
	AkUniqueID & out_uSelectedCueHash,		// Returned cue hash of selected cue. AK_INVALID_UNIQUE_ID if not on a cue.
	AkUInt32 & out_uRequiredEntryPosition	// Returned entry position. Always >= 0.
    )
{
	// Wrap around destination's duration.
	AkUInt32 uDuration = ActiveDuration();
	if ( uDuration > 0 )
		out_uRequiredEntryPosition = in_uMinimumEntryPosition % uDuration;
	else
		out_uRequiredEntryPosition = 0;

	out_uSelectedCueHash = AK_INVALID_UNIQUE_ID;

	// Check rule.
    if ( in_rule.eEntryType == EntryTypeEntryMarker )
	{
		if ( in_uMinimumEntryPosition == 0 )
			out_uSelectedCueHash = m_markers[0].id;
	}
	else if ( in_rule.eEntryType == EntryTypeRandomMarker )
	{
		// Prepare cue filter:
		AkUniqueID uCueFilter = ( in_rule.bDestMatchSourceCueName ) ? in_uCueHashForMatchSrc : in_rule.uCueFilterHash;
		out_uRequiredEntryPosition = GetRandomCue( out_uRequiredEntryPosition, uCueFilter, false, out_uSelectedCueHash );
	}
	else if ( in_rule.eEntryType == EntryTypeRandomUserMarker )
	{
		// Prepare cue filter:
		AkUniqueID uCueFilter = ( in_rule.bDestMatchSourceCueName ) ? in_uCueHashForMatchSrc : in_rule.uCueFilterHash;
		out_uRequiredEntryPosition = GetRandomCue( out_uRequiredEntryPosition, uCueFilter, true, out_uSelectedCueHash );
	}
}

// Select random cue. 
AkInt32 CAkMusicSegment::GetRandomCue(
	AkUInt32			in_uMinPosition, 
	AkUniqueID			in_uCueFilter,
	bool				in_bAvoidEntryCue,
	AkUniqueID &		out_uSelectedCueHash	// Returned cue hash of selected cue. 
	)
{
	AKASSERT( m_markers.Length() >= 2 );

	// Find start index. 
	in_uMinPosition += m_markers[0].uPosition;	// convert to cue position (absolute from beginning of pre-entry).
	AkUInt32 uCueStartIdx = ( in_bAvoidEntryCue ) ? 1 : 0;
	AkUInt32 uTotalCues = m_markers.Length() - 1;	// exclude exit cue
	while ( uCueStartIdx < uTotalCues 
			&& m_markers[uCueStartIdx].uPosition < in_uMinPosition )
	{
		++uCueStartIdx;
	}

	AkUInt32 uNumCues = ( in_uCueFilter != AK_INVALID_UNIQUE_ID ) ? GetNumCuesWithFilter( uCueStartIdx, in_uCueFilter ) : ( uTotalCues - uCueStartIdx );
	if ( uNumCues > 0 )
	{
		AkUInt32 uCue = ( AKRANDOM::AkRandom() % uNumCues );

		if ( in_uCueFilter != AK_INVALID_UNIQUE_ID )
		{
			// Find the corresponding cue among those that match the filter.
			uCue = SelectCueWithFilter( uCueStartIdx, uCue, in_uCueFilter );
		}
		else
		{
			uCue += uCueStartIdx;
		}

		out_uSelectedCueHash = m_markers[uCue].id;
		return m_markers[uCue].uPosition - m_markers[0].uPosition;
	}
	else
	{
		// No user cue: pick the Entry cue.
		out_uSelectedCueHash = m_markers[0].id;
		return 0;
	}
}

// Return number of cues that match filter. Exit cue is NEVER counted in.
AkUInt32 CAkMusicSegment::GetNumCuesWithFilter(
	AkUInt32	in_uCueStartIdx,// Cue index (among all cues) from which to start search (inclusively).
	AkUniqueID	in_uCueFilter
	)
{
	AKASSERT( in_uCueFilter != AK_INVALID_UNIQUE_ID );
	AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
	AkUInt32 uNumCues = m_markers.Length() - 1;	// Skip exit cue
	AkUInt32 uCue = in_uCueStartIdx;
	AkUInt32 uNumMatchingCues = 0;	

	// Entry Cue is an automatic match.
	if ( uCue == 0 )
	{
		uNumMatchingCues = 1;
		uCue = 1;
	}
	
	while ( uCue < uNumCues )
	{
		if ( m_markers[uCue].id == in_uCueFilter )
			++uNumMatchingCues;
		++uCue;
	}
	return uNumMatchingCues;
}

// Return real index in array from an index among cues that match the provided filter hash (including Entry cue).
AkUInt32 CAkMusicSegment::SelectCueWithFilter(
	AkUInt32	in_uCueStartIdx,// Cue index (among all cues) from which to start search (inclusively).
	AkUInt32	in_uIndex,		// Index among cues that match the filter hash (including Entry cue).
	AkUniqueID	in_uCueFilter	// Filter hash.
	)
{
	AKASSERT( in_uCueFilter != AK_INVALID_UNIQUE_ID );
	AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
	
	AkUInt32 uSelIndex = in_uCueStartIdx;
	AkUInt32 uNumCues = m_markers.Length();

	// Special case for the entry cue
	if ( uSelIndex == 0 )
	{
		// Entry cue is an automatic match!
		if ( in_uIndex == 0 )
			return uSelIndex;
		// Entry matches filter, but isn't the one we want
		--in_uIndex;
		++uSelIndex;
	}

	// Skip entry cue.
	do
	{	
		AKASSERT( uSelIndex < uNumCues );
		if ( m_markers[uSelIndex].id == in_uCueFilter )
		{
			// Filter match.
			if ( in_uIndex == 0 )
				return uSelIndex;
			--in_uIndex;
		}
		++uSelIndex;
	}
	while ( uSelIndex < uNumCues );
	AKASSERT( !"Provided index exceeds number of cues matching this filter" );
	return 0;
}

// Returns the duration of the pre-entry.
AkInt32 CAkMusicSegment::PreEntryDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    return m_markers[0].uPosition;
}

// Returns the duration of the post exit.
AkUInt32 CAkMusicSegment::PostExitDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition <= Duration() );
    return ( Duration() - m_markers.Last().uPosition );
}

AkUInt32 CAkMusicSegment::ActiveDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition >= m_markers[0].uPosition );
    return m_markers.Last().uPosition - m_markers[0].uPosition;
}


// Helpers.
// Return marker position, relative to Entry marker.
AkUInt32 CAkMusicSegment::ExitMarkerPosition()
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition >= m_markers[0].uPosition );
    return m_markers.Last().uPosition - m_markers[0].uPosition;
}

AkUInt32 CAkMusicSegment::GetNextMarkerPosition(
    AkInt32			in_iPosition,		// Position from which to start search, relative to Entry marker (>=0).
	AkUniqueID &	io_uCueFilterHash,	// Cue filter name hash. Returned as ID of selected marker.
    bool			in_bDoSkipEntryCue	// If true, will not consider Entry cue.
    )
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    // This method should not be called when the context position is behind the Entry marker.
    AKASSERT( !in_bDoSkipEntryCue || in_iPosition >= 0 || !"Segment position is behind Entry marker; transition should not be queried" );

    // Make position absolute.
    AKASSERT( (AkInt32)m_markers[0].uPosition + in_iPosition >= 0 );
    AkUInt32 uAbsPosition = in_iPosition + m_markers[0].uPosition;

    // Skip Entry marker.
    AKASSERT( m_markers.Length() > 0 );
    MarkersArray::Iterator it = m_markers.Begin();
    if ( in_bDoSkipEntryCue )
        ++it;
    while ( it != m_markers.End() )
    {
        if ( (*it).uPosition >= uAbsPosition 
			&& CueMatchesFilter( (*it), io_uCueFilterHash ) )
		{
			io_uCueFilterHash = (*it).id;
            return (*it).uPosition - m_markers[0].uPosition;
		}
        ++it;
    }

    // Could not find a marker before the end that matched the cue filter. Return the exit cue.
	io_uCueFilterHash = m_markers.Last().id;
	return ExitMarkerPosition();
}

// Note. Might fail if there are no user marker before Exit marker.
// This method MAY be called when the context position is behind the Entry marker.
AKRESULT CAkMusicSegment::GetNextUserMarkerPosition(
    AkUInt32		in_uPosition,		// Position from which to start search, relative to Entry marker (>=0).
	AkUniqueID &	io_uCueFilterHash,	// Cue filter name hash. Returned as ID of selected marker.
	AkUInt32 &		out_uMarkerPosition	// Returned cue position.
    )
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    
    // Make position absolute.
    AkUInt32 uAbsPosition = in_uPosition + m_markers[0].uPosition;

    // Skip Entry marker.
    AkUInt32 uIndex = 1;
    AkUInt32 uLastIndex = m_markers.Length() - 2;

    while ( uIndex <= uLastIndex )
    {
        if ( m_markers[uIndex].uPosition > uAbsPosition
			&& CueMatchesFilter( m_markers[uIndex], io_uCueFilterHash ) )
        {
			io_uCueFilterHash = m_markers[uIndex].id;
            out_uMarkerPosition = m_markers[uIndex].uPosition - m_markers[0].uPosition;
            return AK_Success;
        }
        ++uIndex;
    }

    // Could not find user marker before Exit marker.
    return AK_Fail;
}

// Return the number of user marker in the specified range ([in_iStartPosition, in_iStartPosition+in_uRangeSize[).
void CAkMusicSegment::NotifyUserCuesInRange(
	AkPlayingID in_playingID,		// Playing ID
	const AkMusicGrid& in_rGrid,	// Grid for notification info
	AkInt32  in_iStartPosition, // Start Position relative to entry cue (In Samples)
	AkUInt32 in_uRangeSize		// Range size (In Samples)
	)
{
	// Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );

	AkUInt32 uStartPosition;
	AkUInt32 uEndPosition;

	// Make position absolute and clamp range at 0.
    in_iStartPosition += m_markers[0].uPosition;
	if ( in_iStartPosition >= 0 )
	{
		uStartPosition = (AkUInt32)in_iStartPosition;
		uEndPosition = uStartPosition + in_uRangeSize;
	}
	else
	{
		AkInt32 iEndPosition = in_iStartPosition + in_uRangeSize;
		if ( iEndPosition <= 0 )
			return;
		uStartPosition = 0;
		uEndPosition = (AkUInt32)iEndPosition;
	}

    // Skip Entry marker.
    AkUInt32 uIndex = 1;
	// Ignore exit marker as well.
    AkUInt32 uLastIndex = m_markers.Length() - 2;

	while ( uIndex <= uLastIndex )
    {
        if ( m_markers[uIndex].uPosition >= uStartPosition )
        {
			if ( m_markers[uIndex].uPosition >= uEndPosition )
				break; // They are ordered, no more will be found. leave now

			g_pPlayingMgr->NotifyMusicUserCues( in_playingID, in_rGrid, m_markers[uIndex].GetName() );
        }
        ++uIndex;
    }
}

// Return the number of beat, bar, and grid boundaries in the specified range: [in_iStartPosition, in_iStartPosition+in_uRangeSize[.
void CAkMusicSegment::GetNumMusicGridInRange(
	AkInt32  in_iStartPosition, // Start Position relative to entry cue (In Samples)
	AkUInt32 in_uRangeSize,		// Range size (In Samples)
	AkUInt32 & out_uNumBars,	// Returned number of bar boundaries.
	AkUInt32 & out_uNumBeats,	// Returned number of beat boundaries.
	AkUInt32 & out_uNumGrids	// Returned number of grid boundaries.
	)
{
	// Clamp start position at 0. There is not grid before entry cue.
	AkUInt32 uStartPosition;
	if ( in_iStartPosition >= 0 )
		uStartPosition = in_iStartPosition;
	else
	{
		AkUInt32 uRangeClamp = (AkUInt32)( 0 - in_iStartPosition );
		if ( in_uRangeSize <= uRangeClamp )
		{
			out_uNumBars = 0;
			out_uNumBeats = 0;
			out_uNumGrids = 0;
			return;
		}
		in_uRangeSize -= uRangeClamp;
		uStartPosition = 0;
	}

	// Clamp range at ActiveDuration(). There is not grid after exit cue.
	AkUInt32 uMaxPosition = ActiveDuration();
	if ( ( uStartPosition + in_uRangeSize ) > uMaxPosition )
	{
		// Check if start position is greater than max position, or range size would be 0.
		if ( uStartPosition >= uMaxPosition )
		{
			out_uNumBars = 0;
			out_uNumBeats = 0;
			out_uNumGrids = 0;
			return;
		}
		in_uRangeSize = uMaxPosition - uStartPosition;
	}

	const AkMusicGrid & grid = GetMusicGrid();
	out_uNumBars = GetNumMusicGridInRange( uStartPosition, in_uRangeSize, grid.uBarDuration, 0 );
	out_uNumBeats = GetNumMusicGridInRange( uStartPosition, in_uRangeSize, grid.uBeatDuration, 0 );
	out_uNumGrids = GetNumMusicGridInRange( uStartPosition, in_uRangeSize, grid.uGridDuration, grid.uGridOffset );
}

// Return the number of either grid, beat or bar boundaries in the specified range: [in_iStartPosition, in_iStartPosition+in_uRangeSize[.
AkUInt32 CAkMusicSegment::GetNumMusicGridInRange(
    AkUInt32	in_uStartPosition,	// Start position (in samples), relative to entry cue (>=0).
	AkUInt32	in_uRangeSize,		// Range size (in samples), clamped at entry and exit cues.
    AkUInt32	in_uGridDuration,	// Grid (beat, bar) duration.
    AkUInt32	in_uOffset			// Grid offset.
    )
{
	// IMPORTANT: In very rare cases (extravagant soundbank design), the grid may be invalid (duration == 0).
	if ( in_uGridDuration == 0 )
		return 0;

	// Consider grid offset.
	in_uStartPosition += ( in_uGridDuration - in_uOffset );

	// Formula: ((range + start%gridsize + gridsize - 1)/(int)gridsize) - ((start%gridsize + gridsize - 1)/(int)gridsize)
	AkUInt32 uNum = ( in_uStartPosition % in_uGridDuration ) + in_uGridDuration - 1;
	return ( (in_uRangeSize + uNum) / in_uGridDuration ) - ( uNum / in_uGridDuration );
}

// Returns the position (relative to entry cue) of the cue that is the
// closest to the position provided (exit cue is ignored).
AkInt32 CAkMusicSegment::GetClosestCuePosition(
	AkInt32 in_iPosition	// Position, relative to entry cue
	)
{
	// Transform position into absolute position.
	in_iPosition += m_markers[0].uPosition;

	AkMusicMarker * pCue = NULL;
	AkInt32 iSmallestDistance;

	// REVIEW: Could perform a bisection algorithm.
	AkUInt32 uNumCues = m_markers.Length() - 1;	// Ignore exit cue
	for ( AkUInt32 uCue = 0; uCue<uNumCues; uCue++ )
    {
		AkInt32 iDistance = abs( (int)m_markers[uCue].uPosition - in_iPosition );
		if ( !pCue 
			|| iDistance < iSmallestDistance )
		{
			pCue = &m_markers[uCue];
			iSmallestDistance = iDistance;
		}
    }

	// Return position of chosen cue relative to the entry cue
	AKASSERT( pCue && pCue->uPosition >= m_markers[0].uPosition );
	return pCue->uPosition - m_markers[0].uPosition;
}

bool CAkMusicSegment::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
	{
		CAkMusicTrack* pTrack = (CAkMusicTrack*)((*iter));
		if( pTrack->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	return CAkParameterNodeBase::GetStateSyncTypes( in_stateGroupID, io_pSyncTypes );
}

// Find next Bar, Beat, Grid absolute position.
// Returns AK_Success if a grid position was found before the Exit Cue.
// NOTE This computation is meant to change with compound time signatures.
AKRESULT CAkMusicSegment::GetNextMusicGridValue(
    AkUInt32	in_uMinPosition,	// Start search position (in samples), relative to Entry cue.
    AkUInt32	in_uGridDuration,	// Grid (beat, bar) duration.
    AkUInt32	in_uOffset,			// Grid offset.
	bool		in_bSkipEntryCue,	// Do not return 0 if true.
    AkUInt32 &	out_uExitPosition	// Returned position (relative to Entry cue).
    )
{
	// Trick: Advance min position by one sample if entry cue should not be considered.
	if ( in_bSkipEntryCue )
		in_uMinPosition++;
    AkUInt32 uExitMarker = ExitMarkerPosition();
    out_uExitPosition = in_uOffset;

	// IMPORTANT: In very rare cases (extravagant soundbank designs), the grid may be invalid (duration == 0).
	// In such a case return "Immediate".
	if ( in_uGridDuration > 0 )
	{
		/// REVIEW: 
		/// 1) Inefficient.
		/// 2) Off-by-one error? If in_uMinPosition is exactly on a grid boundary,
		/// we are going to pick the next one.
		while ( out_uExitPosition <= in_uMinPosition )
		{
			out_uExitPosition += in_uGridDuration;
		}
	}
	else
	{
		// NOTE: +1 to be consistent with computation above.
		out_uExitPosition = in_uMinPosition + 1;
	}

    if ( out_uExitPosition <= uExitMarker )
        return AK_Success;

    // Did not find a grid boundary before the Exit marker. 
    return AK_Fail;
}
