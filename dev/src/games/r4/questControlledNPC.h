/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CBehaviorGraph;
class CMovingAgentComponent;
class CActor;
class CQuestControlledNPC;
class IQuestNPCsManagerListener;

///////////////////////////////////////////////////////////////////////////////

class CQuestControlledNPCsManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	typedef TDynArray< CQuestControlledNPC* >			NPCsList;
	typedef THashMap< CName, NPCsList* >					NPCsMap;
	typedef TDynArray< IQuestNPCsManagerListener* >		Listeners;

private:
	NPCsMap												m_npcs;
	Listeners											m_listeners;

public:
	CQuestControlledNPCsManager();
	~CQuestControlledNPCsManager();

	// Takes control over the specified NPCs
	Bool Activate( TDynArray< CQuestControlledNPC* >& npcsArr );

	// Relinquishes control over the specified NPCs
	void Deactivate( TDynArray< CQuestControlledNPC* >& npcsArr );

	// Resets the manager, relinquishing the control over all currently 
	// controlled NPCs.
	void Reset();

	// Raises a behavior event on an NPC with the specified tag
	Bool RaiseBehaviorEvent( const CName& npcTag, const CName& eventName );

	// Checks if the event has been processed on an NPC with the specified tag
	Bool HasBehaviorBeenProcessed( const CName& npcTag, const CName& eventName );

	// Cleans up after a processed behavior event.
	void CleanupAfterBehaviorEvent( const CName& npcTag, const CName& eventName );

	// Checks if an NPC received the specified notification
	Uint32 WasNotificationReceived( const CName& npcTag, const CName& notificationName );

	// Returns the number of controlled entities
	Uint32 GetControlledEntitiesCount( const CName& npcTag ) const;

	// -------------------------------------------------------------------------
	// Notifications
	// -------------------------------------------------------------------------

	// Attaches a new listener
	void AttachListener( IQuestNPCsManagerListener& listener );

	// Detaches a listener
	void DetachListener( IQuestNPCsManagerListener& listener );

private:
	void NotifyError( const String& errMsg ) const;
	void NotifySuccess( const String& msg ) const;

	// -------------------------------------------------------------------------
	// Collection management 
	// -------------------------------------------------------------------------
	Bool IsInMap( CQuestControlledNPC* npc ) const;
	void AddToMap( CQuestControlledNPC* npc );
	void RemoveFromMap( CQuestControlledNPC* npc );

public:
	// -------------------------------------------------------------------------
	// Debug 
	// -------------------------------------------------------------------------
	Bool IsNPCInQuestScene( const CNewNPC* npc ) const;
};

///////////////////////////////////////////////////////////////////////////////

// A helper class that encapsulates the functionality of taking control
// over a selected NPC
class CQuestControlledNPC
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	TSoftHandle< CBehaviorGraph >				m_behavior;
	CName										m_tag;

	CEntity*									m_entity;
	CBehaviorGraphStack*						m_stack;
	TDynArray< CName >							m_processedEvents;

public:
	CQuestControlledNPC( const CName& tag, const TSoftHandle< CBehaviorGraph>& behavior, CEntity* entity, CBehaviorGraphStack* stack );
	~CQuestControlledNPC();

	// Initializes the instance, returns true if the initialization is successful
	// and the NPC can be activated
	Bool Initialize();

	//! Takes control over an NPC
	Bool Activate( String& errMsg );

	//! Relinquishes the control over an NPC
	Bool Deactivate( String& errMsg );

	// Returns the tag of the NPC
	RED_INLINE const CName& GetTag() const { return m_tag; }

	// Raises a behavior event
	Bool RaiseBehaviorEvent( const CName& eventName, String& outErrorMsg );

	// Checks if the event has been processed
	Bool HasBehaviorBeenProcessed( const CName& eventName, String& outErrorMsg );

	// Cleans up after a processed behavior event.
	void CleanupAfterBehaviorEvent( const CName& eventName );

	// Checks if an NPC received the specified notification
	Bool WasNotificationReceived( const CName& notificationName, String& outErrorMsg );

private:
	Bool IsUpToDate() const;
	String GetActiveNodesList( CBehaviorGraphStack* stack ) const;
};

///////////////////////////////////////////////////////////////////////////////

class IQuestNPCsManagerListener
{
public:
	virtual ~IQuestNPCsManagerListener() {}

	virtual void NotifyError( const String& errMsg ) const = 0;
	virtual void NotifySuccess( const String& msg ) const = 0;
};
