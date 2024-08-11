/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "lootNameSelectionEditor.h"
#include "../../common/game/lootDefinitions.h"
#include "sortNames.h"

void CLootNameSelectionEditor::FillChoices()
{
	ASSERT( GCommonGame->GetDefinitionsManager() != nullptr );
	CLootDefinitions* defs = GCommonGame->GetDefinitionsManager()->GetLootDefinitions();
	if ( defs == nullptr )
	{
		return;
	}

	TDynArray< CName > names;
	defs->GetDefinitionsNames( names );
	::SortNames( names );
	names.PushBack( CName::NONE );

	for ( auto nameI = names.Begin(); nameI != names.End(); ++nameI )
	{
		m_ctrlChoice->AppendString( nameI->AsString().AsChar() );
	}
}