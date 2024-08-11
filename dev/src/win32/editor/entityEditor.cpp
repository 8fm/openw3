/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityEditor.h"
#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "effectEditor.h"
#include "animBrowser.h"
#include "editorExternalResources.h"
#include "toolsPanel.h"
#include "undoManager.h"
#include "entityEditorSlotItem.h"
#include "entityEditorHelpers.h"
#include "gameplayParamEditor.h"
#include "wizardTemplate.h"
#include "editorWizardDefinition.h"
#include "entityMimicsPanel.h"
#include "entityMeshGenerator.h"
#include "undoSelection.h"
#include "characterEntityEditor.h"

#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/newNpc.h"

#include "../../common/game/definitionsManager.h"
#include "../../common/game/minigame.h"
#include "../../common/game/aiProfile.h"
#include "../../common/game/attitude.h"
#include "../../common/game/actor.h"
#include "../../common/game/inventoryDefinition.h"
#include "../../common/game/equipmentState.h"
#include "../../common/game/itemEntity.h"
#include "../../common/game/inventoryComponent.h"
#include "../../common/game/wayPointComponent.h"

#include "../../common/engine/renderCommands.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/fxDefinition.h"
#include "../../common/engine/soundEntityParam.h"
#include "../../common/engine/animBehaviorsAndSetsParam.h"
#include "../../common/game/aiParameters.h"
#include "../../common/game/aiPresetParam.h"

#include "../../common/engine/dismembermentComponent.h"

#include "../../common/engine/mesh.h"

#include "../../common/core/depot.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/dataError.h"

#include "meshStats.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/meshSkinningAttachment.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/animDangleComponent.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/characterEntityTemplate.h"
#include "../../common/engine/entityExternalAppearance.h"
#include "../../common/engine/entityTemplateModifier.h"
#include "../../common/engine/dropPhysicsComponentModifier.h"
#include "../../common/engine/appearanceComponentModifier.h"

#include "../../common/game/storySceneVoiceTagsManager.h"

BEGIN_EVENT_TABLE( CEdEntityEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "editCut" ), CEdEntityEditor::OnEditCut )
	EVT_MENU( XRCID( "editCopy" ), CEdEntityEditor::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdEntityEditor::OnEditPaste )
	EVT_MENU( XRCID( "editDelete" ), CEdEntityEditor::OnEditDelete )
	EVT_MENU( XRCID( "editUndo" ), CEdEntityEditor::OnEditUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdEntityEditor::OnEditRedo )
	EVT_MENU( XRCID( "editSelectAll"), CEdEntityEditor::OnSelectAll )
	EVT_MENU( XRCID( "editUnselectAll"), CEdEntityEditor::OnUnselectAll )
	EVT_MENU( XRCID( "editInvertSelection"), CEdEntityEditor::OnInvertSelection )
	EVT_MENU( XRCID( "editFindEntityComponents" ), CEdEntityEditor::OnEditFindEntityComponents )

	EVT_TOOL( XRCID( "entitySave" ), CEdEntityEditor::OnSave )
	EVT_TOOL( XRCID( "saveWithStreamedComponents" ), CEdEntityEditor::OnSaveWithStreamedComponents )
	EVT_TOOL( XRCID( "editRefresh" ), CEdEntityEditor::OnRefreshInEditor )

	EVT_MENU( XRCID( "menuZoomExtentsPreview" ), CEdEntityEditor::OnZoomExtentsPreview )
	EVT_MENU( XRCID( "menuZoomExtentsGraph" ), CEdEntityEditor::OnZoomExtentsGraph )
	EVT_TOOL( XRCID( "toolZoomExtentsPreview" ), CEdEntityEditor::OnZoomExtentsPreview )

	EVT_TOOL( XRCID( "toolShowBB" ), CEdEntityEditor::OnShowBoundingBox )
	EVT_TOOL( XRCID( "toolShowCollision" ), CEdEntityEditor::OnShowCollision )
	EVT_TOOL( XRCID( "toolShowWireframe" ), CEdEntityEditor::OnShowWireframe )
	EVT_TOOL( XRCID( "toolShowTBN" ), CEdEntityEditor::OnShowTBN )

	EVT_MENU( XRCID( "entityPrepare" ), CEdEntityEditor::OnPrepare )
	EVT_MENU( XRCID( "entityExport" ), CEdEntityEditor::OnExportEntity )
	EVT_MENU( XRCID( "entityImport" ), CEdEntityEditor::OnImportEntity )
	EVT_MENU( XRCID( "entityGenProxyMesh" ), CEdEntityEditor::OnGenerateProxyMesh )
	EVT_MENU( XRCID( "entitySetupProxyMesh" ), CEdEntityEditor::OnSetupProxyMesh )	
	EVT_MENU( XRCID( "entityDeduplicateComponents" ), CEdEntityEditor::OnDeduplicateComponents )
	EVT_MENU( XRCID( "entityRemapSkinning" ), CEdEntityEditor::OnRemapSkinning )
	EVT_TOOL( XRCID( "previewDisableForceLod" ), CEdEntityEditor::OnResetForcedLOD )
	EVT_TOOL( XRCID( "previewForceLOD0" ), CEdEntityEditor::OnPreviewLOD0 )
	EVT_TOOL( XRCID( "previewForceLOD1" ), CEdEntityEditor::OnPreviewLOD1 )
	EVT_TOOL( XRCID( "previewForceLOD2" ), CEdEntityEditor::OnPreviewLOD2 )
	EVT_TOOL( XRCID( "previewForceLOD3" ), CEdEntityEditor::OnPreviewLOD3 )

	EVT_MENU( XRCID( "viewDisableAppearanceApplication" ), CEdEntityEditor::OnDisableAppearanceApplication )

	// hide unused ui 
	/*
	EVT_TOOL( XRCID( "entityLOD0" ), CEdEntityEditor::OnPreviewEntityLOD0 )
	EVT_TOOL( XRCID( "entityLOD1" ), CEdEntityEditor::OnPreviewEntityLOD1 )
	EVT_TOOL( XRCID( "entityLOD2" ), CEdEntityEditor::OnPreviewEntityLOD2 )
	EVT_TOOL( XRCID( "entityLOD3" ), CEdEntityEditor::OnPreviewEntityLOD3 )
	*/
	EVT_SIZE( CEdEntityEditor::OnSize )
	EVT_PAINT( CEdEntityEditor::OnPaint )
END_EVENT_TABLE()

IMPLEMENT_CLASS( CEdEntityEditor, wxSmartLayoutPanel );


// Labels for the main tabs. Not all are defined, just as needed.
// Should use labels instead of tab index, since we add/remove tabs depending on what the entity has.
#define PAGE_LABEL_APPEARANCES	wxString("Appearances")
#define PAGE_LABEL_SLOTS		wxString("Slots")
#define PAGE_LABEL_COLORS		wxString("Colors")
#define PAGE_LABEL_INVENTORY	wxString("Inventory")
#define PAGE_LABEL_DISMEMBER	wxString("Dismemberment")

#define ID_ACTAPPMENU_EXPORT				1201
#define ID_ACTAPPMENU_IMPORT    			1202

// Strings used by ai tree config panel
#define AITREE_SELECTION_INHERIT		TXT("Inherit")
#define AITREE_SELECTION_USEAIWIZARD	TXT("Use AI wizard")
namespace
{
	void CollectAllIncludes( CEntityTemplate *entityTemplate, TDynArray< CEntityTemplate* > &loadedIncludesOut );
	CGatheredResource resSavedGameplayEntitiesTags( TXT("gameplay\\globals\\gameplay_tags.csv"), 0 );
	CGatheredResource resSavedReactionFieldsTags( TXT("gameplay\\globals\\reactionfields.csv"), 0 );
}

TDynArray< THandle< CFXDefinition > > CEdEntityEditor::s_fxInClipboard;

extern Bool GDisableAppearances;

static Bool IsTemplateCollapsedInAppearance( CEntityAppearance* appearance, CEntityTemplate* tpl )
{
	ASSERT( tpl->GetEntityObject(), TXT("No entity object in template, this will crash hard") );
	const TDynArray<CComponent*>& components = tpl->GetEntityObject()->GetComponents();
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{
		if ( appearance->IsCollapsed( *it ) )
		{
			return true;
		}
	}
	return false;
}

namespace
{
	CDirectory* DoResourceOpenDialog( const CFileFormat& format, String& outFileName )
	{
		CEdFileDialog dialog;
		dialog.AddFormat( format );

		if ( !dialog.DoOpen( static_cast< ::HWND >( wxTheFrame->GetHandle() ) ) )
		{
			return nullptr;
		}

		String ext = TXT(".") + format.GetExtension();
		String path( dialog.GetFile() );
		if( ! path.EndsWith( ext ) )
		{
			path += ext;
		}

		size_t pos; 
		path.FindSubstring( GDepot->GetRootDataPath(), pos );
		if( pos != 0 )
		{
			wxMessageBox( TXT( "You can open a resource only inside depot" ) );
			return nullptr;
		}

		String relativePath( path.RightString( path.Size() - GDepot->GetRootDataPath().Size() ) );
		String relativeDir( relativePath.StringBefore( TXT( "\\" ), true ) );
		outFileName = relativeDir.Empty() ? relativePath : relativePath.StringAfter( TXT( "\\" ), true );

		if( ! relativeDir.Empty() )
		{
			relativeDir += TXT( "\\" );
		}

		CDirectory* dir = GDepot->FindPath( relativeDir.AsChar() );
		ASSERT( dir != nullptr );

		return dir;
	}

	CDirectory* DoResourceSaveDialog( const CFileFormat& format, String& outFileName, const CDirectory* defaultDir )
	{
		CEdFileDialog dialog;
		dialog.AddFormat( format );
		
		if( defaultDir )
		{
			dialog.SetDirectory( defaultDir->GetAbsolutePath() );
		}

		if ( !dialog.DoSave( static_cast< ::HWND >( wxTheFrame->GetHandle() ), outFileName.AsChar() ) )
		{
			return nullptr;
		}

		String ext = TXT(".") + format.GetExtension();
		String path( dialog.GetFile() );
		if( ! path.EndsWith( ext ) )
		{
			path += ext;
		}

		size_t pos; 
		path.FindSubstring( GDepot->GetRootDataPath(), pos );
		if( pos != 0 )
		{
			wxMessageBox( TXT( "You can create a resource only inside depot" ) );
			return nullptr;
		}

		String relativePath( path.RightString( path.Size() - GDepot->GetRootDataPath().Size() ) );
		String relativeDir( relativePath.StringBefore( TXT( "\\" ), true ) );
		outFileName = relativeDir.Empty() ? relativePath : relativePath.StringAfter( TXT( "\\" ), true );

		if( ! relativeDir.Empty() )
		{
			relativeDir += TXT( "\\" );
		}

		CDirectory* dir = GDepot->FindPath( relativeDir.AsChar() );
		ASSERT( dir != nullptr );

		return dir;
	}
}

void CEdEntitySlotProperties::SetSlot( const EntitySlot* selectedSlot )
{
	m_slot = selectedSlot;
	if ( selectedSlot )
	{
		SetTypedObject( STypedObject( (void*)selectedSlot, ClassID< EntitySlot >() ) );
	}
	else
	{
		SetNoObject();
	}
}

extern void CollectEffectsThatUsesComponent( CComponent* component, CEntityTemplate* entityTemplate, TDynArray< CFXDefinition* >& usedEffects );

String GetAiBaseTreeMenuName( String aiBaseTreeName )
{
	return String( TXT("Custom tree: \"") ) + aiBaseTreeName + String( TXT("\"") );
}

//////////////////////////////////////////////////////////////////////


String GetComponentTypeString( const CComponent* component )
{
	String type = component->HasFlag( NF_IncludedFromTemplate ) ? TXT("Included") : TXT("Local");
	if ( component->HasComponentFlag( CF_UsedInAppearance ) ) type += TXT(", Appearance");
	if ( component->HasComponentFlag( CF_StreamedComponent ) ) type += TXT(", Streamed");
	return type;
}

String GetResourceText( const CComponent* component )
{
	TDynArray< const CResource* > resources;
	component->GetResource( resources );

	String resourceText;
	for ( const CResource* resource : resources )
	{
		if ( resource != nullptr && resource->GetFile() != nullptr )
		{
			resourceText += ( resourceText.Empty() ? String::EMPTY : TXT(", ") ) + resource->GetFile()->GetDepotPath();
		}
	}
	return resourceText;
}

Bool CompareByName( const CComponent* a, const CComponent* b )
{
	return Red::StringCompare( a->GetName().AsChar(), b->GetName().AsChar() ) < 0;
}

Bool CompareByClass( const CComponent* a, const CComponent* b )
{
	return Red::StringCompare( a->GetClass()->GetName().AsChar(), b->GetClass()->GetName().AsChar() ) < 0;
}

Bool CompareByType( const CComponent* a, const CComponent* b )
{
	return Red::StringCompare( GetComponentTypeString( a ).AsChar(), GetComponentTypeString( b ).AsChar() ) < 0;
}

Bool CompareByResources( const CComponent* a, const CComponent* b )
{
	return Red::StringCompare( GetResourceText( a ).AsChar(), GetResourceText( b ).AsChar() ) < 0;
}

Bool CompareByAutohideDistance( const CComponent* a, const CComponent* b )
{
	return a->GetAutoHideDistance() < b->GetAutoHideDistance();
}

class CEntityComponentsDialog : public wxDialog
{
private:
	enum EColumns
	{
		EC_Name,
		EC_Class,
		EC_Type,
		EC_Resources,
		EC_Autohide
	};

	std::function< Bool( const CComponent* a, CComponent* b )> m_comparer;

public:
	wxTextCtrl*					m_filterText;
	wxChoice*					m_classChoice;
	wxTextCtrl*					m_resourceText;
	wxCheckBox*					m_localCheck;
	wxCheckBox*					m_includedCheck;
	wxCheckBox*					m_appearanceCheck;
	wxCheckBox*					m_streamedCheck;
	wxListCtrl*					m_componentsList;
	THandle< CEntity >			m_entity;
	TDynArray< CComponent* >	m_visibleComponents;
	CEdEntityEditor*			m_editor;
	TDynArray< CClass* >		m_classes;

	CEntityComponentsDialog( CEntity* entity, CEdEntityEditor* editor, wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Entity List"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 479,385 ), long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL )
	{
		m_comparer = CompareByName;

		wxXmlResource::Get()->LoadDialog( this, parent, wxT("EntityComponentsDialog") );

		m_entity = entity;
		m_editor = editor;

		m_filterText = XRCCTRL( *this, "m_filterText", wxTextCtrl );
		m_classChoice = XRCCTRL( *this, "m_classChoice", wxChoice );
		m_resourceText = XRCCTRL( *this, "m_resourceText", wxTextCtrl );
		m_localCheck = XRCCTRL( *this, "m_localCheck", wxCheckBox );
		m_includedCheck = XRCCTRL( *this, "m_includedCheck", wxCheckBox );
		m_appearanceCheck = XRCCTRL( *this, "m_appearanceCheck", wxCheckBox );
		m_streamedCheck = XRCCTRL( *this, "m_streamedCheck", wxCheckBox );
		m_componentsList = XRCCTRL( *this, "m_componentsList", wxListCtrl );

		// Find used classes
		for ( CComponent* component : entity->GetComponents() )
		{
			m_classes.PushBackUnique( component->GetClass() );
		}
		::Sort( m_classes.Begin(), m_classes.End(), []( CClass* a, CClass* b ) { 
			return Red::System::StringCompare( a->GetName().AsChar(), b->GetName().AsChar() ) < 0;
		} );

		// Prepare classes choice
		m_classChoice->AppendString( wxT("Any class") );
		for ( CClass* cls : m_classes )
		{
			m_classChoice->AppendString( cls->GetName().AsChar() );
		}
		m_classChoice->SetSelection( 0 );

		// Prepare component list
		m_componentsList->AppendColumn( wxT("Name"), 0, 180 );
		m_componentsList->AppendColumn( wxT("Class"), 0, 150 );
		m_componentsList->AppendColumn( wxT("Type"), 100 );
		m_componentsList->AppendColumn( wxT("Resource"), 100 );
		m_componentsList->AppendColumn( wxT("Autohide"), 100 );

		m_filterText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_classChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_resourceText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_localCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_includedCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_appearanceCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_streamedCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnUpdateComponents ), NULL, this );
		m_componentsList->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( CEntityComponentsDialog::OnSortByColumn ), NULL, this );
		XRCCTRL( *this, "m_selectButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnSelect ), NULL, this );
		XRCCTRL( *this, "m_hideButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnHide ), NULL, this );
		XRCCTRL( *this, "m_isolateButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnIsolate ), NULL, this );
		XRCCTRL( *this, "m_selectAllButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnSelectAll ), NULL, this );
		XRCCTRL( *this, "m_cancelButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntityComponentsDialog::OnCancel ), NULL, this );

		SetEscapeId( XRCCTRL( *this, "m_cancelButton", wxButton )->GetId() );

		UpdateComponentsNow();
	}

	~CEntityComponentsDialog()
	{
	}

	TDynArray< CComponent* > GetSelectedComponents() const 
	{
		TDynArray< CComponent* > components;
		for ( int i=0; i < m_componentsList->GetItemCount(); ++i )
		{
			if ( m_componentsList->GetItemState( i, wxLIST_STATE_SELECTED ) == wxLIST_STATE_SELECTED )
			{
				components.PushBack( m_visibleComponents[i] );
			}
		}
		return components;
	}

	bool PassesFilter( CComponent* component )
	{
		// Filter name
		if ( !m_filterText->GetValue().IsEmpty() &&
			 !component->GetName().ContainsSubstring( m_filterText->GetValue().wc_str() ) )
		{
			return false;
		}

		// Filter class
		if ( m_classChoice->GetSelection() > 0 && !component->GetClass()->IsA( m_classes[m_classChoice->GetSelection() - 1] ) )
		{
			return false;
		}

		// Filter resource
		if ( !m_resourceText->GetValue().IsEmpty() )
		{
			Bool found = false;
			TDynArray< const CResource* > resources;
			component->GetResource( resources );
			for ( const CResource* resource : resources )
			{
				if ( resource != nullptr && resource->GetFile() != nullptr && resource->GetFile()->GetDepotPath().ContainsSubstring( m_resourceText->GetValue().wc_str() ) )
				{
					found = true;
					break;
				}
			}

			// Fail if the resource wasn't found
			if ( !found )
			{
				return false;
			}
		}

		// Filter type
		if ( !m_localCheck->GetValue() && !( component->HasFlag( NF_IncludedFromTemplate ) || component->HasComponentFlag( CF_UsedInAppearance ) ) ) return false;
		if ( !m_includedCheck->GetValue() && component->HasFlag( NF_IncludedFromTemplate ) ) return false;
		if ( !m_appearanceCheck->GetValue() && component->HasComponentFlag( CF_UsedInAppearance ) ) return false;
		if ( !m_streamedCheck->GetValue() && component->HasComponentFlag( CF_StreamedComponent ) ) return false;

		// Passed filters
		return true;
	}

	void FillComponentsList( const TDynArray< CComponent* >& components )
	{
		long id = 0;
		m_visibleComponents.Clear();
		for ( Uint32 i=0; i < components.Size(); ++i )
		{
			CComponent* component = components[i];

			// Filter components
			if ( !PassesFilter( component ) ) continue;

			// Record visible component
			m_visibleComponents.PushBack( component );
		}

		::Sort( m_visibleComponents.Begin(), m_visibleComponents.End(), m_comparer );

		for ( Uint32 i = 0; i < m_visibleComponents.Size(); ++i )
		{
			CComponent* component = m_visibleComponents[i];

			// Add new item
			wxListItem item;
			item.SetId( id );
			item.SetText( component->GetName().AsChar() );
			item.SetData( id );
			m_componentsList->InsertItem( item );

			// Set columns
			m_componentsList->SetItem( id, 0, component->GetName().AsChar() );
			m_componentsList->SetItem( id, 1, component->GetClass()->GetName().AsChar() );			
			m_componentsList->SetItem( id, 2, GetComponentTypeString( component ).AsChar() );			
			m_componentsList->SetItem( id, 3, GetResourceText( component ).AsChar() );
			m_componentsList->SetItem( id, 4, wxString::Format( wxT("%0.2f"), (float)component->GetAutoHideDistance() ) );

			++id;
		}
	}

	void UpdateComponentsNow()
	{
		m_componentsList->Freeze();
		m_componentsList->DeleteAllItems();

		if ( m_entity.IsValid() )
		{
			FillComponentsList( m_entity.Get()->GetComponents() );

			// Always select the first item so it can be easy to use by Ctrl+F -> type -> Enter
			if ( m_componentsList->GetItemCount() > 0 )
			{
				m_componentsList->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
		}

		m_componentsList->Thaw();
	}

	void OnSortByColumn( wxListEvent& event )
	{
		switch( event.GetColumn() )
		{
		case EC_Name:
			m_comparer = CompareByName;
			break;
		case EC_Class:
			m_comparer = CompareByClass;
			break;
		case EC_Type:
			m_comparer = CompareByType;
			break;
		case EC_Resources:
			m_comparer = CompareByResources;
			break;
		case EC_Autohide:
			m_comparer = CompareByAutohideDistance;
			break;
		}
		RunLaterOnce( [&]() { UpdateComponentsNow(); } );
	}
	
	void OnUpdateComponents( wxCommandEvent& event )
	{
		RunLaterOnce( [&]() { UpdateComponentsNow(); } );
	}
	
	void OnSelect( wxCommandEvent& event )
	{
		m_editor->SelectComponents( GetSelectedComponents() );
		EndModal( 0 );
	}

	void OnHide( wxCommandEvent& event )
	{
		m_editor->HideComponents( GetSelectedComponents() );
		EndModal( 0 );
	}

	void OnIsolate( wxCommandEvent& event )
	{
		m_editor->IsolateComponents( GetSelectedComponents() );
		EndModal( 0 );
	}

	void OnSelectAll( wxCommandEvent& event )
	{
		for ( int i=0; i < m_componentsList->GetItemCount(); ++i )
		{
			m_componentsList->SetItemState( i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
		}
	}

	void OnCancel( wxCommandEvent& event )
	{
		EndModal( 0 );
	}
};

//////////////////////////////////////////////////////////////////////

CEdEntityEditor::CEdEntityEditor( wxWindow* parent, CEntityTemplate* entityTemplate )
	: wxSmartLayoutPanel( parent, TXT("EntityEditor"), false )
	, m_template( entityTemplate )
	, m_notebook( nullptr )
	, m_entityPanel( nullptr )
	, m_preview( nullptr )
	, m_hackNeedsSplitBarResize( true )
	, m_isInitialized( false )
	, m_voicetagAppearanceGrid( nullptr )
	, m_currentStreamingLOD( -1 )
	, m_gameplayParamEdit( nullptr )
	, m_mimicsPanel( nullptr )
	, m_undoManager( nullptr )
{
	m_dataErrors.ClearFast();

	// If the template entity object is NULL - it's probably been removed from the code or scripts.
	if ( m_template->GetEntityObject() == NULL )
	{
		wxTheFrame->PauseConfigTimer();
		// Inform the user that this has occurred, and that we'll be making the file usable.
		wxMessageBox( wxT( "Unable to edit selected entity template. It's Entity object is NULL - the entity object will be set to CEntity, and resaved! Check the log for warnings about missing classes."), wxT("Error"), wxICON_WARNING|wxOK );
		m_template->SetEntityClass( CEntity::GetStaticClass() );
		m_template->Save();
		wxTheFrame->ResumeConfigTimer();
	}

	m_template->AddToRootSet();
	
	::CollectAllIncludes( m_template, m_loadedIncludes );

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s - %s"), m_template->GetFile()->GetFileName().AsChar(), m_template->GetFriendlyName().AsChar(), title.AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_ENTITY") ) );
	SetIcon( iconSmall );


	m_freezeEntityPose.Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnFreezePose ), nullptr, this );
	m_freezeEntityPose.AddValue( false, SEdResources::GetInstance().LoadBitmap( TXT("IMG_ASTERISK_DISABLE") ) );
	m_freezeEntityPose.AddValue( true, SEdResources::GetInstance().LoadBitmap( TXT("IMG_ASTERISK") ) );

	m_previewItemSize.Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnTogglePreviewItemSize ), nullptr, this );
	m_previewItemSize.AddValue( IPreviewItem::PS_Small, SEdResources::GetInstance().LoadBitmap( TXT("IMG_COMPONENT_SMALL") ) );
	m_previewItemSize.AddValue( IPreviewItem::PS_Normal, SEdResources::GetInstance().LoadBitmap( TXT("IMG_COMPONENT") ) );

	// Menu
	{
		GetMenuBar()->FindItem( XRCID( "viewDisableAppearanceApplication" ) )->Check( GDisableAppearances );
	}

	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_preview = new CEdEntityPreviewPanel( rp, entityTemplate, this );
		m_preview->m_widgetManager->SetWidgetSpace( RPWS_Local );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		Connect( XRCID("widgetModeMove"),   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnWidgetSelected), NULL, m_preview );
		Connect( XRCID("widgetModeRotate"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnWidgetSelected), NULL, m_preview );
		Connect( XRCID("widgetModeScale"),  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnWidgetSelected), NULL, m_preview );
		Connect( XRCID("widgetModeChange"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnWidgetSelected), NULL, m_preview );

		OnPreviewWidgetModeChanged();

		Connect( XRCID("widgetPivotLocal"),	 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnToggleWidgetSpace), NULL, m_preview );
		Connect( XRCID("widgetPivotGlobal"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityPreviewPanel::OnToggleWidgetSpace), NULL, m_preview );

		OnPreviewWidgetSpaceChanged();

		Connect( XRCID("granularityEntities"),   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnGranularityChanged), NULL, this );
		Connect( XRCID("granularityComponents"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnGranularityChanged), NULL, this );

		OnGranularityChanged( wxCommandEvent() );
	}

	{
		// Initialize undo manager
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetWorld( m_preview->GetWorld() );
		m_preview->SetUndoManager( m_undoManager );

		wxMenuBar* gameMenu = GetMenuBar();
		m_undoManager->SetMenuItems( gameMenu->FindItem( XRCID("editUndo") ), gameMenu->FindItem( XRCID("editRedo") ) );
	}

	// Entity right panel
	{
		wxPanel* rp = XRCCTRL( *this, "RightPanel", wxPanel );
		rp->SetMinSize( wxSize(0, 0) );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_notebook = new wxAuiNotebook( rp, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE );
		m_notebook->SetWindowStyle( wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS );
		m_notebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( CEdEntityEditor::OnNotebookPageChanged ), NULL, this );
		m_notebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( CEdEntityEditor::OnNotebookPageClosed ), NULL, this );
		sizer1->Add( m_notebook , 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		m_entityPanel = wxXmlResource::Get()->LoadPanel( m_notebook, TEXT("EntityPanel") );
		m_notebook->AddPage( m_entityPanel, TEXT("Main"), true );

		m_propertiesNotebook = XRCCTRL( *this, "PropertiesTab", wxNotebook );
		m_propertiesNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CEdEntityEditor::OnPropertiesNotebookPageChanged ), NULL, this );

		// Template parameters
		m_edbClassName				= XRCCTRL( *this, "EntityClassName", wxTextCtrl );
		m_btnChangeEntityClass		= XRCCTRL( *this, "btnPickEntityClass", wxButton );
		m_btnChangeEntityClass->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnChangeEntityClass), NULL, this );
		m_btnAddInclude				= XRCCTRL( *this, "btnAddInclude", wxButton );
		m_btnAddInclude->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnIncludeAdd), NULL, this );
		m_btnRemoveInclude			= XRCCTRL( *this, "btnRemoveInclude", wxButton );
		m_btnRemoveInclude->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnIncludeRemove), NULL, this );
		m_chkShowTemplatesOnly		= XRCCTRL( *this, "chkShowTemplatesOnly", wxCheckBox );
		m_chkShowTemplatesOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnDependencyFilterChanged), NULL, this );
		m_lstIncludes				= XRCCTRL( *this, "IncludesList", wxListBox );
		m_lstIncludes->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnIncludeClicked), NULL, this );
		m_lstDependencies				= XRCCTRL( *this, "DependenciesList", wxListBox );
		m_lstDependencies->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnDependencyClicked), NULL, this );

		// Instance parameters
		m_instancePropertiesList	= XRCCTRL( *this, "m_instancePropertiesList", wxListCtrl );
		XRCCTRL( *this, "m_addInstancePropertiesEntryButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAddInstancePropertiesEntryClicked), NULL, this );
		XRCCTRL( *this, "m_removeInstancePropertiesEntryButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnRemoveInstancePropertiesEntryClicked), NULL, this );
		XRCCTRL( *this, "m_removeComponentInstancePropertiesEntries", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnRemoveComponentInstancePropertiesEntriesClicked), NULL, this );
		XRCCTRL( *this, "m_removeAllInstancePropertiesEntries", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnRemoveAllInstancePropertiesEntriesClicked), NULL, this );

		// Appearances tab
		m_lstAppearances          = XRCCTRL( *this, "m_lstAppearances", wxCheckListBox );
		m_lstAppearances          ->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceSelected), NULL, this );
		m_lstAppearances          ->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceDoubleClicked), NULL, this );
		m_lstAppearances          ->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceToggled), NULL, this );
		m_lstAppearances		  ->Connect( wxEVT_CONTEXT_MENU, wxContextMenuEventHandler( CEdEntityEditor::OnActiveAppearancesListContextMenu ), NULL, this );
		m_lstAppearanceBodyParts  = XRCCTRL( *this, "m_lstAppearanceBodyParts", wxCheckListBox );
		m_lstAppearanceBodyParts  ->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceBodyPartToggled), NULL, this );
		m_lstAppearanceBodyParts  ->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnIncludeClicked), NULL, this );

		Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityEditor::OnActiveAppearancesListMenuExport, this, ID_ACTAPPMENU_EXPORT );
		Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityEditor::OnActiveAppearancesListMenuImport, this, ID_ACTAPPMENU_IMPORT );

		m_btnDuplicateAppearance  = XRCCTRL( *this, "m_btnDuplicateAppearance", wxButton );
		m_btnDuplicateAppearance  ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceDuplicate), NULL, this );
		m_btnAddAppearance        = XRCCTRL( *this, "m_btnAddAppearance", wxButton );
		m_btnAddAppearance        ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceAdd), NULL, this );
		m_btnDelAppearance        = XRCCTRL( *this, "m_btnDelAppearance", wxButton );
		m_btnDelAppearance        ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceDel), NULL, this );

		m_txtAppearancesFilter    = XRCCTRL( *this, "m_txtAppearancesFilter", wxTextCtrl );
		m_txtAppearancesFilter    ->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CEdEntityEditor::OnAppearancesFilterChanged), NULL, this );
		m_txtAppearanceBodyPartsFilter = XRCCTRL( *this, "m_txtAppearanceBodyPartsFilter", wxTextCtrl );
		m_txtAppearanceBodyPartsFilter ->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceBodyPartsFilterChanged), NULL, this );

		m_usesRobe = XRCCTRL( *this, "m_robedAppearance", wxCheckBox );
		m_usesRobe->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnUsesRobeToggled ), NULL, this );

		wxButton* btnHeadN = XRCCTRL( *this, "btnHeadN", wxButton );
		btnHeadN->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnHeadStateNormal ), NULL, this );
		wxButton* btnHeadL = XRCCTRL( *this, "btnHeadL", wxButton );
		btnHeadL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnHeadStateMimicLow ), NULL, this );
		wxButton* btnHeadH = XRCCTRL( *this, "btnHeadH", wxButton );
		btnHeadH->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnHeadStateMimicHigh ), NULL, this );
		wxButton* btnHeadT = XRCCTRL( *this, "btnHeadTest", wxButton );
		btnHeadT->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnHeadSpeechTest ), NULL, this );
		wxButton* btnHeadS = XRCCTRL( *this, "btnHeadSpeech", wxButton );
		btnHeadS->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnHeadSpeechSelect ), NULL, this );

		m_btnAddAppearanceTemplate = XRCCTRL( *this, "btnAddAppearanceTemplate", wxButton);
		m_btnAddAppearanceTemplate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceTemplateAdded ), NULL, this );
		m_btnRemoveAppearanceTemplate = XRCCTRL( *this, "btnRemoveAppearanceTemplate", wxButton );
		m_btnRemoveAppearanceTemplate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceTemplateRemoved ), NULL, this );
		m_btnToggleAppearanceCollapse = XRCCTRL( *this, "btnToggleAppearanceCollapse", wxButton );
		m_btnToggleAppearanceCollapse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnAppearanceTemplateCollapseToggled), NULL, this );
		m_btnCopyAppearanceColoringEntries = XRCCTRL( *this, "m_btnCopyAppsAndEntries", wxButton );
		m_btnCopyAppearanceColoringEntries->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnCopyAppearanceColoringEntries), NULL, this );
		m_btnPasteAppearanceColoringEntries = XRCCTRL( *this, "m_btnPasteAppsAndEntries", wxButton );
		m_btnPasteAppearanceColoringEntries->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdEntityEditor::OnPasteAppearanceColoringEntries), NULL, this );

		// Colors tab
		m_appearancesChoice				 = XRCCTRL( *this, "ColorAppearanceChoice", wxChoice );
		m_appearancesChoice				 ->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnColorAppearanceSelected ), NULL, this );
		m_duplicateMeshColoringButton	 = XRCCTRL( *this, "DuplicateMeshColoringButton", wxButton );
		m_duplicateMeshColoringButton	 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnDuplicateMeshColorButtonClick ), NULL, this );
		m_addMeshColoringButton          = XRCCTRL( *this, "AddMeshColoringButton", wxButton );
		m_addMeshColoringButton			 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAddMeshColorButtonClick ), NULL, this );
		m_removeMeshColoringButton       = XRCCTRL( *this, "RemoveMeshColoringButton", wxButton );
		m_removeMeshColoringButton		 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnRemoveMeshColorButtonClick ), NULL, this );
		m_meshColoringEntriesList        = XRCCTRL( *this, "MeshColoringEntriesList", wxListBox );
		m_meshColoringEntriesList		 ->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntriesListSelected ), NULL, this );
		m_meshColoringEntriesList		 ->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntriesListSelected ), NULL, this );
		m_meshColoringEntryMesh			 = XRCCTRL( *this, "MeshColoringEntryMesh", wxTextCtrl );
		m_primaryColorHueSlider			 = XRCCTRL( *this, "primaryColorHueSlider", wxSlider );
		m_primaryColorHueSpinCtrl		 = XRCCTRL( *this, "primaryColorHueSpinCtrl", wxSpinCtrl );
		m_primaryColorSaturationSlider	 = XRCCTRL( *this, "primaryColorSaturationSlider", wxSlider );
		m_primaryColorSaturationSpinCtrl = XRCCTRL( *this, "primaryColorSaturationSpinCtrl", wxSpinCtrl );
		m_primaryColorLuminanceSlider	 = XRCCTRL( *this, "primaryColorLuminanceSlider", wxSlider );
		m_primaryColorLuminanceSpinCtrl	 = XRCCTRL( *this, "primaryColorLuminanceSpinCtrl", wxSpinCtrl );
		m_secondaryColorHueSlider		 = XRCCTRL( *this, "secondaryColorHueSlider", wxSlider );
		m_secondaryColorHueSpinCtrl		 = XRCCTRL( *this, "secondaryColorHueSpinCtrl", wxSpinCtrl );
		m_secondaryColorSaturationSlider = XRCCTRL( *this, "secondaryColorSaturationSlider", wxSlider );
		m_secondaryColorSaturationSpinCtrl = XRCCTRL( *this, "secondaryColorSaturationSpinCtrl", wxSpinCtrl );
		m_secondaryColorLuminanceSlider	 = XRCCTRL( *this, "secondaryColorLuminanceSlider", wxSlider );
		m_secondaryColorLuminanceSpinCtrl = XRCCTRL( *this, "secondaryColorLuminanceSpinCtrl", wxSpinCtrl );
		m_primaryColorHueSlider			 ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_primaryColorHueSpinCtrl		 ->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_primaryColorSaturationSlider	 ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_primaryColorSaturationSpinCtrl ->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_primaryColorLuminanceSlider	 ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_primaryColorLuminanceSpinCtrl	 ->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorHueSlider		 ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorHueSpinCtrl		 ->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorSaturationSlider ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorSaturationSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorLuminanceSlider	 ->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_secondaryColorLuminanceSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMeshColoringEntryParametersChanged ), NULL, this );
		m_copyColorsButton				 = XRCCTRL( *this, "CopyColorsBtn", wxButton );
		m_copyColorsButton				 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnCopyColorsButtonClicked ), NULL, this );
		m_pasteColorsButton				 = XRCCTRL( *this, "PasteColorsBtn", wxButton );
		m_pasteColorsButton				 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnPasteColorsButtonClicked ), NULL, this );
		m_swapColorsButton				 = XRCCTRL( *this, "SwapColorsBtn", wxButton );
		m_swapColorsButton				 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSwapColorsButtonClicked ), NULL, this );
		m_swapAllColorsButton			 = XRCCTRL( *this, "SwapAllBtn", wxButton );
		m_swapAllColorsButton			 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSwapAllColors ), NULL, this );
		m_pasteAllColorsButton			 = XRCCTRL( *this, "PasteAllBtn", wxButton );
		m_pasteAllColorsButton			 ->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnPasteAllColors ), NULL, this );

		//Effects tab
		m_lstAnimations					= XRCCTRL( *this, "m_lstAnimations", wxListBox );
		m_lstAnimations					->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnAnimationsSelected), NULL, this );
		m_lstAnimations					->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnAnimationsDoubleClicked), NULL, this );
		m_lstEffects					= XRCCTRL( *this, "m_lstEffects", wxListBox );
		m_lstEffects					->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnEffectsSelected), NULL, this );
		m_lstEffects					->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CEdEntityEditor::OnEffectsDoubleClicked), NULL, this );
		m_btnConnect					= XRCCTRL( *this, "m_btnConnect", wxButton );
		m_btnConnect					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectConnect ), NULL, this );
		m_btnDisconnect					= XRCCTRL( *this, "m_btnDisconnect", wxButton );
		m_btnDisconnect					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectDisconnect ), NULL, this );
		m_btnAddEffect					= XRCCTRL( *this, "m_btnAddEffect", wxButton );
		m_btnAddEffect					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectAdd ), NULL, this );
		m_btnEditEffect					= XRCCTRL( *this, "m_btnEditEffect", wxButton );
		m_btnEditEffect					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectEdit ), NULL, this );
		m_btnRenameEffect				= XRCCTRL( *this, "m_btnRenameEffect", wxButton );
		m_btnRenameEffect				->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectRename ), NULL, this );
		m_btnRemoveEffect				= XRCCTRL( *this, "m_btnRemoveEffect", wxButton );
		m_btnRemoveEffect				->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectRemove ), NULL, this );
		m_btnCopyEffect					= XRCCTRL( *this, "m_btnCopyEffect", wxButton );
		m_btnCopyEffect					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectCopy ), NULL, this );
		m_btnPasteEffect				= XRCCTRL( *this, "m_btnPasteEffect", wxButton );
		m_btnPasteEffect				->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectPaste ), NULL, this );
		m_btnRefreshAnims				= XRCCTRL( *this, "m_btnRefreshAnims", wxButton );
		m_btnRefreshAnims				->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnEffectRefreshList ), NULL, this );
		m_soundOverrideParamsCheck		= XRCCTRL( *this, "soundOverrideCheck", wxCheckBox );
		m_soundOverrideParamsCheck		->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSoundOverrideParams ), NULL, this );

		// Sound params properties
		{
			wxPanel* rp = XRCCTRL( *this, "soundParamsPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_soundParamsProps = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_soundParamsProps, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}

		// Slots
		m_lstSlots						= XRCCTRL( *this, "SlotList", wxListBox );
		m_lstSlots						->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnSlotSelectionChanged), NULL, this );
		m_btnAddSlots					= XRCCTRL( *this, "btnAddSlot", wxButton );
		m_btnAddSlots					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSlotAdd ), NULL, this );
		m_btnRemoveSlots				= XRCCTRL( *this, "btnRemoveSlot", wxButton );
		m_btnRemoveSlots				->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSlotRemove ), NULL, this );
		wxBitmapButton* btnForceTPose	= XRCCTRL( *this, "btnForceTPose", wxBitmapButton );
		btnForceTPose					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnForceTPose ), NULL, this );

		wxBitmapButton* btnOverrideSlot	= XRCCTRL( *this, "btnOverrideSlot", wxBitmapButton );
		btnOverrideSlot					->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSlotOverride ), NULL, this );

		m_showSlots.Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSlotShow ), nullptr, this );
		m_showSlots.AddValue( false, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE_CLOSED") ) );
		m_showSlots.AddValue( true, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE") ) );
		m_showSlots.AddButton( XRCCTRL( *this, "btnShowSlots", wxBitmapButton ) );

		m_freezeEntityPose.AddButton( XRCCTRL( *this, "btnFreezeAnim", wxBitmapButton ) );
		m_previewItemSize.AddButton( XRCCTRL( *this, "btnSlotSize", wxBitmapButton ) );

		// Select "Node properties" tab.
		m_propertiesNotebook->SetSelection( 0 );
	}

	// Create tools
	{
		// Tools panel
		m_toolsPanel = new CEdToolsPanel( m_notebook, m_preview );
		m_notebook->AddPage( m_toolsPanel, TEXT("Tools"), false );

		// Mimics control rig panel
		if ( m_template && m_template->GetEntityObject() && m_template->GetEntityObject()->IsA< CActor >() )
		{
			wxPanel* mimicsControlRigPanel = new wxPanel( m_notebook );
			mimicsControlRigPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

			m_mimicsPanel = new CEdEntityEditorMimicsPanel( mimicsControlRigPanel );

			wxBoxSizer* midSizer = new wxBoxSizer( wxHORIZONTAL );
			{
				wxToggleButton* button = new wxToggleButton( mimicsControlRigPanel, wxID_ANY, wxT("Activate") );
				button->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnMimicPanelActivated ), NULL, this );
				midSizer->Add( button, 3, wxALL, 5 );

				wxStaticText* staticText = new wxStaticText( mimicsControlRigPanel, wxID_ANY, wxT("FOV:") );
				midSizer->Add( staticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

				wxSpinCtrl* spinCtrl = new wxSpinCtrl( mimicsControlRigPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 70 );
				spinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMimicPanelFovChanged ), NULL, this );
				spinCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdEntityEditor::OnMimicPanelFovChanged ), NULL, this );
				midSizer->Add( spinCtrl, 1, wxALL, 5 );
			}

			mimicsControlRigPanel->GetSizer()->Add( midSizer, 0, wxEXPAND, 5 );
			mimicsControlRigPanel->GetSizer()->Add( m_mimicsPanel, 1, wxEXPAND );

			m_mimicsPanel->SetEntity( m_preview->GetEntity() );
			m_notebook->AddPage( mimicsControlRigPanel, TEXT("Mimics"), false );
		}
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		settings.m_showEntityComponents = false;
		m_properties = new CEdSelectionProperties( rp, settings, m_undoManager, m_preview->GetWorld() );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
		m_properties->SetEntityEditorAsOwner();

		// We will process selections our way (use CComponent granularity in the CSelectionManager,
		// but allow to edit CEntity properties in the CEdSelectionProperties too)
		SEvents::GetInstance().UnregisterListener( CNAME( SelectionChanged ), m_properties );
	}

	// Create slot properties
	{
		wxPanel* rp = XRCCTRL( *this, "SlotPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_slotProperties = new CEdEntitySlotProperties( this, rp, m_undoManager );
		m_slotProperties ->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnSlotModified ), NULL, this );		
		sizer1->Add( m_slotProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Graph editor
	{
		wxPanel* rp = XRCCTRL( *this, "GraphPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_graph = new CEdEntityGraphEditor( rp, m_preview->GetEntity(), m_template, m_undoManager, this, this );
		sizer1->Add( m_graph, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// AI tab
	{
		// reactions page
		{
			m_aiProfileNotebook = XRCCTRL( *this, "aiProfileNotebook", wxNotebook );
			m_aiProfileNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CEdEntityEditor::OnAiProfileNotebookPageChanged ), NULL, this );

			m_reactionsList = XRCCTRL( *this, "reactionsList", wxListBox );
			m_reactionsList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CEdEntityEditor::OnReactionSelected), NULL, this );

			m_addReaction = XRCCTRL( *this, "addReaction", wxBitmapButton );
			m_addReaction->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAddReaction ), NULL, this );	
			m_removeReaction = XRCCTRL( *this, "removeReaction", wxBitmapButton );
			m_removeReaction->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnRemoveReaction ), NULL, this );	

			m_moveReactionUp = XRCCTRL( *this, "moveReactionUp", wxBitmapButton );
			m_moveReactionUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnMoveReactionUp ), NULL, this );	
			m_moveReactionDown = XRCCTRL( *this, "moveReactionDown", wxBitmapButton );
			m_moveReactionDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnMoveReactionDown ), NULL, this );	

			wxPanel* rp = XRCCTRL( *this, "reactionProperties", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			settings.m_showEntityComponents = false;
			m_reactionProperties = new CEdSelectionProperties( rp, settings, m_undoManager );
			m_reactionProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnReactionChanged ), NULL, this );
			sizer1->Add( m_reactionProperties, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
			m_reactionProperties->SetEntityEditorAsOwner();
		}
		// AI Tree tab
		{	
			m_aiTreeEditChoice = XRCCTRL( *this, "aiTreeEditChoice",	wxChoice );

			m_aiTreeEditChoice->AppendString( AITREE_SELECTION_INHERIT );

			if ( GetAIWizardResource() )
			{
				m_aiTreeEditChoice->AppendString( AITREE_SELECTION_USEAIWIZARD );
			}
			TDynArray< CClass* > supportedClasses;
			SRTTI::GetInstance().EnumDerivedClasses( CAIBaseTree::GetStaticClass() , supportedClasses );
			for ( Uint32 i = 0, classCount = supportedClasses.Size(); i < classCount; ++i )
			{
				m_aiTreeEditChoice->AppendString( GetAiBaseTreeMenuName( supportedClasses[ i ]->GetName().AsString() ).AsChar() );
			}
			m_aiTreeEditChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnAITreeSetupChanged ), NULL, this );
			
			m_runAIWizardButton = XRCCTRL( *this, "runAIWizard",	wxButton );
			m_runAIWizardButton->Connect(	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdEntityEditor::OnRunAIWizard ), NULL, this );

			m_aiWizardSummaryPanel	= XRCCTRL( *this, "aiWizardSummaryPanel",	wxPanel );
			m_aiWizardSummaryGrid	= XRCCTRL( *this, "aiWizardSummaryGrid",	wxGrid );
			m_aiWizardSummaryGrid->CreateGrid( 1, 2 );
	
			wxPanel* rp = XRCCTRL( *this, "aiTreeProperties", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			settings.m_showEntityComponents = false;
			m_aiTreeProperties = new CEdSelectionProperties( rp, settings, m_undoManager );
			m_aiTreeProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnAiTreeChanged ), NULL, this );
			sizer1->Add( m_aiTreeProperties, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
			m_aiTreeProperties->SetEntityEditorAsOwner();

			FillAiTreePanels( );
		}
		// Senses tab
		{
			wxPanel* rp = XRCCTRL( *this, "sensePropertiesPanel", wxPanel );
			ASSERT( rp );

			m_senseChoice = XRCCTRL( *this, "senseChoice", wxChoice );
			ASSERT( m_senseChoice );
			m_senseChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnSenseTypeChanged ), NULL, this );

			m_senseInheritCheckBox = XRCCTRL( *this, "senseInheritCheckBox", wxCheckBox );
			m_senseInheritCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnSenseInheritClicked ), NULL, this );

			m_senseTemplateStaticText = XRCCTRL( *this, "senseInheritTemplate", wxStaticText );

			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			
			PropertiesPageSettings settings;
			settings.m_showEntityComponents = false;
			m_senseProperties = new CEdSelectionProperties( rp, settings, m_undoManager );
			m_senseProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnSensePropertiesChanged ), NULL, this );
			sizer1->Add( m_senseProperties, 1, wxEXPAND | wxALL, 0 );

			rp->SetSizer( sizer1 );
			rp->Layout();
			m_senseProperties->SetEntityEditorAsOwner();

			FillSenseProperties( true );
			CacheSenseParams( AIST_Vision );
			CacheSenseParams( AIST_Absolute );
		}

		// Misc tab
		{			
			m_attitudeGroup = XRCCTRL( *this, "choiceAttitudeGroup", wxChoice );
			m_attitudeGroup->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnAttitudeGroupChoice ), NULL, this );
			m_attitudeGroupApplied = XRCCTRL( *this, "staticTextAttitudeGroupApplied", wxStaticText );
			FillAttitudeGroupChoice();

			m_aiWizardResName = XRCCTRL( *this, "aiWizardResName", wxStaticText );
			m_aiWizardResButton = XRCCTRL( *this, "aiWizardResButton", wxButton );
			m_aiWizardResButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnUseAiWizardRes ), NULL, this );
			FillAiWizardResData();
		}

		// Minigames parameters tab
		{
			m_minigameWWEnable = XRCCTRL( *this, "checkBoxMWWEnable", wxCheckBox );
			m_minigameWWHotSpotMinWidth = XRCCTRL( *this, "textCtrlMWWHotSpotMinWidth", wxTextCtrl );
			m_minigameWWHotSpotMaxWidth = XRCCTRL( *this, "textCtrlMWWHotSpotMaxWidth", wxTextCtrl );
			m_minigameWWDifficulty = XRCCTRL( *this, "choiceMWWDifficulty", wxChoice );
			m_minigameWWStatusText = XRCCTRL( *this, "staticTextMWWCurrParams", wxStaticText );
			m_minigameWWEnable->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnMinigameWWEnable ), NULL, this );
			m_minigameWWHotSpotMinWidth->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdEntityEditor::OnMinigameWWHotSpotMinWidth ), NULL, this );
			m_minigameWWHotSpotMaxWidth->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdEntityEditor::OnMinigameWWHotSpotMaxWidth ), NULL, this );
			m_minigameWWDifficulty->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnMinigameWWDifficulty ), NULL, this );
			UpdateMinigameTabGui();
		}
	}

	// Inventory tab
	{
		m_itemsList = XRCCTRL( *this, "m_itemsListBox", wxListBox );
		m_itemsList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnInventoryItemSelected ), NULL, this );

		m_addItem = XRCCTRL( *this, "m_addItemBtn", wxBitmapButton );
		m_addItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAddInventoryItem ), NULL, this );	
		m_removeItem = XRCCTRL( *this, "m_removeItemBtn", wxBitmapButton );
		m_removeItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnRemoveInventoryItem ), NULL, this );	


		wxPanel* rp = XRCCTRL( *this, "m_itemPropsPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		settings.m_showEntityComponents = false;
		m_itemProperties = new CEdSelectionProperties( rp, settings, m_undoManager );
		m_itemProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnInventoryItemChanged ), NULL, this );
		sizer1->Add( m_itemProperties, 1, wxEXPAND | wxALL, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
		m_itemProperties->SetEntityEditorAsOwner();
	}

	// Equipment tab
	{
		m_categoriesList = XRCCTRL( *this, "m_categoriesListBox", wxListBox );
		m_categoriesList->Connect( wxEVT_LEFT_UP, wxMouseEventHandler(CEdEntityEditor::OnEquipmentCategoryClicked), NULL, this );

		m_addCategory = XRCCTRL( *this, "m_addCategoryBtn", wxBitmapButton );
		m_addCategory->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAddEquipmentCategory ), NULL, this );	
		m_removeCategory = XRCCTRL( *this, "m_removeCategoryBtn", wxBitmapButton );
		m_removeCategory->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnRemoveEquipmentCategory ), NULL, this );	


		wxPanel* rp = XRCCTRL( *this, "m_categoryPropsPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		settings.m_showEntityComponents = false;
		m_categoryProperties = new CEdSelectionProperties( rp, settings, m_undoManager );
		m_categoryProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnEquipmentCategoryChanged ), NULL, this );

		sizer1->Add( m_categoryProperties, 1, wxEXPAND | wxALL, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
		m_categoryProperties->SetEntityEditorAsOwner();
	}

	// Heads tab
	{
		m_currActorSpeech.m_id = DEFAULT_ACTOR_SPEECH_ID;
		m_currActorSpeech.m_line = String::EMPTY;
	}

	// Animation tab
	{
		// Global panel
		{
			m_animTabGlobalCheck = XRCCTRL( *this, "animGlobalCheck", wxCheckBox );
			m_animTabGlobalCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimTabGlobalCheck ), NULL, this );

			wxPanel* rp = XRCCTRL( *this, "animGlobalPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_animTabGlobalPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_animTabGlobalPanelProp, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}

		// Look at panel
		{
			m_animTabLookAtCheck = XRCCTRL( *this, "animLookAtCheck", wxCheckBox );
			m_animTabLookAtCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimTabLookAtCheck ), NULL, this );

			wxPanel* rp = XRCCTRL( *this, "animLookAtPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_animTabLookAtPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_animTabLookAtPanelProp, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}

		// Constraint panel
		{
			m_animTabConstCheck = XRCCTRL( *this, "animConstCheck", wxCheckBox );
			m_animTabConstCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimTabConstCheck ), NULL, this );

			wxPanel* rp = XRCCTRL( *this, "animConstPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_animTabConstPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_animTabConstPanelProp, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}

		// Mimic panel
		{
			m_animTabMimicCheck = XRCCTRL( *this, "animMimicCheck", wxCheckBox );
			m_animTabMimicCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimTabMimicCheck ), NULL, this );

			wxPanel* rp = XRCCTRL( *this, "animMimicPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_animTabMimicPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_animTabMimicPanelProp, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}

		ConnectAnimPanelAddRemoveStyle( "panelBehProp", OnAnimTabBehPropModified, "btnAddBeh", OnAnimTabBehAdded, "btnRemoveBeh", OnAnimTabBehRemoved, "BehaviorList", OnAnimTabBehListChanged );
		ConnectAnimPanelAddRemoveStyle( "panelAnimsetProp", OnAnimTabAnimsetPropModified, "btnAddAnimset", OnAnimTabAnimsetAdded, "btnRemoveAnimset", OnAnimTabAnimsetRemoved, "AnimsetList", OnAnimTabAnimsetListChanged );
		
		m_gameplayParamEdit = new CEdGameplayParamEditor( this );
		
	}

	// Animation Slots tab
	{
		wxButton* btnAddAnimSlots = XRCCTRL( *this, "btnAddAnimSlots", wxButton );
		btnAddAnimSlots->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimSlotsAdded ), NULL, this );

		wxButton* btnRemAnimSlots = XRCCTRL( *this, "btnRemAnimSlots", wxButton );
		btnRemAnimSlots->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnAnimSlotsRemoved ), NULL, this );

		m_listAnimSlots = XRCCTRL( *this, "AnimSlotsList", wxListBox );
		m_listAnimSlots->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdEntityEditor::OnAnimSlotsSelectionChanged ), NULL, this );

		wxPanel* rp = XRCCTRL( *this, "AnimSlotsPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_animSetsPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
		m_animSetsPanelProp->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::OnAnimSlotsModified ), NULL, this );
		sizer1->Add( m_animSetsPanelProp, 1, wxEXPAND | wxALL, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Dismemberment tab
	{
		InitDismemberTab();
	}

	// Voicetags
	{
		// Voiceset panel
		{
			m_voiceTabVoicesetCheck = XRCCTRL( *this, "voicesetCheck", wxCheckBox );
			m_voiceTabVoicesetCheck->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdEntityEditor::OnVoiceTabVoicesetCheck ), NULL, this );

			wxPanel* rp = XRCCTRL( *this, "voicesetPanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			m_voiceTabVoicesetPanelProp = new CEdPropertiesPage( rp, settings, m_undoManager );
			sizer1->Add( m_voiceTabVoicesetPanelProp, 1, wxEXPAND | wxALL, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();
		}
	}

	UpdateVoicetagAppearancesTable();

	SoundUpdateParams();

	// Register hooks
	SEvents::GetInstance().RegisterListener( CNAME( EntityTemplateClassChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPostUndoStep ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EntityTemplateModified ), this );
	SEvents::GetInstance().RegisterListener( CNAME( PreviewEntityModified ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );
	SEvents::GetInstance().RegisterListener( CNAME( UpdateEffectList ), this );	
	
	UpdateTabs();

	UpdateAppearanceList();
	UpdateInstancePropertiesList();
	UpdateAnimEffectsList();
	UpdateDynamicTemplate();
	UpdateReactionsList();
	UpdateCategoriesList();
	UpdateInventoryItemsList();
	UpdateSlotList();
	UpdateIncludeList();
	UpdateDependencyList();
	UpdateEntityClass();
	UpdateAnimationTab();
	UpdateAnimSlotsList();
	UpdateDismembermentList();

	OnPreviewEntityLOD0( wxCommandEvent() );

	// Add a ground plane to the physics world, so that destructibles have something to fall on if they are broken.
	Box bounds = m_preview->GetEntity()->CalcBoundingBox();
	CPhysicsWorld* physicsWorld = nullptr;
	m_preview->GetWorld()->GetPhysicsWorld( physicsWorld );
	physicsWorld->AddPlane( bounds.Min.Z );

	if ( m_slotProperties )
	{
		m_preview->SetEntitySlotProperties( m_slotProperties );
	}

	// Send a fake page changed event, so we can setup the initially active page in OnPropertiesNotebookPageChanged().
	{
		wxNotebookEvent fakeEvent( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, 0, m_propertiesNotebook->GetSelection(), -1 );
		fakeEvent.SetEventObject( m_propertiesNotebook );
		OnPropertiesNotebookPageChanged( fakeEvent );
	}

#ifndef NO_DATA_VALIDATION
	if( GDataError != nullptr )
	{
		TDynArray< CResource::DependencyInfo > dependencies;
		entityTemplate->GetDependentResourcesPaths( dependencies, TDynArray< String >() );
		for ( const CResource::DependencyInfo& info : dependencies )
		{
			m_dependentResourcesPaths.PushBack( info.m_path );
		}

		// check data errors for all dependencies
		for ( Uint32 i = 0; i < m_dependentResourcesPaths.Size(); ++i )
		{
			CDiskFile* file = GDepot->FindFileUseLinks( m_dependentResourcesPaths[i], 0 );
			if ( file )
			{
				CResource* res = file->GetResource();
				if ( res )
				{
					res->OnCheckDataErrors();
				}
			}
		}

		SEntityStreamingState state;
		m_preview->GetEntity()->PrepareStreamingComponentsEnumeration( state, true, SWN_DoNotNotifyWorld );
		m_preview->GetEntity()->ForceFinishAsyncResourceLoads();
		m_preview->GetEntity()->OnCheckDataErrors( false );
		m_preview->GetEntity()->FinishStreamingComponentsEnumeration( state, SWN_DoNotNotifyWorld );

		// flush all gathered errors related with loaded resource
		TSortedArray< SDataError > arrayForErrors;
		GDataError->GetCurrentCatchedFilteredForResource( arrayForErrors, m_dependentResourcesPaths );

		for( auto it = arrayForErrors.Begin(); it != arrayForErrors.End(); ++it )
		{
			const SDataError& error = ( *it );
			m_dataErrors.Insert( error );
		}

		// Connect to Data error reporter
		if( GDataError != nullptr )
		{
			GDataError->RegisterListener( this );
		}
	}
#endif

	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();
	Show();

	m_isInitialized = true;
	
	if( IsModifiedByDLC() )
	{
		GFeedback->ShowWarn( TXT("Entity template is modified by DLC - you won't be able to save it.") );
	}
}

Bool CEdEntityEditor::IsModifiedByDLC() const
{
	const String depotPath = m_template->GetDepotPath();
	const Bool modifiedByDlc = SEntityTemplateModifierManager::GetInstance().IsModifierRegistered( depotPath ) || 
							   SDropPhysicsComponentModifierManager::GetInstance().IsModifierRegistered( depotPath ) ||
							   SAppearanceComponentModifierManager::GetInstance().IsModifierRegistered( depotPath );
	return modifiedByDlc;
}

void CEdEntityEditor::OnGranularityChanged( wxCommandEvent &event )
{
	CSelectionManager* selectionManager = m_preview->GetSelectionManager();

	// Modify granularity

	if ( event.GetId() == XRCID("granularityEntities") )
	{
		selectionManager->SetGranularity( CSelectionManager::SG_Entities );
	}
	else if ( event.GetId() == XRCID("granularityComponents") )
	{
		selectionManager->SetGranularity( CSelectionManager::SG_Components );
	}

	// Update checkboxes

	const CSelectionManager::ESelectionGranularity granularity = selectionManager->GetGranularity();
	CheckMenuItem( XRCID("granularityEntities"), granularity == CSelectionManager::SG_Entities );
	CheckMenuItem( XRCID("granularityComponents"), granularity == CSelectionManager::SG_Components );
}

void CEdEntityEditor::UpdateTabs()
{
	wxPanel* appearancePanel = XRCCTRL( *this, "AppearancesPanel", wxPanel );
	wxPanel* inventoryPanel = XRCCTRL( *this, "InventoryPanel", wxPanel );
	wxPanel* colorPanel = XRCCTRL( *this, "ColorsPanel", wxPanel );
	wxPanel* dismemberPanel = XRCCTRL( *this, "DismemberPanel", wxPanel );

	CEntity* entity = m_preview->GetEntity();
	CAppearanceComponent* appearanceComp = CAppearanceComponent::GetAppearanceComponent( entity );
	CInventoryComponent* inventoryComp = entity->FindComponent<CInventoryComponent>();
	CDismembermentComponent* dismemberComp = entity->FindComponent< CDismembermentComponent >();

	struct PanelDesc
	{
		wxString name;
		size_t index;
		wxPanel* panel;
		CComponent* component;
	};

	// NOTE : These must be in order of increasing page index, and should match the original ordering of the tabs.
	PanelDesc descriptions[] = 
	{
		{ PAGE_LABEL_APPEARANCES,	2,	appearancePanel,	appearanceComp },
		{ PAGE_LABEL_COLORS,		3,	colorPanel,			appearanceComp },
		{ PAGE_LABEL_INVENTORY,		7,	inventoryPanel,		inventoryComp },
		{ PAGE_LABEL_DISMEMBER,		12,	dismemberPanel,		dismemberComp },
	};

	// Track how many of the pages above have not been included, so we can properly offset the page indices.
	Uint32 numPagesRemoved = 0;

	for ( Uint32 d = 0; d < ARRAY_COUNT(descriptions); ++d )
	{
		bool pageFound = false;
		for ( size_t i = 0 ; i < m_propertiesNotebook->GetPageCount(); ++i )
		{
			if ( m_propertiesNotebook->GetPageText( i ) == descriptions[d].name )
			{
				if ( descriptions[d].component == nullptr )
				{
					descriptions[d].panel->Show( false );
					wxNotebookPage* np = m_propertiesNotebook->GetPage( i );
					np->Show( false );
					m_propertiesNotebook->RemovePage( i );
				}

				pageFound = true;
				break;
			}
		}

		// If this page should be hidden, update counter.
		if ( descriptions[d].component == nullptr )
		{
			++numPagesRemoved;
		}
		// If it wasn't supposed to be removed, and we didn't find it, we need to add it back in.
		else if ( !pageFound )
		{
			// Offset the page index by how many pages we've skipped, so it's in the correct spot.
			size_t newIndex = descriptions[d].index - (size_t)numPagesRemoved;
			m_propertiesNotebook->InsertPage( newIndex, descriptions[d].panel, descriptions[d].name );
		}
	}
}

CEdEntityEditor::~CEdEntityEditor()
{
#ifndef NO_DATA_VALIDATION
	// Disconnect to Data error reporter
	if( GDataError != nullptr )
	{
		GDataError->UnregisterListener( this );
	}
#endif

	if(m_gameplayParamEdit) 
	{
		delete m_gameplayParamEdit;
	}

	if ( m_undoManager )
	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}

	if ( m_template->GetEntityObject() )
	{
		SaveOptionsToConfig();
		// Remove global reference
		m_template->RemoveFromRootSet();
	}

	// Unregister hooks
	SEvents::GetInstance().UnregisterListener( this );

	// Destroy the notebook to avoid having the tabs keep references to destroyed widgets
	if ( m_notebook )
	{
		m_notebook->Destroy();
		m_notebook = NULL;
	}
	// release preview ai tree if necessary
	m_previewAITree = nullptr;
}

void CEdEntityEditor::OnSize( wxSizeEvent& event )
{
	m_hackNeedsSplitBarResize = true;
	this->Refresh();
	event.Skip();
}

void CEdEntityEditor::OnPaint( wxPaintEvent &event )
{
	if ( m_hackNeedsSplitBarResize )
	{
		wxSplitterWindow *rightSplitter = XRCCTRL( *this, "RightSplitter", wxSplitterWindow );
		rightSplitter->SetSashPosition( rightSplitter->GetSashPosition() + 1 );
		rightSplitter->SetSashPosition( rightSplitter->GetSashPosition() - 1 );
		m_hackNeedsSplitBarResize = false;
	}
	event.Skip();
}

void CEdEntityEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	// Save layout
	SaveLayout( TXT("/Frames/EntityEditor") );

	wxSplitterWindow *baseSplitter = XRCCTRL( *this, "BaseSplitter", wxSplitterWindow );
	wxSplitterWindow *rightSplitter = XRCCTRL( *this, "RightSplitter", wxSplitterWindow );
	config.Write( TXT("/Frames/EntityEditor/Layout/SplitterBase"), baseSplitter->GetSashPosition() );
	if ( rightSplitter != nullptr )
	{
		config.Write( TXT("/Frames/EntityEditor/Layout/SplitterRight"), rightSplitter->GetSashPosition() );
	}

	if ( m_preview && m_template )
	{
		config.Write( TXT("/Frames/EntityEditor/Shadows"), m_preview->GetShadowsEnabled() ? 1 : 0 );

		{
			CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EntityEditor/") + m_template->GetFile()->GetFileName() );

			config.Write( TXT("LightPosition"), m_preview->GetLightPosition() );

			Vector      cameraPos = m_preview->GetCameraPosition();
			EulerAngles cameraRot = m_preview->GetCameraRotation();
			config.Write( TXT("CameraX"), cameraPos.X );
			config.Write( TXT("CameraY"), cameraPos.Y );
			config.Write( TXT("CameraZ"), cameraPos.Z );
			config.Write( TXT("CameraPitch"), cameraRot.Pitch );
			config.Write( TXT("CameraRoll"),  cameraRot.Roll );
			config.Write( TXT("CameraYaw"),   cameraRot.Yaw );
		}
	}
}

void CEdEntityEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow *baseSplitter = XRCCTRL( *this, "BaseSplitter", wxSplitterWindow );
	wxSplitterWindow *rightSplitter = XRCCTRL( *this, "RightSplitter", wxSplitterWindow );
	baseSplitter->SetSashPosition( config.Read( TXT("/Frames/EntityEditor/Layout/SplitterBase"), baseSplitter->GetSashPosition() ) );
	rightSplitter->SetSashPosition( config.Read( TXT("/Frames/EntityEditor/Layout/SplitterRight"), rightSplitter->GetSashPosition() ) );

	if ( m_preview && m_template )
	{
		m_preview->SetShadowsEnabled( config.Read( TXT("/Frames/EntityEditor/Shadows"), 1 ) == 1 ? true : false );

		{
			CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EntityEditor/") + m_template->GetFile()->GetFileName() );

			m_preview->SetLightPosition( config.Read( TXT("LightPosition"), 135 ) );

			Float cameraX     = config.Read( TXT("CameraX"), 0.f );
			Float cameraY     = config.Read( TXT("CameraY"), 0.f );
			Float cameraZ     = config.Read( TXT("CameraZ"), 0.f );
			Float cameraPitch = config.Read( TXT("CameraPitch"), -1.f );
			Float cameraRoll  = config.Read( TXT("CameraRoll"), 0.f );
			Float cameraYaw   = config.Read( TXT("CameraYaw"), 0.f );

			if ( cameraPitch == -1.f )
			{ // zoom extends if no camera saved
				Box box = m_preview->GetEntity()->CalcBoundingBox();
				if ( box.IsEmpty() )
				{
					m_preview->SetCameraPosition( Vector( 16.f, 16.f, 16.f ) );
				}
				else
				{
					m_preview->SetCameraPosition( box.CalcCenter() + Vector( 1.f, 1.f, 1.f ) * box.CalcSize().Mag3() );
					m_preview->SetCameraRotation( EulerAngles( 0.f, -45.f, 135.f ) );
				}
			}
 			else
 			{
 				m_preview->SetCameraPosition( Vector( cameraX, cameraY, cameraZ ) );
 				m_preview->SetCameraRotation( EulerAngles( cameraRoll, cameraPitch, cameraYaw ) );
 			}
		}
	}

	CEdShortcutsEditor::Load(*this->GetMenuBar(), GetOriginalLabel());
	// Load layout after the shortcuts (duplicate menu after the shortcuts loading)
	LoadLayout( TXT("/Frames/EntityEditor") );
}

void CEdEntityEditor::OnGraphSelectionChanged()
{
	// Get selected objects
	TDynArray< CObject* > objects;
	m_graph->GetSelectedObjects( objects );

	// Show properties
	m_properties->Get().ClearPropertyStyles();
	if ( !objects.Size() )
	{
		objects.PushBack( m_preview->GetEntity() );
		m_properties->Get().SetObjects( objects );
	}
	else
	{
		// Colorize overridden properties
		for ( auto it=objects.Begin(); it != objects.End(); ++it )
		{
			CComponent* component = Cast< CComponent >( *it );
			if ( component )
			{
				TDynArray< CName > propertyNames;
				m_template->GetOverridenPropertiesForComponent( component, propertyNames );
				for ( auto it=propertyNames.Begin(); it != propertyNames.End(); ++it )
				{
					m_properties->Get().SetPropertyStyle( *it, SEdPropertiesPagePropertyStyle( wxColor( 234, 168, 45 ) ) );
				}
			}
		}
		m_properties->Get().SetObjects( objects );
	}

	m_toolsPanel->CancelTool();

	CSelectionManager *selectionManager = m_preview->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction(*selectionManager);
	selectionManager->DeselectAll();
	for ( Uint32 i = 0; i < objects.Size(); ++i )
	{
		CComponent *component = Cast<CComponent>( objects[i] );
		if ( component != NULL )
			selectionManager->Select( component, true );
	}


	// HACK : Prevent user from selecting the dismember fill mesh, if it exists.
	PreventSelectingDismemberFillMesh();
}

void CEdEntityEditor::OnGraphSelectionDeleted()
{
	// remove detached component from the list boxes

	OnAppearanceSelected   ( wxCommandEvent() );
}

void CEdEntityEditor::OnPreviewSelectionChanged()
{
	// Propagate selection to graph
	m_graph->ForceSelectionUpdate();
}

void CEdEntityEditor::OnPreviewWidgetModeChanged()
{
	// Modes
	ERPWidgetMode mode = m_preview->m_widgetManager->GetWidgetMode();

	CheckMenuItem( XRCID("widgetModeMove"),   mode == RPWM_Move );
	CheckMenuItem( XRCID("widgetModeRotate"), mode == RPWM_Rotate );
	CheckMenuItem( XRCID("widgetModeScale"),  mode == RPWM_Scale );	

	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID("widgetModeMove"),	mode == RPWM_Move );
	toolbar->ToggleTool( XRCID("widgetModeRotate"), mode == RPWM_Rotate );
	toolbar->ToggleTool( XRCID("widgetModeScale"),	mode == RPWM_Scale );
}

void CEdEntityEditor::OnPreviewWidgetSpaceChanged()
{
	// Pivot
	ERPWidgetSpace space = m_preview->GetWidgetSpace();

	//Reset menu items
	CheckMenuItem( XRCID("widgetPivotLocal"),	space == RPWS_Local );
	CheckMenuItem( XRCID("widgetPivotGlobal"),	space == RPWS_Global );

	// Reset tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "widgetPivotLocal"),	space == RPWS_Local );
	toolbar->ToggleTool( XRCID( "widgetPivotGlobal"),	space == RPWS_Global );
}

void CEdEntityEditor::OnPreviewItemTransformChanged()
{
	// Refresh properties panels with visual items
	m_slotProperties->RefreshValues();
	m_dismemberWoundProp->Get().RefreshValues();

	// Might not be a dismember item, but we want to make sure it's updated. If we aren't moving a wound, then any dismember component
	// shouldn't have any visible wound, and nothing will happen.
	OnDismemberPreviewItemTransformChanged();
}

namespace
{
	struct SimiliarComponentLookup
	{
	private:
		CComponent *m_componentToLookFor;
	public:
		SimiliarComponentLookup( CComponent *componentToLookFor )
			: m_componentToLookFor( componentToLookFor )
		{}
		Bool operator() ( const CComponent *component ) const
		{
			return component->GetClass() == m_componentToLookFor->GetClass() && m_componentToLookFor->GetName() == component->GetName();
		}
	};

	void CollectAllIncludes( CEntityTemplate *entityTemplate, TDynArray< CEntityTemplate* > &loadedIncludesOut )
	{
		const TDynArray< THandle< CEntityTemplate > >& includes = entityTemplate->GetIncludes();
		for ( Uint32 iInclude = 0; iInclude < includes.Size(); ++iInclude )
		{
			CEntityTemplate *included = includes[iInclude].Get();
			if ( included )
			{
				//loadedIncludesOut.PushBackUnique( included );
				loadedIncludesOut.PushBack( included );
				CollectAllIncludes( included, loadedIncludesOut );
			}
		}
	}

	void CollectAllComponents2( CEntityTemplate *entityTemplate, TDynArray< CComponent* > &components )
	{
		TDynArray< CComponent* > tmpComponents;
		CollectEntityComponents( entityTemplate->GetEntityObject(), tmpComponents );
		components.PushBack( tmpComponents );

		const TDynArray< THandle< CEntityTemplate > >& includes = entityTemplate->GetIncludes();
		for ( Uint32 iInclude = 0; iInclude < includes.Size(); ++iInclude )
		{
			CEntityTemplate *included = includes[iInclude].Get();
			if ( included )
			{
				CollectAllComponents2( included, components );
			}
		}
	}
}

void CEdEntityEditor::OnRefreshInEditor( wxCommandEvent& event )
{
	if ( GGame )
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			// Save the component
			DoSave( false );

			// Dispatch to editor
			SEvents::GetInstance().DispatchEvent( CNAME( EntityTemplateChanged ), CreateEventData( m_template ) );
			SEvents::GetInstance().DispatchEvent( CNAME( UpdateSceneTree ), NULL );
		}
	}
}

void CEdEntityEditor::OnPrepare( wxCommandEvent& event )
{
	m_preview->GetEntity()->OnPrepare();
}

void CEdEntityEditor::OnRemapSkinning( wxCommandEvent& event )
{
}

void CEdEntityEditor::OnZoomExtentsGraph( wxCommandEvent& event )
{
	m_graph->ZoomExtents( false );
}

void CEdEntityEditor::DoSave( Bool withStreamedComponents )
{
	if ( GGame->IsActive() )
	{
		wxMessageBox( TXT("Cannot resave/edit entity while game is active.") );
		return;
	}

	if( IsModifiedByDLC() )
	{
		GFeedback->ShowMsg( TXT( "Entity Editor" ), TXT( "Can't save entity template as it's modified by DLC." ) );
		return;
	}

	// Validate entity before saving
	if( !ValidateEntityOnSave( m_preview->GetEntity() ) )
	{
		CActor* actor = Cast< CActor >( m_preview->GetEntity() );
		if ( actor )
		{
			// Reapply appearance
			CAppearanceComponent* apc = CAppearanceComponent::GetAppearanceComponent( actor );
			CName currentAppearanceName = apc->GetAppearance();
			apc->RemoveCurrentAppearance();
			apc->ApplyAppearance( currentAppearanceName );
		}
		return;
	}
	
	// Warn about root animated component being streamed
	if ( m_preview->GetEntity()->GetRootAnimatedComponent() != nullptr &&
			m_preview->GetEntity()->GetRootAnimatedComponent()->IsStreamed() )
	{
		int action = 0;
		if ( FormattedDialogBox( wxT("Oh, NO!"), wxT("V'Problem'{'The root animated componet is marked as streamed and this''can cause several problems, like broken or wrong attachments!'}R'What do you want to do?'('Set the root component to not be streamed''Save the entity template, i know what i am doing''Abort! Stop the saving!')|H{~B@'&OK')"), &action ) == -1 )
			action = 2;
		switch ( action )
		{
		case 0:
			m_preview->GetEntity()->GetRootAnimatedComponent()->SetStreamed( false );
			break;
		case 1:
			/* do nothing */
			break;
		case 2:
			return;
		}
	}

	// Turn off mimics
	if ( m_mimicsPanel )
	{
		m_mimicsPanel->Activate( false );
	}

	// Update streamed components and destroy them so the entity wont have any included components
	m_preview->GetEntity()->SetStreamingLock( false );
	m_preview->GetEntity()->UpdateStreamedComponentDataBuffers( false );
	m_preview->GetEntity()->DestroyStreamedComponents( SWN_NotifyWorld );
	m_preview->GetEntity()->SetStreamingLock( true );

	// Collect all the components from the preview entity.
	TDynArray< CComponent* > components;
	CollectEntityComponents( m_preview->GetEntity(), components );

	m_graph->ClearLayout();

	// Remove all items before saving
	ClearEquipmentPreviewItems( m_preview->GetEntity() );

	// Stop tools on save
	m_toolsPanel->CancelTool();
	
	UpdateUsedAppearances();
	UpdateUsedDismemberment();
	
	// Save minigame parameters
	OnMinigameWWUpdateDataFromGui();

	// Save gameplay entity localizable name
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( m_preview->GetEntity() );
	if ( gameplayEntity != NULL )
	{
		SLocalizationManager::GetInstance().UpdateStringDatabase( gameplayEntity );
	}

	// Create external proxies
	m_preview->GetEntity()->PrepareEntityForTemplateSaving();


	// Detach template (needed for Capture)
	m_preview->GetEntity()->DetachTemplate();

	// Capture only non-included components - this alters m_preview's entity.
	m_template->CaptureData( m_preview->GetEntity(), withStreamedComponents );

	// Save
	m_template->Save();


	// Save the gameplay entity tags
	C2dArray* gameplayEntitiesTags = resSavedGameplayEntitiesTags.LoadAndGet< C2dArray >();
	if ( gameplayEntitiesTags && 
		gameplayEntitiesTags->GetFile() && 
		gameplayEntitiesTags->GetFile()->IsModified() )
	{
		gameplayEntitiesTags->GetFile()->Save();
	}

	// Save the reaction fields tags
	C2dArray* reactionFieldsTags = resSavedReactionFieldsTags.LoadAndGet< C2dArray >();
	if ( reactionFieldsTags && 
		reactionFieldsTags->GetFile() && 
		reactionFieldsTags->GetFile()->IsModified() )
	{
		reactionFieldsTags->GetFile()->Save();
	}

	SEvents::GetInstance().DispatchEvent( CNAME( EntityTemplateModified ), CreateEventData( m_template ) );

	// Reapply items
	ApplyEquipmentDefinitionPreview();

	//m_preview->GetEntity()->CreateAllStreamedComponents( SWN_NotifyWorld );

	m_graph->ForceSelectionUpdate();
	LoadEntityLOD( m_currentStreamingLOD );

	RefreshComponentComments();
	UpdateDismemberAppearances();
}

void CEdEntityEditor::OnSave( wxCommandEvent& event )
{
	// Before saving the entity, we need to clear any dismemberment preview. Otherwise, the fill mesh
	// will be saved as well, which is bad.
	DeselectDismemberWound();

	DoSave( false );
}

void CEdEntityEditor::OnSaveWithStreamedComponents( wxCommandEvent& event )
{
	if ( wxMessageBox( wxT("Are you sure? This operation is there for the rare case when you need to keep the streamable components in the entity data. Usually this is needed when making editor templates for drag-drop targets. Do you REALLY want to save this entity template with streamed component data? If you answer yes, there is no turning back!"), wxT("Dangerous Operation"), wxYES_NO|wxCENTRE|wxICON_WARNING, this ) == wxYES )
	{
		// Before saving the entity, we need to clear any dismemberment preview. Otherwise, the fill mesh
		// will be saved as well, which is bad.
		DeselectDismemberWound();

		DoSave( true );
	}
}

void CEdEntityEditor::OnImportEntity( wxCommandEvent& event )
{	
	String defaultDir = wxEmptyString;
	String wildCard = TXT("CSV file (*.csv)|*.csv|All files (*.*)|*.*");

	wxFileDialog fileDialog( this, wxT("Select template to import..."), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_OPEN );
	//fileDialog.SetPath( last_opened_path_here );

	if ( fileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = fileDialog.GetPath().wc_str();

		RED_ASSERT( !loadPath.Empty() );
		if ( loadPath.Empty() ) return;

		IFile* file = GFileManager->CreateFileReader( loadPath, FOF_Buffered | FOF_AbsolutePath );
		if ( !file )
		{
			GFeedback->ShowError( TXT("Unable to open the file. %s"), loadPath.AsChar() );
			return;
		}

		C2dArray* propertiesCSV = C2dArray::CreateFromString( loadPath );

		if ( propertiesCSV == NULL )
		{
			wxMessageBox( wxT( "Invalid file format (is this really a CSV file?" ) );
			return;
		}
		else
		{
			// process the csv, get assets and transforms,
			// create components and connect the stuff
			Uint32 numberOfMeshes = 0;						

			Uint32 numberOfRows, numberOfColumns = 0;
			propertiesCSV->GetSize( numberOfColumns, numberOfRows );

			Float matrix[12]={0.0f};
			Int32 meshGraphPosX = 0, meshGraphPosY = 0;
			Int32 destGraphPosX = 220, destGraphPosY = 0;
			for ( Uint32 i = 0; i < numberOfRows; ++i )
			{
				SComponentSpawnInfo spawnInfo;

				Bool validCondition = true;
				Float fVal = 0.0f;

				// Read basic transforms here
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r1x"), i ), matrix[0] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r1y"), i ), matrix[1] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r1z"), i ), matrix[2] );

				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r2x"), i ), matrix[3] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r2y"), i ), matrix[4] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r2z"), i ), matrix[5] );

				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r3x"), i ), matrix[6] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r3y"), i ), matrix[7] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r3z"), i ), matrix[8] );

				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r4x"), i ), matrix[9] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r4y"), i ), matrix[10] );
				validCondition = validCondition && FromString( propertiesCSV->GetValue( TXT("r4z"), i ), matrix[11] );

				spawnInfo.m_spawnPosition.X = matrix[9];
				spawnInfo.m_spawnPosition.Y = matrix[10];
				spawnInfo.m_spawnPosition.Z = matrix[11];
				spawnInfo.m_spawnPosition.W = 1.0f;

				Matrix rot;
				rot.SetRow( 0, Vector( matrix[0], matrix[1], matrix[2], 0.0f) );
				rot.SetRow( 1, Vector( matrix[3], matrix[4], matrix[5], 0.0f) );
				rot.SetRow( 2, Vector( matrix[6], matrix[7], matrix[8], 0.0f) );
				rot.SetRow( 3, Vector( 0.f, 0.f, 0.f, 1.f ) );

				spawnInfo.m_spawnRotation = rot.ToEulerAngles();
								
				String path = propertiesCSV->GetValue( TXT("path"), i );

				// Set specific class data here
				if( Red::StringCompare( path.StringAfter(TXT(".")).AsChar(), TXT("w2mesh") ) == 0 )
				{
					// Load mesh
					CMesh* m = Cast< CMesh > ( GDepot->LoadResource( path ) );
					if( !m )
					{
						RED_ASSERT( m != nullptr, TXT("Mesh does not exist at path: %s"), path );
						continue;
					}
					const CCollisionMesh* coll = m->GetCollisionMesh();
					if ( !coll )
					{
						CMeshComponent* mc = static_cast< CMeshComponent* >( m_preview->GetEntity()->CreateComponent( ClassID< CMeshComponent >(), spawnInfo ) );
						if ( mc != nullptr )
						{
							mc->SetResource( m );
							mc->SetCastingShadows( true );
							mc->SetGraphPosition(meshGraphPosX, meshGraphPosY);
							numberOfMeshes++;
							meshGraphPosY+=25;
						}
					}
					else
					{
						CStaticMeshComponent* sc = static_cast< CStaticMeshComponent* >( m_preview->GetEntity()->CreateComponent( ClassID< CStaticMeshComponent >(), spawnInfo ) );
						if( sc != nullptr )
						{
							sc->SetResource( m );
							sc->SetCastingShadows( true );
							sc->SetGraphPosition(meshGraphPosX, meshGraphPosY);
							numberOfMeshes++;
							meshGraphPosY+=25;
						}
					}
				}
#ifdef USE_APEX
				if ( Red::StringCompare( path.StringAfter(TXT(".")).AsChar(), TXT("redapex") ) == 0 )
				{
					CApexResource* ar = Cast< CApexResource > ( GDepot->LoadResource( path ) );
					if ( !ar )
					{
						continue;
					}
					CDestructionSystemComponent* dm = static_cast< CDestructionSystemComponent* >( m_preview->GetEntity()->CreateComponent( ClassID< CDestructionSystemComponent >(), spawnInfo ) );
					if ( dm != nullptr )
					{
						dm->SetResource( ar );
						dm->SetGraphPosition(destGraphPosX, destGraphPosY);
						destGraphPosY+=25;
					}
				}
#endif
			}
			if( numberOfMeshes > 0 ) 
			{
				if( numberOfMeshes > 1 ) GFeedback->ShowMsg( TXT("Importing finished."), TXT("%d meshes imported."), numberOfMeshes );
				else
					GFeedback->ShowMsg( TXT("Importing finished."), TXT("%d mesh imported."), numberOfMeshes );

				m_graph->ForceLayoutUpdate();
			}			
			else
				GFeedback->ShowMsg( TXT("Importing finished."), TXT("Nothing to import!") );
		}
		delete file;
	}
}

void CEdEntityEditor::OnEditCopy( wxCommandEvent& event )
{
	int selection = m_notebook->GetSelection();
	int graphIndex = m_notebook->GetPageIndex( m_entityPanel );
	CEdEffectEditor* editor = wxDynamicCast( m_notebook->GetPage( selection ), CEdEffectEditor );
	if ( selection == graphIndex )
	{
		m_graph->CopySelection();
	}
	else if ( editor )
	{
		editor->CopySelection();
	}
}

void CEdEntityEditor::OnEditCut( wxCommandEvent& event )
{
	m_graph->CutSelection();
}

void CEdEntityEditor::OnEditPaste( wxCommandEvent& event )
{
	int selection = m_notebook->GetSelection();
	int graphIndex = m_notebook->GetPageIndex( m_entityPanel );
	CEdEffectEditor* editor = wxDynamicCast( m_notebook->GetPage( selection ), CEdEffectEditor );
	if ( selection == graphIndex )
	{
		m_graph->PasteSelection();
	}
	else if ( editor )
	{
		editor->PasteSelection();
	}
}

void CEdEntityEditor::OnEditDelete( wxCommandEvent& event )
{
	if ( m_preview->EnableGraphEditing() )
	{
		m_graph->DeleteSelection();
	}
}

void CEdEntityEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdEntityEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdEntityEditor::OnSelectAll( wxCommandEvent& event )
{
	m_preview->SelectAll();
}

void CEdEntityEditor::OnUnselectAll( wxCommandEvent& event )
{
	m_preview->UnselectAll();
}

void CEdEntityEditor::OnEditFindEntityComponents( wxCommandEvent& event )
{
	CEntityComponentsDialog* dialog = new CEntityComponentsDialog( m_preview->GetEntity(), this, this );
	dialog->ShowModal();
	dialog->Destroy();
}

void CEdEntityEditor::OnInvertSelection( wxCommandEvent& event )
{
	m_preview->InvertSelection();
}

void CEdEntityEditor::DispatchEditorEvent( const CName &name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{
        const auto& eventData = GetEventData< CSelectionManager::SSelectionEventData >( data );

		if ( eventData.m_world == m_preview->GetWorld() )
		{
			if ( m_undoManager && wxTheFrame->GetWorldEditPanel()->GetSelectionTracking() )
			{
				if ( !m_undoManager->IsUndoInProgress() 
					// Do not create selection step if there is some other awaiting undo step, pushing the step here would drop it
					&& m_undoManager->GetStepToAdd() == nullptr 
					)
				{
					CUndoSelection::CreateStep( *m_undoManager, *m_preview->GetSelectionManager(), eventData.m_transactionId );
				}
			}
		}
	}
	else if ( name == CNAME( EditorPostUndoStep ) )
	{
		const CEdUndoManager* undoManager = GetEventData< CEdUndoManager* >( data );
		if ( undoManager == m_undoManager )
		{
			m_preview->GetSelectionManager()->RefreshPivot();
			m_graph->ForceSelectionUpdate();
		}
	}
	else if ( name == CNAME( EditorPropertyPreChange ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyEventData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( propertyEventData.m_page == &m_properties->Get() )
		{
			// We cannot veto the change :-(
			m_template->MarkModified();
		}
	}
	else if ( name == CNAME( EditorPropertyPostChange ) || name == CNAME( EditorPropertyChanging ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyEventData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if( propertyEventData.m_propertyName== CName( TXT("transform") ) )
		{
			m_preview->GetSelectionManager()->RefreshPivot();
		}
	
		if ( name == CNAME( EditorPropertyPostChange ) )
		{
			if ( propertyEventData.m_page == &m_properties->Get() )
			{
				m_preview->Refresh();
				m_graph->ForceLayoutUpdate();
				m_graph->Repaint();

				// kick the preview to update itself
				//CDrawableComponent::RecreateProxiesOfRenderableComponents();  --- NOT FOR EVERY SINGLE DRAWABLE!!!!
				if ( propertyEventData.m_typedObject.m_class->IsA< CDrawableComponent >() )
				{
					static_cast< CDrawableComponent* >( propertyEventData.m_typedObject.m_object )->RefreshRenderProxies();
				}

				if ( propertyEventData.m_typedObject.m_class == ClassID< CMeshComponent >() 
					&& propertyEventData.m_propertyName == TXT("mesh") )
				{
					RunLaterOnce( [this](){ m_properties->Get().RefreshValues(); } );
				}
			}
		}
	}
	else if ( name == CNAME( EntityTemplateClassChanging ) )
	{
		// Create external proxies
		m_preview->GetEntity()->PrepareEntityForTemplateSaving();

		// Detach template (needed for Capture)
		m_preview->GetEntity()->DetachTemplate();

		// Capture only non-included components
		m_template->CaptureData( m_preview->GetEntity() );

		m_preview->GetWorld()->DelayedActions();
	}
	else if ( name == CNAME( PreviewEntityModified ) )
	{
		UpdateTabs();

		CheckForMultipleDismemberment();

		if ( m_mimicsPanel )
		{
			m_mimicsPanel->SetEntity( m_preview->GetEntity() );
		}
	}
	else if ( name == CNAME( EntityTemplateModified ) )
	{
		// Process only changes to this template
		CEntityTemplate* tdata = GetEventData< CEntityTemplate* >( data );
		if ( tdata == m_template )
		{
			// Update preview
			m_preview->SetTemplate( m_template );

			UpdateUsedAppearances();
			if ( m_propertiesNotebook->GetPageText( m_propertiesNotebook->GetSelection() ) == PAGE_LABEL_APPEARANCES )
			{
				UpdateAppearanceList();
			}
			else
			{
				UpdateAppearanceList();
			}

			UpdateIncludeList();

			UpdateDependencyList();

			UpdateInstancePropertiesList();

			UpdateDynamicTemplate();

			// Update selection
			OnGraphSelectionChanged();

			CheckForMultipleDismemberment();

			if ( m_mimicsPanel )
			{
				m_mimicsPanel->SetEntity( m_preview->GetEntity() );
			}

			// Force to re-build effect editors
			for ( auto effectEditorIt = m_effectEditors.Begin(); effectEditorIt != m_effectEditors.End(); ++effectEditorIt )
			{
				CFXDefinition* def = effectEditorIt->m_second->GetFXDefinition();
				effectEditorIt->m_second = EditEffect( def, true );
			}

			// Update graph layout
			m_graph->ForceLayoutUpdate();
		}
	}
	else if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_template )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA<CEntityTemplate>() )
		{
			CEntityTemplate* oldTemplate = (CEntityTemplate*)(reloadInfo.m_oldResource);
			CEntityTemplate* newTemplate = (CEntityTemplate*)(reloadInfo.m_newResource);
			if ( oldTemplate == m_template )
			{
				m_template = newTemplate;
				m_template->AddToRootSet();

				SEvents::GetInstance().DispatchEvent( CNAME( EntityTemplateModified ), CreateEventData( newTemplate ) );

				wxTheFrame->GetAssetBrowser()->OnEditorReload(m_template, this);
			}
		}
	}
	else if ( name == CNAME( UpdateEffectList ) )
	{
		CEntityTemplate* entityTemplate = GetEventData< CEntityTemplate* >( data );
		if ( m_template != entityTemplate )
		{
			UpdateAnimEffectsList();
		}
	}
}

void CEdEntityEditor::StopPlayedEffect()
{
	if ( m_preview )
	{
		CEntity* entity = m_preview->GetEntity();
		if ( entity )
		{
			entity->DestroyAllEffects();
		}
	}
}

void CEdEntityEditor::OnNotebookPageChanged( wxAuiNotebookEvent& event )
{
	// We cannot close default page and tools page
	if ( event.GetSelection() == 0 || event.GetSelection() == 1 )
	{
		m_notebook->SetWindowStyle( wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS );
	}
	else
	{
		m_notebook->SetWindowStyle( wxAUI_NB_DEFAULT_STYLE );
	}

	// Stop any played effect if initialization is done as autoplay effect is played on the entity and window initialization stops it OnNotebookPageChanged (tabs population)
	if( m_isInitialized )
		StopPlayedEffect();
}

void CEdEntityEditor::OnNotebookPageClosed( wxAuiNotebookEvent& event )
{
	m_effectEditors.Erase( event.GetSelection() );
}

void CEdEntityEditor::OnPropertiesNotebookPageChanged( wxNotebookEvent& event )
{
	if( event.GetEventObject() != m_propertiesNotebook )
	{
		event.Skip();
		return;
	}

	// refresh preview
	OnAppearanceSelected( wxCommandEvent() );


	// Handle hiding the page we just left.
	size_t oldPage = event.GetOldSelection();
	if ( oldPage != -1 )
	{
		wxString pageLabel = m_propertiesNotebook->GetPageText( oldPage );
		if ( pageLabel == PAGE_LABEL_SLOTS )
		{
			OnSlotPageHide();
		}
		else if ( pageLabel == PAGE_LABEL_DISMEMBER )
		{
			OnDismemberPageHide();
		}
	}

	// And now handle the new page.
	size_t currentPage = event.GetSelection();
	if ( currentPage != -1 )
	{
		wxString pageLabel = m_propertiesNotebook->GetPageText( currentPage );
		if ( pageLabel == PAGE_LABEL_SLOTS )
		{
			OnSlotPageShow();
		}
		else if ( pageLabel == PAGE_LABEL_DISMEMBER )
		{
			OnDismemberPageShow();
		}
	}

	event.Skip();
}

void CEdEntityEditor::OnResetForcedLOD( wxCommandEvent& event )
{
	// Reset the force LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetWorld()->GetRenderSceneEx(), -1 ) )->Commit();

	// Reset LOD tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );

	toolbar->ToggleTool( XRCID( "entityLOD0"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD1"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD2"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD3"), false );	

	toolbar->ToggleTool( XRCID( "previewForceLOD0"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD1"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD2"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD3"), false );

	PrepareForLODChange();
	m_preview->GetEntity()->DestroyStreamedComponents( SWN_NotifyWorld );

	m_currentStreamingLOD = -1;
}

void CEdEntityEditor::OnPreviewLOD0( wxCommandEvent& event )
{
	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetWorld()->GetRenderSceneEx(), 0 ) )->Commit();

	// Reset LOD tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "previewForceLOD0"), true );
	toolbar->ToggleTool( XRCID( "previewForceLOD1"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD2"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD3"), false );
}

void CEdEntityEditor::OnPreviewLOD1( wxCommandEvent& event )
{
	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetWorld()->GetRenderSceneEx(), 1 ) )->Commit();

	// Reset LOD tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "previewForceLOD0"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD1"), true );
	toolbar->ToggleTool( XRCID( "previewForceLOD2"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD3"), false );
}

void CEdEntityEditor::OnPreviewLOD2( wxCommandEvent& event )
{
	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetWorld()->GetRenderSceneEx(), 2 ) )->Commit();

	// Reset LOD tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "previewForceLOD0"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD1"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD2"), true );
	toolbar->ToggleTool( XRCID( "previewForceLOD3"), false );
}

void CEdEntityEditor::OnPreviewLOD3( wxCommandEvent& event )
{
	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetWorld()->GetRenderSceneEx(), 3 ) )->Commit();

	// Reset LOD tools
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "previewForceLOD0"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD1"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD2"), false );
	toolbar->ToggleTool( XRCID( "previewForceLOD3"), true );
}

void CEdEntityEditor::PrepareForLODChange()
{
	m_preview->GetEntity()->UpdateStreamedComponentDataBuffers();
}

void CEdEntityEditor::LoadEntityLOD( Uint32 lod )
{
	// Analyze textures
	Uint32 textureArrayDataSize = 0;
	Uint32 textureDataSize = 0;
	Uint32 meshDataSize = 0;

	TDynArray< MeshTextureInfo* > usedTextures;
	TDynArray< SMeshTextureArrayInfo* > usedTextureArrays;

	TDynArray< String > meshResourceDepotPaths;
	meshResourceDepotPaths.Reserve( 8 );

	CEntity* e = m_preview->GetEntity();
	if(e)
	{
		// Create LOD components from this LOD up to 2
		e->CreateStreamedComponents( SWN_NotifyWorld );

		TDynArray< CComponent* > cmp = e->GetComponents();
		for( Uint32 iComp = 0; iComp < cmp.Size(); ++iComp )
		{
			CMeshComponent* cmc = Cast< CMeshComponent > ( cmp[ iComp ] );
			if( cmc )
			{
				// generate approx. resource memory alloc size
				CMesh* mesh = cmc->GetMeshNow();

				if(mesh)
				{
					TDynArray< const CResource* > meshComponentResources;
					cmc->GetResource( meshComponentResources );

					Bool uniqueMesh = false;

					for ( Uint32 iResource = 0; iResource < meshComponentResources.Size(); ++iResource )
					{
						const CResource* meshResource = meshComponentResources[ iResource ];
						String depotPath = meshResource->GetDepotPath();
						if( meshResourceDepotPaths.PushBackUnique( std::move( depotPath ) ) )
						{
							uniqueMesh = true;
						}
					}

					if( uniqueMesh )
					{
						for ( Uint32 iMesh = 0; iMesh < mesh->GetNumLODLevels(); ++iMesh )
						{
							meshDataSize += MeshStatsNamespace::CalcMeshLodRenderDataSize( mesh, iMesh );
						}
					}

					// Gather used texture arrays
					TDynArray< SMeshTextureArrayInfo* > usedChunkTextureArrays;
					MeshStatsNamespace::GatherTextureArraysUsedByMesh( mesh, usedChunkTextureArrays );

					// Fill the global mesh texture arrays
					if( usedTextureArrays.Empty() == true )
					{
						usedTextureArrays = usedChunkTextureArrays;
					}
					else
					{
						for( Uint32 j = 0; j < usedChunkTextureArrays.Size(); ++j )
						{
							Bool alreadyCollected = false;
							for( Uint32 k=0; k<usedTextureArrays.Size(); ++k )
							{
								if( SMeshTextureArrayInfo::CmpFuncByDepotPath( usedTextureArrays[k], usedChunkTextureArrays[j] ) == 0 )
								{
									alreadyCollected = true;
									break;
								}
							}

							if ( !alreadyCollected )
							{
								usedTextureArrays.PushBack( usedChunkTextureArrays[j] );
							}
						}
					}
					usedChunkTextureArrays.Clear();

					// Gather used textures
					TDynArray< MeshTextureInfo* > usedChunkTextures;
					MeshStatsNamespace::GatherTexturesUsedByMesh( mesh, usedChunkTextures );
					
					// Fill the global mesh textures
					if( usedTextures.Empty() )
					{
						usedTextures = usedChunkTextures;
					}
					else
					{	
						for ( Uint32 i = 0; i < usedChunkTextures.Size(); ++i )
						{
							MeshTextureInfo* usedChunkTexture = usedChunkTextures[i];
							if( usedChunkTexture && !usedTextures.Empty() )
							{
								auto it = FindIf
								(
									usedTextures.Begin(),
									usedTextures.End(), 
									[ usedChunkTexture ]( MeshTextureInfo* tex )
									{
										return tex && MeshTextureInfo::CmpFuncByDepotPath( &usedChunkTexture, &tex ) == 0;
									}
								);

								if ( it == usedTextures.End() )
								{
									usedTextures.PushBack( usedChunkTexture );
								}
							}
						}
					}
					usedChunkTextures.Clear();
				}
			}
		}

		if( !usedTextures.Empty() )
		{
			for ( Uint32 i = 0; i < usedTextures.Size(); ++i )
			{
				// Accumulate shit
				MeshTextureInfo* texInfo = usedTextures[i];
				textureDataSize += texInfo->m_dataSize;
			}
		}

		// calculate data size for all gathered texture arrays
		if( usedTextureArrays.Empty() == false )
		{
			for( Uint32 i=0; i<usedTextureArrays.Size(); ++i )
			{
				const CTextureArray* textureArray = usedTextureArrays[i]->m_textureArray.Get();

				TDynArray< CBitmapTexture* > arrayTextures;
				textureArray->GetTextures( arrayTextures );
				const Uint32 textureCount = arrayTextures.Size();

				for( Uint32 j=0; j<textureCount; ++j )
				{
					textureArrayDataSize += MeshStatsNamespace::CalcTextureDataSize( arrayTextures[j] );
				}
			}
		}
	}
	m_preview->m_textureArraysDataSize = wxString( "Texture arrays data: " ) + MeshStatsNamespace::MemSizeToText( textureArrayDataSize ) + wxString( " MB" );
	m_preview->m_textureDataSize = wxString( "Texture data: " ) + MeshStatsNamespace::MemSizeToText( textureDataSize ) + wxString( " MB" );
	m_preview->m_meshDataSize = wxString( "Mesh data: " ) + MeshStatsNamespace::MemSizeToText( meshDataSize ) + wxString( " MB" );

	usedTextures.Clear();
}

void CEdEntityEditor::OnPreviewEntityLOD0( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "entityLOD0"), true );	
	toolbar->ToggleTool( XRCID( "entityLOD1"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD2"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD3"), false );	

	PrepareForLODChange();
	m_currentStreamingLOD = 0;
	LoadEntityLOD( m_currentStreamingLOD );
	m_graph->ForceLayoutUpdate();
}

void CEdEntityEditor::OnPreviewEntityLOD1( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "entityLOD0"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD1"), true );	
	toolbar->ToggleTool( XRCID( "entityLOD2"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD3"), false );	

	PrepareForLODChange();
	m_currentStreamingLOD = 1;
	LoadEntityLOD( m_currentStreamingLOD );
	m_graph->ForceLayoutUpdate();
}

void CEdEntityEditor::OnPreviewEntityLOD2( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "entityLOD0"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD1"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD2"), true );	
	toolbar->ToggleTool( XRCID( "entityLOD3"), false );	

	PrepareForLODChange();
	m_currentStreamingLOD = 2;
	LoadEntityLOD( m_currentStreamingLOD );
	m_graph->ForceLayoutUpdate();
}

void CEdEntityEditor::OnPreviewEntityLOD3( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID( "entityLOD0"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD1"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD2"), false );	
	toolbar->ToggleTool( XRCID( "entityLOD3"), true );	

	PrepareForLODChange();
	m_currentStreamingLOD = 3;
	LoadEntityLOD( m_currentStreamingLOD );
	m_graph->ForceLayoutUpdate();
}


namespace {
	template< class T >
	struct CNamedClassPointerSorter
	{
		Bool operator()( T* a, T* b ) const
		{
			return a->GetName().AsString() < b->GetName().AsString();
		}
	};

	template< class T >
	struct SNamedClassPointerSorter
	{
		Bool operator()( T* a, T* b ) const
		{
			return a->GetName() < b->GetName();
		}
	};

	template< class T >
	struct ResourceClassPointerSorter
	{
		Bool operator()( T* a, T* b ) const
		{
			return a->GetDepotPath() < b->GetDepotPath();
		}
	};

	template< class T >
	struct ResourceClassHandleSorter
	{
		Bool operator()( const THandle<T>& a, const THandle<T>& b ) const
		{
			return a->GetDepotPath() < b->GetDepotPath();
		}
	};

}

void CEdEntityEditor::UpdateDynamicTemplate()
{
	
}

String CEdEntityEditor::FormatEffectName( CFXDefinition* def )
{
	// Format effect string
	String display = def->GetName().AsString();
	if ( def->GetAnimationName() )
	{
		display = String::Printf( TXT("%s [%s]"), def->GetName().AsString().AsChar(), def->GetAnimationName().AsString().AsChar() );
	}

	// This is included, effect, mark it
	if ( !def->IsContained( m_template ) )
	{
		display += TXT(" (Included)");
	}

	return display;
}

static void GetEntityAnimations( CEntity* entity, TDynArray< CName >& allAnimations )
{
	for ( ComponentIterator< CAnimatedComponent > it( entity ); it; ++it )
	{
		const TDynArray< THandle< CSkeletalAnimationSet > >& animSets = (*it)->GetAnimationSets();
		for ( Uint32 i=0; i<animSets.Size(); ++i )
		{
			if ( animSets[i] )
			{
				const TDynArray< CSkeletalAnimationSetEntry* >& animations = animSets[i]->GetAnimations();
				for ( Uint32 j=0; j<animations.Size(); ++j )
				{
					CSkeletalAnimationSetEntry* entry = animations[j];
					if ( entry && entry->GetAnimation() )
					{
						allAnimations.PushBackUnique( entry->GetName() );
					}
				}
			}
		}
	}
}

static void GetTemplateAnimations( CEntityTemplate* entityTemplate, TDynArray< CName >& allAnimations )
{
	if ( entityTemplate )
	{
		CEntity* entity = entityTemplate->GetEntityObject();
		if ( entity )
		{
			GetEntityAnimations( entity, allAnimations );
		}

		const TDynArray< THandle< CEntityTemplate > >& includes = entityTemplate->GetIncludes();
		for ( Uint32 i=0; i<includes.Size(); ++i )
		{
			GetTemplateAnimations( includes[i].Get(), allAnimations );
		}
	}
}

class wxSlotListClientData : public wxClientData
{
public:
	THandle< CEntityTemplate >		m_template;
	CName							m_slotName;

public:
	wxSlotListClientData( CEntityTemplate* entityTemplate, const CName& slotName )
		: m_template( entityTemplate )
		, m_slotName( slotName )
	{};
};

static void EmitSlotNames( wxListBox* list, CEntityTemplate* baseTemplate, CEntityTemplate* entityTemplate, CName slotNameToSelect, TDynArray< CName >& slotNames )
{
	// Enumerate
	const TDynArray< EntitySlot >& slots = entityTemplate->GetSlots();
	for ( Uint32 i=0; i<slots.Size(); ++i )
	{
		const EntitySlot& slot = slots[i];
		if ( !slotNames.Exist( slot.GetName() ) )
		{
			// Display each slot only once
			slotNames.PushBackUnique( slot.GetName() );

			// Format slot name
			wxString name( slot.GetName().AsString().AsChar() );
			if ( baseTemplate != entityTemplate )
			{
				name += wxT(" (Included)");
			}

			// Append slot name
			int index = list->Append( name, new wxSlotListClientData( entityTemplate, slot.GetName() ) );

			// Select
			if ( slot.GetName() == slotNameToSelect )
			{
				list->SetSelection( index );
			}
		}
	}

	// Recurse to includes
	const TDynArray< THandle< CEntityTemplate > >& includes = entityTemplate->GetIncludes();
	for ( Uint32 i=0; i<includes.Size(); ++i )
	{
		CEntityTemplate* includedTemplate = includes[i].Get();
		if ( includedTemplate )
		{
			EmitSlotNames( list, baseTemplate, includedTemplate, slotNameToSelect, slotNames );
		}
	}
}

void CEdEntityEditor::UpdateSlotPage()
{
	UpdateSlotList();
}

void CEdEntityEditor::UpdateSlotList( const CName& slotNameToSelect )
{
	// Clear current lists
	m_lstSlots->Freeze();
	m_lstSlots->Clear();

	// Gather list
	TDynArray< CName > slotNames;
	EmitSlotNames( m_lstSlots, m_template, m_template, slotNameToSelect, slotNames );

	// Finalize list
	if ( m_lstSlots->GetCount() )
	{
		m_lstSlots->Enable();
	}
	else
	{
		m_lstSlots->Append( wxT("(No Slots)") );
		m_lstSlots->SetSelection( 0 );
		m_lstSlots->Disable();
	}

	// Refresh
	m_lstSlots->Thaw();
	m_lstSlots->Refresh();

	// Refresh properties
	wxCommandEvent fakeEvent;
	OnSlotSelectionChanged( fakeEvent );
}

void CEdEntityEditor::RefreshVisualSlotItem()
{
	m_preview->ShowActiveSlot( m_showSlots.GetValue() );
}

void CEdEntityEditor::RefreshVisualSlotChanges()
{
	m_preview->RefreshPreviewSlotChanges();
}

void CEdEntityEditor::UpdateIncludeList()
{
	// Start update
	m_lstIncludes->Freeze();
	m_lstIncludes->Clear();

	// List includes
	const TDynArray< THandle< CEntityTemplate > >& includes = m_template->GetIncludes();
	for ( Uint32 i=0; i<includes.Size(); ++i )
	{
		CEntityTemplate* includedTemplate = includes[i].Get();
		if ( includedTemplate )
		{
			m_lstIncludes->AppendString( includedTemplate->GetDepotPath().AsChar() );
		}
	}

	// Finalize
	if ( m_lstIncludes->GetCount() )
	{
		m_lstIncludes->Enable( true );
	}
	else
	{
		m_lstIncludes->Enable( false );
		m_lstIncludes->AppendString( wxT("(No Included Templates)") );
	}

	// End update
	m_lstIncludes->Thaw();
	m_lstIncludes->Refresh();
}

////////////////////////////////////////////////////////////////////////////////////

class CEntityTemplateDependencyCollector : public IFile
{
public:
	TDynArray< CObject* >		m_visitedObjects;
	TDynArray< CResource* >		m_resources;
	Bool						m_onlyEntityTemplates;

public:
	CEntityTemplateDependencyCollector( Bool onlyEntityTemplates )
		: IFile( FF_Writer | FF_MemoryBased | FF_GarbageCollector )
		, m_onlyEntityTemplates( onlyEntityTemplates )
	{
		// Mark all objects are not visited
		for ( BaseObjectIterator it; it; ++it )
		{
			(*it)->ClearFlag( OF_UserFlag );
		}
	}

	void Serialize( void* buffer, size_t size )
	{
	}

	virtual Uint64 GetOffset() const
	{
		return 0;
	}

	virtual Uint64 GetSize() const
	{
		return 0;
	}

	virtual void Seek( Int64 offset )
	{

	}

	// Pointer serialization
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( pointer && pointerClass->IsObject() )
		{
			CObject* obj = static_cast< CObject* >( pointer );

			// Visit every object only once
			if ( !obj->HasFlag( OF_UserFlag ) )
			{
				// Add to the list of visited objects
				obj->SetFlag( OF_UserFlag );

				// Get the resource pointers
				if ( obj->IsA< CResource >() && !obj->HasFlag( OF_DefaultObject ) )
				{
					CResource* res = static_cast< CResource* >( obj );
					if ( !m_onlyEntityTemplates || res->IsA< CEntityTemplate >() )
					{
						m_resources.PushBack( res );
					}
				}

				// Recurse
				obj->OnSerialize( *this );
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////


void CEdEntityEditor::UpdateDependencyList()
{
	// Start update
	m_lstDependencies->Freeze();
	m_lstDependencies->Clear();

	// Collect dependencies
	const Bool onlyEntityTemplates = m_chkShowTemplatesOnly->IsChecked();
	CEntityTemplateDependencyCollector collector( onlyEntityTemplates );
	m_template->OnSerialize( collector );

	// List dependencies
	for ( Uint32 i=0; i<collector.m_resources.Size(); ++i )
	{
		CResource* referencedResource = collector.m_resources[i];
		String path = referencedResource->GetDepotPath();
		if ( !path.Empty() )
		{
			m_lstDependencies->AppendString( path.AsChar() );
		}
	}

	// End update
	m_lstDependencies->Thaw();
	m_lstDependencies->Refresh();
}

void CEdEntityEditor::UpdateEntityClass()
{
	// Display class name
	String className = m_template ? m_template->GetEntityClassName().AsString() : TXT( "None" );
	m_edbClassName->SetValue( className.AsChar() );
}

namespace EntityEditorUtils
{
	Bool IsTemplateIncluded( const CEntityTemplate* testedTemplate, const CEntityTemplate* includedTemplate )
	{
		// The same
		if ( testedTemplate == includedTemplate )
		{
			return true;
		}

		// Recurse
		const TDynArray< THandle< CEntityTemplate > >& includes = testedTemplate->GetIncludes();
		for ( Uint32 i=0; i<includes.Size(); ++i )
		{
			CEntityTemplate* entityTemplate = includes[i].Get();
			if ( entityTemplate )
			{
				if ( IsTemplateIncluded( entityTemplate, includedTemplate ) )
				{
					return true;
				}
			}
		}

		// Not included
		return false;
	}

	Bool IsTemplateIncludedAtLevel( const CEntityTemplate* testedTemplate, const CEntityTemplate* includedTemplate )
	{
		const TDynArray< THandle< CEntityTemplate > >& includes = testedTemplate->GetIncludes();
		for ( Uint32 i=0; i<includes.Size(); ++i )
		{
			CEntityTemplate* entityTemplate = includes[i].Get();
			if ( entityTemplate == includedTemplate)
			{
				return true;
			}
		}

		// Not included
		return false;
	}
}

void CEdEntityEditor::RebuildEntityAfterIncludesChanged()
{
	// Remove all components that are not included anymore - if they will be saved, their external proxies may match to newly added includes that replaced them
	TDynArray< CComponent* > allComponents;
	CollectEntityComponents( m_preview->GetEntity(), allComponents );

	// Grab all includes
	TDynArray< CEntityTemplate* > loadedIncludesOut;
	::CollectAllIncludes( m_template, loadedIncludesOut );
	for ( Uint32 iInclude = 0; iInclude < m_loadedIncludes.Size(); ++iInclude )
	{
		CEntityTemplate *included = m_loadedIncludes[iInclude];
		if ( included )
		{
			if ( !loadedIncludesOut.Exist( included ) )
			{
				TDynArray< CComponent* > components;
				::CollectAllComponents2( included, components );

				for ( Uint32 iComponent = 0; iComponent < components.Size() ; ++iComponent )
				{
					TDynArray< CComponent* >::iterator found = FindIf( allComponents.Begin(), allComponents.End(), ::SimiliarComponentLookup(components[iComponent]) );
					if ( found != allComponents.End() && (*found)->HasFlag( OF_Referenced ) == true )
					{
						m_preview->GetEntity()->DestroyComponent( *found );
						allComponents.Remove( *found );
					}
				}
			}
		}
	}

	// Set new list of included templates
	m_loadedIncludes = loadedIncludesOut;

	// Update preview world - finalize destruction of any destroyed components
	m_preview->GetWorld()->DelayedActions();

	// Create external proxies
	m_preview->GetEntity()->PrepareEntityForTemplateSaving();

	// Detach template (needed for Capture)
	m_preview->GetEntity()->DetachTemplate();

	// Capture only non-included components and separate streamed components
	m_template->CaptureData( m_preview->GetEntity() ); // reload includes

	// Update the lists
	UpdateAnimEffectsList();
	UpdateDependencyList();
	UpdateIncludeList();
	UpdateSlotPage();
	UpdateAppearanceList();
	UpdateInstancePropertiesList();
	RefreshMeshColoringEntriesList();
	UpdateAnimationTab();
	UpdateAnimSlotsList();
	UpdateInventoryItemsList();
	UpdateDismembermentTab();
}

void CEdEntityEditor::OnIncludeAdd( wxCommandEvent& event )
{
	// Get selected entity
	String entityTemplatePath;
	if ( !GetActiveResource( entityTemplatePath, ClassID< CEntityTemplate >() ) &&
		 !GetActiveResource( entityTemplatePath, ClassID< CCharacterEntityTemplate >() ) )
	{
		wxMessageBox( wxT("Please select an entity template"), wxT("Add include"), wxOK | wxICON_INFORMATION );
		return;
	}

	// Load the entity template
	CEntityTemplate* loadedEntityTemplate = LoadResource< CEntityTemplate >( entityTemplatePath );
	if ( !loadedEntityTemplate )
	{
		wxMessageBox( wxT("Failed to load selected entity template. Unable to include it."), wxT("Add include"), wxOK | wxICON_ERROR );
		return;
	}

	// The same template
	if ( m_template == loadedEntityTemplate )
	{
		wxMessageBox( wxT("You cannot include this template back to itself."), wxT("Add include"), wxOK | wxICON_ERROR );
		return;
	}

	// Already included
	if ( EntityEditorUtils::IsTemplateIncludedAtLevel( m_template, loadedEntityTemplate ) )
	{
		wxMessageBox( wxT("This template is already included."), wxT("Add include"), wxOK | wxICON_ERROR );
		return;
	}

	// Check if template is already included
	if ( EntityEditorUtils::IsTemplateIncluded( m_template, loadedEntityTemplate ) )
	{
		if ( wxNO == wxMessageBox( wxT("Selected template is already included at some level by this entity template. Including it once more is not recomended but may be needed because of some creepy stuff. Are you sure of what ya doing ?"), wxT("Add include"), wxYES_NO | wxICON_EXCLAMATION ) )
		{
			// Canceled, thank God!
			return;
		}
	}

	// Checkout
	if ( m_template->MarkModified() )
	{
		// Add the include
		m_template->GetIncludes().PushBackUnique( loadedEntityTemplate );

		// Refresh
		RebuildEntityAfterIncludesChanged();

		// Update tabs
		UpdateTabs();
	}
}

void CEdEntityEditor::OnIncludeRemove( wxCommandEvent& event )
{
	// Get selected template
	Int32 selectionIndex = m_lstIncludes->GetSelection();
	if ( selectionIndex == -1 )
	{
		return;
	}

	// Load the template
	wxString templateName = m_lstIncludes->GetString( selectionIndex );
	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( templateName.c_str().AsWChar() );
	if ( !entityTemplate )
	{
		return;
	}

	// Checkout
	if ( m_template->MarkModified() )
	{
		// Remove from the include list
		Bool removedSomething = false;
		TDynArray< THandle< CEntityTemplate > >& includes = m_template->GetIncludes();
		for ( Uint32 i=includes.Size(); i>0; --i )
		{
			if ( includes[i-1].Get() == entityTemplate )
			{
				removedSomething = true;
				includes.RemoveAt(i-1);
			}
		}

		// Refresh
		if ( removedSomething )
		{
			RebuildEntityAfterIncludesChanged();
			// Update tabs
			UpdateTabs();
		}
	}
}

void CEdEntityEditor::OnDependencyFilterChanged( wxCommandEvent& event )
{
	UpdateDependencyList();
}

void CEdEntityEditor::OnIncludeClicked( wxCommandEvent& event )
{
	// Get the template template name
	String resourcePath = event.GetString().c_str().AsWChar();
	SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( resourcePath ) );
}

void CEdEntityEditor::OnDependencyClicked( wxCommandEvent& event )
{
	// Get selected template
	Int32 selectionIndex = m_lstDependencies->GetSelection();
	if ( selectionIndex == -1 )
	{
		return;
	}

	// Get the template template name
	String resourcePath = m_lstDependencies->GetString( selectionIndex ).c_str().AsWChar();
	SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( resourcePath ) );
}

#include "itemSelectionDialogs/entityClassSelectorDialog.h"
void CEdEntityEditor::OnChangeEntityClass( wxCommandEvent& event )
{	
	// Get current class
	CClass* currentClass = SRTTI::GetInstance().FindClass( m_template->GetEntityClassName() );
	if ( !currentClass )
	{
		currentClass = ClassID< CEntity >();
	}

	// Ask for new class
	CEdEntityClassSelectorDialog selector( this, currentClass );
	if ( CClass* newClass = selector.Execute() )
	{
		m_template->SetEntityClass( newClass );
		UpdateEntityClass();
		SEvents::GetInstance().DispatchEvent( CNAME( EntityTemplateModified ), CreateEventData( m_template ) );
	}
}

void CEdEntityEditor::UpdateAnimEffectsList()
{
	// Clear current lists
	m_lstAnimations->Clear();
	m_lstEffects->Clear();
	
	// In case of no entity get the template entity
	if ( m_preview->GetEntity() == NULL )
	{
		return;
	}
	
	// Add all animations
	TDynArray< CName > animationNames;
	GetEntityAnimations( m_preview->GetEntity(), animationNames );
	GetTemplateAnimations( m_template, animationNames );
	for ( Uint32 i = 0; i < animationNames.Size(); ++i )
	{
		m_lstAnimations->Append( animationNames[i].AsString().AsChar(), (void*)NULL );
	}

	// Add all entity effects
	TDynArray< CFXDefinition* > effects;
	m_template->GetAllEffects( effects );
	for ( Uint32 i=0; i<effects.Size(); i++ )
	{
		CFXDefinition* effect = effects[i];

		// Add to list
		String display = FormatEffectName( effect );
		m_lstEffects->Append( display.AsChar(), (void*) effects[i] );
	}

	// Update buttons
	checkAnimEffectConnection();
}

void CEdEntityEditor::OnEffectDisconnect( wxCommandEvent& event )
{
	// Get current selection from list
	Int32 nEffectSel = m_lstEffects->GetSelection();
	if ( nEffectSel < 0 )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Get effect
	CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nEffectSel ) );
	if ( selectedFX == NULL )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Open for modify
	if ( selectedFX->MarkModified() )
	{
		// Get template this effect is in
		CEntityTemplate* entityTemplate = selectedFX->FindParent< CEntityTemplate >();
		if ( !entityTemplate )
		{
			return;
		}

		// Inform use
		if ( entityTemplate != m_template && entityTemplate->GetFile() )
		{
			String msg = String::Printf( TXT("Selected effect is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
			wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		}

		// Unbind from animation
		selectedFX->BindToAnimation( CName::NONE );

		// Update effect list
		String display = FormatEffectName( selectedFX );
		m_lstEffects->SetString( nEffectSel, display.AsChar() );

		// Update buttons
		UpdateAnimationLength( selectedFX );
		checkAnimEffectConnection();

		// Global list update
		SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
	}	
}

void CEdEntityEditor::OnEffectConnect( wxCommandEvent& event )
{
	// Get selected animation
	Int32 nSel = m_lstAnimations->GetSelection();
	if ( nSel < 0 )
	{
		wxMessageBox( TXT("Please select animation first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Get current selection from list
	Int32 nEffectSel = m_lstEffects->GetSelection();
	if ( nEffectSel < 0 )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Get effect
	CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nEffectSel ) );
	if ( selectedFX == NULL )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Open for modification
	if ( selectedFX->MarkModified() )
	{
		// Get name of the selected animation
		CName selectedAnimationName = CName( m_lstAnimations->GetString( nSel ).wc_str() );
		CSkeletalAnimationSetEntry* anim = m_preview->GetEntity()->FindAnimation( selectedAnimationName );
		if ( !anim || !anim->GetAnimation() )
		{
			String msg = String::Printf( TXT("Animation '%s' was not found in entity. Please select other animation."), selectedAnimationName.AsString().AsChar() );
			wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
			return;
		}

		// Get template this effect is in
		CEntityTemplate* entityTemplate = selectedFX->FindParent< CEntityTemplate >();
		if ( !entityTemplate )
		{
			return;
		}

		// Inform use
		if ( entityTemplate != m_template && entityTemplate->GetFile() )
		{
			String msg = String::Printf( TXT("Selected effect is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
			wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		}

		// Bind effect to animation
		selectedFX->BindToAnimation( selectedAnimationName );

		// Update effect list
		String display = FormatEffectName( selectedFX );
		m_lstEffects->SetString( nEffectSel, display.AsChar() );

		// Update buttons
		UpdateAnimationLength( selectedFX );
		checkAnimEffectConnection();

		// Global list update
		SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
	}
}

void CEdEntityEditor::OnEffectRemove( wxCommandEvent& event )
{
	// Get current selection from list
	Int32 nEffectSel = m_lstEffects->GetSelection();
	if ( nEffectSel < 0 )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Get effect
	CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nEffectSel ) );
	if ( selectedFX == NULL )
	{
		wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Ask
	String msg = String::Printf( TXT("Sure to remove effect '%s'."), selectedFX->GetName().AsString().AsChar() );
	if ( wxNO == wxMessageBox( msg.AsChar(), wxT("Remove effect"), wxICON_QUESTION | wxYES_NO ) )
	{
		return;
	}

	// Open for modify
	if ( selectedFX->MarkModified() )
	{
		// Get template this effect is in
		CEntityTemplate* entityTemplate = selectedFX->FindParent< CEntityTemplate >();
		if ( !entityTemplate )
		{
			return;
		}

		// Inform use
		if ( entityTemplate != m_template && entityTemplate->GetFile() )
		{
			String msg = String::Printf( TXT("Selected effect is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
			wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		}

		// Close edit panel
		CloseEffectEditor( selectedFX );

		// Unbind from animation
		entityTemplate->RemoveEffect( selectedFX );
		m_lstEffects->Delete( nEffectSel );

		// Global list update
		SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
	}
}

void CEdEntityEditor::OnEffectAdd( wxCommandEvent& event )
{
	if ( m_template->MarkModified() )
	{
		wxString wxName = ::wxGetTextFromUser( TXT("Name of the effect to add:"), TXT("Add effect"), wxEmptyString, this );
		if ( !wxName.IsEmpty() )
		{
			String effectName = String( wxName.wc_str() );

			// Add new effect
			CFXDefinition* newEffect = m_template->AddEffect( effectName );
			if ( newEffect != NULL ) 
			{
				// Add effect to list
				m_lstEffects->Append( effectName.AsChar(), (void*) newEffect );
			}

			// Global list update
			SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
		}
	}
}

void CEdEntityEditor::OnEffectEdit( wxCommandEvent& event )
{
	int nSel = m_lstEffects->GetSelection();
	if ( nSel >= 0 )
	{
		CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nSel ) );
		if ( selectedFX != NULL )
		{
			EditEffect( selectedFX );
			return;
		}
	}
	wxMessageBox( wxT("Select an effect first"), wxT("Error"), wxOK|wxCENTRE, this );
}

void CEdEntityEditor::OnEffectRename( wxCommandEvent& event )
{
	int nSel = m_lstEffects->GetSelection();
	if ( nSel >= 0 )
	{
		CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nSel ) );
		if ( selectedFX != nullptr )
		{
			// Not from this template
			if ( selectedFX->IsContained( m_template ) == false )
			{
				GFeedback->ShowWarn( TXT("Effect is not from current entity template. Cannot rename.") );
				return;
			}

			wxString wxName = ::wxGetTextFromUser( TXT("New name of the effect:"), TXT("Rename effect"), wxEmptyString, this );
			if ( wxName.IsEmpty() == false )
			{
				//
				Uint32 rowCount = m_lstEffects->GetCount();
				for( Uint32 i=0; i<rowCount; ++i )
				{
					if( i == nSel )
					{
						continue;
					}

					if( m_lstEffects->GetString( i ) == wxName )
					{
						GFeedback->ShowError( TXT("Effect with this name is already existed in the entity template") );
						return;
					}
				}

				if( selectedFX->GetName().AsString() != wxName )
				{
					String newName = String( wxName.wc_str() );
					selectedFX->SetName( CName( newName ) );

					// Update effect list
					UpdateAnimEffectsList();
				}
			}
		}
	}
	else
	{
		wxMessageBox( wxT("Select an effect first"), wxT("Error"), wxOK|wxCENTRE, this );
	}
}

void CEdEntityEditor::OnEffectCopy( wxCommandEvent& event )
{
	// Cleanup crap	
	s_fxInClipboard.Clear();

	// Copy FXes
	const Bool shiftPressed = RIM_IS_KEY_DOWN( IK_LShift );
	if ( shiftPressed )
	{
		// Store all effects
		const Uint32 numEffects = m_lstEffects->GetCount();
		for ( Uint32 i=0; i<numEffects; ++i )
		{
			CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( i ) );
			if ( selectedFX )
			{
				s_fxInClipboard.PushBack( selectedFX );
			}
		}
	}
	else
	{
		// Get current selection from list
		Int32 nEffectSel = m_lstEffects->GetSelection();
		if ( nEffectSel < 0 )
		{
			wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
			return;
		}

		// Get effect
		CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nEffectSel ) );
		if ( selectedFX == NULL )
		{
			wxMessageBox( TXT("Please select effect first"), wxT("Warning"), wxOK | wxICON_WARNING );
			return;
		}

		// Store in static variable
		s_fxInClipboard.PushBack( selectedFX );
	}
}

void CEdEntityEditor::OnEffectPaste( wxCommandEvent& event )
{
	// No crap in the clipboard
	if ( s_fxInClipboard.Empty() )
	{
		wxMessageBox( TXT("No effect in clipboard"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Checkout
	if ( m_template->MarkModified() )
	{
		// Process single effect
		if ( s_fxInClipboard.Size() == 1 )
		{
			CFXDefinition* fxToPaste = s_fxInClipboard[0].Get();
			if ( fxToPaste )
			{
				String message = TXT( "Paste effect \'" );
				message += fxToPaste->GetName().AsString();
				message += TXT( "\' as:" );

				String suggestedName = fxToPaste->GetName().AsString();

				if ( m_lstEffects->FindString( suggestedName.AsChar() ) != wxNOT_FOUND )
				{
					// Effect of that name already exists, append "_copy" suffix to the suggested name
					suggestedName += TXT( "_copy" );
				}

				wxString wxName = ::wxGetTextFromUser( message.AsChar(), TXT("Paste effect"), suggestedName.AsChar(), this );
				if ( !wxName.IsEmpty() )
				{
					String copiedEffectName = String( wxName.wc_str() );

					// Add new effect
					CFXDefinition* copiedEffect = SafeCast< CFXDefinition >( fxToPaste->Clone( m_template ) );
					copiedEffect->SetName( CName( copiedEffectName ) );
					if ( copiedEffect != NULL ) 
					{
						if ( m_template->AddEffect( copiedEffect ) )
						{
							// Add effect to list
							m_lstEffects->Append( copiedEffectName.AsChar(), (void*) copiedEffect );

							// Global list update
							SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
						}
					}
				}
			}
		}
		else if ( wxYES == wxMessageBox( wxT("Paste all effects from clipboard ?"), wxT("Effect paste"), wxICON_QUESTION | wxYES_NO ) )
		{
			for ( Uint32 i=0; i<s_fxInClipboard.Size(); ++i )
			{
				CFXDefinition* fxToPaste = s_fxInClipboard[i].Get();
				if ( fxToPaste )
				{
					String suggestedName = fxToPaste->GetName().AsString();
					if ( m_lstEffects->FindString( suggestedName.AsChar() ) != wxNOT_FOUND )
					{
						// Effect of that name already exists, append "_copy" suffix to the suggested name
						suggestedName += TXT( "_copy" );
					}

					// Add new effect
					CFXDefinition* copiedEffect = SafeCast< CFXDefinition >( fxToPaste->Clone( m_template ) );
					copiedEffect->SetName( CName( suggestedName.AsChar() ) );
					if ( copiedEffect != NULL ) 
					{
						if ( m_template->AddEffect( copiedEffect ) )
						{
							// Add effect to list
							m_lstEffects->Append( suggestedName.AsChar(), (void*) copiedEffect );

							// Global list update
							SEvents::GetInstance().DispatchEvent( CNAME( UpdateEffectList ), CreateEventData( m_template ) );
						}
					}
				}
			}
		}
	}
}

void CEdEntityEditor::OnEffectRefreshList( wxCommandEvent& event )
{
	UpdateAnimEffectsList();
}

void CEdEntityEditor::OnAnimationsSelected( wxCommandEvent& event )
{
//	checkAnimEffectConnection();
}

void CEdEntityEditor::OnAnimationsDoubleClicked( wxCommandEvent& event )
{
	Int32 nSel = m_lstAnimations->GetSelection();
	if ( nSel >= 0 )
	{
		// Get selected animation	
		String selectedAnimationName = String( m_lstAnimations->GetString( nSel ).wc_str() );
	}
}

void CEdEntityEditor::OnEffectsSelected( wxCommandEvent& event )
{
	checkAnimEffectConnection();
}

Bool CEdEntityEditor::CloseEffectEditor( CFXDefinition* def )
{
	// Open existing editor
	const Uint32 numPages = m_notebook->GetPageCount();
	for ( Uint32 i=0; i<numPages; i++ )
	{
		CEdEffectEditor* editor = wxDynamicCast( m_notebook->GetPage(i), CEdEffectEditor );
		if ( editor )
		{
			// Check if editor is for given effect
			if ( editor->GetFXDefinition() == def )
			{
				m_notebook->DeletePage( i );
				return true;
			}
		}
	}

	// Not opened
	return false;
}

Bool CEdEntityEditor::UpdateAnimationLength( CFXDefinition* def )
{
	// Calculate animation length
	Float animationLength = 0.0f;
	if ( def && m_preview && m_preview->GetEntity() )
	{
		// Find animation
		CSkeletalAnimationSetEntry* entry = m_preview->GetEntity()->FindAnimation( def->GetAnimationName() );
		if ( entry && entry->GetAnimation() )
		{
			animationLength = entry->GetAnimation()->GetDuration();
		}
	}

	// Update any editor
	const Uint32 numPages = m_notebook->GetPageCount();
	for ( Uint32 i=0; i<numPages; i++ )
	{
		CEdEffectEditor* editor = wxDynamicCast( m_notebook->GetPage(i), CEdEffectEditor );
		if ( editor )
		{
			// Check if editor is for given effect
			if ( editor->GetFXDefinition() == def )
			{
				// Select page
				editor->SetAnimationLength( animationLength );
				return true;
			}
		}
	}

	// Not updated
	return false;
}

CEdEffectEditor* CEdEntityEditor::EditEffect( CFXDefinition* def, Bool forceRecreateEditor )
{
	// We can edit only valid effects
	if ( def )
	{
		// Not from this template
		if ( !def->IsContained( m_template ) )
		{
			wxMessageBox( wxT("Effect is not from current entity template. Cannot edit."), wxT("Warning"), wxOK | wxICON_INFORMATION );
			return NULL;
		}

		// Calculate animation length
		Float animationLength = 0.0f;
		if ( def && m_preview && m_preview->GetEntity() )
		{
			// Find animation
			CSkeletalAnimationSetEntry* entry = m_preview->GetEntity()->FindAnimation( def->GetAnimationName() );
			if ( entry && entry->GetAnimation() )
			{
				animationLength = entry->GetAnimation()->GetDuration();
			}
		}

		// Check if effect editor hasn't been already opened
		const Uint32 numPages = m_notebook->GetPageCount();
		for ( Uint32 i=0; i<numPages; i++ )
		{
			CEdEffectEditor* editor = wxDynamicCast( m_notebook->GetPage(i), CEdEffectEditor );
			if ( editor )
			{
				// Check if editor is for given effect
				if ( editor->GetFXDefinition() == def )
				{
					if ( forceRecreateEditor )
					{
						wxString tabCaption = m_notebook->GetPageText( i );
						Bool wasSelected = ( m_notebook->GetSelection() == i );
						m_notebook->Freeze();
						m_notebook->DeletePage( i );
						editor = new CEdEffectEditor( m_notebook, m_preview->GetEntity(), m_preview->GetTemplate(), def );
						m_notebook->InsertPage( i, editor, tabCaption, wasSelected );
						m_notebook->Thaw();
					}
					else
					{
						m_notebook->SetSelection( i );
					}

					editor->SetAnimationLength( animationLength );
					return editor;
				}
			}
		}

		// Create new editor
		if ( CEdEffectEditor *effectEditor = new CEdEffectEditor( m_notebook, m_preview->GetEntity(), m_preview->GetTemplate(), def ) )
		{
			// Format caption and add to tabs
			wxString tabCaption = String( TXT("Effect [") + def->GetName().AsString() + TXT("]") ).AsChar();
			m_notebook->AddPage( effectEditor, tabCaption, true );
			m_effectEditors.Insert( m_notebook->GetPageCount()-1, effectEditor );

			// Created
			effectEditor->SetAnimationLength( animationLength );
			return effectEditor;
		}
	}

	// Not created
	return NULL;
}

Bool CEdEntityEditor::PlayPreviewEffect( CFXDefinition* def )
{
	// Play effect on selected entity
	CEntity* entity = m_preview->GetEntity();
	if ( entity )
	{
		// Play preview
		return entity->PlayEffectPreview( def );
	}

	// Not played
	return false;
}

void CEdEntityEditor::OnEffectsDoubleClicked( wxCommandEvent& event )
{
	// Get current selection from list
	Uint32 nSel = m_lstEffects->GetSelection();
	if ( nSel >= 0 )
	{
		CFXDefinition* selectedFX = static_cast< CFXDefinition* >( m_lstEffects->GetClientData( nSel ) );
		if ( selectedFX != NULL )
		{
			// Check button
			Bool shiftPressed = wxGetKeyState( WXK_SHIFT );
			if ( shiftPressed )
			{
				// Edit clicked effect
				EditEffect( selectedFX );
			}
			else
			{
				// Play preview effect
				PlayPreviewEffect( selectedFX );
			}
		}
	}
}

void CEdEntityEditor::checkAnimEffectConnection()
{
	Int32 nSel = m_lstEffects->GetSelection();
	if ( nSel >= 0 )
	{
		CFXDefinition* selectedFX = static_cast<CFXDefinition*>( m_lstEffects->GetClientData( nSel ) );
		if ( selectedFX->IsBoundToAnimation() )
		{
			m_btnConnect->Enable( false );
			m_btnDisconnect->Enable( true );
		}
		else
		{
			m_btnConnect->Enable( true );
			m_btnDisconnect->Enable( false );
		}
	}
	else
	{
		m_btnConnect->Enable( false );
		m_btnDisconnect->Enable( false );
	}
}

void CEdEntityEditor::OnBodyPartSelected( wxCommandEvent& event )
{
	m_lstBodyPartStates->Clear();
	m_lstBodyPartStateComponents->Clear();
	m_componentsOnBodyStatesList.Clear();

	Int32 nSel = m_lstBodyParts->GetSelection();
	if ( nSel < 0 )
		return;

	CEntityBodyPart * bodyPart = static_cast<CEntityBodyPart *>( m_lstBodyParts->GetClientData( nSel ) );

	bool isIncluded = m_bodyPartsIncludedFlag[ nSel ];
	m_lstBodyPartStateComponents->Enable( !isIncluded );
	m_btnDelBodyPart->Enable( !isIncluded );
	m_btnDuplicateBodyPartState->Enable( !isIncluded );
	m_btnAddBodyPartState->Enable( !isIncluded );
	m_btnDelBodyPartState->Enable( !isIncluded );

	Uint32 numStates = bodyPart->GetStates().Size();
	for ( Uint32 i = 0; i < numStates; ++i )
	{
		CEntityBodyPartState & state = bodyPart->GetState(i);
		m_lstBodyPartStates->Append( state.GetName().AsString().AsChar(), &state );
	}
}

void CEdEntityEditor::OnBodyPartDoubleClicked( wxCommandEvent& event )
{
	Int32 nSel = m_lstBodyParts->GetSelection();
	if ( nSel < 0 )
		return;
	CEntityBodyPart * bodyPart = static_cast<CEntityBodyPart *>( m_lstBodyParts->GetClientData( nSel ) );
	// is included?
	if ( m_bodyPartsIncludedFlag[nSel] )
		return;

	if ( !m_template->MarkModified() )
		return;

	wxString wxName = ::wxGetTextFromUser( TXT("New name of the body part:"), TXT("Rename body part"), bodyPart->GetName().AsString().AsChar(), this );
	if ( wxName.IsEmpty() )
		return;
	CName partName = CName( wxName.wc_str() );

	// Update appearance tab
	OnAppearanceSelected( wxCommandEvent() );
}

static Bool IsComponentUsuableInAppearance( const CComponent* comp )
{
	if ( comp && comp->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* dc = static_cast< const CDrawableComponent* >( comp );
		if ( !dc->IsUsedInAllAppearances() ) return true;
	}

	if ( comp && comp->IsA< CAnimatedComponent >() )
	{
		const CAnimatedComponent* ac = static_cast< const CAnimatedComponent* >( comp );
		if ( !ac->IsIncludedInAllAppearances() ) return true;
	}

	return false;
}

void CEdEntityEditor::UpdateAppearanceList()
{
	m_lstAppearances->Clear();
	m_lstAppearanceBodyParts->Clear();
	m_appearancesOnList.Clear();
	m_bodyPartsOnAppearanceList.Clear();
	m_appearancesIncludedFlag.Clear();

	CAppearanceComponent* appearanceComponent = m_preview->GetEntity()->FindComponent<CAppearanceComponent>();
	if ( !appearanceComponent )
	{
		return;
	}

	TDynArray<String> filters = String( m_txtAppearancesFilter->GetValue().wc_str() ).Split( TXT(";") );

	TDynArray< CEntityAppearance* > allAppearances;
	m_template->GetAllAppearances( allAppearances );
	Sort( allAppearances.Begin(), allAppearances.End(), CNamedClassPointerSorter<CEntityAppearance>() );

	m_appearancesChoice->Clear();
	m_appearancesChoice->Append( wxString( wxT("(none)") ), (void*)NULL );

	Bool showWarningBox = false;

	for ( Uint32 i = 0; i < allAppearances.Size(); ++i )
	{
		CEntityAppearance *appearance = allAppearances[i];

		m_appearancesChoice->Append( appearance->GetName().AsString().AsChar(), appearance );

		if ( !appearance->GetName().AsString().MatchAny( filters ) )
			continue;

		Bool isIncluded = m_template->GetAppearance( appearance->GetName(), false ) == NULL;
		m_appearancesIncludedFlag.PushBack( isIncluded );
		if ( !isIncluded )
			m_lstAppearances->Append( appearance->GetName().AsString().AsChar() );
		else
			m_lstAppearances->Append( appearance->GetName().AsString().AsChar() + wxString(TXT(" (included)")) );

		// Not a perfect fix, but we check to see if we have sane included templates, and attempt to remove any bad ones.
		if ( !appearance->ValidateIncludedTemplates(m_template) )
		{
			showWarningBox = true;
		}

		m_appearancesOnList.PushBack( appearance );

		m_lstAppearances->Check( m_lstAppearances->GetCount() - 1, m_template->IsAppearanceEnabled( appearance->GetName() ) );
	}
	if ( m_lstAppearances->GetCount() > 0 )
	{
		m_lstAppearances->Select( 0 );
		OnAppearanceSelected( wxCommandEvent() );
		RefreshMeshColoringEntriesList();
		EnableMeshColoringEntryControls( false );
	}
	UpdateVoicetagAppearancesTable();

	if ( showWarningBox )
	{
		wxMessageBox( wxT("There were some templates missing in the template includes. Please check the Data Error Reporter for details."), wxT("Missing Template Includes"), wxCENTRE|wxOK|wxICON_WARNING, this );
	}
}

void CEdEntityEditor::UpdateInstancePropertiesList()
{
	// Get local and all entries
	const TDynArray< SComponentInstancePropertyEntry >& localEntries = m_template->GetLocalInstanceProperties();
	m_visibleEntries.Clear();
	m_template->GetAllInstanceProperties( m_visibleEntries );

	// Setup the list
	m_instancePropertiesList->ClearAll();
	m_instancePropertiesList->DeleteAllItems();
	m_instancePropertiesList->AppendColumn( wxT("Component Name"), wxLIST_FORMAT_LEFT, m_instancePropertiesList->GetSize().GetWidth()/2 - 20 );
	m_instancePropertiesList->AppendColumn( wxT("Property Name"), wxLIST_FORMAT_LEFT, m_instancePropertiesList->GetSize().GetWidth()/2 );

	// Fill the list with the entries
	for ( auto it=m_visibleEntries.Begin(); it != m_visibleEntries.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;

		// Add item
		wxListItem item;
		long id = (long)( it - m_visibleEntries.Begin() );
		item.SetId( id );
		item.SetText( entry.m_component.AsChar() );
		item.SetData( id );
		m_instancePropertiesList->InsertItem( item );

		// Set component name
		m_instancePropertiesList->SetItem( id, 0, entry.m_component.AsChar() );

		// Set property name
		if ( !localEntries.Exist( entry ) ) // highlight included entries
		{
			m_instancePropertiesList->SetItem( id, 1, wxString::Format( wxT("%s (included)"), entry.m_property.AsChar() ) );
			m_instancePropertiesList->SetItemTextColour( id, wxColor( 128, 128, 128 ) );
		}
		else // local entry
		{
			m_instancePropertiesList->SetItem( id, 1, entry.m_property.AsChar() );
			m_instancePropertiesList->SetItemTextColour( id, wxColor( 0, 0, 0 ) );
		}
	}
}

void CEdEntityEditor::SetEntityAppearance( const CEntityAppearance* appearance )
{
	CActor* actor = Cast<CActor>( m_preview->GetEntity() );
	CAppearanceComponent* appearanceComponent = m_preview->GetEntity()->FindComponent<CAppearanceComponent>();
	if ( appearanceComponent )
	{
		appearanceComponent->RemoveCurrentAppearance();
		appearanceComponent->ApplyAppearance( *appearance );
	}
	
	if ( m_preview->GetEntity()->IsA< CGameplayEntity >() )
	{
		ApplyEquipmentDefinitionPreview();
	}

	if ( appearanceComponent )
	{
		appearanceComponent->UpdateCurrentAppearanceAttachments();
		/*
		if ( appearance->GetColorVariants().Empty() == false )
		{
			appearanceComponent->ApplyColorVariant( appearance->GetColorVariant( 0 ) );
		}
		else
		{
			CEntityColorVariant nullColorVariant;
			appearanceComponent->ApplyColorVariant( nullColorVariant );
		}
		*/
	}
	m_graph->ForceLayoutUpdate();
}


void CEdEntityEditor::OnAppearanceSelected( wxCommandEvent& event )
{
	m_lstAppearanceBodyParts->Clear();
	m_bodyPartsOnAppearanceList.Clear();

	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	TDynArray<String> filters = String( m_txtAppearanceBodyPartsFilter->GetValue().wc_str() ).Split( TXT(";") );

	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );

	// is included?
	Bool isIncluded = m_appearancesIncludedFlag[ nSel ];
	m_lstAppearanceBodyParts->Enable( !isIncluded );
	m_btnDelAppearance->Enable( !isIncluded );

	TDynArray< THandle< CEntityTemplate > > allTemplates = appearance->GetIncludedTemplates();
	Sort( allTemplates.Begin(), allTemplates.End(), ResourceClassHandleSorter<CEntityTemplate>() );

	for ( Uint32 i = 0; i < allTemplates.Size(); ++i )
	{
		CEntityTemplate* entTempl = allTemplates[i].Get();
		if ( !entTempl->GetDepotPath().MatchAny( filters ) )
		{
			continue;
		}

		wxString entryString = entTempl->GetDepotPath().AsChar();
		if ( IsTemplateCollapsedInAppearance( appearance, entTempl ) )
		{
			entryString += wxT(" (collapsed)");
		}

		m_lstAppearanceBodyParts->Append( entryString );
		m_bodyPartsOnAppearanceList.PushBack( entTempl );
	}

	m_usesRobe->SetValue( appearance->GetUsesRobe() );
	m_usesRobe->Enable( !isIncluded );

	SetEntityAppearance( appearance );

	UpdateCategoriesList();
	m_categoryProperties->Get().SetNoObject();

	if ( m_mimicsPanel )
	{
		m_mimicsPanel->SetEntity( m_preview->GetEntity() );
	}

	RefreshComponentComments();
}

void CEdEntityEditor::OnAppearanceDoubleClicked( wxCommandEvent& event )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );
	// is included?
	if ( m_appearancesIncludedFlag[ nSel ] )
		return;

	if ( !m_template->MarkModified() )
		return;

	wxString wxName = ::wxGetTextFromUser( TXT("New name of the appearance:"), TXT("Rename appearance"), appearance->GetName().AsString().AsChar(), this );
	if ( wxName.IsEmpty() )
		return;
	CName appearanceName = CName( wxName.wc_str() );

	// appearanceName in use?
	if ( m_template->GetAppearance( appearanceName, true ) )
		return;

	appearance->SetName( appearanceName );
	m_lstAppearances->SetString( nSel, appearanceName.AsString().AsChar() );
}

void CEdEntityEditor::OnAppearanceToggled( wxCommandEvent& event )
{
	Int32 nSel = event.GetInt();
	if ( nSel < 0 )
		return;

	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );
	
	if ( !m_template->MarkModified() )
		return;

	if ( m_lstAppearances->IsChecked( nSel ) )
	{
		m_template->GetEnabledAppearancesNames().PushBackUnique( appearance->GetName() );
	}
	else
	{
		m_template->GetEnabledAppearancesNames().RemoveFast( appearance->GetName() );
	}
}

void CEdEntityEditor::OnActiveAppearancesListContextMenu( wxContextMenuEvent& event )
{
	wxMenu menu;

	// Get the selection (if proper)
	Int32 selection = m_lstAppearances->GetSelection();

	if ( selection == -1 )
	{
		return;
	}
	
	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ selection ] );

	// Create popup menu
	menu.Append( ID_ACTAPPMENU_EXPORT, String::Printf( TXT( "Export %ls to file" ), appearance->GetName().AsChar() ).AsChar());
	menu.Append( ID_ACTAPPMENU_IMPORT, wxT("Import from file") );

	PopupMenu( &menu );
}

void CEdEntityEditor::OnActiveAppearancesListMenuExport ( wxCommandEvent& event )
{	
	CEntityExternalAppearance* entityExternalAppearance = CreateObject<CEntityExternalAppearance>( m_preview->GetEntity() );

	Int32 selection = m_lstAppearances->GetSelection();
	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ selection ] );
	
	if( m_preview->GetEntity()->ExportExternalAppearance( appearance->GetName(), entityExternalAppearance ) )
	{
		String fileName = appearance->GetName().AsChar();
		
		CDirectory* dir = nullptr;

		CEntityTemplate* entityTemplate = m_preview->GetEntity()->GetEntityTemplate();
		if( entityTemplate )
		{
			CDiskFile* diskFile = entityTemplate->GetFile();
			if( diskFile )
			{
				dir = diskFile->GetDirectory();
			}			
		}

		dir = DoResourceSaveDialog( CFileFormat( TXT( "w3app" ), TXT( "Entity external appearance" ) ), fileName, dir );
		if ( dir )
		{
			if( entityExternalAppearance->SaveAs( dir, fileName ) == false )
			{
				wxMessageBox( String::Printf( TXT("Unable to save '%ls' because it will overwrite already loaded resource or IO problem!"), appearance->GetName().AsChar() ).AsChar() );
			}
		}
	}
	else
	{
		wxMessageBox( String::Printf( TXT( "Can not export entity appearance %ls, exporter error!" ), appearance->GetName().AsChar() ).AsChar() );
	}

	entityExternalAppearance->RemoveFromRootSet();
	entityExternalAppearance->Discard();
	entityExternalAppearance = NULL;
}

void CEdEntityEditor::OnActiveAppearancesListMenuImport ( wxCommandEvent& event )
{
	String fileName;
	CDirectory* dir = DoResourceOpenDialog( CFileFormat( TXT( "w3app" ), TXT( "Entity external appearance" ) ), fileName );

	THandle<CResource> resource;
	if ( dir )
	{
		resource = GDepot->LoadResource( dir->GetDepotPath() + fileName );
		if( resource )
		{
			CEntityExternalAppearance* entityAppearanceImport =  Cast<CEntityExternalAppearance>(resource);
			if( m_preview->GetEntity()->ImportExternalAppearance( entityAppearanceImport ) )
			{
				UpdateAppearanceList();
			}
			else
			{
				wxMessageBox( String::Printf( TXT( "Can not add entity appearance %ls, appearance with this name already exists!" ), entityAppearanceImport->GetName().AsChar() ).AsChar() );
			}		
		}
	}
}

void CEdEntityEditor::OnUsesRobeToggled( wxCommandEvent& event )
{
	Bool selected = event.GetInt() == 1;
	Int32 selection = m_lstAppearances->GetSelection();

	if ( selection != -1 )
	{
		CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ selection ] );

		if ( !m_template->MarkModified() )
			return;

		appearance->SetUsesRobe( selected );
	}
}

void CEdEntityEditor::OnAppearanceBodyPartToggled( wxCommandEvent& event )
{
//	Int32 nSel = m_lstAppearances->GetSelection();
//	if ( nSel < 0 )
//		return;
//	CEntityAppearance * appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );
//
//	nSel = event.GetInt();
//	const CEntityBodyPart * bodyPart = m_bodyPartsOnAppearanceList[ nSel ];
//
//	if ( !m_template->MarkModified() )
//		return;
//
//	if ( m_lstAppearanceBodyParts->IsChecked( nSel ) )
//		appearance->AddBodyPart( *bodyPart );
//	else
//		appearance->RemoveBodyPart( *bodyPart );
//
//	CActor* actor = Cast<CActor>( m_preview->GetEntity() );
//	if ( actor )
//	{
//		actor->ApplyAppearance( appearance->GetName() );
//	}
//	else
//	{
//		CAppearanceComponent* appearanceComponent = m_preview->GetEntity()->FindComponent<CAppearanceComponent>();
//		if ( appearanceComponent )
//		{
//			appearanceComponent->ApplyAppearance( appearance->GetName() );
//		}
//	}
}

void CEdEntityEditor::OnAppearanceDuplicate( wxCommandEvent& event )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;
	const CEntityAppearance * oldAppearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );

	if ( !m_template->MarkModified() )
		return;

	wxString wxName = ::wxGetTextFromUser( TXT("Name of the duplicate appearance:"), TXT("Duplicate appearance"), oldAppearance->GetName().AsString().AsChar(), this );
	if ( wxName.IsEmpty() )
		return;

	CName appearanceName = CName( wxName.wc_str() );

	// appearanceName in use?
	if ( m_template->GetAppearance( appearanceName, true ) )
		return;

	CEntityAppearance appearance( *oldAppearance );
	CName oldAppearanceName = appearance.GetName();
	appearance.SetName( appearanceName );

	m_template->AddAppearance( appearance );
	UpdateAppearanceList();

	TDynArray< SEntityTemplateColoringEntry > entries;
	m_template->GetColoringEntriesForAppearance( oldAppearanceName, entries );

	for ( auto it=entries.Begin(); it != entries.End(); ++it )
	{
		const SEntityTemplateColoringEntry& entry = *it;
		m_template->AddColoringEntry( appearanceName, entry.m_componentName, entry.m_colorShift1, entry.m_colorShift2 );
	}

	m_lstAppearances->Select( m_lstAppearances->FindString( appearanceName.AsString().AsChar(), true ) );
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAppearanceAdd( wxCommandEvent& event )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	if ( !m_template->MarkModified() )
		return;

	wxString wxName = ::wxGetTextFromUser( TXT("Name of the appearance to add:"), TXT("Add appearance"), wxEmptyString, this );
	if ( wxName.IsEmpty() )
		return;

	CName appearanceName = CName( wxName.wc_str() );

	// appearanceName in use?
	if ( m_template->GetAppearance( appearanceName, true ) )
		return;

	m_template->AddAppearance( CEntityAppearance( appearanceName ) );
	UpdateAppearanceList();
	m_lstAppearances->Select( m_lstAppearances->FindString( appearanceName.AsString().AsChar(), true ) );
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAppearanceDel( wxCommandEvent& event )
{
	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	if ( !m_template->MarkModified() )
		return;

	// Remove the current appearance's components
	appearanceComponent->RemoveCurrentAppearance();

	const CEntityAppearance *appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );
	m_template->RemoveAppearance( *appearance );

	UpdateAppearanceList();
	if ( m_lstAppearances->GetCount() > 0 )
		m_lstAppearances->Select( Min( nSel, (Int32)m_lstAppearances->GetCount()-1 ) );
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAppearanceTemplateAdded( wxCommandEvent& event )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	CEntityAppearance* appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );

	// Get selected entity
	String entityTemplatePath;
	if ( !GetActiveResource( entityTemplatePath, ClassID< CEntityTemplate >() ) )
	{
		wxMessageBox( wxT("Please select an entity template"), wxT("Add include"), wxOK | wxICON_INFORMATION );
		return;
	}

	// Load the entity template
	CEntityTemplate* loadedEntityTemplate = LoadResource< CEntityTemplate >( entityTemplatePath );
	if ( !loadedEntityTemplate )
	{
		wxMessageBox( wxT("Failed to load selected entity template. Unable to include it."), wxT("Add template include"), wxOK | wxICON_ERROR );
		return;
	}

	// The same template
	if ( m_template == loadedEntityTemplate )
	{
		wxMessageBox( wxT("You cannot include this template back to itself."), wxT("Add template include"), wxOK | wxICON_ERROR );
		return;
	}

	// Already included
	if ( appearance->GetIncludedTemplates().Exist( loadedEntityTemplate ) )
	{
		wxMessageBox( wxT("This appearance already contains the selected template."), wxT("Add template include"), wxOK | wxICON_ERROR );
		return;
	}

	// Check if template is already included
	//if ( IsTemplateIncluded( m_template, loadedEntityTemplate ) )
	//{
	//	if ( wxNO == wxMessageBox( wxT("Selected template is already included at some level by this entity template. Including it once more is not recomended but may be needed because of some creepy stuff. Are you sure of what ya doing ?"), wxT("Add include"), wxYES_NO | wxICON_EXCLAMATION ) )
	//	{
	//		// Canceled, thank God!
	//		return;
	//	}
	//}

	// Checkout
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Add the include
	appearance->IncludeTemplate( loadedEntityTemplate );

	// Refresh
	//RebuildEntityAfterIncludesChanged();
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAppearanceTemplateRemoved( wxCommandEvent& event )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_preview->GetEntity() );
	if ( appearanceComponent && appearanceComponent->HasFlag( NF_IncludedFromTemplate ) )
	{
		wxMessageBox( wxT("Appearances can only be modified in the entity template that contain the appearance component"), wxT("Not here"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	Int32 templSel = m_lstAppearanceBodyParts->GetSelection();
	if ( templSel < 0 )
		return;

	// Get selected entity
	String entityTemplatePath = m_lstAppearanceBodyParts->GetString( templSel );
	if ( entityTemplatePath.EndsWith( TXT(" (collapsed)") ) )
	{
		size_t substringIndex;
		entityTemplatePath.FindSubstring( TXT(" (collapsed)"), substringIndex );
		entityTemplatePath = entityTemplatePath.LeftString( substringIndex );
	}

	// Load the entity template
	CEntityTemplate* loadedEntityTemplate = LoadResource< CEntityTemplate >( entityTemplatePath );

	if ( !m_template->MarkModified() )
		return;

	CEntityAppearance *appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );

	appearance->RemoveTemplate( loadedEntityTemplate );
	
	
	if ( appearanceComponent )
	{
		appearanceComponent->ExcludeAppearanceTemplate( loadedEntityTemplate );
	}

	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAppearanceTemplateCollapseToggled( wxCommandEvent& event )
{
	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return;

	Int32 templSel = m_lstAppearanceBodyParts->GetSelection();
	if ( templSel < 0 )
		return;

	// Get selected entity
	String entityTemplatePath = m_lstAppearanceBodyParts->GetString( templSel );
	if ( entityTemplatePath.EndsWith( TXT(" (collapsed)") ) )
	{
		size_t substringIndex;
		 entityTemplatePath.FindSubstring( TXT(" (collapsed)"), substringIndex );
		entityTemplatePath = entityTemplatePath.LeftString( substringIndex );
	}

	// Load the entity template
	CEntityTemplate* loadedEntityTemplate = LoadResource< CEntityTemplate >( entityTemplatePath );

	if ( !m_template->MarkModified() )
		return;

	CEntityAppearance *appearance = static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );

	// Toggle appearance
	if ( IsTemplateCollapsedInAppearance( appearance, loadedEntityTemplate ) )
	{
		appearance->DecollapseTemplate( loadedEntityTemplate );
	}
	else
	{
		appearance->CollapseTemplate( loadedEntityTemplate );
	}

	// Refresh appearance
	OnAppearanceSelected( wxCommandEvent() );

	// Restore selection
	m_lstAppearances->SetSelection( nSel );
	m_lstAppearanceBodyParts->SetSelection( templSel );
}

void CEdEntityEditor::OnAppearancesFilterChanged( wxCommandEvent& event )
{
	UpdateAppearanceList();
}

void CEdEntityEditor::OnAppearanceBodyPartsFilterChanged( wxCommandEvent& event )
{
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnDisableAppearanceApplication( wxCommandEvent& event )
{
	GDisableAppearances = GetMenuBar()->FindItem( XRCID( "viewDisableAppearanceApplication" ) )->IsChecked();
	SEvents::GetInstance().DispatchEvent( CNAME( EntityTemplateModified ), CreateEventData( m_template ) );
}

SAppearancesAndColoringEntries CEdEntityEditor::m_appearancesAndColorEntriesToCopy;

void CEdEntityEditor::OnCopyAppearanceColoringEntries( wxCommandEvent& event )
{
	m_appearancesAndColorEntriesToCopy.coloringEntries.Clear();
	for ( CEntityAppearance* appearance : m_appearancesOnList )
	{
		m_template->GetColoringEntriesForAppearance( appearance->GetName(), m_appearancesAndColorEntriesToCopy.coloringEntries );
	}

	m_appearancesAndColorEntriesToCopy.appearances.Clear();
	m_appearancesAndColorEntriesToCopy.appearances.PushBack( m_appearancesOnList );
}

void CEdEntityEditor::OnPasteAppearanceColoringEntries( wxCommandEvent& event )
{
	for ( CEntityAppearance* appearance : m_appearancesAndColorEntriesToCopy.appearances )
	{
		m_template->AddAppearance( *appearance );
	}
	for ( SEntityTemplateColoringEntry& entry : m_appearancesAndColorEntriesToCopy.coloringEntries )
	{
		m_template->AddColoringEntry( entry.m_appearance, entry.m_componentName, entry.m_colorShift1, entry.m_colorShift2 );
	}
	UpdateAppearanceList();
}

//////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::OnAddInstancePropertiesEntryClicked( wxCommandEvent& event )
{
	// Make sure we can modify the template
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Find selected component (if any)
	TDynArray< CObject* > selection;
	CComponent* component = nullptr;
	m_graph->GetSelectedObjects( selection );
	for ( auto it=selection.Begin(); it != selection.End(); ++it )
	{
		CComponent* comp = Cast< CComponent >( *it );
		if ( comp != nullptr )
		{
			component = comp;
			break;
		}
	}
	
	if ( component == nullptr )
	{
		wxMessageBox( wxT("Please, choose component first"), wxT("Add Entry"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	// Show the dialog for new entry
	String componentName = component->GetName();
	Int32 propertyInd = -1;

	TDynArray< CProperty* > properties;
	component->GetClass()->GetProperties( properties );

	// remove non editable properties
	properties.Erase( RemoveIf( properties.Begin(), properties.End(), []( CProperty* p ){ return !p->IsEditable(); } ), properties.End() );

	// sort properties alphabetically by their names
	Sort( properties.Begin(), properties.End(), []( CProperty* a, CProperty* b )
		{ return Red::System::StringCompare( a->GetName().AsChar(), b->GetName().AsChar() ) < 0; } );

	// Prepare choices for formatted dialog box
	String propertiesNamesList;
	for ( const CProperty* prop : properties )
	{
		propertiesNamesList += String::Printf( TXT("'%ls'"), prop->GetName().AsChar() );
	}

	// Format dialog box content string
	wxString text;
	text.Printf( wxT("V{'Component Name:'S|'Property Name:'C(%ls)}|H{||||||||||||B@'&OK'|B'&Cancel'}"), propertiesNamesList.AsChar() );

	int button = FormattedDialogBox( this, wxT("Add Entry"), text, &componentName, &propertyInd );
	if ( button == 1 || button == -1 ) // cancel
	{
		return;
	}

	// if user didn't choose any property, display a message and return
	if ( propertyInd < 0 || propertyInd >= (Int32)properties.Size() )
	{
		wxMessageBox( wxT("Wrong property chosen"), wxT("Add Entry"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	String propertyName = properties[ propertyInd ]->GetName().AsString();

	// Check if there is already such an entry
	TDynArray< CName > names;
	m_template->GetInstancePropertiesForComponent( CName( componentName ), names );
	if ( names.Exist( CName( propertyName ) ) )
	{
		wxMessageBox( wxT("An entry for this component and property pair already exists"), wxT("Add Entry"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	// Add new entry and update the list
	m_template->AddInstancePropertyEntry( CName( componentName ), CName( propertyName ) );
	UpdateInstancePropertiesList();
}

void CEdEntityEditor::OnRemoveInstancePropertiesEntryClicked( wxCommandEvent& event )
{
	// Make sure we can modify the template
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Get local entries
	const TDynArray< SComponentInstancePropertyEntry >& localEntries = m_template->GetLocalInstanceProperties();

	// Scan the selected item(s)
	TDynArray< SComponentInstancePropertyEntry > toDelete;
	for ( long item=-1;; )
	{
		item = m_instancePropertiesList->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item == -1 )
		{
			break;
		}

		// Get index from item
		intptr_t index = (intptr_t)m_instancePropertiesList->GetItemData( item );
		ASSERT( index >= 0 && index < (intptr_t)m_visibleEntries.Size(), TXT("m_visibleEntries array and actual list state out of sync") );

		// Check if the selected item is a local one
		if ( !localEntries.Exist( m_visibleEntries[index] ) )
		{
			wxMessageBox( wxT("Cannot remove included instance property entries!"), wxT("Remove Entry"), wxOK|wxCENTRE|wxICON_ERROR, this );
			return;
		}

		toDelete.PushBack( m_visibleEntries[index] );
	}

	// Delete the entries
	for ( auto it=toDelete.Begin(); it != toDelete.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;
		m_template->RemoveInstancePropertyEntry( entry.m_component, entry.m_property );
	}

	// Update UI
	UpdateInstancePropertiesList();
}

void CEdEntityEditor::OnRemoveComponentInstancePropertiesEntriesClicked( wxCommandEvent& event )
{
	// Make sure we can modify the template
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Get local entries
	const TDynArray< SComponentInstancePropertyEntry >& localEntries = m_template->GetLocalInstanceProperties();

	// Scan the selected item(s)
	TDynArray< CName > componentNames;
	for ( long item=-1;; )
	{
		item = m_instancePropertiesList->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item == -1 )
		{
			break;
		}

		// Get index from item
		intptr_t index = (intptr_t)m_instancePropertiesList->GetItemData( item );
		ASSERT( index >= 0 && index < (intptr_t)m_visibleEntries.Size(), TXT("m_visibleEntries array and actual list state out of sync") );

		// Check if the selected item is a local one
		if ( !localEntries.Exist( m_visibleEntries[index] ) )
		{
			wxMessageBox( wxT("Cannot remove included instance property entries!"), wxT("Remove Entry"), wxOK|wxCENTRE|wxICON_ERROR, this );
			return;
		}

		componentNames.PushBackUnique( m_visibleEntries[index].m_component );
	}

	// Collect the entries for all the selected components
	TDynArray< SComponentInstancePropertyEntry > toDelete;
	for ( auto it=componentNames.Begin(); it != componentNames.End(); ++it )
	{
		TDynArray< CName > names;
		m_template->GetInstancePropertiesForComponent( *it, names );
		for ( auto it2=names.Begin(); it2 != names.End(); ++it2 )
		{
			toDelete.PushBackUnique( SComponentInstancePropertyEntry( *it, *it2 ) );
		}
	}

	// Delete the entries
	for ( auto it=toDelete.Begin(); it != toDelete.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;
		m_template->RemoveInstancePropertyEntry( entry.m_component, entry.m_property );
	}

	// Update UI
	UpdateInstancePropertiesList();
}

void CEdEntityEditor::OnRemoveAllInstancePropertiesEntriesClicked( wxCommandEvent& event )
{
	// Make sure we can modify the template
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Confirm with the user
	if ( !YesNo( TXT("Are you sure that you want to delete all entries?") ) )
	{
		return;
	}

	// Make sure the user knows what is going on when we only delete local entries
	if ( m_template->GetLocalInstanceProperties().Size() != m_visibleEntries.Size() )
	{
		wxMessageBox( wxT("Please note that only local entries will be deleted and not entries included from other templates!"), wxT("Remove All Entries"), wxOK|wxCENTRE|wxICON_WARNING, this );
	}

	// Do the job
	m_template->ClearInstancePropertyEntries();

	// Update UI
	UpdateInstancePropertiesList();
}

///////////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::OnColorAppearanceSelected( wxCommandEvent& event )
{
	RefreshMeshColoringEntriesList();

	Int32 selection = m_appearancesChoice->GetSelection();
	if ( selection > 0 )
	{
		m_lstAppearances->Select( selection - 1 );
	}
	OnAppearanceSelected( wxCommandEvent() );
}

void CEdEntityEditor::SetColorControlsFromColorShifts( const CColorShift& primary, const CColorShift& secondary )
{
	m_primaryColorHueSlider->SetValue( primary.m_hue );
	m_primaryColorHueSpinCtrl->SetValue( primary.m_hue );
	m_primaryColorSaturationSlider->SetValue( primary.m_saturation );
	m_primaryColorSaturationSpinCtrl->SetValue( primary.m_saturation );
	m_primaryColorLuminanceSlider->SetValue( primary.m_luminance );
	m_primaryColorLuminanceSpinCtrl->SetValue( primary.m_luminance );
	m_secondaryColorHueSlider->SetValue( secondary.m_hue );
	m_secondaryColorHueSpinCtrl->SetValue( secondary.m_hue );
	m_secondaryColorSaturationSlider->SetValue( secondary.m_saturation );
	m_secondaryColorSaturationSpinCtrl->SetValue( secondary.m_saturation );
	m_secondaryColorLuminanceSlider->SetValue( secondary.m_luminance );
	m_secondaryColorLuminanceSpinCtrl->SetValue( secondary.m_luminance );
}

void CEdEntityEditor::OnMeshColoringEntriesListSelected( wxCommandEvent& event )
{
	if ( m_meshColoringEntriesList->GetSelection() == -1 )
	{
		EnableMeshColoringEntryControls( false );
		return;
	}
	EnableMeshColoringEntryControls( true );

	const SEntityTemplateColoringEntry& entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];
	m_meshColoringEntryMesh->SetValue( entry.m_componentName.AsString().AsChar() );

	SetColorControlsFromColorShifts( entry.m_colorShift1, entry.m_colorShift2 );
}

void CEdEntityEditor::OnDuplicateMeshColorButtonClick( wxCommandEvent& event )
{
	Int32 appIndex;
	if ( ( appIndex = m_appearancesChoice->GetSelection() ) == -1 )
	{
		wxMessageBox( wxT("You need to select an appearance (or \"(none)\") first"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	String name;
	if ( !InputBox( this, TXT("Mesh component name"), TXT("Enter the mesh component name that will be colored"), name, false ) )
	{
		return;
	}
	name.Trim();
	if ( name.Empty() )
	{
		wxMessageBox( wxT("You cannot use an empty string for a name"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	CName appearanceName = appIndex == 0 ? CName::NONE : CName( m_appearancesChoice->GetStringSelection() );
	CName existingMeshName = CName( m_meshColoringEntriesList->GetStringSelection() );
	CName newMeshName = CName( name );
	CColorShift shift1, shift2;

	m_template->FindColoringEntry( appearanceName, existingMeshName, shift1, shift2 );
	m_template->AddColoringEntry( appearanceName, newMeshName, shift1, shift2 );
	RefreshMeshColoringEntriesList();
	m_preview->GetEntity()->ApplyMeshComponentColoring();
	m_meshColoringEntriesList->SetStringSelection( newMeshName.AsString().AsChar() );
	OnMeshColoringEntriesListSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnAddMeshColorButtonClick( wxCommandEvent& event )
{
	Int32 appIndex;
	if ( ( appIndex = m_appearancesChoice->GetSelection() ) == -1 )
	{
		wxMessageBox( wxT("You need to select an appearance (or \"(none)\") first"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	String name;
	if ( !InputBox( this, TXT("Mesh component name"), TXT("Enter the mesh component name that will be colored"), name, false ) )
	{
		return;
	}
	name.Trim();
	if ( name.Empty() )
	{
		wxMessageBox( wxT("You cannot use an empty string for a name"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	CName appearanceName = appIndex == 0 ? CName::NONE : CName( m_appearancesChoice->GetStringSelection() );
	CName meshName = CName( name );
	m_template->AddColoringEntry( appearanceName, meshName, CColorShift(), CColorShift() );
	RefreshMeshColoringEntriesList();
	m_preview->GetEntity()->ApplyMeshComponentColoring();
	m_meshColoringEntriesList->SetStringSelection( meshName.AsString().AsChar() );
	OnMeshColoringEntriesListSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnRemoveMeshColorButtonClick( wxCommandEvent& event )
{
	String name;
	Int32 appIndex;

	if ( ( appIndex = m_appearancesChoice->GetSelection() ) == -1 )
	{
		wxMessageBox( wxT("You need to select an appearance (or \"(none)\") first"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}

	if ( !YesNo( TXT("Are you sure that you want to remove this mesh coloring entry?") ) )
	{
		return;
	}

	CName appearanceName = appIndex == 0 ? CName::NONE : CName( m_appearancesChoice->GetStringSelection() );
	CName meshName = CName( m_meshColoringEntriesList->GetStringSelection() );
	
	// Update to zero shifting so that the mesh proxy matrices will be reset
	m_template->UpdateColoringEntry( appearanceName, meshName, CColorShift(), CColorShift() );
	m_preview->GetEntity()->ApplyMeshComponentColoring();

	// Now remove the entry
	m_template->RemoveColoringEntry( appearanceName, meshName );
	RefreshMeshColoringEntriesList();
	m_meshColoringEntriesList->SetSelection( -1 );
	OnMeshColoringEntriesListSelected( wxCommandEvent() );
}

void CEdEntityEditor::OnMeshColoringEntryParametersChanged( wxCommandEvent& event )
{
	if ( !m_template->MarkModified() )
	{
		return;
	}

	if ( m_meshColoringEntriesList->GetSelection() == -1 )
	{
		EnableMeshColoringEntryControls( false );
		return;
	}
	EnableMeshColoringEntryControls( true );

	SEntityTemplateColoringEntry entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];

	Uint16 primaryHue = entry.m_colorShift1.m_hue		 != m_primaryColorHueSlider->GetValue()		   ? m_primaryColorHueSlider->GetValue()		: m_primaryColorHueSpinCtrl->GetValue();
	Int8 primarySat	  = entry.m_colorShift1.m_saturation != m_primaryColorSaturationSlider->GetValue() ? m_primaryColorSaturationSlider->GetValue() : m_primaryColorSaturationSpinCtrl->GetValue();
	Int8 primaryLum   = entry.m_colorShift1.m_luminance	 != m_primaryColorLuminanceSlider->GetValue()  ? m_primaryColorLuminanceSlider->GetValue()  : m_primaryColorLuminanceSpinCtrl->GetValue();
	Uint16 secondaryHue = entry.m_colorShift2.m_hue		   != m_secondaryColorHueSlider->GetValue()		   ? m_secondaryColorHueSlider->GetValue()		  : m_secondaryColorHueSpinCtrl->GetValue();
	Int8 secondarySat	= entry.m_colorShift2.m_saturation != m_secondaryColorSaturationSlider->GetValue() ? m_secondaryColorSaturationSlider->GetValue() : m_secondaryColorSaturationSpinCtrl->GetValue();
	Int8 secondaryLum   = entry.m_colorShift2.m_luminance  != m_secondaryColorLuminanceSlider->GetValue()  ? m_secondaryColorLuminanceSlider->GetValue()  : m_secondaryColorLuminanceSpinCtrl->GetValue();

	entry.m_colorShift1.m_hue = primaryHue;
	entry.m_colorShift1.m_saturation = primarySat;
	entry.m_colorShift1.m_luminance = primaryLum;
	entry.m_colorShift2.m_hue = secondaryHue;
	entry.m_colorShift2.m_saturation = secondarySat;
	entry.m_colorShift2.m_luminance = secondaryLum;

	SetColorControlsFromColorShifts( entry.m_colorShift1, entry.m_colorShift2 );

	if ( entry.m_componentName != CName::NONE )
	{
		m_template->UpdateColoringEntry( entry.m_appearance, entry.m_componentName, entry.m_colorShift1, entry.m_colorShift2 );
		m_preview->GetEntity()->ApplyMeshComponentColoring();
	}
}

void CEdEntityEditor::OnCopyColorsButtonClicked ( wxCommandEvent& event )
{
	if ( m_meshColoringEntriesList->GetSelection() == -1 )
	{
		return;
	}

	SEntityTemplateColoringEntry entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];
	CEdColorShiftPairEditorPopup::DoCopyColorShiftsToClipboard( &entry.m_colorShift1, &entry.m_colorShift2 );

	OnMeshColoringEntryParametersChanged( wxCommandEvent() );
}

void CEdEntityEditor::OnPasteColorsButtonClicked ( wxCommandEvent& event )
{
	Bool pasted1, pasted2;
	CColorShift primary, secondary;

	CEdColorShiftPairEditorPopup::PasteColorShiftsFromClipboard( primary, secondary, pasted1, pasted2 );
	SetColorControlsFromColorShifts( primary, secondary );

	OnMeshColoringEntryParametersChanged( wxCommandEvent() );
}

void CEdEntityEditor::OnSwapColorsButtonClicked ( wxCommandEvent& event )
{
	if ( m_meshColoringEntriesList->GetSelection() == -1 )
	{
		return;
	}

	SEntityTemplateColoringEntry entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];
	CColorShift tempColorShift = entry.m_colorShift1;
	entry.m_colorShift1 = entry.m_colorShift2;
	entry.m_colorShift2 = tempColorShift;

	SetColorControlsFromColorShifts( entry.m_colorShift1, entry.m_colorShift2 );
	
	OnMeshColoringEntryParametersChanged( wxCommandEvent() );
}

void CEdEntityEditor::EnableMeshColoringEntryControls( Bool enable )
{
	m_meshColoringEntryMesh->Enable( enable );
	m_primaryColorHueSlider->Enable( enable );
	m_primaryColorHueSpinCtrl->Enable( enable );
	m_primaryColorSaturationSlider->Enable( enable );
	m_primaryColorSaturationSpinCtrl->Enable( enable );
	m_primaryColorLuminanceSlider->Enable( enable );
	m_primaryColorLuminanceSpinCtrl->Enable( enable );
	m_secondaryColorHueSlider->Enable( enable );
	m_secondaryColorHueSpinCtrl->Enable( enable );
	m_secondaryColorSaturationSlider->Enable( enable );
	m_secondaryColorSaturationSpinCtrl->Enable( enable );
	m_secondaryColorLuminanceSlider->Enable( enable );
	m_secondaryColorLuminanceSpinCtrl->Enable( enable );
}

void CEdEntityEditor::RefreshMeshColoringEntriesList()
{
	// Fill appearances
	CEntityAppearance* appearance = RetrieveSelectedColorAppearance();
	CName appearanceName = appearance ? appearance->GetName() : CName::NONE;
	auto entries = m_template->GetAllColoringEntries();
	wxString previousSelection = m_meshColoringEntriesList->GetStringSelection();

	m_meshColoringListToEntryMap.Clear();
	m_meshColoringEntriesList->Clear();

	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		auto entry = entries[i];
		if ( entry.m_appearance == appearanceName )
		{
			m_meshColoringListToEntryMap[m_meshColoringEntriesList->Append( entry.m_componentName.AsString().AsChar() )] = i;
		}
	}

	m_meshColoringEntriesList->SetStringSelection( previousSelection );
}

CEntityAppearance* CEdEntityEditor::RetrieveSelectedColorAppearance()
{
	Int32 appearanceIndex = m_appearancesChoice->GetSelection();

	// If there's no selection, we probably just refreshed the appearance list, and so any previous selection is cleared.
	if ( appearanceIndex < 0 ) return NULL;

	return static_cast<CEntityAppearance*>( m_appearancesChoice->GetClientData( appearanceIndex ) );
}

void CEdEntityEditor::OnSwapAllColors ( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_template->GetAllColoringEntries().Size(); ++i )
	{
		SEntityTemplateColoringEntry& entry = const_cast< SEntityTemplateColoringEntry& >( m_template->GetAllColoringEntries()[i] );
		CColorShift tempShift = entry.m_colorShift1;
		entry.m_colorShift1 = entry.m_colorShift2;
		entry.m_colorShift2 = tempShift;
	}

	if ( m_meshColoringEntriesList->GetSelection() != -1 )
	{
		SEntityTemplateColoringEntry entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];
		SetColorControlsFromColorShifts( entry.m_colorShift1, entry.m_colorShift2 );
	}
	else if ( m_appearancesChoice->GetSelection() > 0 )
	{
		m_meshColoringEntriesList->Select( 0 );
		OnMeshColoringEntriesListSelected( wxCommandEvent() );
	}

	OnMeshColoringEntryParametersChanged( wxCommandEvent() );
}

void CEdEntityEditor::OnPasteAllColors ( wxCommandEvent& event )
{
	Bool pasted1, pasted2;
	CColorShift primary, secondary;

	if ( !CEdColorShiftPairEditorPopup::PasteColorShiftsFromClipboard( primary, secondary, pasted1, pasted2 ) )
		return;

	Int32 appearanceNumber = m_appearancesChoice->GetSelection();

	// '(none)' appearance
	if ( appearanceNumber == 0 )
		return;

	CName appearanceName = CName( m_appearancesChoice->GetString( appearanceNumber ) );

	for ( Uint32 i = 0; i < m_template->GetAllColoringEntries().Size(); ++i )
	{
		SEntityTemplateColoringEntry& entry = const_cast< SEntityTemplateColoringEntry& >( m_template->GetAllColoringEntries()[i] );
		if ( entry.m_appearance == appearanceName )
		{
			entry.m_colorShift1 = primary;
			entry.m_colorShift2 = secondary;
		}
	}

	if ( m_meshColoringEntriesList->GetSelection() != -1 )
	{
		SEntityTemplateColoringEntry entry = m_template->GetAllColoringEntries()[m_meshColoringListToEntryMap[m_meshColoringEntriesList->GetSelection()]];
		SetColorControlsFromColorShifts( entry.m_colorShift1, entry.m_colorShift2 );
	}
	else
	{
		m_meshColoringEntriesList->Select( 0 );
		OnMeshColoringEntriesListSelected( wxCommandEvent() );
	}

	OnMeshColoringEntryParametersChanged( wxCommandEvent() );
}

///////////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::UpdateReactionsList()
{
	m_reactionsList->Clear();

	TDynArray< CAIProfile* > profiles;
	m_template->GetAllParameters< CAIProfile >( profiles );
	for ( TDynArray< CAIProfile* >::iterator profIt = profiles.Begin(); 
		profIt != profiles.End(); ++profIt )
	{
		CAIProfile* profile = *profIt;
		if ( !profile )
		{
			continue;
		}

		const TDynArray< THandle< CAIReaction > >& fields = profile->GetReactions();
		for ( TDynArray< THandle< CAIReaction > >::const_iterator fieldIt = fields.Begin(); 
			fieldIt != fields.End(); ++fieldIt )
		{
			CAIReaction* reaction = (*fieldIt).Get();
			if ( !reaction )
			{
				continue;
			}

			CEntityTemplate* entityTemplate = reaction->FindParent< CEntityTemplate >();
			if( entityTemplate != m_template )
			{
				wxString text;
				if( entityTemplate->GetFile() )
				{
					text = wxString::Format( wxT("%s (Inherited: %s)" ), reaction->m_fieldName.AsString().AsChar(), entityTemplate->GetFile()->GetDepotPath().AsChar() );
				}
				else
				{
					text = wxString::Format( wxT( "%s (Inherited: NULL)" ), reaction->m_fieldName.AsString().AsChar() );
				}

				m_reactionsList->Append( text, (void*)reaction );
			}
			else
			{
				m_reactionsList->Append( reaction->m_fieldName.AsString().AsChar(), (void*)reaction );
			}
		}
	}
}


void CEdEntityEditor::OnSlotAdd( wxCommandEvent& event )
{
	// Ask for slot name
	String slotNameString;
	if ( !InputBox( this, TXT("New slot"), TXT("Enter name of the new slot"), slotNameString, false ) )
	{
		return;
	}

	// Create the slot
	CName slotName( slotNameString.AsChar() );
	if ( !m_template->AddSlot( slotName ) )
	{
		wxMessageBox( wxT("Unable to add slot to the entity template"), wxT("Add slot"), wxOK | wxICON_ERROR );
		return;
	}

	// Update the list of the slots
	UpdateSlotList( slotName );
}

void CEdEntityEditor::OnSlotRemove( wxCommandEvent& event )
{
	// The selected slot
	const EntitySlot* selectedSlot = NULL;

	// Get selected slot
	Int32 selection = m_lstSlots->GetSelection();
	if ( selection != -1 && m_lstSlots->HasClientObjectData() )
	{
		wxSlotListClientData* data = static_cast< wxSlotListClientData* >( m_lstSlots->GetClientObject( selection ) );
		if ( data )
		{
			// Make sure template is valid
			if ( data->m_template.Get() != m_template )
			{
				wxMessageBox( wxT("Selected slot is not from this template. Cannot remove."), wxT("Remove slot"), wxOK | wxICON_ERROR );
				return;
			}

			// Remove slot
			if ( !m_template->RemoveSlot( data->m_slotName ) )
			{
				wxMessageBox( wxT("Failed to remove selected slot."), wxT("Remove slot"), wxOK | wxICON_ERROR );
				return;
			}

			// Update list
			UpdateSlotList();
		}
	}
}

void CEdEntityEditor::OnSlotSelectionChanged( wxCommandEvent& event )
{
	// The selected slot
	const EntitySlot* selectedSlot = nullptr;

	// Get selected slot
	Int32 selection = m_lstSlots->GetSelection();
	if ( selection != -1 && m_lstSlots->HasClientObjectData() )
	{
		wxSlotListClientData* data = static_cast< wxSlotListClientData* >( m_lstSlots->GetClientObject( selection ) );
		if ( data )
		{
			CEntityTemplate* entityTemplate = data->m_template.Get();
			if ( entityTemplate )
			{
				selectedSlot = entityTemplate->FindSlotByName( data->m_slotName, false );
			}
		}
	}

	// Update properties
	m_slotProperties->SetSlot( selectedSlot );

	if ( m_preview->m_dispSlotActive )
	{
		if ( m_freezeEntityPose.GetValue() )
		{
			m_preview->RefreshPreviewSlotItem( selectedSlot );
			
			if ( selectedSlot->GetBoneName() != CName::NONE )
			{
				m_preview->SetWidgetSpace( ERPWidgetSpace::RPWS_Local );
			}
			else
			{
				m_preview->SetWidgetSpace( ERPWidgetSpace::RPWS_Global );
			}
			OnPreviewWidgetSpaceChanged();
		}
		RefreshVisualSlotItem();
	}
}

void CEdEntityEditor::OnSlotModified( wxCommandEvent& event )
{
	// Get selected slot
	Bool somethingChanged = false;
	const Uint32 count = m_lstSlots->GetCount();
	for ( Uint32 i=0; i<count; ++i )
	{
		wxSlotListClientData* data = static_cast< wxSlotListClientData* >( m_lstSlots->GetClientObject( i ) );
		if ( data )
		{
			CEntityTemplate* entityTemplate = data->m_template.Get();
			if ( !entityTemplate || !entityTemplate->FindSlotByName( data->m_slotName, false ) )
			{
				somethingChanged = true;
				break;
			}
		}
	}
	// Redraw is needed
	if ( somethingChanged )
	{
		UpdateSlotList();
	}
	if ( m_preview->m_dispSlotActive )
	{
		RefreshVisualSlotChanges();
	}
	ApplyEquipmentDefinitionPreview();
}

void CEdEntityEditor::OnSlotShow( wxCommandEvent& event )
{
	RefreshVisualSlotItem();
}

void CEdEntityEditor::OnSlotOverride( wxCommandEvent& event )
{
	Int32 selection = m_lstSlots->GetSelection();
	if ( selection != -1 && m_lstSlots->HasClientObjectData() )
	{
		wxSlotListClientData* data = static_cast< wxSlotListClientData* >( m_lstSlots->GetClientObject( selection ) );
		if ( data )
		{
			CEntityTemplate* entityTemplate = data->m_template.Get();
			if ( entityTemplate )
			{
				const EntitySlot* selectedSlot = entityTemplate->FindSlotByName( data->m_slotName, false );
				if ( selectedSlot )
				{
					CName slotName = data->m_slotName;

					EntitySlotInitInfo info;
					info.m_transform = selectedSlot->GetTransform();
					info.m_boneName = selectedSlot->GetBoneName();
					info.m_componentName = selectedSlot->GetComponentName();

					if ( m_template->AddSlot( slotName, &info ) )
					{
						// Update the list of the slots
						UpdateSlotList( slotName );
						return;
					}
				}
			}
		}

		wxMessageBox( wxT("Unable to override slot to the entity template"), wxT("Override slot"), wxOK | wxICON_ERROR );
	}
}

void CEdEntityEditor::OnSlotPageShow()
{
	/* intentionally empty */
}

void CEdEntityEditor::OnSlotPageHide()
{
	// Unfreeze animation
	m_freezeEntityPose.SetValue( false );
	// Hide slots
	m_showSlots.SetValue( false );
}

void CEdEntityEditor::OnAiProfileNotebookPageChanged( wxNotebookEvent& event )
{
	const Int32 sel = event.GetSelection();	
	if( sel >= 0)
	{
		const wxString text = m_aiProfileNotebook->GetPageText( sel );
		if( text == TXT("Reactions") )
		{
			UpdateReactionsList();
		}
		if ( text == TXT("Misc") ) // Misc tab
		{
			FillAttitudeGroupChoice();
		}
		if ( text == TXT("Minigame") )
		{
			UpdateMinigameTabGuiWWStatus();
		}
	}	
	event.Skip();
}

void CEdEntityEditor::OnReactionSelected( wxCommandEvent& event )
{
	Int32 selected = m_reactionsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CAIReaction* selectedReaction = static_cast< CAIReaction* >( m_reactionsList->GetClientData( selected ) );
	if ( !selectedReaction )
	{
		return;
	}
	m_reactionProperties->Get().SetObject( selectedReaction );
}

void CEdEntityEditor::OnAddReaction( wxCommandEvent& event )
{
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	if ( !profile )
	{
		// create a new profile if the template doesn't have one
		profile = CreateObject< CAIProfile >( m_template );
		m_template->AddParameterUnique( profile );
		m_template->MarkModified();
	}

	CAIReaction* reaction = CreateObject< CAIReaction >( profile ); 
	profile->AddReaction( reaction );

	m_template->MarkModified();
	UpdateReactionsList();
}

void CEdEntityEditor::OnRemoveReaction( wxCommandEvent& event )
{
	Int32 selected = m_reactionsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CAIReaction* selectedReaction = static_cast< CAIReaction* >( m_reactionsList->GetClientData( selected ) );
	if ( !selectedReaction )
	{
		return;
	}

	// find the parent profile of the reaction
	CAIProfile* profile = Cast< CAIProfile >( selectedReaction->GetParent() );
	if ( !profile )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedReaction->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected reaction is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
	}

	profile->RemoveReaction( selectedReaction );
	selectedReaction->Discard();

	entityTemplate->MarkModified();
	UpdateReactionsList();
	m_reactionProperties->Get().SetNoObject();
}

void CEdEntityEditor::OnMoveReactionUp( wxCommandEvent& event )
{
	MoveReaction( true );
}

void CEdEntityEditor::OnMoveReactionDown( wxCommandEvent& event )
{
	MoveReaction( false );
}

void CEdEntityEditor::MoveReaction( Bool up )
{
	Int32 selected = m_reactionsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CAIReaction* selectedReaction = static_cast< CAIReaction* >( m_reactionsList->GetClientData( selected ) );
	if ( !selectedReaction )
	{
		return;
	}

	// find the parent profile of the reaction
	CAIProfile* profile = Cast< CAIProfile >( selectedReaction->GetParent() );
	if ( !profile )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedReaction->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected reaction is from included template '%s', cannot move."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	profile->MoveReaction( selectedReaction, up );

	entityTemplate->MarkModified();
	UpdateReactionsList();

	if( up )
		selected--;
	else
		selected++;

	selected = ::Max( selected, 0 );
	m_reactionsList->SetSelection( selected, true );
}

void CEdEntityEditor::OnReactionChanged( wxCommandEvent& event )
{
	UpdateReactionsList();

	Int32 selected = m_reactionsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CAIReaction* selectedReaction = static_cast< CAIReaction* >( m_reactionsList->GetClientData( selected ) );
	if ( !selectedReaction )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedReaction->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected reaction is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
	}
	entityTemplate->MarkModified();
}

void CEdEntityEditor::OnSenseTypeChanged( wxCommandEvent& event )
{
	FillSenseProperties( true );
}

void CEdEntityEditor::OnSensePropertiesChanged( wxCommandEvent& event )
{
	m_template->MarkModified();
}

void CEdEntityEditor::OnSenseInheritClicked( wxCommandEvent& event )
{
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	if ( !profile )
	{
		// create a new profile if the template doesn't have one
		profile = CreateObject< CAIProfile >( m_template );
		m_template->AddParameterUnique( profile );
		m_template->MarkModified();
	}

	EAISenseType senseType = GetSelectedSenseType();

	if( m_senseInheritCheckBox->IsChecked() )
	{
		CAISenseParams* senseParams = profile->GetSenseParams( senseType );		

		m_senseProperties->Get().SetNoObject();
		profile->SetSenseParams( senseType, NULL );		

		if( senseParams )
		{
			senseParams->Discard();
		}

		m_template->MarkModified();
	}
	else
	{
		CAISenseParams* senseParams = profile->GetSenseParams( senseType );
		if( senseParams == NULL )
		{
			CAISenseParams* senseParams = CreateObject< CAISenseParams >( profile ); 
			profile->SetSenseParams( senseType, senseParams );
			m_template->MarkModified();
		}		
	}

	FillSenseProperties( false );
}

EAISenseType CEdEntityEditor::GetSelectedSenseType()
{
	Int32 i = m_senseChoice->GetSelection();
	if( i >= 0 )
	{
		wxString sense = m_senseChoice->GetString( i );
		if( sense == wxT( "Vision" ) )
			return AIST_Vision;
		else if( sense == wxT( "Absolute" ) )
			return AIST_Absolute;
	}

	return AIST_Invalid;
}

void CEdEntityEditor::FillSenseProperties( Bool setCheckBox )
{
	EAISenseType senseType = GetSelectedSenseType();

	CEntityTemplate* templ = CacheSenseParams( senseType );
	if( templ && templ != m_template )
	{
		m_senseTemplateStaticText->SetLabel( templ->GetFile()->GetDepotPath().AsChar() );
	}
	else
	{
		m_senseTemplateStaticText->SetLabel( TXT("") );
	}

	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	if( profile )
	{
		CAISenseParams* senseParams = profile->GetSenseParams( senseType );		
		if( senseParams )
		{
			m_senseProperties->Enable();
			m_senseProperties->Get().SetObject( senseParams );			

			if( setCheckBox )
				m_senseInheritCheckBox->SetValue( false );

			return;
		}
	}

	// No profile or no params, use inheritance
	CAISenseParams* senseParams = GetCachedSenseParams( senseType );
	m_senseProperties->Disable();
	m_senseProperties->Get().SetObject( senseParams );

	if( setCheckBox )
		m_senseInheritCheckBox->SetValue( true );
}

CEntityTemplate* CEdEntityEditor::CacheSenseParams( EAISenseType senseType )
{
	CEntity* entity = m_preview->GetEntity();
	if ( entity && entity->IsA< CNewNPC >() )
	{
		return SafeCast< CNewNPC >( entity )->CacheSenseParams( senseType );
	}

	return NULL;
}

CAISenseParams* CEdEntityEditor::GetCachedSenseParams( EAISenseType senseType ) const
{
	CEntity* entity = m_preview->GetEntity();
	if ( entity && entity->IsA< CNewNPC >() )
	{
		return SafeCast< CNewNPC >( entity )->GetSenseParams( senseType );
	}

	return NULL;
}

Bool CEdEntityEditor::RunAiWizard( )
{
	CAIWizardTemplateParam*const aiWizardTemplateParam		= m_template->FindParameter< CAIWizardTemplateParam >( false );
	CAIWizardTemplateParam*const deepAIWizardTemplateParam	= m_template->FindParameter< CAIWizardTemplateParam >( true );
	CWizardDefinition* const aiWizardResource				= static_cast<CWizardDefinition*>( ( aiWizardTemplateParam && aiWizardTemplateParam->m_aiWizardResource.Get() ) ? aiWizardTemplateParam->m_aiWizardResource.Get() : ( deepAIWizardTemplateParam ? deepAIWizardTemplateParam->m_aiWizardResource.Get() : nullptr ) );

	if ( aiWizardResource == nullptr )
	{
		return false;
	}	
	CAIPresetsTemplateParam* aiPresetsTemplateParam			= m_template->FindParameter< CAIPresetsTemplateParam >( false );

	
	ASSERT( aiPresetsTemplateParam );
	
	aiPresetsTemplateParam->m_templateList.Clear();
	CEdWizardSavedAnswers *const savedAnswers = aiPresetsTemplateParam->GetAIWizardAswers();
	CEdWizardTemplate* wizard = new CEdWizardTemplate( this, "aiPresetWizard", aiWizardResource, aiPresetsTemplateParam, savedAnswers );
	if( wizard->IsLoaded()  )
	{
		wizard->RunWizard( wizard->GetFirstPage() );
		aiPresetsTemplateParam->m_templateList.Clear();
		wizard->Commit();
		wizard->Destroy();
	}	
	return true;
}

void CEdEntityEditor::OnAITreeSetupChanged ( wxCommandEvent& event )
{
#ifdef AI_WIZARD_BACKWARD_COMPAT
	// Run som cleanup for backward compat :
	CAIProfile* aiprofile			= m_template->FindParameter< CAIProfile >( false );	
	if ( aiprofile )
	{
		aiprofile->ClearOldDataFromIncludes();
	}
#endif

	// Now depending on selection do stuff
	const String selectionString = m_aiTreeEditChoice->GetStringSelection().wc_str();
	
	if ( selectionString == AITREE_SELECTION_INHERIT )
	{
		// Wipe out all we are inheriting
		CAIPresetsTemplateParam* aiPresetsTemplateParam			= m_template->FindParameter< CAIPresetsTemplateParam >( false );
		m_template->RemoveParameter( aiPresetsTemplateParam );

		CAIBaseTreeTemplateParam* aiBaseTreeTemplateParam		= m_template->FindParameter< CAIBaseTreeTemplateParam >( false );	
		m_template->RemoveParameter( aiBaseTreeTemplateParam );
	}
	else if ( selectionString == AITREE_SELECTION_USEAIWIZARD )
	{
		// Removing base custom tree
		CAIBaseTreeTemplateParam* aiBaseTreeTemplateParam		= m_template->FindParameter< CAIBaseTreeTemplateParam >( false, false );	// aiPresetsList can have an AIBaseTree inside
		m_template->RemoveParameter( aiBaseTreeTemplateParam );

		// adding an CAIPresetsTemplateParam if there isn't one
		CAIPresetsTemplateParam* aiPresetsTemplateParam			= m_template->FindParameter< CAIPresetsTemplateParam >( false );
		if ( aiPresetsTemplateParam == nullptr )
		{
			aiPresetsTemplateParam = CreateObject< CAIPresetsTemplateParam >( m_template );
			m_template->AddParameterUnique( aiPresetsTemplateParam );
		}
	}
	else  // custom tree
	{	
		// removing ai wizard stuff
		CAIPresetsTemplateParam* aiPresetsTemplateParam			= m_template->FindParameter< CAIPresetsTemplateParam >( false );
		m_template->RemoveParameter( aiPresetsTemplateParam );

		// We need an CAIBaseTreeTemplateParam to set our custom tree 
		CAIBaseTreeTemplateParam* aiBaseTreeTemplateParam		= m_template->FindParameter< CAIBaseTreeTemplateParam >( false );
		if ( aiBaseTreeTemplateParam == nullptr )
		{
			aiBaseTreeTemplateParam = CreateObject< CAIBaseTreeTemplateParam >( m_template );
			m_template->AddParameterUnique( aiBaseTreeTemplateParam );
		}
		
	

		// Custom ai trees
		const TDynArray< String > stringArray = selectionString.Split(TXT("\""));
		if ( stringArray.Size() > 1 )
		{
			CClass *const treeClass			= SRTTI::GetInstance().FindClass( CName( stringArray[ 1 ].AsChar() ) );
			ChangeAITree( treeClass );
		}
	}

	FillAiTreePanels( );
	FillAiWizardResData();

	RebuildEntityAfterIncludesChanged();
	m_gameplayParamEdit->RefreshPropList();
	UpdateTabs();
}

void CEdEntityEditor::OnRunAIWizard( wxCommandEvent& event )
{
	RunAiWizard();

	FillAiTreePanels( );
	FillAiWizardResData();

	RebuildEntityAfterIncludesChanged();
	m_gameplayParamEdit->RefreshPropList();
	UpdateTabs();
}

void CEdEntityEditor::ChangeAITree ( CClass* treeClass )
{
	ASSERT( treeClass && treeClass->IsAbstract() == false, TXT("Something is wrong with that ai tree") );

	CAIBaseTreeTemplateParam* aiBaseTreeTemplateParam = m_template->FindParameter< CAIBaseTreeTemplateParam >( false );

	THandle < CAIBaseTree > aiBaseTree;

	aiBaseTree = treeClass->CreateObject< CAIBaseTree >();
	aiBaseTree->InitializeData();
	aiBaseTreeTemplateParam->m_aiBaseTree = aiBaseTree;
}

CResource *const CEdEntityEditor::GetAIWizardResource()
{
	CAIWizardTemplateParam*const aiWizardTemplateParam		= m_template->FindParameter< CAIWizardTemplateParam >( false );
	CAIWizardTemplateParam*const deepAIWizardTemplateParam	= m_template->FindParameter< CAIWizardTemplateParam >( true );
	CResource *const aiWizardResource = ( aiWizardTemplateParam && aiWizardTemplateParam->m_aiWizardResource.Get() ) ? aiWizardTemplateParam->m_aiWizardResource.Get() : ( deepAIWizardTemplateParam ? deepAIWizardTemplateParam->m_aiWizardResource.Get() : nullptr );

	return aiWizardResource;
}

THandle< CAIBaseTree > CEdEntityEditor::FillAiTreeWithParams( Bool usePreviewTree )
{
	if( m_template == nullptr )
	{
		return nullptr;
	}
	THandle< CAIBaseTree > aiBaseTree;
	TDynArray< CAIPresetParam * > aiPresetsList;
	CActor::GetAITemplateParams( m_template, aiBaseTree, &aiPresetsList );

	if ( !aiBaseTree )
	{
		return nullptr;
	}
	// [ Step ] Choosing which ai base tree to populate
	// In one case we want the preview ai tree in another we want the actual template ai tree
	THandle< CAIBaseTree > previewTree = aiBaseTree;
	if ( usePreviewTree )
	{
		THandle< ISerializable > newTree =  aiBaseTree->Clone( nullptr );

		m_previewAITree = static_cast< CAIBaseTree* >( newTree.Get() );
		previewTree = m_previewAITree;
	}
	// [ Step ] Merging parameters 
	IAIParameters *const aiBaseTreeParams = previewTree->GetParams();
	for ( Uint32 i = 0; i < aiPresetsList.Size(); ++i )
	{
		CAIPresetParam *const aiPresetParam = aiPresetsList[ i ];
		const TDynArray< THandle< IAIParameters > > & redefinitionParameters	= aiPresetParam->GetRedefinitionParameters();
		for( Uint32 j = 0; j < redefinitionParameters.Size() ; ++j )
		{
			IAIParameters* injectedParams = redefinitionParameters[ j ].Get();
			if( injectedParams )
			{
				aiBaseTreeParams->MergeParams( injectedParams );
			}
		}
	}
	return previewTree;
}
void CEdEntityEditor::OnAiTreeChanged ( wxCommandEvent& event )
{
	m_template->MarkModified();
}

void CEdEntityEditor::OnUseAiWizardRes( wxCommandEvent& event )
{
	String resPath;
	if ( GetActiveResource( resPath, ClassID< CWizardDefinition >() ) )
	{
		CWizardDefinition* def = LoadResource< CWizardDefinition >( resPath );
		if ( !def )
		{
			wxMessageBox( wxT("Failed to load wizard definition."), wxT("Wizard definition error"), wxOK | wxICON_ERROR );
			return;
		}
		//CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
		CAIWizardTemplateParam* aiWizardTemplateParam = m_template->FindParameter< CAIWizardTemplateParam >( false );
		if ( aiWizardTemplateParam == nullptr )
		{
			aiWizardTemplateParam = CreateObject< CAIWizardTemplateParam >( m_template );
			m_template->AddParameterUnique( aiWizardTemplateParam );
		}
	
		aiWizardTemplateParam->m_aiWizardResource = def;

		FillAiWizardResData();
	}
	else
	{
		wxMessageBox( wxT("Please select a wizard resource"), wxT("Invalid resource"), wxOK | wxICON_INFORMATION );
	}
}

void CEdEntityEditor::FillAiWizardResData()
{
	CAIWizardTemplateParam* aiWizardTemplateParam = m_template->FindParameter< CAIWizardTemplateParam >( false );
	if( aiWizardTemplateParam )
	{
		if ( aiWizardTemplateParam->m_aiWizardResource )
		{
			m_aiWizardResName->SetLabelText( aiWizardTemplateParam->m_aiWizardResource->GetFile()->GetFileName().AsChar() );
		}

		m_aiWizardResButton->Enable( true );
	}
	else
	{
		CAIWizardTemplateParam* inheritAIWizardTemplateParam = m_template->FindParameter< CAIWizardTemplateParam >( true );
		if ( inheritAIWizardTemplateParam && inheritAIWizardTemplateParam->m_aiWizardResource )
		{

			m_aiWizardResName->SetLabelText( inheritAIWizardTemplateParam->m_aiWizardResource->GetFile()->GetFileName().AsChar() );
		}
		else
		{
			m_aiWizardResName->SetLabelText( wxEmptyString );
		}
		m_aiWizardResButton->Enable( false );
	}	
}

void CEdEntityEditor::FillAiTreePanels( )
{
	CEntityTemplate* imheritedTemplate								= GetInheritedAIMainTemplate();
	CAIPresetsTemplateParam *const aiPresetsTemplateParam			= m_template->FindParameter< CAIPresetsTemplateParam >( false );
	CAIPresetsTemplateParam *const deepAIPresetsTemplateParam		= m_template->FindParameter< CAIPresetsTemplateParam >( true );
	CAIBaseTree *  localAIBaseTree									= GetLocalAIBaseTreeOnTemplate();
	CAIBaseTreeTemplateParam * const deepAIBaseTreeTemplateParam	= m_template->FindParameter< CAIBaseTreeTemplateParam >( true );
	
#ifdef AI_WIZARD_BACKWARD_COMPAT
	///////////////////////////////////////////////////////////////////////////////////////////////
	struct Local
	{
		static Bool Pred( CAIProfile* aiProfile )
		{
			return ( aiProfile->GetAiResource() != NULL );
		}
	};

	CAIProfile* aiProfile		= m_template->FindParameter< CAIProfile >( false, Local::Pred );
	if( aiProfile )
	{
		if ( aiProfile->GetAiResource() )
		{
			GFeedback->ShowError(TXT("Error: AI tree on this entity is causing trouble, to fix it please re-setup the AI tree and save the entity"));
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////
#endif 

	// Disable Button ( will enable it only for aiwizard )
	m_runAIWizardButton->Disable();

	THandle< CAIBaseTree > aiBaseTree;
	Bool readOnly = false;
	
	if( aiPresetsTemplateParam == nullptr && localAIBaseTree == nullptr )
	{
		m_aiTreeEditChoice->SetStringSelection( AITREE_SELECTION_INHERIT );
		if ( deepAIBaseTreeTemplateParam && deepAIBaseTreeTemplateParam->m_aiBaseTree )
		{
			ASSERT( aiPresetsTemplateParam == nullptr );
			ASSERT( localAIBaseTree == nullptr );
		
			aiBaseTree = FillAiTreeWithParams( true );
		
			// making sure the tree is not editable ( it is a preview )
			readOnly	= true;
		}
	}
	else
	{		
		if( aiPresetsTemplateParam ) // it means we selected the Ai wizard :
		{
			m_runAIWizardButton->Enable();

			m_aiTreeEditChoice->SetStringSelection( AITREE_SELECTION_USEAIWIZARD );
			if ( aiPresetsTemplateParam )
			{
				aiBaseTree = FillAiTreeWithParams( true );
			}
		
			// making sure the tree is not editable ( it is a preview )
			readOnly	= true; 
		}
		else // We are on a custom ai base tree
		{	
			ASSERT( localAIBaseTree );
			m_aiTreeEditChoice->SetStringSelection( GetAiBaseTreeMenuName( localAIBaseTree->GetClass()->GetName().AsString() ).AsChar() );
			// setting ai tree in editor
			aiBaseTree	= localAIBaseTree;
			
			// making sure the tree is editable
			readOnly	= false;
		}
	}
	m_aiTreeProperties->Get().SetObject( aiBaseTree.Get() );	
	m_aiTreeProperties->Get().SetReadOnly( readOnly );

	// [Step] Now fillup the ai wizard saved answer panel
	const Uint32 rowCount = m_aiWizardSummaryGrid->GetNumberRows();
	if ( rowCount != 0 )
	{
		m_aiWizardSummaryGrid->DeleteRows( 0, rowCount ); // first clean the grid
	}
	if ( aiPresetsTemplateParam == nullptr )
	{
		return;
	}
	CEdWizardSavedAnswers *const savedAnswers = aiPresetsTemplateParam->GetAIWizardAswers(); // do not use deep because well we dont care about answers from inherited entities
	if ( savedAnswers )
	{
		wxSizer *const aiWizardSummarySizer = m_aiWizardSummaryPanel->GetSizer();

		const TDynArray< CEDSavedAnswer > & edSavedAnswerList = savedAnswers->GetList();
		
		
		for ( Uint32 i = 0; i < edSavedAnswerList.Size(); ++i )
		{
			m_aiWizardSummaryGrid->InsertRows( i, 1 );
			m_aiWizardSummaryGrid->SetCellValue( i, 0, edSavedAnswerList[ i ].m_questionName.AsChar() );
			m_aiWizardSummaryGrid->SetCellValue( i, 1, edSavedAnswerList[ i ].m_answer.AsChar() );
			m_aiWizardSummaryGrid->SetReadOnly( i, 0 );
			m_aiWizardSummaryGrid->SetReadOnly( i, 1 );
		}
	
		m_aiWizardSummaryGrid->SetColLabelValue( 0, "Question" );
		m_aiWizardSummaryGrid->SetColLabelValue( 1, "Answer" );
		m_aiWizardSummaryGrid->SetColSize( 0, 200 );
		m_aiWizardSummaryGrid->SetColSize( 1, 400 );
		
		m_aiWizardSummaryPanel->Layout();
	}
}
 
CEntityTemplate* CEdEntityEditor::GetInheritedAIMainTemplate()const
{
	CEntity* previewEntity = m_preview->GetEntity();
	if ( previewEntity && previewEntity->IsA< CActor >() )
	{
		CEntityTemplate* previewEntityTemplate = previewEntity->GetEntityTemplate();
		if ( previewEntityTemplate )
		{
			CAIBaseTreeTemplateParam* aiBaseTreeTemplateParam = previewEntityTemplate->FindParameter< CAIBaseTreeTemplateParam >( true );
			if( aiBaseTreeTemplateParam )
			{
				return SafeCast< CEntityTemplate >( aiBaseTreeTemplateParam->GetParent() );
			}		
		}
	}

	return nullptr;
}

THandle< CAIBaseTree > CEdEntityEditor::GetCachedAiTree() const
{
	THandle< CAIBaseTree > aiBaseTree;
	CEntity* entity = m_preview->GetEntity();
	if ( entity && entity->IsA< CActor >() )
	{
		CActor *const actor = SafeCast< CActor >( entity );
		
		CActor::GetAITemplateParams( actor->GetEntityTemplate(), aiBaseTree );
	}
	return aiBaseTree;
}

CAIBaseTree* CEdEntityEditor::GetLocalAIBaseTreeOnTemplate( Bool backwardCompatHack ) const
{
	CAIBaseTree* aiBaseTree = nullptr;
	if( m_template == nullptr )
	{
		return nullptr;
	}
#ifdef AI_WIZARD_BACKWARD_COMPAT
	struct Local
	{
		static Bool Pred( CAIProfile* aiProfile )
		{
			return ( aiProfile->GetAiResource() != NULL );
		}
	};

	CAIProfile* aiProfile		= m_template->FindParameter< CAIProfile >( false, Local::Pred );
	CAIProfile* deepAIProfile	= m_template->FindParameter< CAIProfile >( true, Local::Pred );
	if( aiProfile )
	{
		aiBaseTree = aiProfile->GetAiResource();
	}
	
#endif

	CAIBaseTreeTemplateParam * const aiBaseTreeTemplateParam = m_template->FindParameter< CAIBaseTreeTemplateParam >( false );
	
	if ( aiBaseTree == nullptr && aiBaseTreeTemplateParam ) // backward compat
	{
		aiBaseTree = aiBaseTreeTemplateParam->m_aiBaseTree;
	}
#ifdef AI_WIZARD_BACKWARD_COMPAT
	if( aiBaseTree == nullptr && deepAIProfile && backwardCompatHack ) // if we have no aibase tree yet take deep one
	{
		aiBaseTree = deepAIProfile->GetAiResource();
	}
#endif
	return aiBaseTree;
}

void CEdEntityEditor::OnAttitudeGroupChoice( wxCommandEvent& event )
{
	String attitudeGroup = m_attitudeGroup->GetStringSelection();
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	if ( !profile )
	{
		// create a new profile if the template doesn't have one
		profile = CreateObject< CAIProfile >( m_template );
		m_template->AddParameterUnique( profile );
		m_template->MarkModified();
	}

	profile->m_attitudeGroup = CName( attitudeGroup );

	// Show applied attitude group
	RefreshAppliedAttitudeGroup();
}

void CEdEntityEditor::FillAttitudeGroupChoice()
{
	m_attitudeGroup->Clear();

	const C2dArray& attitudeGroupsArray = SAttitudesResourcesManager::GetInstance().Get2dArray();

	const Uint32 &numRows = attitudeGroupsArray.GetNumberOfRows();
	wxArrayString choices;
	for ( Uint32 i = 0; i < numRows; ++i )
	{
		choices.Add( attitudeGroupsArray.GetValue( 0, i ).AsChar() );
	}

	// Read current attitude group
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	Int32 index = wxNOT_FOUND;
	if ( profile )
	{
		wxString attGroup( profile->GetAttitudeGroup().AsString().AsChar() );
		index = choices.Index( attGroup );
		if ( index == wxNOT_FOUND )
		{
			choices.Add( attGroup );
		}
		index = choices.Index( attGroup );
		ASSERT( index != wxNOT_FOUND );
	}

	m_attitudeGroup->Append( choices );
	m_attitudeGroup->SetSelection( index );

	// Show applied attitude group
	RefreshAppliedAttitudeGroup();
}

// Show applied attitude group
void CEdEntityEditor::RefreshAppliedAttitudeGroup()
{
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( true, SAttitudesAIProfilePred() );
	if ( profile )
	{
		wxString attGroup( profile->GetAttitudeGroup().AsString().AsChar() );
		m_attitudeGroupApplied->SetLabel( attGroup );
	}
	else
	{
		m_attitudeGroupApplied->SetLabel( wxT("NONE") );
	}
}
//////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::UpdateMinigameTabGui()
{
	CAIProfile* currentProfile = m_template->FindParameter< CAIProfile >( false );
	if ( currentProfile )
	{
		SAIMinigameParams *minigameParams = currentProfile->GetMinigameParams();
		ASSERT( minigameParams );
		CAIMinigameParamsWristWrestling* wwParams = minigameParams->GetWristWrestlingParams();
		if ( wwParams )
		{
			m_minigameWWHotSpotMinWidth->Enable();
			m_minigameWWHotSpotMinWidth->Enable();
			m_minigameWWDifficulty->Enable();

			m_minigameWWHotSpotMinWidth->ChangeValue( ToString< Int32 >( wwParams->m_hotSpotMinWidth ).AsChar() );
			m_minigameWWHotSpotMaxWidth->ChangeValue( ToString< Int32 >( wwParams->m_hotSpotMaxWidth ).AsChar() );
			String gameDifficultyTxt = ToString( wwParams->m_gameDifficulty );
			m_minigameWWDifficulty->SetStringSelection( gameDifficultyTxt.AsChar() );
			m_minigameWWEnable->SetValue( true );
		}
		else
		{
			// Set default values - disable controls
			MinigameTabGuiDisableWWControls();
		}
	}
	else
	{
		// Set default values - disable controls
		MinigameTabGuiDisableWWControls();
	}

	UpdateMinigameTabGuiWWStatus();
}

void CEdEntityEditor::UpdateMinigameTabGuiWWStatus()
{
	String statusText = String::EMPTY;

	//CAIProfile* profile = m_template->FindParameter< CAIProfile >( true, Minigame::SMinigameWristWrestlingPred() );
	//if ( profile )
	//{
	//	SAIMinigameParams *minigameParams = profile->GetMinigameParams();
	//	ASSERT( minigameParams );
	//	CAIMinigameParamsWristWrestling* wwParams = minigameParams->GetWristWrestlingParams();
	//	if ( wwParams )
	//	{
	//		String gameDifficultyTxt = ToString( wwParams->m_gameDifficulty );
	//		statusText = String::Printf( TXT("Wrist Wrestling current params: Difficulty: %s Hot Spot width (%d, %d)"),
	//			gameDifficultyTxt.AsChar(), wwParams->m_hotSpotMinWidth, wwParams->m_hotSpotMaxWidth );
	//	}
	//}

	if ( statusText == String::EMPTY )
	{
		statusText = TXT("Wrist Wrestling current params: NONE defined");
	}
	m_minigameWWStatusText->SetLabel( statusText.AsChar() );
}

void CEdEntityEditor::MinigameTabGuiDisableWWControls()
{
	// Set default values - disable controls
	m_minigameWWEnable->SetValue( false );
	m_minigameWWHotSpotMinWidth->ChangeValue( wxT("") );
	m_minigameWWHotSpotMaxWidth->ChangeValue( wxT("") );
	m_minigameWWDifficulty->SetStringSelection( wxT("Normal") );
	m_minigameWWHotSpotMinWidth->Disable();
	m_minigameWWHotSpotMaxWidth->Disable();
	m_minigameWWDifficulty->Disable();
}

void CEdEntityEditor::OnMinigameWWUpdateDataFromGui()
{
	Bool isWWEnabled = m_minigameWWEnable->GetValue();

	// Get profile or create it if there is no profile and we need one
	CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
	if ( !profile && isWWEnabled )
	{
		// create a new profile if the template doesn't have one
		profile = CreateObject< CAIProfile >( m_template );
		m_template->AddParameterUnique( profile );
		m_template->MarkModified();
	}

	SAIMinigameParams *minigameParams = profile->GetMinigameParams();
	ASSERT( minigameParams );

	if ( isWWEnabled )
	{
		if ( minigameParams->m_wristWrestling == NULL )
		{
			minigameParams->m_wristWrestling = CreateObject< CAIMinigameParamsWristWrestling >( profile );
			ASSERT( minigameParams->m_wristWrestling );
		}

		String hotSpotMinWidthStr = m_minigameWWHotSpotMinWidth->GetValue();
		String hotSpotMaxWidthStr = m_minigameWWHotSpotMaxWidth->GetValue();
		String gameDifficultyTxt = m_minigameWWDifficulty->GetStringSelection();
		
		Int32 hotSpotMinWidth = -1;
		if ( FromString( hotSpotMinWidthStr, hotSpotMinWidth ) )
		{
			if ( hotSpotMinWidth > 0 && hotSpotMinWidth < 100 )
			{
				minigameParams->m_wristWrestling->m_hotSpotMinWidth = hotSpotMinWidth;
			}
		}

		Int32 hotSpotMaxWidth = -1;
		if ( FromString( hotSpotMaxWidthStr, hotSpotMaxWidth ) )
		{
			if ( hotSpotMaxWidth > 0 && hotSpotMaxWidth < 100 )
			{
				minigameParams->m_wristWrestling->m_hotSpotMaxWidth = hotSpotMaxWidth;
			}
		}
		
		FromString( gameDifficultyTxt, minigameParams->m_wristWrestling->m_gameDifficulty );
	}
	else
	{
		if ( profile )
		{
			if ( minigameParams->m_wristWrestling )
			{
				minigameParams->m_wristWrestling->Discard();
				minigameParams->m_wristWrestling = NULL;
			}
		}
	}
}

void CEdEntityEditor::OnMinigameWWEnable( wxCommandEvent& event )
{
	Bool isWWEnabled = m_minigameWWEnable->GetValue();
	if ( !isWWEnabled )
	{
		MinigameTabGuiDisableWWControls();
	}
	else
	{
		// Init GUI with default values
		m_minigameWWHotSpotMinWidth->ChangeValue( wxT("6") );
		m_minigameWWHotSpotMaxWidth->ChangeValue( wxT("20") );
		m_minigameWWDifficulty->SetStringSelection( wxT("Normal") );
		m_minigameWWHotSpotMinWidth->Enable();
		m_minigameWWHotSpotMaxWidth->Enable();
		m_minigameWWDifficulty->Enable();
	}
	
	OnMinigameWWUpdateDataFromGui();
	UpdateMinigameTabGuiWWStatus();
}

void CEdEntityEditor::OnMinigameWWHotSpotMinWidth( wxFocusEvent& event )
{
	// Check if values are correct
	String hotSpotMinWidthStr = m_minigameWWHotSpotMinWidth->GetValue();
	Int32 hotSpotMinWidth = -1;
	if ( FromString( hotSpotMinWidthStr, hotSpotMinWidth ) )
	{
		if ( hotSpotMinWidth > 0 && hotSpotMinWidth < 100 )
		{
			OnMinigameWWUpdateDataFromGui();
			UpdateMinigameTabGuiWWStatus();
			return;
		}
	}

	wxMessageBox( TXT("Incorrect data - will not be saved"), TXT("Incorrect data - will not be saved") );

	UpdateMinigameTabGuiWWStatus();
}

void CEdEntityEditor::OnMinigameWWHotSpotMaxWidth( wxFocusEvent& event )
{
	// Check if values are correct
	String hotSpotMaxWidthStr = m_minigameWWHotSpotMaxWidth->GetValue();
	Int32 hotSpotMaxWidth = -1;
	if ( FromString( hotSpotMaxWidthStr, hotSpotMaxWidth ) )
	{
		if ( hotSpotMaxWidth > 0 && hotSpotMaxWidth < 100 )
		{
			OnMinigameWWUpdateDataFromGui();
			UpdateMinigameTabGuiWWStatus();
			return;
		}
	}

	wxMessageBox( TXT("Incorrect data - will not be saved"), TXT("Incorrect data - will not be saved") );

	UpdateMinigameTabGuiWWStatus();
}

void CEdEntityEditor::OnMinigameWWDifficulty( wxCommandEvent& event )
{
	OnMinigameWWUpdateDataFromGui();

	UpdateMinigameTabGuiWWStatus();
}

//////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::OnEquipmentCategoryClicked( wxMouseEvent& event )
{
	event.Skip();

	Int32 selected = m_categoriesList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CEquipmentDefinitionEntry* selectedEntry = static_cast< CEquipmentDefinitionEntry* >( m_categoriesList->GetClientData( selected ) );
	//selectedEntry->m_availableItems = &m_cachedEquipmentContents[ selectedEntry->GetCategory() ];
	if ( !selectedEntry )
	{
		return;
	}
	m_categoryProperties->Get().SetObject( selectedEntry );

	// Expand all starting 1 deeper than root property (to avoid double property display bug)
	const TDynArray<CBasePropItem*>& rootProperty = m_categoryProperties->Get().GetRootItem()->GetChildren();
	if ( rootProperty.Size() == 0 )
	{
		return;
	}
	ExpandEquipmentEntryProperties( rootProperty[0]->GetChildren() );
}

void CEdEntityEditor::ExpandEquipmentEntryProperties( const TDynArray<CBasePropItem*>& items )
{
	for( Uint32 i=0; i < items.Size(); ++i )
	{
		ExpandEquipmentEntryProperties( items[i]->GetChildren() );
		if ( !items[i]->IsExpanded() )
		{
			items[i]->Expand();
		}
	}
}

void CEdEntityEditor::OnAddEquipmentCategory( wxCommandEvent& event )
{
	CEntityAppearance * appearance = GetSelectedAppearance();
	if ( !appearance )
	{
		return;
	}

	if ( !m_template->GetAppearances().Exist( *appearance ) )
	{
		String msg = String::Printf( TXT("This appearance is from included template. Don't edit it here.") );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	CEquipmentDefinition* definition = appearance->FindParameter< CEquipmentDefinition >();
	if ( !definition )
	{
		// create a new definitiond if the template doesn't have one
		definition = CreateObject< CEquipmentDefinition >();
		definition->SetParent( m_template );
		appearance->AddParameter( definition );
		m_template->MarkModified();
	}

	CEquipmentDefinitionEntry* entry = CreateObject< CEquipmentDefinitionEntry >( definition ); 
	definition->AddEntry( entry );
	UpdateCategoriesList();
}

void CEdEntityEditor::OnRemoveEquipmentCategory( wxCommandEvent& event )
{
	Int32 selected = m_categoriesList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CEquipmentDefinitionEntry* selectedEntry = static_cast< CEquipmentDefinitionEntry* >( m_categoriesList->GetClientData( selected ) );
	if ( !selectedEntry )
	{
		return;
	}

	// find the parent profile of the reaction
	CEquipmentDefinition* definition = Cast< CEquipmentDefinition >( selectedEntry->GetParent() );
	if ( !definition )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedEntry->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected entry is from included template '%s'. Don't edit it here."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	definition->RemoveEntry( selectedEntry );
	selectedEntry->Discard();

	entityTemplate->MarkModified();
	UpdateCategoriesList();
	m_categoryProperties->Get().SetNoObject();
}

void CEdEntityEditor::OnEquipmentCategoryChanged( wxCommandEvent& event )
{
	UpdateCategoriesList();

	Int32 selected = m_categoriesList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CEquipmentDefinitionEntry* selectedEntry = static_cast< CEquipmentDefinitionEntry* >( m_categoriesList->GetClientData( selected ) );
	//selectedEntry->m_availableItems = &m_cachedEquipmentContents[ selectedEntry->GetCategory() ];
	if ( !selectedEntry )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedEntry->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected entry is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
	}
	entityTemplate->MarkModified();
}

void CEdEntityEditor::UpdateCategoriesList()
{
	CEntityAppearance * appearance = GetSelectedAppearance();
	if ( !appearance )
	{
		return;
	}

	m_categoriesList->Clear();

	TDynArray< CEquipmentDefinition* > equipmentDefinitions;
	CEquipmentDefinition* definition = appearance->FindParameter< CEquipmentDefinition >();
	if ( definition )
	{
		//definition->m_availableCategories = &m_cachedCategories;

		const TDynArray< CEquipmentDefinitionEntry* >& entries = definition->GetEntries();
		for ( TDynArray< CEquipmentDefinitionEntry* >::const_iterator entryIt = entries.Begin(); 
			entryIt != entries.End(); ++entryIt )
		{
			CEquipmentDefinitionEntry* entry = *entryIt;
			//entry->m_availableItems = &m_cachedEquipmentContents[ entry->GetCategory() ];
			if ( !entry )
			{
				continue;
			}

			String name = entry->m_category.AsString();

			m_categoriesList->Append( name.AsChar(), (void*)entry );
		}
	}

	ApplyEquipmentDefinitionPreview();
}

CEntityAppearance* CEdEntityEditor::GetSelectedAppearance()
{
	Int32 nSel = m_lstAppearances->GetSelection();
	if ( nSel < 0 )
		return NULL;

	return static_cast<CEntityAppearance *>( m_appearancesOnList[ nSel ] );
}


void CEdEntityEditor::ApplyEquipmentDefinitionPreview()
{
	ClearEquipmentPreviewItems( m_preview->GetEntity() );
	ApplyEquipmentPreview( m_preview->GetEntity() );
}

//////////////////////////////////////////////////////////////////////////

void CEdEntityEditor::OnInventoryItemSelected( wxCommandEvent& event )
{
	Int32 selected = m_itemsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CInventoryDefinitionEntry* selectedEntry = static_cast< CInventoryDefinitionEntry* >( m_itemsList->GetClientData( selected ) );
	if ( !selectedEntry )
	{
		return;
	}
	m_itemProperties->Get().SetObject( selectedEntry );
	m_itemProperties->Enable( selectedEntry->FindParent< CEntityTemplate >() == m_template );

	// Expand all starting 1 deeper than root property (to avoid double property display bug)
	const TDynArray<CBasePropItem*>& rootProperty = m_itemProperties->Get().GetRootItem()->GetChildren();
	if ( rootProperty.Size() == 0 )
	{
		return;
	}
	ExpandInventoryItemProperties( rootProperty[0]->GetChildren() );
}

void CEdEntityEditor::ExpandInventoryItemProperties( const TDynArray<CBasePropItem*>& items )
{
	for( Uint32 i=0; i < items.Size(); ++i )
	{
		ExpandEquipmentEntryProperties( items[i]->GetChildren() );
		if ( !items[i]->IsExpanded() )
		{
			items[i]->Expand();
		}
	}
}

void CEdEntityEditor::OnAddInventoryItem( wxCommandEvent& event )
{
	CInventoryDefinition* definition = m_template->FindParameter< CInventoryDefinition >( false );
	if ( !definition )
	{
		// create a new definition if the template doesn't have one
		definition = CreateObject< CInventoryDefinition >( m_template );
		m_template->AddParameterUnique( definition );
		m_template->MarkModified();
	}

	CInventoryDefinitionEntry* entry = CreateObject< CInventoryDefinitionEntry >( definition ); 
	definition->AddEntry( entry );
	UpdateInventoryItemsList();
}

void CEdEntityEditor::OnRemoveInventoryItem( wxCommandEvent& event )
{
	Int32 selected = m_itemsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CInventoryDefinitionEntry* selectedEntry = static_cast< CInventoryDefinitionEntry* >( m_itemsList->GetClientData( selected ) );
	if ( !selectedEntry )
	{
		return;
	}

	// find the parent profile of the reaction
	CInventoryDefinition* definition = Cast< CInventoryDefinition >( selectedEntry->GetParent() );
	if ( !definition )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedEntry->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected entry is from included template '%s'. Don't edit it here."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	definition->RemoveEntry( selectedEntry );
	selectedEntry->Discard();

	if ( definition->GetEntries().Empty() )
	{
		m_template->RemoveParameter( definition );
	}

	entityTemplate->MarkModified();
	UpdateInventoryItemsList();
	m_itemProperties->Get().SetNoObject();
}

void CEdEntityEditor::OnInventoryItemChanged( wxCommandEvent& event )
{
	UpdateInventoryItemsList();

	Int32 selected = m_itemsList->GetSelection();
	if ( selected < 0 )
	{
		return;
	}

	CInventoryDefinitionEntry* selectedEntry = static_cast< CInventoryDefinitionEntry* >( m_itemsList->GetClientData( selected ) );
	if ( !selectedEntry )
	{
		return;
	}

	// Get template this effect is in
	CEntityTemplate* entityTemplate = selectedEntry->FindParent< CEntityTemplate >();
	if ( !entityTemplate )
	{
		return;
	}

	// Inform use
	if ( entityTemplate != m_template && entityTemplate->GetFile() )
	{
		String msg = String::Printf( TXT("Selected entry is from included template '%s'. You will need to save that template manualy."), entityTemplate->GetFile()->GetDepotPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
	}
	entityTemplate->MarkModified();
}

void CEdEntityEditor::UpdateInventoryItemsList()
{
	m_itemsList->Clear();
	//m_cachedEquipmentContents.Clear();
	//m_cachedCategories.Clear();

	TDynArray< CInventoryDefinition* > equipmentDefinitions;
	m_template->GetAllParameters< CInventoryDefinition >( equipmentDefinitions );
	for ( TDynArray< CInventoryDefinition* >::iterator defIt = equipmentDefinitions.Begin(); 
		defIt != equipmentDefinitions.End(); ++defIt )
	{
		CInventoryDefinition* definition = *defIt;
		if ( !definition )
		{
			continue;
		}

		const TDynArray< CInventoryDefinitionEntry* >& entries = definition->GetEntries();
		for ( TDynArray< CInventoryDefinitionEntry* >::const_iterator entryIt = entries.Begin(); 
			entryIt != entries.End(); ++entryIt )
		{
			CInventoryDefinitionEntry* entry = *entryIt;
			if ( !entry )
			{
				continue;
			}

			// Compose description of the entry to be shown in items list box
			String name = entry->GetEntryDescription();
			if ( entry->FindParent<CEntityTemplate>() != m_template )
			{
				name += TXT( " (included)" );
			}

			// Cache entry contents for the appearance equipment tab to use
			// MGTODO: improve, for other initializer types
		/*	if ( entry->m_initializer && entry->m_initializer->IsA< CInventoryInitializerUniform >() )
			{
				m_cachedEquipmentContents[ entry->GetCategory() ].PushBackUnique( entry->m_initializer->EvaluateItemName( entry->GetCategory() ) );
				m_cachedCategories.PushBackUnique( entry->GetCategory() );
			}*/
			
			m_itemsList->Append( name.AsChar(), (void*)entry );
		}
	}
}

void CEdEntityEditor::UpdateVoicetagAppearancesTable()
{
	UpdateAnimTabParam< CVoicesetParam >( m_voiceTabVoicesetCheck, m_voiceTabVoicesetPanelProp );

	// Voicetag-appearance table
	if ( m_preview->GetEntity() == NULL || m_preview->GetEntity()->IsA< CActor >() == false )
	{
		return;
	}

	CProperty* voicetagAppearanceProperty = m_template->GetClass()->FindProperty( CNAME( voicetagAppearances ) );
	if ( voicetagAppearanceProperty == NULL )
	{
		return;
	}

	if ( m_voicetagAppearanceGrid == NULL )
	{
		wxPanel* voicetagAppearancePanel = XRCCTRL( *this, "VoicetagsAppearancesPanel", wxPanel );
		m_voicetagAppearanceGrid = new CGridEditor( voicetagAppearancePanel );

		const C2dArray* voiceTagArray = SStorySceneVoiceTagsManager::GetInstance().ReloadVoiceTags();
		if ( voiceTagArray )
		{
			m_voicetagAppearanceGrid->RegisterCustomColumnDesc( TXT( "Voicetag" ), CGridChoiceColumnDesc::CreateFrom2da( voiceTagArray, 0 ) );
		}
		
		voicetagAppearancePanel->GetSizer()->Add( m_voicetagAppearanceGrid, 1, wxEXPAND );
		voicetagAppearancePanel->Layout();
	}

	
	wxArrayString appearancesValues;
	const TDynArray< CName >& usedAppearances = m_template->GetEnabledAppearancesNames();
	for( TDynArray< CName >::const_iterator appearanceIter = usedAppearances.Begin();
		appearanceIter != usedAppearances.End(); ++appearanceIter )
	{
		appearancesValues.Add( appearanceIter->AsString().AsChar() );
	}

	m_template->RefreshVoicetagAppearances();
	m_voicetagAppearanceGrid->UnregisterCustomColumnDesc( TXT( "Appearance" ) );
	m_voicetagAppearanceGrid->RegisterCustomColumnDesc( TXT( "Appearance" ), CGridChoiceColumnDesc::CreateFromStrings( appearancesValues, NULL ) );
	m_voicetagAppearanceGrid->SetObject( voicetagAppearanceProperty->GetOffsetPtr( m_template ), voicetagAppearanceProperty );
	m_voicetagAppearanceGrid->SetColSize( 1, 200 );
	m_voicetagAppearanceGrid->SetColSize( 2, 200 );
}

void CEdEntityEditor::OnVoiceTabVoicesetCheck( wxCommandEvent& event )
{
	AnimTabCheck< CVoicesetParam >( m_voiceTabVoicesetCheck, m_voiceTabVoicesetPanelProp );
}

void CEdEntityEditor::OnMimicPanelActivated( wxCommandEvent& event )
{
	if ( m_mimicsPanel )
	{
		m_mimicsPanel->Activate( event.IsChecked() );
	}
}

void CEdEntityEditor::OnMimicPanelFovChanged( wxCommandEvent& event )
{
	m_preview->SetCameraFov( (Float)event.GetInt() );
}

void CEdEntityEditor::UpdateUsedAppearances()
{
	// Remove appearance names from usedAppearances that does not exist any more
	TDynArray<CName>& enabledAppearances = m_template->GetEnabledAppearancesNames();
	const Int32 enableAppearancesSize = enabledAppearances.SizeInt() - 1;
	for ( Int32 i = enableAppearancesSize; i >= 0; --i )
	{
		if ( !m_template->GetAppearance( enabledAppearances[i], true ) )
		{
			enabledAppearances.EraseFast( enabledAppearances.Begin() + i );
		}
	}
}


void CEdEntityEditor::ForceEntityTPose()
{
	CEntity* ent = m_preview->GetEntity();
	if ( ent )
	{
		CAnimatedComponent* ac = ent->GetRootAnimatedComponent();
		if ( ac )
		{
			// Freeze animation if it isn't already. Otherwise the T-Pose will only hold until the next update.
			m_freezeEntityPose.SetValue( true );

			// All animated components will be updated through hierarchy
			ac->ForceTPoseAndRefresh();

			ent->ScheduleUpdateTransformNode();
		}
	}
}

void CEdEntityEditor::ToggleFreezeEntityPose()
{
	m_freezeEntityPose.SetValue( !m_freezeEntityPose.GetValue() );
}


void CEdEntityEditor::OnFreezePose( wxCommandEvent& event )
{
	CEntity* ent = m_preview->GetEntity();
	if ( ent )
	{
		if ( m_freezeEntityPose.GetValue() )
		{
			ent->FreezeAllAnimatedComponents();
			m_preview->EnableManualSlotChange( true );
		}
		else
		{
			ent->UnfreezeAllAnimatedComponents();
			m_preview->EnableManualSlotChange( false );

		}
	}
}

void CEdEntityEditor::OnForceTPose( wxCommandEvent& event )
{
	ForceEntityTPose();
}

void CEdEntityEditor::OnTogglePreviewItemSize( wxCommandEvent& event )
{
	if ( m_preview != nullptr )
	{
		m_preview->SetPreviewItemSize( m_previewItemSize.GetValue() );
	}
}


void CEdEntityEditor::OnSoundOverrideParams( wxCommandEvent& event )
{
	if( m_soundOverrideParamsCheck->GetValue() )
	{
		// Create new parameter
		CSoundEntityParam* newParam = CreateObject< CSoundEntityParam >( m_template );

		// Check if we can copy some settings from base class
		CSoundEntityParam* param = m_template->FindParameter< CSoundEntityParam >( true );
		if ( param != NULL )
		{
			// Copy properties
			TDynArray< Uint8 > data;
			
			CMemoryFileWriter writer( data );
			param->OnSerialize( writer );

			CMemoryFileReader reader( data, 0 );
			newParam->OnSerialize( reader );
		}
		
		// Add new parameter
		m_template->AddParameterUnique( newParam );
	}
	else
	{
		// Delete existing parameter
		CSoundEntityParam* param = m_template->FindParameter< CSoundEntityParam >( false );
		ASSERT( param != NULL );
		m_template->RemoveParameter( param );
	}

	// Update GUI
	SoundUpdateParams();
}

void CEdEntityEditor::SoundUpdateParams()
{
	// Check if param is defined in current entity template
	CSoundEntityParam* param = m_template->FindParameter< CSoundEntityParam >( false );
	if ( param != NULL )
	{
		// Set checkbox and present parameters
		m_soundOverrideParamsCheck->SetValue( true );
		m_soundParamsProps->SetObject( param );
		m_soundParamsProps->Enable( true );

		return;
	}

	// Try to find sound parameters in base templates
	param = m_template->FindParameter< CSoundEntityParam >( true );
	if ( param != NULL )
	{
		// Set checkbox and present parameters read-only
		m_soundOverrideParamsCheck->SetValue( false );
		m_soundParamsProps->SetObject( param );
		m_soundParamsProps->Enable( false );

		return;
	}

	// No parameter defined, present default object
	m_soundOverrideParamsCheck->SetValue( false );
	m_soundParamsProps->SetObject( CSoundEntityParam::GetStaticClass()->GetDefaultObject< CSoundEntityParam >() );
	m_soundParamsProps->Enable( false );
}

Bool CEdEntityEditor::ValidateEntityOnSave( CEntity* previewEntity )
{
	TDynArray< String > log;
	Bool valid = Validate( log ) && previewEntity->OnValidate( log );
	if ( !valid )
	{
		String fullLog;
		fullLog += TXT("Fix all of the following problems:\n");

		const Uint32 logSize = log.Size();
		for ( Uint32 i = 0; i < logSize; ++i )
		{
			VALIDATION_FAIL( log[i].AsChar() );
			fullLog += TXT("- ") + log[i] + TXT("\n");
		}
		wxMessageBox( fullLog.AsChar(), TXT("Validation failed! Entity WAS NOT SAVED!"), wxOK | wxICON_ERROR|wxCENTRE );
	}

	return valid;
}

Bool CEdEntityEditor::Validate( TDynArray< String >& log )
{
	ValidateAnimTabs( log );
	ValidateAttitudeGroup( log );

	//...

	return log.Size() == 0;
}

void CEdEntityEditor::ValidateAttitudeGroup( TDynArray< String >& log )
{
	if ( GCommonGame != nullptr )
	{
		CAttitudeManager* am = GCommonGame->GetSystem< CAttitudeManager >();
		CAIProfile* profile = m_template->FindParameter< CAIProfile >( false );
		if ( am != nullptr && profile != nullptr )
		{
			CName attitudeGroup = profile->m_attitudeGroup;
			if ( attitudeGroup != CName::NONE && attitudeGroup != CNAME( default ) && !am->AttitudeGroupExists( attitudeGroup ) )
			{
				log.PushBack( String::Printf( TXT("Attitude group does not exist '%s'"), attitudeGroup.AsString().AsChar() ) );
			}
		}
	}
}

void CEdEntityEditor::RefreshComponentComments()
{
	class RefreshComments : public CEdRunnable
	{
		CEdEntityEditor* m_editor;

	public:
		RefreshComments( CEdEntityEditor* e )
			: m_editor( e )
		{
			TriggerAfter( 0.5f );
		}

		virtual void Run()
		{
			m_editor->InternalRefreshComponentComments();
		}
	};

	RunLaterOnceEx( new RefreshComments( this ) );
}

void CEdEntityEditor::InternalRefreshComponentComments()
{
	m_componentComments.ClearFast();

	if ( CEntity* e = m_preview->GetEntity() )
	{
		String str;

		for ( ComponentIterator< CAnimDangleComponent > it( e ); it; ++it )
		{
			if ( (*it)->PrintDebugComment( str ) )
			{
				m_componentComments.Insert( *it, str );
			}
		}
	}
}

Bool CEdEntityEditor::FindComponentComment( const CComponent* c, String& outStr ) const
{
	return m_componentComments.Find( c, outStr );
}

Int32 CEdEntityEditor::GetSelectedStreamingLOD() const
{
	return m_currentStreamingLOD;
}

void CEdEntityEditor::OnDataErrorReported( const SDataError& error )
{
#ifndef NO_DATA_VALIDATION

	if ( m_dependentResourcesPaths.Exist( error.m_resourcePath ) )
	{
		m_dataErrors.Insert( error );
	}

#endif
}

void CEdEntityEditor::GetDataErrors( TDynArray< String >& arrayForErrors )
{
	for ( Uint32 i = 0; i < m_dataErrors.Size(); ++i )
	{
		const String& error = m_dataErrors[i].ToString();
		arrayForErrors.PushBackUnique( error );
	}
}

void CEdEntityEditor::AddDataErrors( const TDynArray< SDataError >& dataErrors )
{
	m_dataErrors.PushBackUnique( dataErrors );
	m_dataErrors.Sort();
}

IEditorPreviewCameraProvider::Info CEdEntityEditor::GetPreviewCameraInfo() const
{
	IEditorPreviewCameraProvider::Info res;
	res.m_cameraPostion  = m_preview->GetCameraPosition();
	res.m_cameraRotation = m_preview->GetCameraRotation();
	res.m_cameraFov      = m_preview->GetCameraFov();
	res.m_lightPosition  = m_preview->GetLightPosition();
	res.m_envPath		 = TXT("environment\\definitions\\") + m_preview->m_envChoice->GetString( m_preview->m_envChoice->GetSelection() );
	return res;
}

void CEdEntityEditor::SelectComponents( const TDynArray< CComponent* >& components )
{
	m_graph->SelectComponents( components );
}

void CEdEntityEditor::HideComponents( const TDynArray< CComponent* >& components )
{
	m_graph->HideComponents( components );
}

void CEdEntityEditor::IsolateComponents( const TDynArray< CComponent* >& components )
{
	m_graph->IsolateComponents( components );
}

void CEdEntityEditor::OnGenerateProxyMesh( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Collecting entity proxy meshes"), false );

	// Generate the mesh based on if everything were streaming in.
	LoadEntityLOD( 0 );

	CEdEntityMeshGenerator meshGen( m_template );
	if ( !meshGen.Prepare( m_preview->GetEntity() ) )
	{
		GFeedback->EndTask();
		GFeedback->ShowError( TXT("Failure while preparing meshes:\n%s"), meshGen.GetErrorMessage().AsChar() );
		return;
	}

	// Can now restore current LOD selection.
	LoadEntityLOD( m_currentStreamingLOD );

	GFeedback->EndTask();


	// Set up "Settings" UI, let user select generation settings, and where to save mesh.
	CEdEntityMeshGeneratorSettingsDlg settingsDlg( this );

	{
		CFilePath path( m_template->GetDepotPath() );
		String filePrefix = path.GetFileName() + TXT("_proxy");

		CEdEntityMeshGenerator::Settings settings;
		// disabling as requested. always use default value, instead of estimate.
		//settings.m_showDistance = meshGen.EstimateShowDistance();
		settings.m_screenSize = meshGen.EstimateScreenSize( settings.m_showDistance );

		settingsDlg.SetSettings( settings );
		
		// as requested, adding fixed path for W3 BOB
		settingsDlg.SetOutputFolder( TXT("dlc\\bob\\data\\environment\\entity_proxy\\") ); //m_template->GetFile()->GetDirectory()->GetDepotPath() + filePrefix + TXT("\\")
		settingsDlg.SetBaseName( filePrefix );

		const TDynArray< StringAnsi >& texNames = meshGen.GetTextureNames();
		for ( Uint32 i = 0; i < texNames.Size(); ++i )
		{
			const StringAnsi& name = texNames[i];

			// We default to just saving diffuse and normals.
			Bool saveByDefault = ( name == "Diffuse" );// || ( name == "Normals" );

			settingsDlg.AddTexture( name, saveByDefault );
		}
	}

	// All the work is actually done in a loop. This way, if some of the files already exist, we can return to the settings screen to make
	// changes. Also, if the processing fails, we can make adjustments, rather than having to reopen the tool (which gives default settings).
	Bool success = false;
	while ( !success )
	{
		if ( settingsDlg.ShowModal() != wxID_OK )
		{
			return;
		}

		// Give settings to the generator, and update settings dialog with any modifications that may have been made.
		CEdEntityMeshGenerator::Settings actualSettings = meshGen.ProvideSettings( settingsDlg.GetSettings() );
		settingsDlg.SetSettings( actualSettings );

		GFeedback->BeginTask( TXT("Collecting meshes"), false );

		// Recollect the scene with the latest settings.
		if ( !meshGen.ReCollect( m_preview->GetEntity() ) )
		{
			GFeedback->EndTask();
			GFeedback->ShowError( TXT("Failure while collecting meshes:\n%s"), meshGen.GetErrorMessage().AsChar() );
			continue;
		}

		GFeedback->EndTask();

		// Check if any of our expected outputs already exist.
		{
			TDynArray< StringAnsi > textures;
			settingsDlg.GetTexturesToSave( textures );

			TDynArray< String > existingFiles;
			meshGen.CheckFilesExist( settingsDlg.GetOutputFolder(), settingsDlg.GetBaseName(), textures, existingFiles );
			if ( !existingFiles.Empty() )
			{
				String files;
				for ( Uint32 i = 0; i < existingFiles.Size(); ++i )
				{
					files += TXT("\n") + existingFiles[i];
				}
				if ( !GFeedback->AskYesNo( TXT("The following files already exist:%s\n\nWould you like to replace them?"), files.AsChar() ) )
				{
					// Don't want to overwrite, so loop again.
					continue;
				}
			}
		}


		GFeedback->BeginTask( TXT("Generating entity proxy"), false );

		// Do the remesh
		{
			if ( !meshGen.Process() )
			{
				GFeedback->EndTask();
				GFeedback->ShowError( TXT("Failure while generating proxy mesh:\n%s"), meshGen.GetErrorMessage().AsChar() );
				continue;
			}
		}

		// Save out the mesh and textures.
		{
			TDynArray< StringAnsi > textures;
			settingsDlg.GetTexturesToSave( textures );
			if ( !meshGen.SaveFiles( settingsDlg.GetOutputFolder(), settingsDlg.GetBaseName(), textures ) )
			{
				GFeedback->EndTask();
				GFeedback->ShowError( TXT("Failure while saving generated files:\n%s"), meshGen.GetErrorMessage().AsChar() );
				continue;
			}
		}

		GFeedback->EndTask();


		// Remove any existing proxy meshes, so we don't end up stacking up multiple copies of the same mesh.
		{
			// Store them in a separate array, so we don't have to deal with removing components while iterating through them. Not the most
			// efficient, but simpler and this isn't performance-critical.
			TDynArray< CMeshComponent* > proxyMeshes;
			for ( ComponentIterator< CMeshComponent > iter( m_preview->GetEntity() ); iter; ++iter )
			{
				CMeshComponent* mc = *iter;
				if ( mc->GetMeshNow() != nullptr && mc->GetMeshNow()->IsEntityProxy() )
				{
					proxyMeshes.PushBack( mc );
				}
			}

			for ( Uint32 i = 0; i < proxyMeshes.Size(); ++i )
			{
				m_preview->GetEntity()->DestroyComponent( proxyMeshes[i] );
			}
		}


		// Add the mesh to the entity, and we're done!
		{
			CMesh* proxyMesh = meshGen.GetGeneratedMesh();
			if ( proxyMesh != nullptr )
			{
				CMeshComponent* proxyComp = CreateObject< CMeshComponent >();
				proxyComp->SetResource( proxyMesh );
				proxyComp->SetStreamed( false );
				proxyComp->SetName( TXT("CMeshComponent_DistantProxy") );
				m_preview->GetEntity()->AddComponent( proxyComp );

				// TODO : Place it somewhere that's visible?
				//proxyComp->SetGraphPosition( ... );

				m_graph->ForceLayoutUpdate();
			}
		}


		success = true;
	}
}

void CEdEntityEditor::OnSetupProxyMesh( wxCommandEvent& event )
{	
	// Set up "Settings" UI
	CEdEntityProxySetupDlg settingsDlg( this );	
	
	GFeedback->BeginTask( TXT("Collecting meshes..."), false );

	for ( ComponentIterator< CMeshComponent > iter( m_preview->GetEntity() ); iter; ++iter )
	{
		CMeshComponent* mc = *iter;		
		settingsDlg.Insert( mc );		
	}
	settingsDlg.Fit();
	GFeedback->EndTask();

	if ( settingsDlg.ShowModal() != wxID_OK )
	{
		return;
	}
}

void CEdEntityEditor::OnDeduplicateComponents( wxCommandEvent& event )
{
	ASSERT( m_preview );

	CEntity* entity = m_preview->GetEntity();

	if ( !entity )
	{
		GFeedback->ShowError( TXT("No preview entity.") );
		return;
	}

	SEntityStreamingState state;
	entity->PrepareStreamingComponentsEnumeration( state, true, SWN_DoNotNotifyWorld );
	
	entity->CreateStreamedComponents( EStreamingWorldNotification() );
	entity->ForceFinishAsyncResourceLoads();
	Uint32 removedComponents = 0;

	for( CComponent* component : entity->GetComponents() )
	{
		for( CComponent* otherComponent : entity->GetComponents() )
		{
			if ( otherComponent == component )
			{
				continue;
			}

			TDynArray< const CResource* > resources1;
			component->GetResource( resources1 );
			TDynArray< const CResource* > resources2;
			otherComponent->GetResource( resources2 );

			if ( resources1.Size() != 1 || resources2.Size() != 1 || !resources1[0] || !resources2[0] )
			{
				continue;
			}

			// can there be components with multiple resources in this context?
			if ( resources1[0]->GetDepotPath() == resources2[0]->GetDepotPath() &&
				 otherComponent->GetPosition() == component->GetPosition() &&
				 otherComponent->GetRotation() == component->GetRotation()
			   )
			{
				entity->DestroyComponent( otherComponent );
				++removedComponents;
			}
		}
	}

	entity->FinishStreamingComponentsEnumeration( state );
	GFeedback->ShowMsg( TXT(""), TXT("%d component(s) have been removed."), removedComponents );
}

void CEdEntityEditor::OnZoomExtentsPreview( wxCommandEvent& event )
{
	m_preview->ShowZoomExtents( m_preview->GetEntity()->CalcBoundingBox() );
}

void CEdEntityEditor::OnShowBoundingBox( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowBB") );
	m_preview->ShowBB( status );
}

void CEdEntityEditor::OnShowCollision( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowCollision") );
	m_preview->ShowCollision( status );
}

void CEdEntityEditor::OnShowWireframe( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowWireframe") );
	m_preview->ShowWireframe( status );
}

void CEdEntityEditor::OnShowTBN( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "mainToolbar", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowTBN") );
	m_preview->ShowTBN( status );
}

class boxSeparator
{
public:
	void Clear(){ boxes.Clear(); }
	void insert( Float x, Float y, Float w, Float h )
	{
		box b;
		b.minx = x;
		b.maxx = x+w;
		b.miny = y-h;
		b.maxy = y;
		boxes.PushBack( b );
	}
	void SpreadRectangles()
	{
		findCollisions();
		Int32 ind = 0;
		while( colls.Size()>0 )
		{
			Int32 i;
			Int32 numc = colls.Size();
			for( i=0;i<numc;i++ )
			{
				Separate( boxes[colls[i].x], boxes[colls[i].y] );
			}
			findCollisions();
			ind++;
			if( ind>300 ){ break; }
		}
	}
	void GetBoxPos( Int32 i, Int32 & x, Int32 & y )
	{
		x = boxes[i].minx;
		y = boxes[i].maxy;
	}
private:
	class box
	{
	public:
		Float minx;
		Float maxx;
		Float miny;
		Float maxy;
	};
	class indexPair
	{
	public:
		indexPair( Int32 xx = 0, Int32 yy = 0 ){ x= xx; y = yy; }
		Int32 x;
		Int32 y;
	};
	bool Box1( Float ma, Float xa, Float mb, Float xb ){ if( ma>=xb || xa<=mb ){ return false; }else{ return true; } }
	bool Collides( box & a, box & b )
	{
		if( !Box1( a.minx, a.maxx, b.minx, b.maxx ) ){ return false; }
		if( !Box1( a.miny, a.maxy, b.miny, b.maxy ) ){ return false; }
		return true;
	}
	void findCollisions()
	{
		colls.Clear();
		Int32 num = boxes.Size();
		Int32 i;
		Int32 j;
		for( i=0;i<num;i++ )
		{
			for( j=i+1;j<num;j++ )
			{
				if( Collides( boxes[i], boxes[j] ) )
				{
					indexPair p( i, j );
					colls.PushBack( p );
				}
			}
		}
	}
	void Separate( box & a, box & b )
	{
		Float aPos[2] = { (a.minx+a.maxx)*0.5f, (a.miny+a.maxy)*0.5f };
		Float bPos[2] = { (b.minx+b.maxx)*0.5f, (b.miny+b.maxy)*0.5f };
		Float del[2]  = { bPos[0]-aPos[0], bPos[1]-aPos[1] };
		Float d2 = del[0]*del[0]+del[1]*del[1];
		if( d2< 0.001f )
		{
			del[0] = 0.0f; 
			del[1] = 15.0f; 
		}
		else
		{
			Float len = MSqrt( d2 );
			del[0] = (del[0]/len)*15.0f;
			del[1] = (del[1]/len)*15.0f;
		}
		a.minx += -del[0];
		a.maxx += -del[0];
		a.miny += -del[1];
		a.maxy += -del[1];
		b.minx += del[0];
		b.maxx += del[0];
		b.miny += del[1];
		b.maxy += del[1];
	}
	TDynArray<box> boxes;
	TDynArray<indexPair> colls;
};

void CEdEntityEditor::RealignOverlappingComponents( Bool all )
{
	TDynArray< CObject* > objects;
	if( all )
	{
		m_graph->GetAllObjects( objects );
	}
	else
	{
		m_graph->GetSelectedObjects( objects );
	}
	boxSeparator sep;
	for ( auto it=objects.Begin(); it != objects.End(); ++it )
	{
		CComponent* component = Cast< CComponent >( *it );
		if ( component )
		{
			Int32 x = 0;
			Int32 y = 0;
			Int32 w = 0;
			Int32 h = 0;
			m_graph->GetBlockWidthHeight( component, w, h );
			component->GetGraphPosition( x, y );
			sep.insert( x, y, w, h );
		}
	}
	sep.SpreadRectangles();
	Int32 ind = 0;
	for ( auto it=objects.Begin(); it != objects.End(); ++it )
	{
		CComponent* component = Cast< CComponent >( *it );
		if ( component )
		{
			Int32 x = 0;
			Int32 y = 0;
			sep.GetBoxPos( ind, x, y );
			component->SetGraphPosition( x, y );
			ind++;
		}
	}
}
