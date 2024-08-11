/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "changeMaterialFlags.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/materialGraph.h"
#include "resourceIterator.h"

BEGIN_EVENT_TABLE( CEdChangeMaterialFlagsDlg, wxDialog )
	EVT_BUTTON( XRCID("m_apply"), CEdChangeMaterialFlagsDlg::OnBatch )
	EVT_CHECKLISTBOX( XRCID("m_flags"), CEdChangeMaterialFlagsDlg::OnFlagsChanged )
	END_EVENT_TABLE()

CEdChangeMaterialFlagsDlg::CEdChangeMaterialFlagsDlg( wxWindow* parent, CContextMenuDir* ccontextMenuDir )
	: m_materialFlagCount ( 4 ) // wtf, WTF ??????
	, m_activeLabel ( wxString(""))
	, m_inactiveLabel( wxString("Set at least one flag"))
	, m_contextMenuDir( *ccontextMenuDir )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ChangeFlags") );
	SetTitle( TXT("Change material flags") );

	m_materialFlags = XRCCTRL( *this, "m_flags", wxCheckListBox );
	m_applyButton = XRCCTRL( *this, "m_apply", wxButton );
	m_activeLabel = m_applyButton-> GetLabel();

	// set flag names
	m_materialFlagNames.Insert( 0, TXT("m_canUseOnMeshes") );
	m_materialFlagNames.Insert( 1, TXT("m_canUseOnParticles") );	
	m_materialFlagNames.Insert( 2, TXT("m_canUseOnCollapsableObjects") );

	m_applyButton->Enable( false );
	m_applyButton->SetLabel( m_inactiveLabel );
	for( Uint32 i = 0; i < m_materialFlagNames.Size(); i++)
	{
		m_materialFlags->Append( m_materialFlagNames[i].AsChar() );
	}
}

CEdChangeMaterialFlagsDlg::~CEdChangeMaterialFlagsDlg()
{
	/* intentionally empty */
}

void CEdChangeMaterialFlagsDlg::OnFlagsChanged( wxCommandEvent &event )
{
	Bool nonZeroSelection = false;
	for( Uint32 i = 0; i < m_materialFlags->GetCount(); ++i )
	{
		if( m_materialFlags->IsChecked( i ) )
		{
			nonZeroSelection = true;
			break;
		}
	}

	m_applyButton->Enable( nonZeroSelection );
	if ( nonZeroSelection )
	{
		m_applyButton->SetLabel( m_activeLabel );
	}
	else
	{
		m_applyButton->SetLabel( m_inactiveLabel );
	}
}


void CEdChangeMaterialFlagsDlg::OnBatch( wxCommandEvent &event )
{
	for ( CResourceIteratorAdapter< CMaterialGraph > materialGraph( m_contextMenuDir, TXT("Checking out materials...") ); materialGraph; ++materialGraph )
	{
		ChangeMaterialFlags( materialGraph.Get() );
	}

	Destroy();
}

void CEdChangeMaterialFlagsDlg::ChangeMaterialFlags( CMaterialGraph* materialGraph )
{
	if( materialGraph != nullptr )
	{
		// m_canUseOnMeshes
		Int32 index = m_materialFlagNames.GetIndex( TXT("m_canUseOnMeshes") );
		if( index != -1 && index < ( Int32 ) m_materialFlagNames.Size() )
		{
			materialGraph->SetUseOnMeshes( m_materialFlags->IsChecked( index ) );
		}

		// m_canUseOnParticles
		index = m_materialFlagNames.GetIndex( TXT("m_canUseOnParticles") );
		if( index != -1 && index < ( Int32 ) m_materialFlagNames.Size() )
		{
			materialGraph->SetUseOnParticles( m_materialFlags->IsChecked( index ) );
		}
		
		// m_canUseOnCollapsableObjects
		index = m_materialFlagNames.GetIndex( TXT("m_canUseOnCollapsableObjects") );
		if( index != -1 && index < ( Int32 ) m_materialFlagNames.Size() )
		{
			materialGraph->SetUseOnCollapsableObjects( m_materialFlags->IsChecked( index ) );
		}
	}
}
