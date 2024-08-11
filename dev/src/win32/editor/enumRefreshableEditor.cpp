#include "build.h"
#include "enumRefreshableEditor.h"

CEdEnumRefreshableEditor::CEdEnumRefreshableEditor( CPropertyItem* item ) 
	: ISelectionEditor( item )
{

}

Bool CEdEnumRefreshableEditor::SaveValue()
{
	//GetPropertyItem()->GetPage()->RefreshValues();
	RunLaterOnce( [ this ](){ m_propertyItem->GetPage()->RefreshValues(); } );

	const Int32 selection = m_ctrlChoice->GetSelection();
	const String value = selection != -1 ? m_ctrlChoice->GetString( selection ).wc_str() : m_ctrlChoice->GetTextCtrl()->GetValue().wc_str();
	
	CPropertyDataBuffer buffer( m_propertyItem->GetPropertyType() );
	if ( m_propertyItem->GetPropertyType()->FromString( buffer.Data(), value ) )
	{
		m_propertyItem->Write( buffer.Data() );

		return true;
	}

	return false;
}

Bool CEdEnumRefreshableEditor::GrabValue( String& displayValue )
{
	CPropertyDataBuffer buffer( m_propertyItem->GetPropertyType() );
	if ( !m_propertyItem->Read( buffer.Data(), 0 ) && m_propertyItem->GetPropertyType()->ToString( buffer.Data(), displayValue ) )
	{
		return true;
	}
	return false;
}

void CEdEnumRefreshableEditor::FillChoices()
{
	const CEnum* e = static_cast< const CEnum* >( m_propertyItem->GetPropertyType() );
	if ( e )
	{
		const TDynArray< CName >& options = e->GetOptions();

		const Uint32 size = options.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_ctrlChoice->Append( options[ i ].AsChar() );
		}
	}
}
