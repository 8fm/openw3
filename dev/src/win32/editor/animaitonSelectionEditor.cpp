/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "animationSelectionEditor.h"
#include "animBrowser.h"

IAnimationSelectionEditor::IAnimationSelectionEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_animBrowser( NULL )
{	
	m_iconAnimBrowser = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_PICK") );
	m_iconReset = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_DELETE") );
}

void IAnimationSelectionEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconAnimBrowser, wxCommandEventHandler( IAnimationSelectionEditor::OnSpawnAnimBrowser ), this );
	m_propertyItem->AddButton( m_iconReset, wxCommandEventHandler( IAnimationSelectionEditor::OnResetAnimation ), this );
}

void IAnimationSelectionEditor::CloseControls()
{	
	// Close animation browser
	if ( m_animBrowser )
	{
		m_animBrowser->Destroy();
		m_animBrowser = NULL;
	}
}

Bool IAnimationSelectionEditor::GrabValue( String& displayData )
{
	m_propertyItem->Read( &m_animationChosen );
	displayData = m_animationChosen.AsString();
	return false;
}

Bool IAnimationSelectionEditor::SaveValue()
{
	m_propertyItem->Write( &m_animationChosen );
	return true;
}

void IAnimationSelectionEditor::OnSpawnAnimBrowser( wxCommandEvent &event )
{
	const CAnimatedComponent *component = RetrieveAnimationComponent();
	if ( !component )
	{
		wxMessageBox( TXT("No entity selected or entity does not have animated component"), TXT("Error"), wxOK );
		return;
	}

	// Spawn animation browser	
	m_animBrowser = new CEdAnimBrowser( m_propertyItem->GetPage() );
	m_animBrowser->CloneEntityFromComponent( component );

	m_animBrowser->ShowForSelection();

	CName currentAnimationName;
	m_propertyItem->Read( &currentAnimationName );
	m_animBrowser->SelectAnimation( currentAnimationName.AsString() );
	m_animBrowser->RefreshBrowser();

	m_animBrowser->Connect( wxEVT_ANIM_CONFIRMED, wxCommandEventHandler( IAnimationSelectionEditor::OnAnimationConfirmed ), NULL, this );
	m_animBrowser->Connect( wxEVT_ANIM_ABANDONED, wxCommandEventHandler( IAnimationSelectionEditor::OnAnimationAbandoned ), NULL, this );
}

void IAnimationSelectionEditor::OnAnimationConfirmed( wxCommandEvent &event )
{
	m_animationChosen = CName( m_animBrowser->GetSelectedAnimation() );	
	m_propertyItem->SavePropertyValue();
}

void IAnimationSelectionEditor::OnAnimationAbandoned( wxCommandEvent &event )
{
}

void IAnimationSelectionEditor::OnResetAnimation( wxCommandEvent &event )
{
	m_animationChosen = CName::NONE;
	m_propertyItem->SavePropertyValue();
}
