#include "build.h"
#include "../../common/game/characterResource.h"
#include "../../common/game/character.h"
#include "characterEdCustomTypes.h"
#include "characterResourceContainer.h"
#include "gridCustomCellEditors.h"
#include "gridCellEditors.h"

const wxColour CCharacterCellAttrCustomizer::INHERITED_PROPERTY_COLOUR = wxColour( 200, 100, 100 );

void CCharacterCellAttrCustomizer::CustomizeCellAttr( wxGridCellAttr* attr, const IRTTIType* gridType, const void *gridData, const IRTTIType* cellRttiType, const void *cellData, const CProperty* property ) const
{
	RED_FATAL_ASSERT( gridType->GetType() == RT_Class && ( ( CClass * )gridType )->GetName() == CName( TXT("CCharacter") ), "Grid editor should use CCharacter data" );

	CCharacter* character = ( CCharacter* ) gridData;

	if ( character->IsInherited( property ) )
	{
		attr->SetBackgroundColour( INHERITED_PROPERTY_COLOUR );
	}
};

wxString CGridCharacterNameCellDesc::ToString( void *data ) const 
{
	CCharacter** character = static_cast< CCharacter ** >( data );

	if  ( *character == NULL )
	{
		return wxString( TXT("None") );
	}
	else
	{
		return ( *character )->GetName().AsChar();
	}
}

Bool CGridCharacterNameCellDesc::FromString( void *data, const wxString &text ) const
{
	return false;
}
