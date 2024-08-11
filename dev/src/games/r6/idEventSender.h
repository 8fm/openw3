#pragma once
/*
class CIDInterlocutorComponent;
class CIDTopicInstance;
struct SGeneralEventData;
struct SInterlocutorEventData;
struct SAnimationEventData;
struct SAIEventData;
struct SEncounterPhaseData;
*/

#include "idEvent.h"

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CEventSender : public CObject
{	
	DECLARE_ENGINE_CLASS( CEventSender, CObject, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	TDynArray< CIdEvent* >		m_Events;
	/*
	TDynArray< SGeneralEventData >		m_GeneralEvents;
	TDynArray< SInterlocutorEventData >	m_GeneralInterlocutorEvents;
	TDynArray< SAIEventData >			m_AIEvents;
	TDynArray< SAnimationEventData >	m_AnimationEvents;
	TDynArray< String >					m_FactsToAdd;
	TDynArray< String >					m_FactsToRemove;
	TDynArray< IQuestSpawnsetAction* >	m_QuestSpawnsets;		//!< Community spawnsets this block activates
	TDynArray< SEncounterPhaseData >	m_EncounterSpawnsets;
	*/
	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	void	ActivateAll				( CIDTopicInstance* topicInstance );
/*
private:
	void	AddFacts				( );
	void	RemoveFacts				( );
	void	RaiseGeneralEvents		( );
	void	RaiseGeneralEvent		( CEntity* entity, CName eventName );
	void	RaiseInterlocutorEvents	( CIDTopicInstance* topicInstance );
	void	RaiseAIEvents			( CIDTopicInstance* topicInstance );
	void	RaiseAnimationEvents	( CIDTopicInstance* topicInstance );
	void	RaiseQuestSpawnsets		( );
	void	RaiseEncounterSets		( );
*/
};


BEGIN_CLASS_RTTI( CEventSender )
	PARENT_CLASS( CObject )	
	PROPERTY_INLINED( m_Events, TXT("") )
	/*
	PROPERTY_INLINED( m_GeneralEvents, TXT("") )
	PROPERTY_INLINED( m_GeneralInterlocutorEvents, TXT("") )
	PROPERTY_INLINED( m_AIEvents, TXT("") )
	PROPERTY_INLINED( m_AnimationEvents, TXT("") )
	PROPERTY_EDIT( m_FactsToAdd, TXT("") )
	PROPERTY_EDIT( m_FactsToRemove, TXT("") )
	PROPERTY_INLINED( m_QuestSpawnsets, TXT("") )
	PROPERTY_INLINED( m_EncounterSpawnsets, TXT("") )
	*/
END_CLASS_RTTI()
