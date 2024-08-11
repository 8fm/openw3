#pragma once

#include "encounterTypes.h"
#include "spawnTreeNode.h"
#include "spawnTreeEntryListElement.h"
#include "spawnTreeNodeListOperations.inl"

class CSpawnTreeBaseEntryGenerator : public ISpawnTreeLeafNode, public I2dArrayPropertyOwner
{
	typedef TListOperations< TDynArray< THandle< ISpawnTreeBaseNode > >, ISpawnTreeBaseNode > ListOperations;

	DECLARE_ENGINE_CLASS( CSpawnTreeBaseEntryGenerator, ISpawnTreeLeafNode, 0 );

private:
	TDynArray< THandle< ISpawnTreeBaseNode > >			m_childNodes;

public:

	void		Activate( CSpawnTreeInstance& instance ) override;
	void		Deactivate( CSpawnTreeInstance& instance ) override;
	void		UpdateLogic( CSpawnTreeInstance& instance ) override;
	Color		GetBlockColor() const override;

	Bool		IsSpawnableByDefault() const override;
	String		GetEditorFriendlyName() const override;

	//child nodes handling	
	void				AddChild( IEdSpawnTreeNode* child ) { child->GenerateId(); ListOperations::AddChild( m_childNodes, child ); }
	ISpawnTreeBaseNode*	GetChildMember( Uint32 i ) const override{ return m_childNodes[i].Get(); }
	Uint32				GetChildMembersCount() const override{ return m_childNodes.Size(); }
	void				RemoveChild( IEdSpawnTreeNode* node ) { ListOperations::RemoveChildHandle( m_childNodes, node ); }
	Bool				UpdateChildrenOrder() override;

	void				AddNodeToTree( IEdSpawnTreeNode* nodeToAdd, ISpawnTreeBaseNode* parent );

	Bool IsHiddenByDefault() const override{ return true; }
	
	Bool CanBeHidden() const override {	return true; }

	// Called before object is saved
	//void OnPreSave() override;	

	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
private:
	void ReGenerateEntries();

private:
	void funcAddNodeToTree( CScriptStackFrame& stack, void* result );
	void funcAddInitializerToNode( CScriptStackFrame& stack, void* result );
	void funcRemoveChildren( CScriptStackFrame& stack, void* result );
	void funcSetName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CSpawnTreeBaseEntryGenerator )
	PARENT_CLASS( ISpawnTreeLeafNode )
	PROPERTY( m_childNodes );
	NATIVE_FUNCTION( "RemoveChildren"			, funcRemoveChildren	);
	NATIVE_FUNCTION( "AddNodeToTree"			, funcAddNodeToTree		);
	NATIVE_FUNCTION( "AddInitializerToNode"		, funcAddInitializerToNode );
	NATIVE_FUNCTION( "SetName"					, funcSetName );
END_CLASS_RTTI()

struct SCreatureDefinitionWrapper
{
	DECLARE_RTTI_STRUCT( SCreatureDefinitionWrapper )
	CName	m_creatureDefinition;
};
BEGIN_CLASS_RTTI( SCreatureDefinitionWrapper )
PROPERTY_CUSTOM_EDIT( m_creatureDefinition,	TXT("Creature definition"), TXT("CreatureDefinitionsEditor") );
END_CLASS_RTTI()

struct SWorkCategoriesWrapper
{
	DECLARE_RTTI_STRUCT( SWorkCategoriesWrapper )
	TDynArray< CName >	m_categories;
};
BEGIN_CLASS_RTTI( SWorkCategoriesWrapper )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_categories, TXT("Used action categories"), TXT("2daValueSelection") );
END_CLASS_RTTI()

struct SWorkCategoryWrapper
{
	DECLARE_RTTI_STRUCT( SWorkCategoryWrapper )
	CName	m_category;

};
BEGIN_CLASS_RTTI( SWorkCategoryWrapper )
	PROPERTY_CUSTOM_EDIT( m_category, TXT("Used action categories"), TXT("2daValueSelection") );
END_CLASS_RTTI()



struct SCreatureEntryEntryGeneratorNodeParam
{
	DECLARE_RTTI_STRUCT( SCreatureEntryEntryGeneratorNodeParam )

	SCreatureEntryEntryGeneratorNodeParam()
		: m_qualityMin( 1 )
		, m_qualityMax( 1 )
		, m_group( 1 )
	{	}

	String									m_comment;
	Int32									m_qualityMin;
	Int32									m_qualityMax;
	TagList									m_spawnWayPointTag;
	SCreatureDefinitionWrapper				m_creatureDefinition;
	CName									m_appearanceName;
	CName									m_tagToAssign;
	Int32									m_group;
	THandle< CSpawnTreeBaseEntryGenerator >	m_parent;

	CEntityTemplate* GetEntityTemplate();

};
BEGIN_CLASS_RTTI( SCreatureEntryEntryGeneratorNodeParam )	
	PROPERTY_EDIT( m_comment, TXT("") );
	PROPERTY_EDIT( m_qualityMin, TXT("") );
	PROPERTY_EDIT( m_qualityMax, TXT("") );
	PROPERTY_EDIT( m_spawnWayPointTag, TXT("") );	
	PROPERTY_EDIT( m_creatureDefinition, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_appearanceName, TXT("Name of the appearance to be applied"), TXT("EntityAppearanceSelect") );
	PROPERTY_EDIT( m_tagToAssign, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_group, TXT("Group that the creature belongs to, 0 is critical and will spawn regardless of the global spawn limits"), TXT("ScriptedEnum_EEncounterSpawnGroup") )
END_CLASS_RTTI()