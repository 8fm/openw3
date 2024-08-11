#pragma once

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/


#include "behTreeMetanode.h"
#include "behTreeVars.h"
#include "aiParameters.h"



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTemplateDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeTemplateDefinition : public IBehTreeMetanodeDefinition 
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeTemplateDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, AITemplate );
protected:
	static const String EMPTY_NODE_NAME;
	static const String CAPTION_PREFIX;

	THandle< CBehTree >				m_res;
	THandle< IAIParameters >		m_aiParameters;

public:
	CBehTreeNodeTemplateDefinition()									{}

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;

	String							GetNodeCaption() const override;
	Bool							IsValid() const override;

	const CBehTree*					GetResource() const					{ return m_res.Get(); };

	void							OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeTemplateDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_EDIT( m_res, TXT("Behavior tree") );
	PROPERTY_INLINED( m_aiParameters, TXT("AI paramters") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSubtreeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSubtreeDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSubtreeDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, Subtree );
protected:
	CName					m_treeName;
	THandle< CAITree >		m_data;
public:
	CBehTreeNodeSubtreeDefinition()
		: m_treeName( CName::NONE )
		, m_data( NULL )												{}


	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
	
	void							OnSerialize( IFile& file ) override;

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSubtreeDefinition );
PARENT_CLASS(  IBehTreeMetanodeDefinition);
PROPERTY_EDIT( m_treeName, TXT("Subtree name") );
PROPERTY_INLINED( m_data, TXT("Ai resource") );
BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSubtreeListDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSubtreeListDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSubtreeListDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, SubtreeList );
protected:
	CName					m_listName;
public:
	CBehTreeNodeSubtreeListDefinition()
		: m_listName( CName::NONE )
	{}
	void							SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const override;
	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
	Uint32							OnSpawnList( IBehTreeNodeInstance* const* nodeList, Uint32 nodeCount, CBehTreeSpawnContext& context ) const override;

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSubtreeListDefinition );
PARENT_CLASS(  IBehTreeMetanodeDefinition);
PROPERTY_EDIT( m_listName, TXT("Subtree list name") );
BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseConditionalTreeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeBaseConditionalTreeDefinition : public IBehTreeMetanodeDefinition
{ 
	DECLARE_BEHTREE_NODE( CBehTreeNodeBaseConditionalTreeDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, BaseConditionalTree );
protected:
	IBehTreeNodeDefinition*		m_child;	
	Bool						m_invert;

	virtual Bool CheckCondition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const { return true; }

public:
	CBehTreeNodeBaseConditionalTreeDefinition()
		: m_child( NULL )		
		, m_invert( false )												{}

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;

	// editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif // NO_EDITOR_GRAPH_SUPPORT
	
};

BEGIN_CLASS_RTTI( CBehTreeNodeBaseConditionalTreeDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_RO( m_child, TXT("Child") );
	PROPERTY_EDIT( m_invert, TXT("Invert conditions") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalTreeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionalTreeDefinition : public CBehTreeNodeBaseConditionalTreeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalTreeDefinition, CBehTreeNodeBaseConditionalTreeDefinition, IBehTreeNodeInstance, ConditionalTree );
protected:	
	CBehTreeValBool				m_val;	

	Bool CheckCondition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
public:
	CBehTreeNodeConditionalTreeDefinition()
		: m_val( true )
		{}	

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalTreeDefinition );
	PARENT_CLASS( CBehTreeNodeBaseConditionalTreeDefinition );
	PROPERTY_EDIT( m_val, TXT("Conditional value") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalFlagTreeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionalFlagTreeDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalFlagTreeDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, ConditionalFlagTree );
protected:
	IBehTreeNodeDefinition*		m_child;
	CBehTreeValInt				m_val;
	Int32						m_flag;
public:
	CBehTreeNodeConditionalFlagTreeDefinition()
		: m_child( NULL )
		, m_val( 0 )
		, m_flag( 0 )													{}

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
	
	// editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif // NO_EDITOR_GRAPH_SUPPORT

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalFlagTreeDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_RO( m_child, TXT("Child") );
	PROPERTY_EDIT( m_val, TXT("Conditional value") );
	PROPERTY_EDIT( m_flag, TXT("Flag") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalChooseBranchDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionalChooseBranchDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalChooseBranchDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, ConditionalChooseBranch );
protected:
	IBehTreeNodeDefinition*		m_child1;
	IBehTreeNodeDefinition*		m_child2;
	CBehTreeValBool				m_val;
public:
	CBehTreeNodeConditionalChooseBranchDefinition()
		: m_child1( NULL )
		, m_child2( NULL )
		, m_val( true )													{}
	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;

	// editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
	Bool							CorrectChildrenOrder();
#endif // NO_EDITOR_GRAPH_SUPPORT

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalChooseBranchDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_RO( m_child1, TXT("Child 'true'") );
	PROPERTY_RO( m_child2, TXT("Child 'false'") );
	PROPERTY_EDIT( m_val, TXT("Conditional value") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// IBehTreeNodeConditionalBaseNodeDefinition 
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeConditionalBaseNodeDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionalBaseNodeDefinition , IBehTreeMetanodeDefinition, IBehTreeNodeInstance, ConditionalBaseDecorator );
protected:
	IBehTreeNodeDefinition*		m_child;
	Uint32						m_childNodeToDisableCount;
	Bool						m_invertCondition;

public:
	IBehTreeNodeConditionalBaseNodeDefinition()
		: m_child( NULL )
		, m_childNodeToDisableCount( 1 )
		, m_invertCondition( false )						{}

	// IBehTreeMetanodeDefinition interface
	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;

	// editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif // NO_EDITOR_GRAPH_SUPPORT

	String							GetNodeCaption() const override;

protected:
	virtual Bool					CheckCondition( CBehTreeSpawnContext& context ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeConditionalBaseNodeDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_EDIT( m_childNodeToDisableCount, TXT(" if val is false that number of children will be disabled down the tree ") );
	PROPERTY_RO( m_child, TXT("Child") );
	PROPERTY_EDIT( m_invertCondition, TXT("Invert condition") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalBoolNodeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionalNodeDefinition : public IBehTreeNodeConditionalBaseNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalNodeDefinition, IBehTreeNodeConditionalBaseNodeDefinition, IBehTreeNodeInstance, ConditionalBoolDecorator );
protected:
	CBehTreeValBool		m_val;
public:
	CBehTreeNodeConditionalNodeDefinition()
		: m_val( true )	 {}

	String	GetNodeCaption() const override;

protected:
	Bool	CheckCondition( CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalNodeDefinition );
	PARENT_CLASS( IBehTreeNodeConditionalBaseNodeDefinition );
	PROPERTY_EDIT( m_val, TXT("Conditional value") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalNameNodeDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionalNameNodeDefinition : public IBehTreeNodeConditionalBaseNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalNameNodeDefinition, IBehTreeNodeConditionalBaseNodeDefinition, IBehTreeNodeInstance, ConditionalNameDecorator );
protected:
	CBehTreeValCName			m_val;
	CBehTreeValCName			m_nameToCompare;
public:
	String	GetNodeCaption() const override;

protected:
	Bool	CheckCondition( CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalNameNodeDefinition );
	PARENT_CLASS( IBehTreeNodeConditionalBaseNodeDefinition );
	PROPERTY_EDIT( m_val, TXT("Conditional value") );
	PROPERTY_EDIT( m_nameToCompare, TXT("String to compare") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();
