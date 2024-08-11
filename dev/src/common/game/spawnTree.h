#pragma once

#include "edSpawnTreeNode.h"
#include "encounterCreatureDefinition.h"
#include "../core/instanceDataLayout.h"

enum EFindSpawnResult
{
	FSR_FoundOne,
	FSR_FoundNone,
	FSR_NoneDefined,
};

class ISpawnTreeBaseNode;
class CEncounter;

class CSpawnTree : public CResource, public ICreatureDefinitionContainer
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSpawnTree, CResource, "spawntree", "Spawn Tree" );

public:

	CSpawnTree()
		: m_rootNode( NULL )
		, m_dataLayoutComputed( false )														{}

	void						RequestDataLayout()											{ ComputeLayout(); }
	const InstanceDataLayout&	GetDataLayout() const										{ return m_dataLayout; }
	ISpawnTreeBaseNode*			GetRootNode() const											{ return m_rootNode; }
	void						FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent );
	void						CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames );
	

	// IEdSpawnTreeNode interface
	CObject*					AsCObject() override;
	IEdSpawnTreeNode*			GetParentNode() const override;
	Bool						CanBeCopied() const override;
	Bool						CanAddChild() const override;
	void						AddChild( IEdSpawnTreeNode* node ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32						GetNumChildren() const override;
	IEdSpawnTreeNode*			GetChild( Int32 index ) const override;
	virtual Bool				GenerateIdsRecursively() override;
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	void						PreStructureModification() override;
	void						GetContextMenuSpecialOptions( SpecialOptionsList& outOptions ) override;
	void						RunSpecialOption( Int32 option ) override;
	ICreatureDefinitionContainer* AsCreatureDefinitionContainer() override;

	// ICreatureDefinitionContainer interface
	CEncounterCreatureDefinition*	AddCreatureDefinition() override;
	void							RemoveCreatureDefinition( CEncounterCreatureDefinition* def ) override;
	CEncounterCreatureDefinition*	GetCreatureDefinition( CName name ) override;

protected:
	void						ComputeLayout();
	void						ClearLayout();

	ISpawnTreeBaseNode*							InternalGetRootTreeNode() const override;
	TDynArray< CEncounterCreatureDefinition* >& InternalGetCreatureDefinitions() override;			// ICreatureDefinitionContainer interface

	ISpawnTreeBaseNode*								m_rootNode;
	InstanceDataLayout								m_dataLayout;				// NOTICE: might be mutable because of lazy computation policy
	Bool											m_dataLayoutComputed;
	TDynArray< CEncounterCreatureDefinition* >		m_creatureDefinition;
};

BEGIN_CLASS_RTTI( CSpawnTree );
	PARENT_CLASS( CResource );
	PROPERTY( m_rootNode );
	PROPERTY( m_creatureDefinition );
	PROPERTY_EDIT( m_spawnTreeType, TXT("Edit mode for spawn tree") );
END_CLASS_RTTI();



class CSpawnTreeInstance
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

protected:
	InstanceBuffer*					m_instanceBuffer;
	CEncounter*						m_encounter;
public:
	CSpawnTreeInstance();
	~CSpawnTreeInstance();

	void							Bind( const InstanceDataLayout& dataLayout, CEncounter* encounter );
	void							Unbind();
	Bool							IsBinded() const										{ return m_instanceBuffer != NULL; }

	CEncounter*						GetEncounter() const									{ return m_encounter; }

	template< class T >
	T& operator[]( const TInstanceVar<T>& var )
	{
		ASSERT( m_instanceBuffer );
		return m_instanceBuffer->operator []( var );
	}

	template< class T >
	const T& operator[]( const TInstanceVar<T>& var ) const
	{
		ASSERT( m_instanceBuffer );
		return m_instanceBuffer->operator []( var );
	}

};