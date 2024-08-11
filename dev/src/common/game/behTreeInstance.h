/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiScriptableStorage.h"
#include "aiStorage.h"
#include "behTreeEvent.h"


class CBehTree;
class IBehTreeNodeInstance;
class CBehTreeSpawnContext;

class CBehTreeInstance : public CObject, public CAIStorage, public CAIScriptableStorage
{
	DECLARE_ENGINE_CLASS( CBehTreeInstance, CObject, 0 );

protected:
	typedef TDynArray< IBehTreeNodeInstance* > tNodeList;
	typedef TArrayMap< SBehTreeEvenListeningData, tNodeList > tEventListeners;
	typedef TDynArray< THandle< IScriptable > > tDependentObjects;
	typedef TDynArray< THandle< IBehTreeMetanodeOnSpawnDefinition > > tOnReattachCallback;

	IBehTreeNodeInstance*					m_treeInstance;
	const IBehTreeNodeDefinition*			m_treeDefinition;	//!< Instanced tree definition
	CBehTreeMachine*						m_machine;			//!< BehTree machine
	CActor*									m_actor;			//!< Actor
	CNewNPC*								m_npc;				//!< NPC
	tEventListeners							m_eventListeners;
	Float									m_localTime;
	Float									m_localDelta;
	THandle< CNode >						m_actionTarget;
	TArrayMap< CName, THandle< CNode > >	m_namedTarget;
	THandle< CActor >						m_combatTarget;
	tDependentObjects						m_dependentObjects;
	tOnReattachCallback						m_onReattachCallback;
	tNodeList								m_delayedDestruction;

#ifdef EDITOR_AI_DEBUG
	Bool							m_isDebugged;
#endif

	static const Char*		NULL_TEXT;

	void OnEvent( CBehTreeEvent& e, SBehTreeEvenListeningData::EType type ) const;
	void ProcessDelayedNodeDeletion();

public:
	CBehTreeInstance()
		: m_treeInstance( NULL )
		, m_treeDefinition( NULL )
		, m_machine( NULL )
		, m_localTime( 0.f )
		, m_localDelta( 0.f )
#ifdef EDITOR_AI_DEBUG
		, m_isDebugged( false )
#endif
	{
	}

	virtual ~CBehTreeInstance()
	{
		ASSERT( !m_treeInstance && !m_treeDefinition && !m_machine );
	}

	//! Get entity - added for future R6 use!
	CEntity* GetEntity() const;

	//! Get actor
	RED_INLINE CActor* GetActor() const																{ return m_actor; }

	//! Get NPC
	RED_INLINE CNewNPC* GetNPC() const																{ return m_npc; }

	//! Set owner (actor and npc)
	void SetOwner( CActor* actor );

	//! Get actor name for logging
	const Char* GetActorDebugName() const;

	//! Get tree machine
	RED_INLINE CBehTreeMachine* GetMachine() const													{ return m_machine; }

	//! Set tree machine
	RED_INLINE void SetMachine( CBehTreeMachine* mach )												{ m_machine = mach; }

	//! Get definition root node
	RED_INLINE const IBehTreeNodeDefinition* GetDefinitionRootNode() const							{ return m_treeDefinition; }

	//! Get instance root node
	RED_INLINE IBehTreeNodeInstance* GetInstanceRootNode() const									{ return m_treeInstance; }

	//! Bind to tree and data buffer
	void Bind( const IBehTreeNodeDefinition* treeDefinition, CBehTreeSpawnContext& context );

	void OnSpawn( CBehTreeSpawnContext& context );

	//! Unbind from tree
	void Unbind();

	//! Asynchronous reattachment processing
	void OnReattachAsync( CBehTreeSpawnContext& context );

	//! Synchronous reattachment processing code
	void OnReattach( CBehTreeSpawnContext& context );

	void OnDetached();
	
	//! Force reevaluations 'now'
	void ForcedUpdate();

	void Activate();
	void Deactivate();

	//! Tick
	void Update( Float delta );

	//! Is currently active behavior branch a combat one
	virtual Bool IsInCombat() const;
	virtual void SetIsInCombat( Bool inCombat );

	//! Process animation event
	void ProcessAnimationEvent( const CExtAnimEvent* event, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo ) const;

	//! Process gameplay event
	void OnGameplayEvent( CName name, void* additionalData = NULL, IRTTIType* additionalDataType = NULL ) const;

	Float GetLocalTime() const																		{ return m_localTime; }
	Float GetLocalTimeDelta() const																	{ return m_localDelta; }

	const THandle< CNode >& GetActionTarget() const { return m_actionTarget; }
	void SetActionTarget( const THandle< CNode >& node ) { m_actionTarget = node; }

	THandle< CNode > GetNamedTarget( CName targetName );
	void SetNamedTarget( const CName targetName, const THandle< CNode >& node );

	const THandle< CActor >& GetCombatTarget() const { return m_combatTarget; }
	virtual void SetCombatTarget( const THandle< CActor >& node, Bool registerAsAttacker = true );
	virtual void ClearCombatTarget();
	virtual void OnCombatTargetDestroyed();

	void BindEventListeners( CBehTreeSpawnContext& context );
	void RemoveEventListener( const SBehTreeEvenListeningData& data, IBehTreeNodeInstance* node );

	void OnSerialize( IFile& file ) override;
	void OnFinalize() override;

	Bool OnPoolRequest();

	virtual void DescribeTicketsInfo( TDynArray< String >& info );

	void AddMetanodeToReattachCallback( const IBehTreeMetanodeOnSpawnDefinition* node );

	RED_INLINE void AddDependentObject( const THandle< IScriptable >& obj )							{ m_dependentObjects.PushBack( obj ); }
	RED_INLINE void RemoveDependentObject( const THandle< IScriptable >& obj )						{ m_dependentObjects.Remove( obj ); }

	void SaveState( IGameSaver* saver );
	Bool LoadState( IGameLoader* loader );

	void PrepareDestruction(); // ctremblay Hack. Use to split Unbind in two phase, unbind and destruction. 
	void DestroyRootAndListeners();

	void RegisterNodeForDeletion( IBehTreeNodeInstance* node );

#ifdef EDITOR_AI_DEBUG
	void SetIsDebugged( Bool b )																	{ m_isDebugged = b; }
	Bool GetIsDebugged() const																		{ return m_isDebugged; }
#endif

};

BEGIN_CLASS_RTTI( CBehTreeInstance )
	PARENT_CLASS( CObject )
	PROPERTY_NOSERIALIZE( m_dependentObjects  )
	PROPERTY_NOSERIALIZE( m_onReattachCallback )
END_CLASS_RTTI()



RED_INLINE CEntity* CBehTreeInstance::GetEntity() const
{
	return FindParent< CEntity >();
}

