#pragma once

#include "edSpawnTreeNode.h"
#include "spawnTree.h"

class CSimpleBufferWriter;
class CSimpleBufferReader;

class CSpawnTreeInitializationContext;

//////////////////////////////////////////////////////////////////////////
// Base class for any encounter member
class ISpawnTreeBaseNode : public CObject, public IEdSpawnTreeNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeBaseNode, CObject );
protected:
	CName									m_nodeName;

	TInstanceVar< Bool >					i_enabled;
	TInstanceVar< Bool >					i_active;

	virtual Bool				TestConditions( CSpawnTreeInstance& instance ) const;

public:
	template < class TList, class TChildType >
	struct TListOperations
	{
		static void GetRootClassForChildren( TDynArray< CClass* >& rootClasses );
		static void AddChild( TList& list, IEdSpawnTreeNode* child, Int32 index = -1 );
		static void MoveChild( TList& list, Uint32 indexFrom, Uint32 indexTo );
		static void RemoveChild( TList& list, IEdSpawnTreeNode* child );
		static void RemoveChildHandle( TList& list, IEdSpawnTreeNode* child );
		static Bool UpdateChildrenOrder( TList& list );
	};

public:
	ISpawnTreeBaseNode()
	{
	}

	RED_INLINE CName	GetName() const												{ return m_nodeName; }
	void				SetEnabled( CSpawnTreeInstance& instance, Bool enabled );
	RED_INLINE Bool		IsEnabled( const CSpawnTreeInstance& instance  ) const		{ return instance[ i_enabled ]; }
	RED_INLINE Bool		IsActive( const CSpawnTreeInstance& instance  ) const		{ return instance[ i_active ]; }

	
	virtual void				Activate( CSpawnTreeInstance& instance );
	virtual void				Deactivate( CSpawnTreeInstance& instance );
	virtual void				UpdateLogic( CSpawnTreeInstance& instance );

	virtual Bool				SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName );
	virtual void				GetSpawnPhases( TDynArray< CName >& outPhaseNames );
	virtual void				EnableMember( CSpawnTreeInstance& instance, CName& name, Bool enable );
	virtual void				OnFullRespawn( CSpawnTreeInstance& instance ) const;
	virtual void				CollectSpawnTags( TagList& tagList );
	virtual void				CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames );
	virtual void				FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent );
	virtual void				CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& creatureList );
	virtual void				UpdateEntriesSetup( CSpawnTreeInstance& instance ) const;

	// gets child spawn tree node
	virtual ISpawnTreeBaseNode*	GetChildMember( Uint32 i ) const;
	virtual Uint32				GetChildMembersCount() const;

	// gets transient spawn tree node - so possibly a node from a different spawn tree
	virtual ISpawnTreeBaseNode*	GetTransientChildMember( Uint32 i ) const;
	virtual Uint32				GetTransientChildMembersCount() const;

	virtual Bool				IsNodeStateSaving( CSpawnTreeInstance& instance ) const;
	virtual void				SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const;
	virtual Bool				LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const;
	virtual void				OnEvent( CSpawnTreeInstance& instance, CName eventName ) const;

	// Instance buffer interface
	virtual void				OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void				OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context );
	virtual void				OnDeinitData( CSpawnTreeInstance& instance );

	// CObject interface
	void						OnPostLoad() override;
	Bool						OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	// Editor interface
	virtual Bool				IsSpawnableByDefault() const;
	virtual Bool				IsUtilityNode() const;

	// IEdSpawnTreeNode interface
	CObject*					AsCObject() override;
	IEdSpawnTreeNode*			GetParentNode() const override;
	Bool						CanAddChild() const override;
	void						AddChild( IEdSpawnTreeNode* node ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32						GetNumChildren() const override;
	IEdSpawnTreeNode*			GetChild( Int32 index ) const override;
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool						IsHiddenByDefault() const override;
	Bool						CanBeHidden() const override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
	EDebugState					GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const override;
	void						GatherBudgetingStats( CSpawnTreeInstance& instance, SBudgetingStats& stats ) {}
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeBaseNode );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_nodeName, TXT("Name tag to be used for identification, name based enabling etc.") );
	PROPERTY_RO( m_id, TXT("Randomly generated node id") );
  #ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_graphPosX );
	PROPERTY_NOT_COOKED( m_graphPosY );
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("Comment") );  
#endif 
	
END_CLASS_RTTI();

class ISpawnTreeBranch : public ISpawnTreeBaseNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeBranch, ISpawnTreeBaseNode );
public:
	String						GetEditorFriendlyName() const override;
};
BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeBranch );
	PARENT_CLASS( ISpawnTreeBaseNode );
END_CLASS_RTTI();


class ISpawnTreeDecorator : public ISpawnTreeBranch
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeDecorator, ISpawnTreeBranch );
protected:
	ISpawnTreeBaseNode*			m_childNode;

public:
	ISpawnTreeDecorator()
		: m_childNode( NULL )													{}

	void						UpdateLogic( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;

	// Editor interface
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanAddChild() const override;
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const override;
	Uint32						GetChildMembersCount() const override;
	void						AddChild( IEdSpawnTreeNode* node ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
};


BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeDecorator );
	PARENT_CLASS( ISpawnTreeBranch );
	PROPERTY_RO( m_childNode, TXT("Child node") );
END_CLASS_RTTI();

