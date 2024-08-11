#include "build.h"
#include "creatureDefinitionsEditor.h"
#include "../../common/game/encounter.h"
#include "sortNames.h"

//#pragma optimize ("",off)
void CCreatureDefinitionsEditor::FillChoices()
{
	ICreatureDefinitionContainer* defContainer = NULL;
	STypedObject propParent = m_propertyItem->GetParentObject( 0 );
	IEdSpawnTreeNode* node = dynamic_cast< IEdSpawnTreeNode* >( propParent.AsObject() );

	CBasePropItem* propItem = m_propertyItem;
	while( node == nullptr && propItem != nullptr )
	{
		propItem = propItem->GetParent();
		propParent =  propItem->GetParentObject( 0 );		
		node = dynamic_cast< IEdSpawnTreeNode* >( propParent.AsObject() );
	}

	while ( node && !defContainer )
	{
		defContainer = node->AsCreatureDefinitionContainer();
		node = node->GetParentNode();
	}

	if ( !defContainer )
	{
		return;
	}

	CEncounterCreatureDefinition::CompiledCreatureList creatureDefs;
	defContainer->CompileCreatureDefinitions( creatureDefs );

	TDynArray< CName > names;
	for ( auto cdI = creatureDefs.Begin(); cdI != creatureDefs.End(); ++cdI )	
	{
		names.PushBack( (*cdI)->m_definitionName );
	}

	::SortNames( names );

	for ( auto nameI = names.Begin(); nameI != names.End(); ++nameI )
	{
		m_ctrlChoice->AppendString( nameI->AsString().AsChar() );
	}
}
//#pragma optimize ("",on)
