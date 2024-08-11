/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiProfile.h"

typedef Uint8 NewNPCSenseUpdateFlags;

/// Base class for NPC sense
class CNewNPCSense
{
public:
	enum ENewNPCSenseUpdateFlag : NewNPCSenseUpdateFlags
	{
		FLAG_NO_CHANGES									= 0,
		FLAG_NOTICED_OBJECTS_APPEARS					= FLAG( 0 ),
		FLAG_NOTICED_OBJECTS_VANISHED					= FLAG( 1 ),
		FLAG_VISIBILITY_FLAGS_CHANGED					= FLAG( 2 ),
	};



	static const Float			UPDATE_KNOWLEDGE_INTERVAL;
	static const Float			PLAYER_CHECK_INTERVAL;
	static const Float			NPC_CHECK_INTERVAL;

protected:

	CNewNPC*					m_npc;					//!< The NPC	
	Float						m_updateKnowladgeTimer; //!< Time to the next knowledge update
	Float						m_timeToPlayerCheck;	//!< Time to player check
	Float						m_timeToNPCCheck;		//!< Time to npc check
	EngineTime					m_lastTimePlayerNoticed;//!< Last time when player has been noticed

public:

	CNewNPCSense( CNewNPC* npc )
		: m_npc( npc )
		, m_updateKnowladgeTimer( GEngine->GetRandomNumberGenerator().Get< Float >( UPDATE_KNOWLEDGE_INTERVAL ) )
		, m_timeToPlayerCheck( GEngine->GetRandomNumberGenerator().Get< Float >( PLAYER_CHECK_INTERVAL ) )
		, m_timeToNPCCheck( GEngine->GetRandomNumberGenerator().Get< Float >( NPC_CHECK_INTERVAL ) )
		, m_lastTimePlayerNoticed( -1000.0f )
	{
	}

	virtual ~CNewNPCSense();

	//! Initialize
	virtual void Initialize() = 0;

	//! Set template sense params pointer
	virtual void SetSenseParams( EAISenseType senseType, CAISenseParams* senseParams ) = 0;

	//! Get template sense params pointer
	virtual CAISenseParams* GetSenseParams( EAISenseType senseType ) const = 0;

	//! Get max range
	virtual Float GetRangeMax() const = 0;

	//! Get max range for specified sense type
	virtual Float GetRangeMax( EAISenseType senseType ) const = 0;

	//! Check if given sense type is valid
	virtual Bool IsValid( EAISenseType senseType ) const = 0;

	//! Get last time player has been noticed
	const EngineTime& GetLastTimePlayerNoticed() const { return m_lastTimePlayerNoticed; }

	//! Get sense info
	virtual String GetInfo() const = 0;

	//! Force update stored information
	void ForceUpdateKnowledge();

	//! Update stored information
	NewNPCSenseUpdateFlags UpdateKnowledge( Float timeDelta, TNoticedObjects& noticedObjects );

	//! Check if sense should be updated at all
	RED_FORCE_INLINE Bool ShouldUpdate( Float timeDelta ) const
	{
		return ShouldUpdatePlayer( timeDelta ) || ShouldUpdateNPCs( timeDelta );
	}

	//! Check if sense should update player info
	RED_FORCE_INLINE Bool ShouldUpdatePlayer( Float timeDelta ) const
	{
		return ( m_timeToPlayerCheck - timeDelta < 0.0f );
	}

	//! Check if sense should update NPCs info
	RED_FORCE_INLINE Bool ShouldUpdateNPCs( Float timeDelta ) const
	{
		return ( m_timeToNPCCheck - timeDelta < 0.0f );
	}

	//! Initialize sense update
	virtual Bool BeginUpdate( TNoticedObjects& noticedObjects ) = 0;

	//! Finalize sense update
	virtual NewNPCSenseUpdateFlags EndUpdate( TNoticedObjects& noticedObjects, NewNPCSenseUpdateFlags updated, Float timeDelta );

	//! Update player
	virtual NewNPCSenseUpdateFlags UpdatePlayer( TNoticedObjects& noticedObjects ) = 0;

	//! Update NPCs
	virtual NewNPCSenseUpdateFlags UpdateNPC( CNewNPC* npc, TNoticedObjects& noticedObjects ) = 0;

	//! Generate debug fragments
	virtual void GenerateDebugFragments( CRenderFrame* frame ) = 0;
};
