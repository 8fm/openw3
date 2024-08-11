#pragma once

#include "behTreeMetanode.h"
#include "behTreeTask.h"


////////////////////////////////////////////////////////////////////////
// Node that does 'anything' when AI is created
////////////////////////////////////////////////////////////////////////
class IBehTreeMetanodeOnSpawnDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeMetanodeOnSpawnDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, OnSpawn );

private:
	IBehTreeNodeDefinition*			m_childNode;
	Bool							m_runWhenReattachedFromPool;

protected:
	virtual void					RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const = 0;

public:
	IBehTreeMetanodeOnSpawnDefinition()
		: m_childNode( NULL )
		, m_runWhenReattachedFromPool( true )							{}

	void							OnReattach( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const;

	// Editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeMetanodeOnSpawnDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_INLINED_RO( m_childNode, TXT("Child node (optional)") );
	PROPERTY_EDIT( m_runWhenReattachedFromPool, TXT("Run when reaatached from pool") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Script object that we can attach to scripted OnSpawn metanode
////////////////////////////////////////////////////////////////////////
class IBehTreeOnSpawnEffector : public IBehTreeObjectDefinition
{
	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeOnSpawnEffector );
private:
	mutable CBehTreeInstance*		m_owner;

public:
	IBehTreeOnSpawnEffector()
		: m_owner( NULL )												{}

	void							Run( CBehTreeSpawnContext* context, CBehTreeInstance* owner ) const;

private:
	void							funcGetActor( CScriptStackFrame& stack, void* result );
	void							funcGetObjectFromAIStorage( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IBehTreeOnSpawnEffector );
	PARENT_CLASS( IBehTreeObjectDefinition );
	NATIVE_FUNCTION( "GetActor", funcGetActor );
	NATIVE_FUNCTION( "GetObjectFromAIStorage", funcGetObjectFromAIStorage );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Node that runs custom script code on spawn
////////////////////////////////////////////////////////////////////////
class CBehTreeMetanodeScriptOnSpawnDefinition : public IBehTreeMetanodeOnSpawnDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeMetanodeScriptOnSpawnDefinition, IBehTreeMetanodeOnSpawnDefinition, IBehTreeNodeInstance, ScriptOnSpawn );

protected:
	THandle< IBehTreeOnSpawnEffector >		m_scriptOnSpawn;
	
public:
	////////////////////////////////////////////////////////////////////
	// CBehTreeMetanodeOnSpawnDefinition interface
	void					RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const override;

	////////////////////////////////////////////////////////////////////
	// IBehTreeNodeDefinition interface
	String					GetNodeCaption() const override;
	Bool					IsValid() const override;
};

BEGIN_CLASS_RTTI( CBehTreeMetanodeScriptOnSpawnDefinition );
	PARENT_CLASS( IBehTreeMetanodeOnSpawnDefinition );
	PROPERTY_INLINED( m_scriptOnSpawn, TXT("Script effector that runs of spawn") );
END_CLASS_RTTI();