/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behTreeArbitratorPriorities.h"
#include "behTreeNodeQuestActions.h"
#include "behTreeStateSerializer.h"



///////////////////////////////////////////////////////////////////////////////
// CQuestScriptedActionsBlockAIListener
///////////////////////////////////////////////////////////////////////////////
class CQuestScriptedActionsBlockAIListener : public CBehTreeExternalListener
{
public:
	enum EActionState
	{
		AS_PROGRESS,
		AS_COMPLETED_SUCCCESS,
		AS_COMPLETED_FAILED,
	} m_state;

	CQuestScriptedActionsBlockAIListener()
		: m_state( AS_PROGRESS ) {}

	void OnBehaviorCompletion( Bool success ) override
	{
		m_state = success ? AS_COMPLETED_SUCCCESS : AS_COMPLETED_FAILED;

		Unregister();
	}

	void OnBehaviorDestruction() override
	{
		m_state = AS_COMPLETED_FAILED;
	}
};

struct SScriptedActionData
{
	DECLARE_RTTI_SIMPLE_CLASS( SScriptedActionData )
public:
	THandle< CActor >						m_actor;
	CQuestScriptedActionsBlockAIListener *	m_listener;
	Int16									m_behaviorId;

	SScriptedActionData( CActor *const actor = nullptr )
		: m_actor( THandle< CActor >( actor ) )
		, m_listener( nullptr )
		, m_behaviorId( -1 )		{}
	~SScriptedActionData();
};

BEGIN_CLASS_RTTI( SScriptedActionData );
END_CLASS_RTTI();

struct SScriptedActionSerializedState
{
	DECLARE_RTTI_STRUCT( SScriptedActionSerializedState );

public:
	SScriptedActionSerializedState();
	~SScriptedActionSerializedState();											// NOTICE: Its freeing up IGameDataStorage objects!
	struct Val
	{
		IdTag										m_guid;
		IGameDataStorage*							m_aiState;
	};

	TDynArray< Val >								m_list;

	void			Push( const IdTag& guid, IGameDataStorage* aiState );
	void			Free();
};

BEGIN_CLASS_RTTI( SScriptedActionSerializedState );
END_CLASS_RTTI();

class CBaseQuestScriptedActionsBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBaseQuestScriptedActionsBlock, CQuestGraphBlock )

protected:
	CName											m_npcTag;
	mutable THandle< CAIQuestScriptedActionsTree >	m_forcedAction;				// lazy cached ai tree
	
	ETopLevelAIPriorities							m_actionsPriority;
	Bool											m_onlyOneActor;
	Bool											m_handleBehaviorOutcome;

	// runtime data
	TInstanceVar< TDynArray< SScriptedActionData > >	i_scriptedActionDataArray;
	TInstanceVar< SScriptedActionSerializedState >		i_serializedState;

	virtual CName							GetCancelEventName()const = 0;
public:
	CBaseQuestScriptedActionsBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

private:
	//! CGraphBlock interface
	virtual void							OnRebuildSockets();
	virtual EGraphBlockShape				GetBlockShape() const;
	virtual Color							GetClientColor() const;
	virtual String							GetBlockCategory() const;
	virtual Bool							CanBeAddedToGraph( const CQuestGraph* graph ) const;
public:
	Bool									IsHandlingBehaviorOutcome() const	{ return m_handleBehaviorOutcome; }
	void									SetHandleBahviorOutcome( Bool b )	{ m_handleBehaviorOutcome = b; OnRebuildSockets(); }

#endif // NO_EDITOR_GRAPH_SUPPORT

public:

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const override;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
	virtual void OnExecute( InstanceBuffer& data ) const override;
	virtual void OnDeactivate( InstanceBuffer& data ) const override;
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const override;

	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const override;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const override;

private:
	Bool WaitForActorToAppear( InstanceBuffer& data ) const;

	virtual CName							GetEventName() const = 0;
	virtual CAIQuestScriptedActionsTree*	ComputeAIActions() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBaseQuestScriptedActionsBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_EDIT( m_npcTag, TXT( "Who should perform the actions?" ) )
	PROPERTY( m_handleBehaviorOutcome );
	PROPERTY_EDIT( m_actionsPriority, TXT("AI actions priority") );
	PROPERTY_EDIT( m_onlyOneActor, TXT("Set to false if you want all actors under this tag to be affected") );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
// CQuestScriptedActionsBlock
///////////////////////////////////////////////////////////////////////////////
class CQuestScriptedActionsBlock : public CBaseQuestScriptedActionsBlock
{
	DECLARE_ENGINE_CLASS( CQuestScriptedActionsBlock, CBaseQuestScriptedActionsBlock, 0 );
	typedef CBaseQuestScriptedActionsBlock Super;

private:
	THandle< IAIActionTree >						m_ai;
	TDynArray< IActorLatentAction* >				m_actions;
public:
	CQuestScriptedActionsBlock();

	const TDynArray< IActorLatentAction* >&	GetActionsList() const				{ return m_actions; }

	void									OnPostLoad() override;

protected:
	CName									GetCancelEventName() const override;
private:
	CAIQuestScriptedActionsTree*			ComputeAIActions() const override;
	CName									GetEventName() const	override;
};

BEGIN_CLASS_RTTI( CQuestScriptedActionsBlock );
	PARENT_CLASS( CBaseQuestScriptedActionsBlock );
	PROPERTY_INLINED( m_ai, TXT( "Ai tree representing scripted behavior" ) );
	PROPERTY( m_actions );
END_CLASS_RTTI();
