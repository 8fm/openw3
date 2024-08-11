/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "traitListSelection.h"
#include "../../games/r6/traitData.h"
#include "../../games/r6/r6GameResource.h"
#include "editorExternalResources.h"
#include "../../common/core/gatheredResource.h"


CTraitDataListSelection::CTraitDataListSelection( CPropertyItem* item )
	: ISelectionEditor( item )
	, m_traitData(NULL)
{
	RED_FATAL_ASSERT( 0, "Not finished" );
	//CR6Game* r6Game = static_cast< CR6Game* >( GGame );
	//CR6GameResource* gameResource = Cast< CR6GameResource >( GGame->GetGameResource() );
	//if (gameResource && gameResource->GetTraitDataPath())
	//{
	//	CGatheredResource resTraitData( gameResource->GetTraitDataPath().AsChar(), RGF_Startup );
	//	CTraitDataListSelection::m_traitData = resTraitData.LoadAndGet< CTraitData >();
	//}
}

CTraitListSelection::CTraitListSelection( CPropertyItem* item )
	: CTraitDataListSelection( item )
{
}

void CTraitListSelection::FillChoices()
{
	if ( !m_traitData )
	{
		return;
	}

	const TDynArray< STraitTableEntry >& traits = m_traitData->GetTraitTable();

	for ( TDynArray< STraitTableEntry >::const_iterator it = traits.Begin(); it != traits.End(); ++it )
	{
		const STraitTableEntry& tableEntry = *it;

		m_ctrlChoice->AppendString( tableEntry.m_traitName.AsChar() );
	}
}

CSkillListSelection::CSkillListSelection( CPropertyItem* item )
	: CTraitDataListSelection( item )
{
}

void CSkillListSelection::FillChoices()
{
	if ( !m_traitData )
	{
		return;
	}

	const TDynArray< SSkillTableEntry >& skills = m_traitData->GetSkillTable();

	for ( TDynArray< SSkillTableEntry >::const_iterator it = skills.Begin(); it != skills.End(); ++it )
	{
		const SSkillTableEntry& tableEntry = *it;

		m_ctrlChoice->AppendString( tableEntry.m_skillName.AsChar() );
	}
}
