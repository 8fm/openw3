#pragma once

#include "spawnTreeNode.h"
#include "spawnTreeEntryListElement.h"

class CSpawnTreeEntryList : public ISpawnTreeBranch
{
	DECLARE_ENGINE_CLASS( CSpawnTreeEntryList, ISpawnTreeBranch, 0 );
	typedef TListOperations< TDynArray< ISpawnTreeBaseNode* >, ISpawnTreeBaseNode > ListOperations;

protected:
	TDynArray< ISpawnTreeBaseNode* >		m_entries;

public:
	// logic
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						UpdateLogic( CSpawnTreeInstance& instance ) override;

	// CObject interface
	Bool						OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

	// IEdSpawnTree interface
	Bool						IsSpawnableByDefault() const override;
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanAddChild() const override;
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const override;
	Uint32						GetChildMembersCount() const override;
	void						AddChild( IEdSpawnTreeNode* child ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Bool						UpdateChildrenOrder() override;
	Bool						CanBeHidden() const override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeEntryList );
	PARENT_CLASS( ISpawnTreeBranch );
	PROPERTY( m_entries );
END_CLASS_RTTI();

class ISpawnTreeCompositeNode : public ISpawnTreeBranch
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeCompositeNode, ISpawnTreeBranch );
	typedef TListOperations< TDynArray< ISpawnTreeBaseNode* >, ISpawnTreeBaseNode > ListOperations;
protected:
	static const Uint16 INVALID_CHILD = 0xffff;

	TDynArray< ISpawnTreeBaseNode* >			m_childNodes;
public:
	ISpawnTreeCompositeNode()										{}

	// IEdSpawnTreeNode interface
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanAddChild() const override;
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const override;
	Uint32						GetChildMembersCount() const override;
	void						AddChild( IEdSpawnTreeNode* child ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Bool						UpdateChildrenOrder() override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeCompositeNode );
	PARENT_CLASS( ISpawnTreeBranch );
	PROPERTY( m_childNodes );
END_CLASS_RTTI();

class CSpawnTreeNode : public ISpawnTreeCompositeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeNode, ISpawnTreeCompositeNode, 0 );
	
protected:
	TInstanceVar< Uint16 >					i_currentActiveChild;

public:
	CSpawnTreeNode()												{}

	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						UpdateLogic( CSpawnTreeInstance& instance ) override;


	// IEdSpawnTreeNode interface
	String						GetEditorFriendlyName() const override;

	// Instance buffer interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
};

BEGIN_CLASS_RTTI( CSpawnTreeNode );
	PARENT_CLASS( ISpawnTreeCompositeNode );
END_CLASS_RTTI();

class CSpawnTreeParallelNode : public ISpawnTreeCompositeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeParallelNode, ISpawnTreeCompositeNode, 0 );

public:
	CSpawnTreeParallelNode()										{}

	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						UpdateLogic( CSpawnTreeInstance& instance ) override;


	// IEdSpawnTreeNode interface
	String						GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeParallelNode );
	PARENT_CLASS( ISpawnTreeCompositeNode );
END_CLASS_RTTI();