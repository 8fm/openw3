#include "build.h"
#include "encounterPhasesEditor.h"

#include "../../common/game/questEncounterPhaseSetterBlock.h"

#include "sortNames.h"


void CEncounterPhasesEditor::FillChoices()
{
	IEncounterPhaseNamesGetter* phaseGetter = NULL;
	CBasePropItem* pitem = m_propertyItem;
	while ( phaseGetter == NULL && pitem != NULL )
	{
		phaseGetter = dynamic_cast< IEncounterPhaseNamesGetter* >( pitem->GetParentObject( 0 ).AsObject() );
		pitem = pitem->GetParent();
	}

	if ( !phaseGetter )
	{
		return;
	}

	TDynArray< CName > phaseNames;
	phaseGetter->GetEncounterPhaseNames( m_propertyItem->GetProperty(), phaseNames );

	::SortNames( phaseNames );

	for ( Uint32 i = 0, n = phaseNames.Size(); i < n; ++i )
	{
		m_ctrlChoice->AppendString( phaseNames[ i ].AsString().AsChar() );
	}

}
Bool CEncounterPhasesEditor::IsTextEditable() const
{
	return true;
}