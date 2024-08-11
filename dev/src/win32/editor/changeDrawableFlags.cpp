/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "changeDrawableFlags.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/drawableComponent.h"
#include "resourceIterator.h"

BEGIN_EVENT_TABLE( CEdChangeDrawableFlagsDlg, wxDialog )
	EVT_BUTTON( XRCID("m_apply"), CEdChangeDrawableFlagsDlg::OnBatch )
END_EVENT_TABLE()

CEdChangeDrawableFlagsDlg::CEdChangeDrawableFlagsDlg( wxWindow* parent, CContextMenuDir* contextMenuDir )
	: m_bitField( nullptr )
	, m_contextMenuDir( *contextMenuDir )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ChangeFlags") );
	SetTitle( TXT("Change drawable flags") );

	wxPanel* flagsPanel = XRCCTRL( *this, "m_flags", wxPanel );
	flagsPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	wxTextCtrl* label = new wxTextCtrl( flagsPanel, wxID_ANY, wxT("Select flags you want to change in first column and specify their new values in second column"),
		wxDefaultPosition, wxDefaultSize, wxTE_CENTRE|wxTE_MULTILINE|wxTE_NO_VSCROLL|wxTE_READONLY|wxTE_WORDWRAP|wxNO_BORDER );
	label->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	flagsPanel->GetSizer()->Add( label, 0, wxEXPAND|wxALL|wxALIGN_CENTER_HORIZONTAL, 3 );

	// add options to light channels box
	static CName typeName( TXT("EDrawableFlags" ) );
	m_bitField = static_cast< CBitField* >( SRTTI::GetInstance().FindType( typeName, RT_BitField ) );

	if( m_bitField != nullptr )
	{
		for ( Uint32 i=0; i<32; i++ )
		{
			CName bitName = m_bitField->GetBitName( i );
			if ( bitName && !m_mapCheckBoxToBit.FindPtr( bitName.AsString() ) )
			{
				Uint32& value = m_mapCheckBoxToBit.GetRef( bitName.AsString() );
				value = i;

				FlagCheckbox fc;
				fc.m_valCBox = new wxCheckBox( flagsPanel, wxID_ANY, bitName.AsChar() );
				fc.m_valCBox->Enable( false );
				fc.m_name = bitName.AsString();

				wxCheckBox* enablingCheckbox = new wxCheckBox( flagsPanel, wxID_ANY, wxEmptyString );
				enablingCheckbox->SetValue( false );
				enablingCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdChangeDrawableFlagsDlg::FlagValueEnabled ), NULL, this );

				m_drawableFlags.Insert( enablingCheckbox, fc );

				wxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
				sizer->Add( enablingCheckbox, 0, wxLEFT|wxRIGHT, 5 );
				sizer->Add( fc.m_valCBox, 0, wxALL, 0 );
				flagsPanel->GetSizer()->Add( sizer, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 0 );
			}
		}
	}
	else
	{
		GFeedback->ShowError( TXT(" Change drawable flags editor did not find EDrawableFlags type. Editor will be closed.") );
		Close();
	}
	Layout();
	Refresh(true);
}

CEdChangeDrawableFlagsDlg::~CEdChangeDrawableFlagsDlg()
{
}

void CEdChangeDrawableFlagsDlg::OnBatch( wxCommandEvent &event )
{
	for ( CResourceIteratorAdapter< CEntityTemplate > entityTemplate( m_contextMenuDir, TXT("Checking out entities...") ); entityTemplate; ++entityTemplate )
	{
		ChangeDrawableFlags( entityTemplate.Get() );
	}

	Close();
}

void CEdChangeDrawableFlagsDlg::ChangeDrawableFlags( CEntityTemplate* entityTemplate )
{
	if ( CEntity* entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() ) )
	{
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		entity->ForceFinishAsyncResourceLoads();

		{ // Do the actual modification - the rest is just needed for a template to be properly re-saved
			for ( CComponent* component : entity->GetComponents() )
			{
				if ( CDrawableComponent* drawableComponent = Cast< CDrawableComponent >( component ) )
				{
					for ( THashMap< wxCheckBox*, FlagCheckbox >::iterator it = m_drawableFlags.Begin(); it != m_drawableFlags.End(); ++it )
					{
						if ( it->m_first->IsChecked() )
						{
							Uint32 bitField = m_mapCheckBoxToBit.GetRef( it->m_second.m_name );
							drawableComponent->EnableDrawableFlags( it->m_second.m_valCBox->IsChecked(), FLAG( bitField ) );
						}
					}
				}
			}
		}

		entity->UpdateStreamedComponentDataBuffers();
		entity->PrepareEntityForTemplateSaving();
		entity->DetachTemplate();

		entityTemplate->CaptureData( entity );

		// Destroy instance
		entity->Discard();

		// Second instance to convert any new properties
		entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
		if ( entity != nullptr )
		{
			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			entity->UpdateStreamedComponentDataBuffers();
			entity->Discard();
		}
	}
}

void CEdChangeDrawableFlagsDlg::FlagValueEnabled( wxCommandEvent& event )
{
	wxCheckBox* clickedCBox = wxStaticCast( event.GetEventObject(), wxCheckBox );
	FlagCheckbox* fc = m_drawableFlags.FindPtr( clickedCBox );

	if ( clickedCBox && fc != nullptr )
	{
		fc->m_valCBox->Enable( event.GetInt() != 0 );
	}
}
