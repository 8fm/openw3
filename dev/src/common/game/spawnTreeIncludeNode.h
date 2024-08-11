#pragma once

#include "spawnTreeNode.h"

class CSpawnTreeIncludeTreeNode : public ISpawnTreeBranch
{
	DECLARE_ENGINE_CLASS( CSpawnTreeIncludeTreeNode, ISpawnTreeBranch, 0 );
protected:
	THandle< CSpawnTree >					m_spawnTree;						// to make it const we would have to mark CSpawnTree attributes mutable (because of lazy computations policy)

	TInstanceVar< TGenericPtr >				i_spawnTreeInstanceData;			// CSpawnTreeInstance*

	RED_INLINE CSpawnTreeInstance*		GetPrivateInstance( const CSpawnTreeInstance& instance ) const		{ return reinterpret_cast< CSpawnTreeInstance* >( instance[ i_spawnTreeInstanceData ] ); }
public:
	CSpawnTreeIncludeTreeNode();
	~CSpawnTreeIncludeTreeNode();

	// Base interface
	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						UpdateLogic( CSpawnTreeInstance& instance ) override;

	Bool						SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName ) override;
	void						GetSpawnPhases( TDynArray< CName >& outPhaseNames ) override;
	void						EnableMember( CSpawnTreeInstance& instance, CName& name, Bool enable ) override;
	void						OnFullRespawn( CSpawnTreeInstance& instance ) const override;
	void						CollectSpawnTags( TagList& tagList ) override;
	void						CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames );
	void						FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent ) override;
	void						CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& creatureList ) override;
	void						UpdateEntriesSetup( CSpawnTreeInstance& instance ) const override;
	

	// Instance buffer interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
	void						OnDeinitData( CSpawnTreeInstance& instance ) override;

	// Editor interface
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const override;
	Uint32						GetChildMembersCount() const override;
	ISpawnTreeBaseNode*			GetTransientChildMember( Uint32 i ) const;
	Uint32						GetTransientChildMembersCount() const;
	Bool						IsUtilityNode() const override;
								
	void						GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats ){};
	
	// IEdSpawnTreeNode interface
	Bool						CanAddChild() const override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32						GetNumChildren() const override;
	IEdSpawnTreeNode*			GetChild( Int32 index ) const override;
	Bool						IsHiddenByDefault() const override;
	Bool						CanBeHidden() const override;
	Color						GetBlockColor() const override;
	String						GetBlockCaption() const override;
	String						GetEditorFriendlyName() const override;
	Bool						HoldsInstanceBuffer() const override;
	CSpawnTreeInstance*			GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer = NULL ) override;

	void						OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_CLASS_RTTI( CSpawnTreeIncludeTreeNode );
	PARENT_CLASS( ISpawnTreeBranch );
	PROPERTY_EDIT( m_spawnTree, TXT("Spawn tree resource attached") );
END_CLASS_RTTI();