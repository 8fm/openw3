#include "build.h"
#include "traitEdCustomTypes.h"
#include "gridCustomCellEditors.h"
#include "gridCellEditors.h"


wxGridCellRenderer* CGridTraitNameColumnDesc::GetCellRenderer() const
{
	return new CGridCellDefaultRenderer;
}

wxGridCellEditor* CGridTraitNameColumnDesc::GetCellEditor() const
{
	wxArrayString choices;
	for ( TDynArray< STraitTableEntry >::iterator it = m_traits.Begin(); it != m_traits.End(); ++it )
	{
		STraitTableEntry& tableEntry = *it;

		choices.Add( tableEntry.m_traitName.AsChar() );
	}

    return new CGridCellChoiceEditor( choices );
}

wxString CGridAbilityCellDesc::ToString( void *data ) const 
{
	CTraitAbilityWrapper** wrapper = static_cast< CTraitAbilityWrapper ** >( data );

	if  ( ( *wrapper )->m_ability == NULL )
	{
		return wxString( TXT("Empty") );
	}
	else
	{
		return ( *wrapper )->m_ability->GetClass()->GetName().AsChar();
	}
}

Bool CGridAbilityCellDesc::FromString( void *data, const wxString &text ) const
{
	return false;
}


wxString CGridRequirementCellDesc::ToString( void *data ) const 
{
	CTraitRequirementWrapper** wrapper = static_cast< CTraitRequirementWrapper ** >( data );

	if ( ( *wrapper )->m_requirements.Size() == 0 )
	{
		return wxString( TXT("Empty") );
	}
	else
	{
		String returnedVal;
		UINT32 reqSize = ( *wrapper )->m_requirements.Size();
		for ( Uint32 i = 0; i < reqSize; i++ )
		{
			IRequirement* req = ( *wrapper )->m_requirements[i];
			if ( req == NULL )
			{
				continue;
			}
			CClass* classInfo = req->GetClass();
			TDynArray < CProperty* > properties;
			classInfo->GetProperties( properties );

			String nameStr = classInfo->GetName().AsString();
			
			String propsStr;
			classInfo->ToString( req, propsStr );

			String reqStr = String::Printf( TXT("%s(%s)"), nameStr.AsChar(), propsStr.AsChar() );

			returnedVal += reqStr;

			if ( i < reqSize - 1 )
			{
				returnedVal.Append( ',' );
			}
		}
		return returnedVal.AsChar();
	}
}

Bool CGridRequirementCellDesc::FromString( void *data, const wxString &text ) const
{
	return false;
}
