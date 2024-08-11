#include "build.h"
#include "gameTimePropertyEditor.h"

CGameTimePropertyEditor::CGameTimePropertyEditor( CPropertyItem* propertyItem, Bool dayPeriodOnly )
	: ICustomPropertyEditor( propertyItem )
	, m_dayPeriodOnly( dayPeriodOnly )
{
	m_icon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
}

void CGameTimePropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CGameTimePropertyEditor::OnSpawnGameTimeEditor ), this );
}

void CGameTimePropertyEditor::OnSpawnGameTimeEditor( wxCommandEvent &event )
{
	GameTime time;
	if ( m_propertyItem->Read( &time ) )
	{	
		CEdGameTimeEditor editor( m_propertyItem->GetPage(), &time, m_dayPeriodOnly );
		if ( editor.ShowModal() == wxID_OK )
		{
			m_propertyItem->Write( &time );
		}
		m_propertyItem->GrabPropertyValue();
	}
}

Bool CGameTimePropertyEditor::GrabValue( String& displayValue )
{	
	GameTime time;
	displayValue = TXT("None");
	if ( m_propertyItem->Read( &time ) )
	{
		displayValue = time.ToString();
	}
	return true;	
}