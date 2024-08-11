#pragma once

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/


#include "behTreeDecorator.h"
#include "behTreeTask.h"

class CBehTreeNodeScriptTerminalInstance;
class CBehTreeNodeScriptDecoratorInstance;




////////////////////////////////////////////////////////////////////////
// BehTree terminal scripted node
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeScriptTerminalDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeScriptTerminalDefinition, IBehTreeNodeDefinition, CBehTreeNodeScriptTerminalInstance, ScriptTerminal );
protected:
	static const String NULL_TASK_NODE_NAME;
	static const String CAPTION_PREFIX;

	THandle< IBehTreeTaskDefinition >	m_taskOrigin;
	mutable Uint16						m_taskFlags;					// lazy computation
	Bool								m_skipIfActive;
	Bool								m_runMainOnActivation;
public:
	CBehTreeNodeScriptTerminalDefinition()
		: m_taskFlags( IBehTreeTask::FLAGS_UNINITIALIZED )
		, m_skipIfActive( false )
		, m_runMainOnActivation( true )									{}

	//! Get node name
	String GetNodeCaption() const override;
	Bool IsValid() const override;
	eEditorNodeType	GetEditorNodeType() const override;

	IBehTreeTaskDefinition* GetTask() const override;
	Bool SetTask( IBehTreeTaskDefinition* task ) override;
	Bool IsSupportingTasks() const override;

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeScriptTerminalDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
	PROPERTY_INLINED_RO( m_taskOrigin, TXT("Task") );
	PROPERTY_EDIT( m_skipIfActive, TXT("Skip execution if task already active") );
	PROPERTY_EDIT( m_runMainOnActivation, TXT("Skip execution if task already active") );
END_CLASS_RTTI();

class CBehTreeNodeScriptTerminalInstance : public IBehTreeNodeInstance, public CBehTreeScriptedInstance
{
	typedef IBehTreeNodeInstance Super;
protected:
	Bool	m_skipIfActive;

	CBehTreeInstance* ScriptGetOwner() const override;
	IBehTreeNodeInstance* ScriptGetNode() override;
public:
	typedef CBehTreeNodeScriptTerminalDefinition Definition;

	CBehTreeNodeScriptTerminalInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~CBehTreeNodeScriptTerminalInstance();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	void Complete( eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	void OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context ) override;
	void OnDestruction() override;
};


////////////////////////////////////////////////////////////////////////
// BehTree scripted decorator
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeScriptDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeScriptDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeScriptDecoratorInstance, ScriptDecorator );
protected:
	static const String NULL_TASK_NODE_NAME;
	static const String CAPTION_PREFIX;

	THandle< IBehTreeTaskDefinition >	m_taskOrigin;
	Bool								m_forwardAvailability;
	Bool								m_forwardTestIfNotAvailable;
	Bool								m_invertAvailability;
	Bool								m_skipIfActive;
	Bool								m_runMainOnActivation;
	mutable Uint16						m_taskFlags;					// lazy computation

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeScriptDecoratorDefinition()
		: m_forwardAvailability( false )
		, m_invertAvailability( false )
		, m_skipIfActive( false )
		, m_runMainOnActivation( true )
		, m_taskFlags( IBehTreeTask::FLAGS_UNINITIALIZED )				{}

	//! Get node name
	String GetNodeCaption() const override;
	Bool IsValid() const override;
	eEditorNodeType	GetEditorNodeType() const override;

	IBehTreeTaskDefinition* GetTask() const override;
	Bool SetTask( IBehTreeTaskDefinition* task ) override;
	Bool IsSupportingTasks() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeScriptDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_forwardAvailability, TXT("Forward availability computation") );
	PROPERTY_EDIT( m_forwardTestIfNotAvailable, TXT("Forward test to children if node is not-available") );
	PROPERTY_EDIT( m_invertAvailability, TXT("Invert availability test") );
	PROPERTY_EDIT( m_skipIfActive, TXT("Do not re-execute child if it is already running"));
	PROPERTY_EDIT( m_runMainOnActivation, TXT("Skip execution if task already active") );
	PROPERTY_INLINED_RO( m_taskOrigin, TXT("Task") );
END_CLASS_RTTI();

class CBehTreeNodeScriptDecoratorInstance : public IBehTreeNodeDecoratorInstance, public CBehTreeScriptedInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Bool								m_forwardAvailability;
	Bool								m_forwardTestIfNotAvailable;
	Bool								m_invertAvailability;
	Bool								m_skipIfActive;

	CBehTreeInstance* ScriptGetOwner() const override;
	IBehTreeNodeInstance* ScriptGetNode() override;
public:
	typedef CBehTreeNodeScriptDecoratorDefinition Definition;

	CBehTreeNodeScriptDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~CBehTreeNodeScriptDecoratorInstance();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	void Complete( eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	void OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context ) override;
	void OnDestruction() override;
};

////////////////////////////////////////////////////////////////////////
// Same as script decorator - but limits supported script tasks list.
// Its just a test, it might be we will never use it (and trash it
// finally), it might be its handy for designers.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeScriptConditionalDecoratorDefinition : public CBehTreeNodeScriptDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeScriptConditionalDecoratorDefinition, CBehTreeNodeScriptDecoratorDefinition, CBehTreeNodeScriptDecoratorInstance, ScriptConditionalDecorator );
public:
	CBehTreeNodeScriptConditionalDecoratorDefinition() {}

	Bool IsSupportingTaskClass( const CClass* classId ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeScriptConditionalDecoratorDefinition );
	PARENT_CLASS( CBehTreeNodeScriptDecoratorDefinition );
END_CLASS_RTTI();
