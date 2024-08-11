/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "behTreeNodeQuestActions.h"

class CQuestPokeScriptedActionsBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestPokeScriptedActionsBlock, CQuestGraphBlock )

protected:
	

	CName											m_npcTag;
	CName											m_pokeEvent;
	Bool											m_onlyOneActor;
	Bool											m_handleBehaviorOutcome;
public:
	CQuestPokeScriptedActionsBlock();

	virtual CName							GetCancelEventName()const	{return CName::NONE;}
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
	//virtual void OnSerialize( IFile& file ) override;

	// Object interface
	void OnPropertyPostChange( IProperty* property )override;
};

BEGIN_CLASS_RTTI( CQuestPokeScriptedActionsBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_EDIT( m_npcTag, TXT( "Who should perform the actions?" ) )
	PROPERTY_EDIT( m_pokeEvent, TXT("The event specified in the CPokeScriptedAction to interrupt the scripted action") );
	PROPERTY( m_handleBehaviorOutcome );
	PROPERTY_EDIT( m_onlyOneActor, TXT("Set to false if you want all actors under this tag to be affected") );
END_CLASS_RTTI();
