/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "communityEmbeddedCustomEditor.h"
#include "communityEditor.h"

#include "../../common/core/depot.h"

CEdEmbeddedCommunityCustomEditor::CEdEmbeddedCommunityCustomEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_embeddedResourceEditor( NULL )
{
	m_iconImport = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
	m_iconEdit = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	m_iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
}

CEdEmbeddedCommunityCustomEditor::~CEdEmbeddedCommunityCustomEditor()
{

}

Bool CEdEmbeddedCommunityCustomEditor::GrabValue( String& displayValue )
{
	CObject* propertyValuePointer = NULL;
	m_propertyItem->Read( &propertyValuePointer );
	
	if ( propertyValuePointer == NULL )
	{
		displayValue = TXT( "NULL" );
	}
	else
	{
		displayValue = String::Printf( TXT( "Embedded %s" ), m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}
	
	return true;
}

Bool CEdEmbeddedCommunityCustomEditor::SaveValue()
{
	return ICustomPropertyEditor::SaveValue();
}

void CEdEmbeddedCommunityCustomEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconClear, wxCommandEventHandler( CEdEmbeddedCommunityCustomEditor::OnResourceClear ), this );
	m_propertyItem->AddButton( m_iconEdit, wxCommandEventHandler( CEdEmbeddedCommunityCustomEditor::OnResourceEdit ), this );
	m_propertyItem->AddButton( m_iconImport, wxCommandEventHandler( CEdEmbeddedCommunityCustomEditor::OnResourceImport ), this );
}

void CEdEmbeddedCommunityCustomEditor::OnResourceImport( wxCommandEvent& event )
{
	String activeResourcePath;
	GetActiveResource( activeResourcePath );
	
	CCommunity* communityResource = Cast< CCommunity >( GDepot->LoadResource( activeResourcePath ) );
	if ( communityResource != NULL )
	{
		CObject* rootObject = m_propertyItem->GetRootObject( 0 ).AsObject();
		CCommunity* communityCopy = Cast< CCommunity >( communityResource->Clone( rootObject ) );

		m_propertyItem->Write( &communityCopy );
		m_propertyItem->GrabPropertyValue();
	}
}

void CEdEmbeddedCommunityCustomEditor::OnResourceEdit( wxCommandEvent& event )
{
	CObject* propertyValuePointer = NULL;
	m_propertyItem->Read( &propertyValuePointer );

	CCommunity* community = static_cast< CCommunity* >( propertyValuePointer );
	if ( community == NULL )
	{
		return;
	}
	
	m_embeddedResourceEditor = new CEdCommunityEditor( m_propertyItem->GetPage()->GetParent(), community );
	m_embeddedResourceEditor->Show();
}

void CEdEmbeddedCommunityCustomEditor::OnResourceClear( wxCommandEvent& event )
{
	CCommunity* nullPointer = NULL;
	m_propertyItem->Write( &nullPointer );
	m_propertyItem->GrabPropertyValue();
}
