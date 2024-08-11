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
// AkMusicTransAware.cpp
//
// Class for music transition aware nodes. 
// Holds a map of music transition rules (based on exceptions), 
// and provides services for rule look-up.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkMusicTransAware.h"

static AkMusicTransitionRule s_panicRule;

CAkMusicTransAware::CAkMusicTransAware( AkUniqueID in_ulID )
:CAkMusicNode( in_ulID )
{
}

CAkMusicTransAware::~CAkMusicTransAware()
{
	FlushTransitionRules();
    m_arTrRules.Term();
}

AKRESULT CAkMusicTransAware::SetMusicTransNodeParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly )
{
	AKRESULT eResult = SetMusicNodeParams( io_rpData, io_rulDataSize, in_bPartialLoadOnly );
	if ( eResult != AK_Success )
		return eResult;

	FlushTransitionRules();

	AkUInt32 numRules = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
	if( numRules )
	{
		if ( m_arTrRules.Reserve( numRules ) != AK_Success )
			return AK_Fail;

		for( AkUInt32 i = 0; i < numRules; ++i )
		{
			AkMusicTransitionRule * pRule = m_arTrRules.AddLast();
			AKASSERT( pRule );	// reserved.

			AkUInt32 uNumSrc = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			if ( pRule->srcIDs.Reserve(uNumSrc) != AK_Success )
				return AK_Fail;

			for( AkUInt32 s = 0; s < uNumSrc; ++s )
			{
				AkUniqueID src = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				AKASSERT(pRule->srcIDs.Length() == 0 || pRule->srcIDs.Last() <= src ); //sorted array
				pRule->srcIDs.AddLast( src );
			}

			AkUInt32 uNumDst = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			if ( pRule->destIDs.Reserve(uNumDst) != AK_Success )
				return AK_Fail;

			for( AkUInt32 d = 0; d < uNumDst; ++d )
			{
				AkUniqueID dst = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				AKASSERT(pRule->destIDs.Length() == 0 || pRule->destIDs.Last() <= dst ); //sorted array
				pRule->destIDs.AddLast( dst );
			}

			pRule->srcRule.fadeParams.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRule->srcRule.fadeParams.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRule->srcRule.fadeParams.iFadeOffset		= AkTimeConv::MillisecondsToSamples( READBANKDATA( AkInt32, io_rpData, io_rulDataSize ) );
			pRule->srcRule.eSyncType					= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRule->srcRule.uCueFilterHash				= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRule->srcRule.bPlayPostExit				= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			pRule->destRule.fadeParams.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRule->destRule.fadeParams.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRule->destRule.fadeParams.iFadeOffset		= AkTimeConv::MillisecondsToSamples( READBANKDATA( AkInt32, io_rpData, io_rulDataSize ) );
			pRule->destRule.uCueFilterHash				= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRule->destRule.uJumpToID					= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRule->destRule.eEntryType					= READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
			pRule->destRule.bPlayPreEntry				= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			pRule->destRule.bDestMatchSourceCueName		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			bool bIsTransObjectEnabled		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
			AkMusicTransitionObject * pTransObj = bIsTransObjectEnabled ? pRule->AllocTransObject() : NULL;
			if( pTransObj )
			{
				pTransObj->segmentID						= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				pTransObj->fadeInParams.transitionTime		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
				pTransObj->fadeInParams.eFadeCurve			=(AkCurveInterpolation) READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				pTransObj->fadeInParams.iFadeOffset			= AkTimeConv::MillisecondsToSamples( READBANKDATA( AkInt32, io_rpData, io_rulDataSize ) );
				pTransObj->fadeOutParams.transitionTime		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
				pTransObj->fadeOutParams.eFadeCurve			= (AkCurveInterpolation) READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				pTransObj->fadeOutParams.iFadeOffset		= AkTimeConv::MillisecondsToSamples( READBANKDATA( AkInt32, io_rpData, io_rulDataSize ) );

				pTransObj->bPlayPreEntry = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
				pTransObj->bPlayPostExit  = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			}

#ifndef AK_OPTIMIZED
			pRule->index = i;
#endif
		}
	}
	return eResult;
}

// Looks up transition rules list from end to beginning, returns when it finds a match.
// Note, returns the whole AkMusicTransitionRule, so that the client can see if the rule
// applies to a certain kind of node, or if it is general (e.g. a switch container might
// want to know if the destination is specifically a sequence).
const AkMusicTransitionRule & CAkMusicTransAware::GetTransitionRule( 
    AkUniqueID  in_srcID,   // Source (departure) node ID.
    AkUniqueID  in_destID   // Destination (arrival) node ID.
    )
{
    AKASSERT( m_arTrRules.Length( ) >= 1 ||
              !"Transition aware must own at least one default rule" );

    int iRule = (int)m_arTrRules.Length( ) - 1;
    while ( iRule >= 0 )
    {
        AkMusicTransitionRule & rule = m_arTrRules[iRule];
        if ( ( rule.srcIDs[0] == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.srcIDs.BinarySearch(in_srcID) != rule.srcIDs.End() ) &&
             ( rule.destIDs[0] == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.destIDs.BinarySearch(in_destID) != rule.destIDs.End() ) )
        {
            return rule;
        }
        --iRule;
    }
    AKASSERT( !"Could not find music transition rule" );
    return m_arTrRules[0];
}

const AkMusicTransitionRule & CAkMusicTransAware::GetTransitionRule( 
	CAkParameterNodeBase * in_pOwnerNode,		// Owner node.
	AkUniqueID  in_srcID,				// Source (departure) node ID.
	CAkParameterNodeBase * in_pSrcParentNode,	// Source (departure) parent node (can be NULL).
    AkUniqueID  in_destID,				// Destination (arrival) node ID.        
    CAkParameterNodeBase * in_pDestParentNode,	// Destination (arrival) parent node (can be NULL).
	bool & out_bIsDestSequenceSpecific	// True when rule's destination is a sequence.
	)
{
    AKASSERT( m_arTrRules.Length( ) >= 1 ||
              !"Transition aware must own at least one default rule" );

	int iRule = (int)m_arTrRules.Length( ) - 1;
    while ( iRule >= 0 )
    {
		CAkParameterNodeBase * pActualDestParentNode = NULL;

        AkMusicTransitionRule & rule = m_arTrRules[iRule];
	
		if( rule.srcIDs.Length() > 0 && rule.destIDs.Length() > 0 )
		{
			if ( ( rule.srcIDs[0] == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
				rule.srcIDs.BinarySearch( in_srcID ) != rule.srcIDs.End() ||
				AscendentMatch( in_pOwnerNode, rule.srcIDs, in_pSrcParentNode ) ) &&
				( rule.destIDs[0] == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
				rule.destIDs.BinarySearch( in_destID ) != rule.destIDs.End() ||
				( pActualDestParentNode = AscendentMatch( in_pOwnerNode, rule.destIDs, in_pDestParentNode ) ) != NULL ) )
			{
				if ( pActualDestParentNode && 
					pActualDestParentNode->NodeCategory() == AkNodeCategory_MusicRanSeqCntr )
					out_bIsDestSequenceSpecific = true;
				else
					out_bIsDestSequenceSpecific = false;
				return rule;
			}
		}
        --iRule;
    }
    AKASSERT( !"Could not find music transition rule" );
    return m_arTrRules[0];
}

const AkMusicTransitionRule & CAkMusicTransAware::GetPanicTransitionRule()
{
	if ( s_panicRule.srcIDs.Length() == 0 )
		s_panicRule.srcIDs.AddLast( AK_MUSIC_TRANSITION_RULE_ID_ANY );
	s_panicRule.srcRule.eSyncType = SyncTypeExitMarker;
	s_panicRule.srcRule.bPlayPostExit = true;
	s_panicRule.srcRule.fadeParams.transitionTime = 0;
	s_panicRule.srcRule.fadeParams.iFadeOffset = 0;
	s_panicRule.srcRule.uCueFilterHash = AK_INVALID_UNIQUE_ID;
	if (s_panicRule.destIDs.Length() == 0)
		s_panicRule.destIDs.AddLast( AK_MUSIC_TRANSITION_RULE_ID_ANY );
	s_panicRule.destRule.eEntryType = EntryTypeEntryMarker;
	s_panicRule.destRule.bPlayPreEntry = false;
	s_panicRule.destRule.fadeParams.transitionTime = 0;
	s_panicRule.destRule.fadeParams.iFadeOffset = 0;
	s_panicRule.destRule.uCueFilterHash = AK_INVALID_UNIQUE_ID;
	s_panicRule.destRule.uJumpToID = 0;
	s_panicRule.destRule.bDestMatchSourceCueName = false;
	s_panicRule.pTransObj = NULL;
#ifndef AK_OPTIMIZED
	s_panicRule.index = 0;
#endif

	return s_panicRule;
}

void CAkMusicTransAware::TermPanicTransitionRule()
{
	s_panicRule.srcIDs.Term();
	s_panicRule.destIDs.Term();
}

CAkParameterNodeBase * CAkMusicTransAware::AscendentMatch(
	CAkParameterNodeBase *  in_pOwnerNode,		// Owner node.
	const AkMusicTransitionRule::TransitionNodeArray&		in_ruleIDs,
	CAkParameterNodeBase *  in_pNode
	)
{
	AKASSERT( in_ruleIDs.Length() > 0 && in_ruleIDs[0] != AK_MUSIC_TRANSITION_RULE_ID_ANY );

	while ( in_pNode &&
			in_pNode != in_pOwnerNode )
	{
		if ( in_ruleIDs.BinarySearch(in_pNode->ID()) != in_ruleIDs.End() )
			return in_pNode;
		in_pNode = in_pNode->Parent();
	}
	return NULL;
}


// Interface for Wwise.
// -------------------
AKRESULT CAkMusicTransAware::SetRules(
		AkUInt32 in_NumRules,
		AkWwiseMusicTransitionRule* in_pRules
		)
{
	FlushTransitionRules();

	if ( in_NumRules == 0 
		|| m_arTrRules.Reserve( in_NumRules ) != AK_Success )
	{
		return AK_Fail;
	}

	for( AkUInt32 i = 0 ;  i < in_NumRules; ++i )
	{
		AkMusicTransitionRule * pRule = m_arTrRules.AddLast();
		AKASSERT( pRule );	// reserved.

		if ( pRule->srcIDs.Reserve(in_pRules[i].uNumSrc) != AK_Success )
			return AK_Fail;

		for( AkUInt32 s = 0; s < in_pRules[i].uNumSrc; ++s )
		{
			AKASSERT( pRule->srcIDs.Length() == 0 || in_pRules[i].srcIDs[s] >= pRule->srcIDs.Last() );//sorted array
			pRule->srcIDs.AddLast(in_pRules[i].srcIDs[s]);
		}
		
		if ( pRule->destIDs.Reserve(in_pRules[i].uNumDst) != AK_Success )
			return AK_Fail;

		for( AkUInt32 d = 0; d < in_pRules[i].uNumDst; ++d )
		{
			AKASSERT( pRule->destIDs.Length() == 0 || in_pRules[i].destIDs[d] >= pRule->destIDs.Last() );//sorted array
			pRule->destIDs.AddLast(in_pRules[i].destIDs[d]);
		}

		pRule->srcRule.bPlayPostExit =				in_pRules[i].bSrcPlayPostExit;
		pRule->srcRule.eSyncType =					in_pRules[i].eSrcSyncType;
		pRule->srcRule.uCueFilterHash =				in_pRules[i].uSrcCueFilterHash;
		pRule->srcRule.fadeParams.transitionTime =	in_pRules[i].srcFade.transitionTime;
		pRule->srcRule.fadeParams.eFadeCurve =		in_pRules[i].srcFade.eFadeCurve;
		pRule->srcRule.fadeParams.iFadeOffset =		AkTimeConv::MillisecondsToSamples( in_pRules[i].srcFade.iFadeOffset );

		pRule->destRule.fadeParams.transitionTime =	in_pRules[i].destFade.transitionTime;
		pRule->destRule.fadeParams.eFadeCurve =		in_pRules[i].destFade.eFadeCurve;
        pRule->destRule.fadeParams.iFadeOffset =	AkTimeConv::MillisecondsToSamples( in_pRules[i].destFade.iFadeOffset );
		pRule->destRule.uCueFilterHash =			in_pRules[i].uDestCueFilterHash;
		pRule->destRule.uJumpToID =					in_pRules[i].uDestJumpToID;
		pRule->destRule.eEntryType =				in_pRules[i].eDestEntryType;
		pRule->destRule.bPlayPreEntry =				in_pRules[i].bDestPlayPreEntry;
		pRule->destRule.bDestMatchSourceCueName =	in_pRules[i].bDestMatchSourceCueName;
		
		if( in_pRules[i].bIsTransObjectEnabled )
		{
			AkMusicTransitionObject * pTransObj = pRule->AllocTransObject();
			if( pTransObj )
			{
				pTransObj->bPlayPostExit = in_pRules[i].bPlayPostExit;
				pTransObj->bPlayPreEntry = in_pRules[i].bPlayPreEntry;
				pTransObj->fadeInParams.transitionTime = in_pRules[i].transFadeIn.transitionTime;
				pTransObj->fadeInParams.eFadeCurve = 	in_pRules[i].transFadeIn.eFadeCurve;
				pTransObj->fadeInParams.iFadeOffset =    AkTimeConv::MillisecondsToSamples( in_pRules[i].transFadeIn.iFadeOffset );
				pTransObj->fadeOutParams.transitionTime =in_pRules[i].transFadeOut.transitionTime;
				pTransObj->fadeOutParams.eFadeCurve = 	in_pRules[i].transFadeOut.eFadeCurve;
				pTransObj->fadeOutParams.iFadeOffset =   AkTimeConv::MillisecondsToSamples( in_pRules[i].transFadeOut.iFadeOffset );
				pTransObj->segmentID = in_pRules[i].segmentID;
			}
			// else there is not enough memory to create a transition segment rule.
		}

#ifndef AK_OPTIMIZED
		pRule->index = i;
#endif
	}

	return AK_Success;
}

void CAkMusicTransAware::FlushTransitionRules()
{
	m_arTrRules.Term();
}

AKRESULT CAkMusicTransAware::PrepareMusicalDependencies()
{
	AKRESULT eResult = CAkMusicNode::PrepareMusicalDependencies();
	if( eResult == AK_Success )
	{
		for( AkUInt32 i = 0; i < m_arTrRules.Length( ); ++i )
		{
			AkMusicTransitionObject* pTransObject = m_arTrRules[i].pTransObj;
			if( pTransObject )
			{
				eResult = PrepareNodeData( pTransObject->segmentID );
			}
			if( eResult != AK_Success )
			{
				for( AkUInt32 flush = 0; flush < i; ++flush )
				{
					pTransObject = m_arTrRules[flush].pTransObj;
					if( pTransObject )
					{
						UnPrepareNodeData( pTransObject->segmentID );
					}
				}
				break;
			}
		}

		if( eResult != AK_Success )
		{
			CAkMusicNode::UnPrepareMusicalDependencies();
		}
	}


	return eResult;
}

void CAkMusicTransAware::UnPrepareMusicalDependencies()
{
	for( AkUInt32 i = 0; i < m_arTrRules.Length( ); ++i )
	{
		AkMusicTransitionObject* pTransObject = m_arTrRules[i].pTransObj;
		if( pTransObject )
		{
			UnPrepareNodeData( pTransObject->segmentID );
		}
	}

	CAkMusicNode::UnPrepareMusicalDependencies();
}
