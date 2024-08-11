/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questsSystem.h"

class CQuestControlledNPCsManager;
class CQuestUsedFastTravelCondition;
struct SUsedFastTravelEvent;

class CR4QuestSystem : public CQuestsSystem
{
	DECLARE_ENGINE_CLASS( CR4QuestSystem, CQuestsSystem, 0 );

protected:
	CQuestControlledNPCsManager*				m_npcsManager;
	TDynArray< CQuestUsedFastTravelCondition* >	m_fastTravelListeners;

public:
	CR4QuestSystem();
	~CR4QuestSystem();

	// Deactivates the system
	virtual void Deactivate();
	
	void AttachFastTravelListener( CQuestUsedFastTravelCondition& listener );
	void DetachFastTravelListener( CQuestUsedFastTravelCondition& listener );
	virtual Bool OnUsedFastTravelEvent( const SUsedFastTravelEvent& event );

public:
	// ------------------------------------------------------------------------
	// NPCs management
	// ------------------------------------------------------------------------
	RED_INLINE CQuestControlledNPCsManager* GetNPCsManager() { return m_npcsManager; }

public:
	// -------------------------------------------------------------------------
	// Debug 
	// -------------------------------------------------------------------------
	virtual Bool IsNPCInQuestScene( const CNewNPC* npc ) const;

	ASSIGN_GAME_SYSTEM_ID( GS_QuestsSystem )
};

BEGIN_CLASS_RTTI( CR4QuestSystem );
	PARENT_CLASS( CQuestsSystem );
END_CLASS_RTTI();