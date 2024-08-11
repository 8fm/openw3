#include "build.h"
#include "2daTagListBoxEditor.h"

#include "../../common/core/depot.h"

CEd2daTagListBoxEditor::CEd2daTagListBoxEditor( CPropertyItem* item, String& filename, String& valueColumn )
	: CListBoxEditor( item ), m_2daFilename( filename ), m_2daValueColumn( valueColumn )
{

}

CEd2daTagListBoxEditor::~CEd2daTagListBoxEditor(void)
{
}

Bool CEd2daTagListBoxEditor::GrabValue( String& displayValue )
{
	TagList descriptors;
	m_propertyItem->Read( &descriptors );
	displayValue = descriptors.ToString();
	return true;
}

wxArrayString CEd2daTagListBoxEditor::GetListElements()
{
	wxArrayString elements;

	C2dArray* definitions2da = Cast< C2dArray >( 
		GDepot->LoadResource( m_2daFilename ) );

	if ( definitions2da != NULL && definitions2da->Empty() == false )
	{
		Uint32 numberOfColumns = 0;
		Uint32 numberOfRows = 0;
		definitions2da->GetSize( numberOfColumns, numberOfRows );


		Uint32 definitionColumn = -1;
		for ( Uint32 i = 0; i < numberOfColumns; ++i )
		{
			if ( definitions2da->GetHeader( i ) == m_2daValueColumn )
			{
				definitionColumn = i;
				break;
			}
		}

		for ( Uint32 i = 0; i < numberOfRows; ++i )
		{
			String definition = definitions2da->GetValue( definitionColumn, i );
			elements.Add( definition.AsChar() );
		}
	}

	return elements;
}

void CEd2daTagListBoxEditor::SelectPropertyElements()
{
	TagList descriptors;
	m_propertyItem->Read( &descriptors );

	for ( Uint32 i = 0; i < m_listBoxCtrl->GetCount(); ++i )
	{
		const Char* item = m_listBoxCtrl->GetString( i ).wc_str();
		if ( item )
		{
			const TDynArray< CName >& tags = descriptors.GetTags();
			for ( Uint32 j=0; j<tags.Size(); j++ )
			{
				if ( tags[j] == item )
				{
					m_listBoxCtrl->Check( i );
					break;
				}
			}
		}
	}
}

void CEd2daTagListBoxEditor::SelectElement( wxString element )
{
	String tag = element;
	ToggleTag( tag, true );
}

void CEd2daTagListBoxEditor::DeselectElement( wxString element )
{
	String tag = element;
	ToggleTag( tag, false );
}

void CEd2daTagListBoxEditor::ToggleTag( String& element, Bool selected )
{
	TagList descriptors;
	m_propertyItem->Read( &descriptors );


	if ( selected == true )
	{
		CName tag( element );
		descriptors.AddTag( tag );
	}
	else
	{
		CName tag( element );
		descriptors.SubtractTag( tag );
	}

	m_propertyItem->Write( &descriptors );
}