/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#if 0
#include "traitEmbeddedCustomEditor.h"
#include "traitEditor.h"
#include "../../games/r6/traitData.h"

#include "../../common/core/depot.h"

CEdTraitEmbeddedCustomEditor::CEdTraitEmbeddedCustomEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_embeddedResourceEditor( NULL )
{
	m_iconImport = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
	m_iconEdit = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	m_iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
}

CEdTraitEmbeddedCustomEditor::~CEdTraitEmbeddedCustomEditor()
{

}

Bool CEdTraitEmbeddedCustomEditor::GrabValue( String& displayValue )
{
	CObject* propertyValuePointer = NULL;
	m_propertyItem->Read( &propertyValuePointer );
	
	if ( propertyValuePointer == NULL )
	{
		displayValue = TXT( "NULL" );
	}
	else
	{
		displayValue = String::Printf( TXT( "Embedded %s" ), m_propertyItem->GetPropertyType()->GetName().AsChar() );
	}
	
	return true;
}

Bool CEdTraitEmbeddedCustomEditor::SaveValue()
{
	return ICustomPropertyEditor::SaveValue();
}

void CEdTraitEmbeddedCustomEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconClear, wxCommandEventHandler( CEdTraitEmbeddedCustomEditor::OnResourceClear ), this );
	m_propertyItem->AddButton( m_iconEdit, wxCommandEventHandler( CEdTraitEmbeddedCustomEditor::OnResourceEdit ), this );
	m_propertyItem->AddButton( m_iconImport, wxCommandEventHandler( CEdTraitEmbeddedCustomEditor::OnResourceImport ), this );
}

void CEdTraitEmbeddedCustomEditor::OnResourceImport( wxCommandEvent& event )
{
	String activeResourcePath;
	GetActiveResource( activeResourcePath );
	
	CTraitData* traitResource = Cast< CTraitData >( GDepot->LoadResource( activeResourcePath ) );
	if ( traitResource != NULL )
	{
		CObject* rootObject = m_propertyItem->GetRootObject( 0 ).AsObject();
		CTraitData* traitDataCopy = Cast< CTraitData >( traitResource->Clone( rootObject ) );

		m_propertyItem->Write( &traitDataCopy );
		m_propertyItem->GrabPropertyValue();
	}
}

void CEdTraitEmbeddedCustomEditor::OnResourceEdit( wxCommandEvent& event )
{
	CObject* propertyValuePointer = NULL;
	m_propertyItem->Read( &propertyValuePointer );

	CTraitData* traitData = static_cast< CTraitData* >( propertyValuePointer );
	if ( traitData == NULL )
	{
		return;
	}
	
	m_embeddedResourceEditor = new CEdTraitEditor( m_propertyItem->GetPage()->GetParent(), traitData );
	m_embeddedResourceEditor->Show();
}

void CEdTraitEmbeddedCustomEditor::OnResourceClear( wxCommandEvent& event )
{
	CTraitData* nullPointer = NULL;
	m_propertyItem->Write( &nullPointer );
	m_propertyItem->GrabPropertyValue();
}
#endif