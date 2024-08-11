/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityEditor.h"
#include "entityEditorWoundItem.h"
#include "../../common/engine/dismembermentComponent.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/meshComponent.h"



class wxDismemberWoundsListClientData : public wxClientData
{
public:
	CName m_woundName;
	Bool m_editable;
	Bool m_disabledByInclude;

public:
	wxDismemberWoundsListClientData( const CName& name, Bool editable, Bool disabledByInclude )
		: m_woundName( name )
		, m_editable( editable )
		, m_disabledByInclude( disabledByInclude )
	{}
};


//////////////////////////////////////////////////////////////////////////
// This stuff could all be member functions, but it seems cleaner to not clutter up CEdEntityEditor with functions
// that are only going to be used here.


static CEntityDismemberment* GetBaseEntityDismemberment( CEntityTemplate* entityTemplate, Bool createIfNotFound )
{
	CEntityDismemberment* dismember = entityTemplate->FindParameter< CEntityDismemberment >( false, false );
	if ( dismember == nullptr && createIfNotFound )
	{
		dismember = new CEntityDismemberment();
		entityTemplate->AddParameterUnique( dismember );
	}
	return dismember;
}

static CDismembermentComponent* GetDismembermentComponent( CEdEntityEditor* editor )
{
	CEntity* entity = editor->GetPreviewPanel()->GetEntity();
	return entity ? entity->FindComponent< CDismembermentComponent >() : nullptr;
}


//////////////////////////////////////////////////////////////////////////


void CEdEntityEditor::InitDismemberTab()
{
	m_listDismemberWounds	= XRCCTRL( *this, "WoundList", wxCheckListBox );
	m_listDismemberWounds->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdEntityEditor::OnDismemberWoundSelectionChanged, this );
	m_listDismemberWounds->Bind( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdEntityEditor::OnDismemberWoundChecked, this );

	XRCCTRL( *this, "btnOverrideWound", wxButton )		->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnWoundOverride, this );
	XRCCTRL( *this, "btnAddWound", wxButton )			->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnWoundAdded, this );
	XRCCTRL( *this, "btnRemoveWound", wxButton )		->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnWoundRemoved, this );
	XRCCTRL( *this, "btnDismemberForceTPose", wxButton )->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnForceTPose, this );
	XRCCTRL( *this, "btnExportWounds", wxButton )		->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnWoundsExport, this );
	XRCCTRL( *this, "dismemberAppearances", wxChoice )	->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdEntityEditor::OnDismemberAppearanceSelected, this );

	m_showWounds.Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdEntityEditor::OnWoundsToggleShow, this );
	m_showWounds.AddValue( false, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE_CLOSED") ) );
	m_showWounds.AddValue( true, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE") ) );
	m_showWounds.AddButton( XRCCTRL( *this, "btnShowWounds", wxBitmapButton ) );


	m_freezeEntityPose.AddButton( XRCCTRL( *this, "btnDismemberFreezeAnim", wxBitmapButton ) );
	m_previewItemSize.AddButton( XRCCTRL( *this, "btnDismemberSize", wxBitmapButton ) );

	{
		wxPanel* rp = XRCCTRL( *this, "DismemberPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		PropertiesPageSettings settings;
		m_dismemberWoundProp = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_dismemberWoundProp->Get().Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdEntityEditor::OnDismemberWoundModified, this );

		sizer1->Add( m_dismemberWoundProp, 1, wxEXPAND | wxALL, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}
}


void CEdEntityEditor::UpdateDismembermentTab()
{
	UpdateDismembermentList();
	UpdateDismemberAppearances();
}

void CEdEntityEditor::UpdateDismembermentList( const CName& woundNameToSelect /*= CName::NONE*/ )
{
	CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
	if ( dismemberComponent == nullptr )
	{
		return;
	}


	// Clear current lists
	m_listDismemberWounds->Freeze();
	m_listDismemberWounds->Clear();

	{
		TDynArray< CName > woundNames;
		CEntityDismemberment::GetAllWoundNamesRecursive( m_template, woundNames );

		CEntityDismemberment* baseDismember = GetBaseEntityDismemberment( m_template, false );

		for ( const CName& woundName : woundNames )
		{
			// Can only edit the wounds that are held directly by our template.
			Bool editable = ( baseDismember != nullptr && baseDismember->FindWoundByName( woundName ) != nullptr );

			wxString nameString( woundName.AsChar() );
			if ( !editable )
			{
				nameString += wxT(" (Included)");
			}

			Bool disabledByInclude = CEntityDismemberment::IsWoundDisabledByIncludeRecursive( m_preview->GetEntity(), woundName );
			if ( disabledByInclude )
			{
				nameString += wxT(" (Disabled by include)");
			}

			Int32 index = m_listDismemberWounds->Append( nameString, new wxDismemberWoundsListClientData( woundName, editable, disabledByInclude ) );

			if ( woundName == woundNameToSelect )
			{
				m_listDismemberWounds->SetSelection( index );
			}

			m_listDismemberWounds->Check( index, dismemberComponent->IsWoundEnabled( woundName ) );
		}
	}

	// Finalize list
	if ( m_listDismemberWounds->GetCount() > 0 )
	{
		m_listDismemberWounds->Enable();
	}
	else
	{
		m_listDismemberWounds->Append( wxT("(No Wounds)") );
		m_listDismemberWounds->SetSelection( 0 );
		m_listDismemberWounds->Disable();
	}

	// Refresh
	m_listDismemberWounds->Thaw();
	m_listDismemberWounds->Refresh();

	// Refresh properties
	wxCommandEvent fakeEvent;
	OnDismemberWoundSelectionChanged( fakeEvent );
}


void CEdEntityEditor::OnWoundsToggleShow( wxCommandEvent& /*event*/ )
{
	UpdateVisibleWound();
}


void CEdEntityEditor::OnWoundOverride( wxCommandEvent& /*event*/ )
{
	Int32 selection = m_listDismemberWounds->GetSelection();
	if ( selection != -1 && m_listDismemberWounds->HasClientObjectData() )
	{
		wxDismemberWoundsListClientData* data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selection ) );
		RED_ASSERT( data != nullptr, TXT("Null wound client data?") );
		if ( data == nullptr )
		{
			return;
		}

		if ( data->m_editable )
		{
			GFeedback->ShowMsg( TXT("Override wound"), TXT("The wound '%s' is already in this template, so can't be overridden here."), data->m_woundName.AsChar() );
			return;
		}

		CDismembermentWound* existingWound = CEntityDismemberment::FindNonConstWoundByNameRecursive( m_template, data->m_woundName );
		RED_ASSERT( existingWound != nullptr, TXT("Wound %s not found") );
		if ( existingWound == nullptr )
		{
			return;
		}

		CDismembermentWound* newWound = new CDismembermentWound( *existingWound );

		CEntityDismemberment* baseDismember = GetBaseEntityDismemberment( m_template, true );
		if ( !baseDismember->AddWound( newWound ) )
		{
			GFeedback->ShowError( TXT("Failed to add wound.") );
			delete newWound;
			return;
		}

		// Clear currently visible wound, so it'll get set properly to the new one.
		CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
		if ( dismemberComponent != nullptr )
		{
			dismemberComponent->SetVisibleWound( CName::NONE );
		}

		UpdateDismembermentList( newWound->GetName() );
	}
}


void CEdEntityEditor::OnWoundAdded( wxCommandEvent& /*event*/ )
{
	if ( !m_template->MarkModified() )
	{
		return;
	}


	// Ask for slot name
	String woundNameString;
	if ( !InputBox( this, TXT("New wound"), TXT("Enter name of the new wound"), woundNameString, false ) )
	{
		return;
	}

	CEntityDismemberment* dismemberment = GetBaseEntityDismemberment( m_template, true );

	CName woundName( woundNameString.AsChar() );
	if ( !dismemberment->AddWound( woundName ) )
	{
		GFeedback->ShowError( TXT("Unable to add wound to the entity") );
		return;
	}

	// Update the list of the wounds
	UpdateDismembermentList( woundName );
}


void CEdEntityEditor::OnWoundRemoved( wxCommandEvent& /*event*/ )
{
	Int32 selectedIndex = m_listDismemberWounds->GetSelection();
	if( selectedIndex < 0 )
	{
		return;
	}

	wxDismemberWoundsListClientData* data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selectedIndex ) );
	RED_WARNING( data != nullptr, "Null wounds list client data?" );
	if ( data == nullptr )
	{
		return;
	}

	if ( !data->m_editable )
	{
		GFeedback->ShowError( TXT("Cannot remove this wound. It was included in some other entity template") );
		return;
	}


	CEntityDismemberment* dismemberment = GetBaseEntityDismemberment( m_template, false );
	RED_WARNING( dismemberment != nullptr, "No dismemberment found for entity, but an editable wound?" );
	if ( dismemberment == nullptr )
	{
		return;
	}

	if ( !m_template->MarkModified() )
	{
		return;
	}

	if ( !dismemberment->RemoveWound( data->m_woundName ) )
	{
		GFeedback->ShowError( TXT("Couldn't remove wound? Maybe you have multiple dismember components?") );
		return;
	}

	CDismembermentComponent* disComp = GetDismembermentComponent( this );
	if ( disComp != nullptr && data->m_woundName == disComp->GetVisibleWoundName() )
	{
		disComp->SetVisibleWound( CName::NONE );
	}

	// If this was the last wound in the entity dismemberment, we can delete it.
	if ( dismemberment->GetWounds().Empty() )
	{
		m_template->RemoveParameter( dismemberment );
		dismemberment->Discard();
	}

	// Update the list of the wounds
	UpdateDismembermentList();
}

void CEdEntityEditor::OnWoundsExport( wxCommandEvent& /*event*/ )
{
	TDynArray< CName > woundNames;
	TDynArray< CEntityDismemberment* > dismembers;
	m_template->GetAllParameters< CEntityDismemberment >( dismembers );

	CEntityDismemberment* baseDismember = GetBaseEntityDismemberment( m_template, false );

	// Count how many wounds we have
	Uint32 numComponents = 0;
	for ( CEntityDismemberment* dismember : dismembers )
	{
		numComponents += dismember->GetWounds().Size();
	}

	if ( numComponents == 0 )
	{
		GFeedback->ShowError( TXT("Nothing to export.") );
		return;
	}


	String defaultDir = wxEmptyString;
	String wildCard = TXT("MaxScript file (*.ms)|*.ms|All files (*.*)|*.*");
	wxFileDialog fileDialog( this, wxT("Save"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );

	if ( fileDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	String fPath = fileDialog.GetPath().wc_str();
	RED_ASSERT( !fPath.Empty() );
	if ( fPath.Empty() ) return;


	FILE* f = _wfopen( fPath.AsChar(), TXT("w"));
	if ( !f )
	{
		GFeedback->ShowError( TXT("Cant Create File '%s'."), fPath.AsChar() );
		return;
	}


	for ( CEntityDismemberment* dismember : dismembers )
	{
		for ( CDismembermentWound* wound : dismember->GetWounds() )
		{
			const EngineTransform & tr = wound->GetTransform();
			String name = wound->GetName().AsString();
			Matrix tm;
			tr.GetRotation().ToMatrix(tm);
			tm.SetTranslation( tr.GetPosition()*100.0f );
			tm.SetRow(0, tm.GetRow(0)*tr.GetScale().X );
			tm.SetRow(1, tm.GetRow(1)*tr.GetScale().Y );
			tm.SetRow(2, tm.GetRow(2)*tr.GetScale().Z );

			Matrix flip = Matrix::IDENTITY;
			flip.SetRow( 0, Vector(-1.0f,0.0f,0.0f) );
			flip.SetRow( 1, Vector(0.0f,-1.0f,0.0f) );

			tm = Matrix::Mul( flip, tm );

			fprintf(f,("sphere radius:100.0 segs:48 name: \"%s\" transform: (matrix3 [%g,%g,%g] [%g,%g,%g] [%g,%g,%g] [%g,%g,%g])\n"),
				UNICODE_TO_ANSI( name.AsChar() ),
				tm.GetRow(0).X, tm.GetRow(0).Y, tm.GetRow(0).Z,
				tm.GetRow(1).X, tm.GetRow(1).Y, tm.GetRow(1).Z,
				tm.GetRow(2).X, tm.GetRow(2).Y, tm.GetRow(2).Z,
				tm.GetRow(3).X, tm.GetRow(3).Y, tm.GetRow(3).Z
				);
		}
	}

	fclose(f);

	GFeedback->ShowMsg(  TXT("Export complete"), TXT("Exported %u wounds to '%s'."), numComponents, fPath.AsChar() );
}

void CEdEntityEditor::OnWoundPropertyModified( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	String propName = eventData->m_propertyName.AsString();
	if ( propName == TXT("name") )
	{
		CDismembermentWound* wound = eventData->m_typedObject.As< CDismembermentWound >();
		if ( wound != nullptr ) // can be type different than CDismembermentWound
		{
			UpdateDismembermentList( wound->GetName() );
		}
	}
}

void CEdEntityEditor::OnDismemberWoundSelectionChanged( wxCommandEvent& /*event*/ )
{
	wxDismemberWoundsListClientData* data = nullptr;

	// Get selected slot
	Int32 selection = m_listDismemberWounds->GetSelection();
	if ( selection != -1 && m_listDismemberWounds->HasClientObjectData() )
	{
		data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selection ) );
		RED_WARNING( data != nullptr, "No wound client data?" );
	}

	CDismembermentWound* selectedWound = nullptr;
	
	if ( data != nullptr )
	{
		selectedWound = CEntityDismemberment::FindNonConstWoundByNameRecursive( m_template, data->m_woundName );
		RED_WARNING( selectedWound != nullptr, "Couldn't find wound %s", data->m_woundName.AsChar() );
	}


	// Update properties
	m_dismemberWoundProp->Get().SetTypedObject( STypedObject( selectedWound, ClassID< CDismembermentWound >() ) );
	m_dismemberWoundProp->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnWoundPropertyModified ), NULL, this );
	if ( data != nullptr )
	{
		m_dismemberWoundProp->Enable( data->m_editable );
	}


	UpdateVisibleWound();
}


void CEdEntityEditor::OnDismemberWoundModified( wxCommandEvent& /*event*/ )
{
	Int32 selection = m_listDismemberWounds->GetSelection();
	if ( selection == -1 )
	{
		return;
	}

	wxDismemberWoundsListClientData* data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selection ) );
	RED_WARNING( data != nullptr, "No wound client data?" );
	if ( data == nullptr )
	{
		return;
	}

	CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
	RED_ASSERT( dismemberComponent != nullptr );

	// If it's the visible wound, force an update.
	if ( data->m_woundName == dismemberComponent->GetVisibleWoundName() )
	{
		dismemberComponent->ForceUpdateWound();
	}

	// Even if it's not visible, we need to refresh the preview item.
	m_preview->RefreshPreviewWoundChanges();
}


void CEdEntityEditor::OnDismemberPageShow()
{
	UpdateDismembermentTab();
}


void CEdEntityEditor::OnDismemberPageHide()
{
	m_freezeEntityPose.SetValue( false );
	m_showWounds.SetValue( false );
}



void CEdEntityEditor::RefreshVisualWoundItem( const CDismembermentWound* selectedWound )
{
	m_preview->RefreshPreviewWoundItem( selectedWound );
}


void CEdEntityEditor::UpdateVisibleWound()
{
	CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
	if ( dismemberComponent == nullptr )
	{
		return;
	}

	// Get selected slot
	wxDismemberWoundsListClientData* data = nullptr;
	Int32 selection = m_listDismemberWounds->GetSelection();
	if ( selection != -1 && m_listDismemberWounds->HasClientObjectData() )
	{
		data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selection ) );
		RED_WARNING( data != nullptr, "No wound client data?" );
	}

	CName woundName = CName::NONE;

	if ( m_showWounds.GetValue() && data != nullptr )
	{
		woundName = data->m_woundName;
	}

	dismemberComponent->SetVisibleWound( woundName );
	RefreshVisualWoundItem( dismemberComponent->GetVisibleWound() );
}


void CEdEntityEditor::OnDismemberPreviewItemTransformChanged()
{
	CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
	if ( dismemberComponent != nullptr )
	{
		dismemberComponent->ForceUpdateWoundTransform();
	}
}



void CEdEntityEditor::CheckForMultipleDismemberment()
{
	TDynArray< CDismembermentComponent* > dismemberComponents;
	CollectEntityComponents( m_preview->GetEntity(), dismemberComponents );
	if ( dismemberComponents.Size() > 1 )
	{
		GFeedback->ShowError( TXT("Multiple dismemberment components found in entity. Only one per entity is supported.") );
	}
}



void CEdEntityEditor::DeselectDismemberWound()
{
	m_listDismemberWounds->DeselectAll();
	// Send dummy event, since Deselect doesn't seem to do it.
	OnDismemberWoundSelectionChanged( wxCommandEvent( wxEVT_COMMAND_LISTBOX_SELECTED ) );
}


void CEdEntityEditor::PreventSelectingDismemberFillMesh()
{
	CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
	if ( dismemberComponent )
	{
		// Deselect fill mesh if it was selected.
		CMeshComponent* fillMeshComponent = dismemberComponent->GetFillMeshComponent();
		if ( fillMeshComponent != nullptr && fillMeshComponent->IsSelected() )
		{
			CSelectionManager* selectionManager = m_preview->GetSelectionManager();
			selectionManager->Deselect( fillMeshComponent );
		}
	}
}


void CEdEntityEditor::OnDismemberAppearanceSelected( wxCommandEvent& /*event*/ )
{
	wxChoice* appearancesList = XRCCTRL( *this, "dismemberAppearances", wxChoice );

	CName appearanceName( appearancesList->GetStringSelection().wc_str() );

	const CEntityAppearance * appearance = m_template->GetAppearance( appearanceName, true );

	if ( appearance != nullptr )
	{
		SetEntityAppearance( appearance );

		CDismembermentComponent* dismemberComponent = GetDismembermentComponent( this );
		if ( dismemberComponent != nullptr )
		{
			dismemberComponent->ForceUpdateWound();
		}
	}


	CName selectedWound = CName::NONE;
	Int32 selection = m_listDismemberWounds->GetSelection();
	if ( selection != -1 && m_listDismemberWounds->HasClientData() )
	{
		
		wxDismemberWoundsListClientData* data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( selection ) );
		RED_WARNING( data != nullptr, "No wound client data?" );
		if ( data != nullptr )
		{
			selectedWound = data->m_woundName;
		}
	}

	UpdateDismembermentList( selectedWound );
}


void CEdEntityEditor::OnDismemberWoundChecked( wxCommandEvent& event )
{
	Int32 index = event.GetInt();


	CAppearanceComponent* appComponent = m_preview->GetEntity()->FindComponent< CAppearanceComponent >();
	if ( appComponent == nullptr )
	{
		// Do nothing if no appearances. Wounds are always enabled.
		m_listDismemberWounds->Check( index, true );
		return;
	}

	wxDismemberWoundsListClientData* data = static_cast< wxDismemberWoundsListClientData* >( m_listDismemberWounds->GetClientObject( index ) );
	RED_WARNING( data != nullptr, "Null client data in item %u '%s'", index, m_listDismemberWounds->GetString( index ).wc_str() );
	if ( data != nullptr )
	{
		const CName& appearanceName = appComponent->GetAppearance();
		const CName& woundName = data->m_woundName;
		const Bool disable = !m_listDismemberWounds->IsChecked( index );

		CEntityDismemberment* dismember = GetBaseEntityDismemberment( m_template, true );

		dismember->SetWoundDisabledForAppearance( woundName, appearanceName, disable );


		// If it's been disabled by an included template, we can't enable it. So make sure the check box stays unchecked.
		if ( !disable && CEntityDismemberment::IsWoundDisabledByIncludeRecursive( m_preview->GetEntity(), woundName ) )
		{
			m_listDismemberWounds->Check( index, false );
		}
	}

	event.Skip();
}


void CEdEntityEditor::UpdateUsedDismemberment()
{
	CEntityDismemberment* dismember = GetBaseEntityDismemberment( m_template, false );
	if ( dismember == nullptr )
	{
		return;
	}

	TDynArray< CEntityAppearance* > appearances;
	m_template->GetAllAppearances( appearances );

	TDynArray< CName > appearanceNames( appearances.Size() );
	for ( Uint32 i = 0; i < appearances.Size(); ++i )
	{
		appearanceNames[ i ] = appearances[ i ]->GetName();
	}

	TDynArray< SDismembermentWoundFilter >& disabledWounds = dismember->GetDisabledWounds();
	for ( Int32 i = disabledWounds.SizeInt() - 1; i >= 0; --i )
	{
		SDismembermentWoundFilter& filter = disabledWounds[ i ];

		// If there's not wound or appearance that matches, we can remove this filter.
		if ( CEntityDismemberment::FindWoundByNameRecursive( m_template, filter.m_wound ) == nullptr 
			|| !appearanceNames.Exist( filter.m_appearance ) )
		{
			disabledWounds.RemoveAt( i );
		}
	}
}

void CEdEntityEditor::UpdateDismemberAppearances()
{
	// Repopulate the appearance list, and select whichever appearance is currently active.

	Bool hasAppearances = m_preview->GetEntity()->FindComponent< CAppearanceComponent >() != nullptr;

	wxChoice* appearancesList = XRCCTRL( *this, "dismemberAppearances", wxChoice );

	appearancesList->Freeze();
	appearancesList->Clear();

	if ( hasAppearances )
	{
		TDynArray< const CEntityAppearance* > appearances;
		m_template->GetAllAppearances( appearances );
		for ( const CEntityAppearance* appearance : appearances )
		{
			appearancesList->Append( appearance->GetName().AsChar() );
		}

		// Select current appearance
		Int32 idx = appearancesList->FindString( m_preview->GetEntity()->FindComponent< CAppearanceComponent >()->GetAppearance().AsChar() );
		appearancesList->Select( idx );
	}


	appearancesList->Thaw();

	appearancesList->Enable( hasAppearances );


	// Make sure we're refreshed with this selected appearance.
	wxCommandEvent dummyEvent;
	OnDismemberAppearanceSelected( dummyEvent );
}
