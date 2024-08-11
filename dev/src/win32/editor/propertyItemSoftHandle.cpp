/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemSoftHandle.h"
#include "../../common/core/depot.h"
#include "../../common/core/diskFile.h"

CPropertyItemSoftHandle::CPropertyItemSoftHandle( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
}

void CPropertyItemSoftHandle::Init( CProperty *prop, Int32 arrayIndex /*= -1*/ )
{
	CPropertyItem::Init( prop, arrayIndex );
	ASSERT( !IsInlined() );
}

void CPropertyItemSoftHandle::Init( IRTTIType *type, Int32 arrayIndex /*= -1*/ )
{
	CPropertyItem::Init( type, arrayIndex );
	ASSERT( !IsInlined() );
}

STypedObject CPropertyItemSoftHandle::GetParentObject( Int32 objectIndex ) const
{
	HALT( "GetParentObject called for soft handle property" );
	return STypedObject();
}

void CPropertyItemSoftHandle::Expand()
{
}

void CPropertyItemSoftHandle::CreateMainControl()
{	
	AddButton( m_page->GetStyle().m_iconClear, wxCommandEventHandler( CPropertyItemSoftHandle::OnObjectClear ) );

	String selectedResource;
	if ( GetActiveResource( selectedResource ) )
	{
		AddButton( m_page->GetStyle().m_iconUse, wxCommandEventHandler( CPropertyItemSoftHandle::OnObjectUse ) );
	}

	AddButton( m_page->GetStyle().m_iconBrowse, wxCommandEventHandler( CPropertyItemSoftHandle::OnObjectBrowser ) );

	// use parent's method to create textControl
	CPropertyItem::CreateMainControl();
}

void CPropertyItemSoftHandle::OnObjectClear( wxCommandEvent& event )
{
	m_displayValue = TXT("NULL");
	SavePropertyValue( false );
	GrabPropertyValue();
}

void CPropertyItemSoftHandle::OnObjectUse( wxCommandEvent& event )
{
	if ( m_propertyType )
	{
		String selectedResource;
		if ( GetActiveResource( selectedResource ) )
		{
			// look up the file
			CDiskFile* file = GDepot->FindFile( selectedResource.AsChar() );
			if ( !file )
			{
				wxMessageBox( wxT("Selected file is not in the depot"), wxT("Error"), wxICON_ERROR | wxOK );
				return;
			}

			// extract the class
			const CClass* fileClass = file->GetResourceClass();
			if ( !fileClass )
			{
				wxMessageBox( wxT("Selected file is not a resource"), wxT("Error"), wxICON_ERROR | wxOK );
				return;
			}

			// verify the class
			RED_ASSERT( m_propertyType->GetType() == RT_SoftHandle );
			const CRTTISoftHandleType* softHandleType = static_cast< const CRTTISoftHandleType* >( m_propertyType );
			const CClass* expectedClass = softHandleType->GetPointedType();
			if ( expectedClass && !fileClass->IsA( expectedClass ) )
			{
				const String txt = String::Printf( TXT("Selected file is '%ls', expecting '%ls'"), 
					fileClass->GetName().AsChar(),
				expectedClass->GetName().AsChar() ); 
				wxMessageBox( txt.AsChar(), wxT("Error"), wxICON_ERROR | wxOK );
				return;
			}

			// set
			m_displayValue = selectedResource;
			SavePropertyValue( false );
			GrabPropertyValue();
		}
	}
}

void CPropertyItemSoftHandle::OnObjectBrowser( wxCommandEvent& event )
{
	// We can browse only determined object
	if ( m_isDetermined )
	{
		// Get the value
		BaseSoftHandle softHandle;
		if ( Read( &softHandle, 0 ) )
		{
			String resourcePath = softHandle.GetPath();
			SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( resourcePath ) );
		}
	}
}

Bool CPropertyItemSoftHandle::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	ASSERT( !"ReadImp called for soft handle type" );
	return false;	
}

Bool CPropertyItemSoftHandle::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	ASSERT( !"WriteImp called for soft handle type" );
	return false;	
}

