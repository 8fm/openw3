#pragma once

#include "questCondition.h"
#include "../../common/engine/triggerManagerImpl.h"
#include "../../common/engine/triggerAreaComponent.h"
#include "../../common/engine/globalEventsManager.h"

class CQuestTriggerCondition : public IQuestCondition, public ITriggerAreaListener, public IGlobalEventsListener
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestTriggerCondition, IQuestCondition )

protected:
	CName				m_triggerTag;
	CName				m_tag;

	Bool				m_isFulfilled;

	typedef	THandle< CTriggerAreaComponent >	TTrigger;
	typedef CTriggerAreaComponent*				TTriggerPtr;
	typedef TDynArray< TTrigger >				TTriggers;
	typedef THandle< CComponent >				TActivator;
	typedef CComponent*							TActivatorPtr;
	typedef TDynArray< TActivator >				TActivators;
	
	TTriggers			m_triggers;
	TActivators			m_activators;
	TTriggers			m_triggersToRemove;
	TActivators			m_activatorsToRemove;
	
	Bool				m_wasRegistered;
	Bool				m_collectingTriggers;

public:
	CQuestTriggerCondition();
	virtual ~CQuestTriggerCondition();

	void SetTriggerTag( CName triggerTag )
	{
		m_triggerTag = triggerTag;
	}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Trigger Tag: %s, Actor Tag: %s" ), 
			m_triggerTag.AsString().AsChar(), m_tag.AsString().AsChar() ); 
	}
#endif
	
	CName GetTriggerTag() { return m_triggerTag; }
	CName GetActorTag() { return m_tag; }

	// IterateTaggedNodes interface
	RED_INLINE Bool EarlyTest( CNode* node )
	{
		return (node != nullptr );
	}

	RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
	{
		CEntity* entity = Cast< CEntity >( node );
		if( entity )
		{
			if( m_collectingTriggers )
			{
				if ( m_triggers.Size() > 1 )
				{
					WARN_GAME( TXT( "More than one trigger with tag %s required by quest condition %s was found in the world" ), m_triggerTag.AsString().AsChar(), GetName().AsString().AsChar() );
				}

				for ( ComponentIterator< CTriggerAreaComponent > triggerIt( entity ); triggerIt; ++triggerIt )
				{
					AddTrigger( *triggerIt );
				}			

			}
			else
			{
				AddActivatorEntity( entity );
			}
		}
	}

protected:

	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	virtual void OnTriggerAdded( const TTrigger& trigger ) { }
	virtual void OnTriggerRemoved( const TTrigger& trigger ) { }
	virtual void OnActivatorAdded( const TActivator& activator ) { }
	virtual void OnActivatorRemoved( const TActivator& activator ) { }

	virtual void RemoveUnusedComponents();

	Bool CollectTriggers();
	Bool CollectActivators();
	Bool AddTrigger( const TTrigger& trigger );
	Bool RemoveTrigger( const TTrigger& trigger );
	Bool AddActivator( const TActivator& activator );
	Bool RemoveActivator( const TActivator& activator );
	void AddActivatorEntity( CEntity* activatorEntity );

	//! ITriggerAreaListener
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator ) override { };
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator ) override { };

	//! IGlobalEventsListener
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

	Bool RegisterCallback( Bool reg );
	Bool IsTriggerActivated( const TTrigger& trigger, const TActivator& skipActivator = nullptr );
	static Bool IsInside( const TTrigger& trigger, const TActivator& activator );
};

BEGIN_ABSTRACT_CLASS_RTTI( CQuestTriggerCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_triggerTag, TXT("Tag of the trigger area."), TXT( "TaggedEntitySelector" ) );
	PROPERTY_EDIT( m_tag, TXT("Actor's tag") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestInsideTriggerCondition : public CQuestTriggerCondition
{
	DECLARE_ENGINE_CLASS( CQuestInsideTriggerCondition, CQuestTriggerCondition, 0 )

private:
	Bool						m_isInside;

public:
	CQuestInsideTriggerCondition();

	void SetIsInside( Bool isInside )
	{
		m_isInside = isInside;
	}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "%s, Is Inside: %s" ), 
			CQuestTriggerCondition::GetDescription().AsChar(), 
			( m_isInside ) ? TXT( "true" ) : TXT( "false" ) ); 
	}
#endif
	
	Bool IsInside() { return m_isInside; }
	virtual void OnActivate() override;

	void UpdateCondition();

	virtual void OnTriggerAdded( const TTrigger& trigger ) override;
	virtual void OnActivatorAdded( const TActivator& activator ) override;

	// ITriggerAreaListener
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator ) override;
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator ) override;
};

BEGIN_CLASS_RTTI( CQuestInsideTriggerCondition )
	PARENT_CLASS( CQuestTriggerCondition )
	PROPERTY_EDIT( m_isInside, TXT("Is the player expected to be inside the area?") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestEnterTriggerCondition : public CQuestTriggerCondition
{
	DECLARE_ENGINE_CLASS( CQuestEnterTriggerCondition, CQuestTriggerCondition, 0 )

private:

	Bool					m_onAreaEntry;
	typedef TSortedMap< TTrigger, Bool >	TTriggersStates;
	TTriggersStates			m_states;

public:
	
	CQuestEnterTriggerCondition();

	virtual void OnActivate() override;

	virtual void OnTriggerAdded( const TTrigger& trigger ) override;
	virtual void OnTriggerRemoved( const TTrigger& trigger ) override;
	virtual void OnActivatorAdded( const TActivator& activator ) override;

	virtual void RemoveUnusedComponents() override;

	// ITriggerAreaListener
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator ) override;
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator ) override;
};

BEGIN_CLASS_RTTI( CQuestEnterTriggerCondition )
	PARENT_CLASS( CQuestTriggerCondition )
	PROPERTY_EDIT( m_onAreaEntry, TXT("Should the trigger react to player entering or exiting the area?") );
END_CLASS_RTTI()
