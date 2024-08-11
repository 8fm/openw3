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
// AkMusicTransAware.h
//
// Class for music transition aware nodes. 
// Holds a map of music transition rules (based on exceptions), 
// and provides services for rule look-up.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_TRANSITION_AWARE_H_
#define _MUSIC_TRANSITION_AWARE_H_

#include "AkMusicStructs.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkMusicNode.h"


class CAkMusicTransAware : public CAkMusicNode
{
public:
    CAkMusicTransAware(
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicTransAware();

	AKRESULT SetMusicTransNodeParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );

    // Looks up transition rules list from end to beginning, returns when it finds a match.
    // Note, returns the whole AkMusicTransitionRule, so that the client can see if the rule
    // applies to a certain kind of node, or if it is general (e.g. a switch container might
    // want to know if the destination is specifically a sequence).
    const AkMusicTransitionRule & GetTransitionRule( 
        AkUniqueID  in_srcID,   // Source (departure) node ID.
        AkUniqueID  in_destID   // Destination (arrival) node ID.        
        );
    const AkMusicTransitionRule & GetTransitionRule( 
		CAkParameterNodeBase * in_pOwnerNode,		// Owner node.
		AkUniqueID  in_srcID,				// Source (departure) node ID.
		CAkParameterNodeBase * in_pSrcParentNode,	// Source (departure) parent node (can be NULL).
        AkUniqueID  in_destID,				// Destination (arrival) node ID.        
        CAkParameterNodeBase * in_pDestParentNode,	// Destination (arrival) parent node (can be NULL).
		bool & out_bIsDestSequenceSpecific	// True when rule's destination is a sequence.
        );
	static const AkMusicTransitionRule & GetPanicTransitionRule();

	static void TermPanicTransitionRule();

    // ISSUE: Who sorts them?
    AKRESULT AddRule(
        AkMusicTransitionRule & in_rule
        );

	AKRESULT SetRules(
		AkUInt32 in_NumRules,
		AkWwiseMusicTransitionRule* in_pRules
		);
    // TODO.

protected:

	virtual AKRESULT PrepareMusicalDependencies();
	virtual void UnPrepareMusicalDependencies();

private:
	void FlushTransitionRules();
	CAkParameterNodeBase * AscendentMatch(
		CAkParameterNodeBase *  in_pOwnerNode,		// Owner node.
		const AkMusicTransitionRule::TransitionNodeArray&	in_ruleIDs,
		CAkParameterNodeBase *  in_pNode
		);

protected:
    typedef AkArray<AkMusicTransitionRule, const AkMusicTransitionRule&, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(AkMusicTransitionRule)> RulesArray;
    RulesArray m_arTrRules;
	
};

#endif // _MUSIC_TRANSITION_AWARE_H_
