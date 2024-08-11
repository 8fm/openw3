/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "materialListManager.h"
#include "undoMeshEditor.h"
#include "undoManager.h"
#include "autoSizeListCtrl.h"
#include "../../common/engine/descriptionGraphBlock.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/graphBlock.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/materialInstance.h"

BEGIN_EVENT_TABLE( CEdMaterialListManager, wxPanel )
	EVT_BUTTON   ( XRCID("UseShader"),         CEdMaterialListManager::OnUseShader )
	EVT_BUTTON   ( XRCID("UseAllShader"),      CEdMaterialListManager::OnUseAllShader )
	EVT_BUTTON   ( XRCID("AddShader"),         CEdMaterialListManager::OnAddShader )
	EVT_BUTTON   ( XRCID("RemapMaterials"),    CEdMaterialListManager::OnRemap )
	EVT_BUTTON   ( XRCID("CopyInstance"),      CEdMaterialListManager::OnCopyInstance )
	EVT_BUTTON   ( XRCID("RemoveUnused"),      CEdMaterialListManager::OnRemoveUnused )
	EVT_CHOICE   ( XRCID("ShaderChoice"),      CEdMaterialListManager::OnStdShaderListSelection )
	EVT_CHECKBOX ( XRCID("HighlightMaterial"), CEdMaterialListManager::OnHighlight )
	EVT_CHECKBOX ( XRCID("LocalInstance"),     CEdMaterialListManager::OnLocalInstanceChecked )
	EVT_BUTTON   ( XRCID("SaveMatInstance"),   CEdMaterialListManager::OnSaveMaterialInstanceToFile )
	EVT_UPDATE_UI( wxID_ANY, CEdMaterialListManager::OnUpdateUI )
	EVT_LIST_ITEM_SELECTED   ( XRCID("MaterialList"),  CEdMaterialListManager::OnMaterialSelected )
	EVT_LIST_ITEM_DESELECTED ( XRCID("MaterialList"),  CEdMaterialListManager::OnMaterialDeselected )
	EVT_LIST_END_LABEL_EDIT  ( XRCID("MaterialList"),  CEdMaterialListManager::OnMaterialRenamed )
	EVT_LIST_ITEM_ACTIVATED  ( XRCID("MaterialList"),  CEdMaterialListManager::OnItemDoubleClicked )
END_EVENT_TABLE()

CEdMaterialListManager::CEdMaterialListManager( 
		wxWindow* parent, CObject* owner, const String& rootPath, CEdUndoManager* undoManager 
	)
	: m_owner( owner )
	, m_selected( -1 )
	, m_undoManager( undoManager )
{
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPropertyPostChange ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPropertyChanging ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPropertyPostTransaction ), this );

	wxXmlResource::Get()->LoadPanel( this, parent, TXT("MaterialListManager") );

	// Create properties in panel
	{
		wxPanel* rp = XRCCTRL( *this, "MaterialPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, undoManager );
		m_properties->Get().Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdMaterialListManager::OnMaterialPropertiesChanged, this );
		sizer1->Add( m_properties, 1, wxEXPAND|wxRIGHT|wxBOTTOM, 5 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_applyToAll        = XRCCTRL( *this, "UseAllShader", wxButton );
	m_copyBtn           = XRCCTRL( *this, "CopyInstance", wxButton );
	m_removeBtn         = XRCCTRL( *this, "RemoveUnused", wxButton );
	m_remapBtn          = XRCCTRL( *this, "RemapMaterials", wxButton );
	m_stdShadersList    = XRCCTRL( *this, "ShaderChoice", wxChoice );
	m_shaderDescription = XRCCTRL( *this, "ShaderDesc", wxTextCtrl );
	m_highlightCheck    = XRCCTRL( *this, "HighlightMaterial", wxCheckBox );
	m_instanceCheck     = XRCCTRL( *this, "LocalInstance", wxCheckBox );
	m_saveInstanceBtn	= XRCCTRL( *this, "SaveMatInstance", wxButton );

	// Material list
	{
		m_materialList = XRCCTRL( *this, "MaterialList", CEdAutosizeListCtrl );
		m_materialList->AppendColumn( TXT("Name"), wxLIST_FORMAT_LEFT, 200 );
		m_materialList->AppendColumn( TXT("Local"), wxLIST_FORMAT_LEFT, 60 );
		m_materialList->AppendColumn( TXT("Base"), wxLIST_FORMAT_LEFT, -1 );
		SetDropTargetWindow( m_materialList );
	}

	// scan for std shaders	
	CDirectory* rootDir = GDepot->FindPath( rootPath.AsChar() );
	ASSERT( rootDir );

	for ( CDiskFile* file : rootDir->GetFiles() )
	{
		if ( file->GetFileName().EndsWith( TXT(".w2mg") ) )
		{		
			file->Load();
			if( IMaterial* mat = Cast< IMaterial >( file->GetResource() ) )
			{
				m_stdShadersFiles.PushBack( file );
				m_stdShadersList->Append( mat->GetMaterialDefinition()->GetShaderName().AsChar() );
			}
		}
	}	

	Layout();
	Show();
}

CEdMaterialListManager::~CEdMaterialListManager()
{
	SEvents::GetInstance().UnregisterListener( this );
}

Bool CEdMaterialListManager::SelectByName( const String& materialName )
{
	// Search for matching index
	const Uint32 numMaterials = GetNumMaterials();
	for ( Uint32 i=0; i<numMaterials; i++ )
	{
		const String name = GetMaterialName( i );
		if ( name == materialName )
		{
			m_materialList->SetSelection( i );
			m_materialList->EnsureVisible( i );
			return true;
		}
	}

	// Not found
	return false;
}

namespace
{
	String GetBaseMaterialText( const IMaterial* material )
	{
		ASSERT ( material );
		ASSERT ( material->GetFile() );

		if ( const CMaterialInstance* inst = Cast< CMaterialInstance >( material ) )
		{ // on-disk instance
			return CFilePath( inst->GetFile()->GetFileName() ).GetFileName() + TXT(" (instance)");
		}
		else if ( const IMaterialDefinition* def = Cast< IMaterialDefinition >( material ) )
		{ // on-disk graph
			return def->GetShaderName() + TXT(" (shader)");
		}
		else
		{ // sth unsupported yet
			return TXT("<unknown type>");
		}
	}
}

Bool CEdMaterialListManager::IsDirectlyBasedOnGraph( IMaterial* mat ) const
{
	// is based on graph when it's not...
	if ( !mat || !mat->GetMaterialDefinition() )
	{
		return false; // ...broken
	}

	if ( mat->GetParent() == m_owner && mat->GetBaseMaterial()->IsA< CMaterialInstance >() )
	{
		return false; // ...local and has a base that is an on-disk instance
	}

	if ( mat->GetParent() != m_owner && mat->IsA< CMaterialInstance >() )
	{
		return false; // ...a disk-based instance
	}

	return true;
}

void CEdMaterialListManager::UpdateMaterialList()
{
	m_materialList->Freeze();
	int topItem = m_materialList->GetTopItem();
	m_materialList->DeleteAllItems();

	// Add materials
	const Int32 numMaterials = GetNumMaterials();

	for ( Int32 i=0; i<numMaterials; ++i )
	{
		// Get material
		IMaterial* material = GetMaterial( i );

		const String& name = GetMaterialName( i );
		m_materialList->InsertItem( i, name.AsChar() );

		if ( !material )
		{
			m_materialList->SetItem( i, 2, TXT("<broken>") );
		}
		else if ( material->GetParent() == m_owner )
		{
			ASSERT( material->IsA< CMaterialInstance >() );
			ValidateAndFixMaterialInstanceParameters( Cast< CMaterialInstance >( material ) );

			if ( IMaterial* baseMaterial = material->GetBaseMaterial() )
			{
				m_materialList->SetItem( i, 1, TXT("Yes") );
				m_materialList->SetItem( i, 2, GetBaseMaterialText( baseMaterial ).AsChar() );
			}
			else
			{
				m_materialList->SetItem( i, 2, TXT("<broken>") );
			}
		}
		else
		{
			m_materialList->SetItem( i, 2, GetBaseMaterialText( material ).AsChar() );
		}
	}

	// Clamp selection
	m_selected = Min< Int32 >( m_selected, numMaterials-1 );

	// Set selection
	if ( !numMaterials )
	{
		// Reset selection
		m_selected = -1;
		// Add something to list
		m_materialList->Disable();
		m_materialList->InsertItem( 0, TXT("(no materials)") );
	}
	else
	{
		// Select default element
		if ( m_selected < 0 )
		{
			m_selected = 0;
		}

		// Select
 		m_materialList->Enable();
 		m_materialList->SetSelection( m_selected );
 		m_materialList->SetTopItem( topItem );
	}

	// Update list
	m_materialList->Thaw();
}

void CEdMaterialListManager::OnMaterialSelected( wxListEvent& event )
{
#ifdef USE_NVIDIA_FUR
	if ( GetNumMaterials() == 0 )
	{
		event.Skip();
		return;
	}
#endif

	TDynArray< int > selection = m_materialList->GetSelectedItems();
	
	m_selected = selection.Empty() ? -1 : selection[0];
	ASSERT ( m_selected != -1 ); // shouldn't really happen when using wxListCtrl
	
	IMaterial* material = GetMaterial( m_selected );

	// Update properties
	if ( material && material->IsContained( m_owner ) )
	{
		m_properties->Get().SetObject( material );
		ExpandRootItems();
		m_instanceCheck->SetValue( true );
		m_saveInstanceBtn->Enable();
	}
	else
	{
		m_properties->Get().SetNoObject();
		m_instanceCheck->SetValue( false );
		m_saveInstanceBtn->Disable();
	}

	if ( m_highlightCheck->GetValue() && m_selected >= 0 )
	{
		HighlightMaterial( m_selected, true );
	}

	// Update shader combo selection

	if ( !IsDirectlyBasedOnGraph( material ) )
	{
		m_stdShadersList->SetSelection( wxNOT_FOUND );
	}
	else
	{
		ASSERT( material && material->GetMaterialDefinition() ); // CanChangeShaderFor should filter out broken stuff
		Int32 idx = m_stdShadersFiles.GetIndex( material->GetMaterialDefinition()->GetFile() );
		m_stdShadersList->SetSelection( idx );
	}

	UpdateSelectedShaderDesc();

	event.Skip();
}

void CEdMaterialListManager::OnMaterialDeselected( wxListEvent& event )
{
	if ( m_highlightCheck->GetValue() )
	{
		HighlightMaterial( event.GetIndex(), false );
	}

	wxPoint mp = m_materialList->ScreenToClient( wxGetMousePosition() );

	int flags = 0;
	m_materialList->HitTest( mp, flags );
	if ( !(flags & wxLIST_HITTEST_ONITEM) )
	{ // clicked outside items area (deselecting all)
		m_properties->Get().SetNoObject();
		m_instanceCheck->SetValue( false );
		m_saveInstanceBtn->Disable();
		m_stdShadersList->SetSelection( wxNOT_FOUND );
		m_selected = -1;
		UpdateSelectedShaderDesc();
	}
}

void CEdMaterialListManager::OnMaterialRenamed( wxListEvent& event )
{
	String newName = event.GetText().c_str();

	if ( newName == GetMaterialName( m_selected ) )
	{
		event.Skip();
		return;
	}

	if ( !IsNameUnique( newName ) )
	{
		GFeedback->ShowError( TXT("Material with this name already exists") );
		event.Veto();
		return;
	}

	if ( RenameMaterial( event.m_itemIndex, newName ) )
	{
		event.Skip();
	}
	else
	{
		event.Veto();
	}
}

void CEdMaterialListManager::OnItemDoubleClicked( wxListEvent& event )
{
	IMaterial* mat = GetMaterial( event.GetIndex() );

	if ( mat && mat->GetParent() == m_owner )
	{ // if it's local, go one level up
		mat = mat->GetBaseMaterial();
	}

	if ( mat && mat->GetFile() )
	{
		wxTheFrame->GetAssetBrowser()->SelectFile( mat->GetDepotPath() );
	}
}

Bool CEdMaterialListManager::IsNameUnique( const String& name ) const
{
	for ( Uint32 i = 0, count = GetNumMaterials(); i < count; ++i )
	{
		if ( GetMaterialName( i ) == name )
		{
			return false;
		}
	}

	return true;
}

IMaterial* CEdMaterialListManager::AcquireSelectedStdShader( String* outName )
{
	Int32 selectedIdx = m_stdShadersList->GetSelection();

	if ( selectedIdx > -1 && selectedIdx < m_stdShadersFiles.SizeInt() )
	{
		CDiskFile* shaderFile = m_stdShadersFiles[ selectedIdx ];
		shaderFile->Load();

		if ( outName )
		{
			*outName = String( m_stdShadersList->GetString( selectedIdx ).c_str() );
		}

		return Cast< IMaterial > ( shaderFile->GetResource() );
	}

	return nullptr;
}

void CEdMaterialListManager::ApplyShader( Uint32 chunkSelection )
{
	if ( IMaterial* resourceMaterial = AcquireSelectedStdShader() )
	{
		// Get instance
		CMaterialInstance* instance = Cast< CMaterialInstance >( GetMaterial( chunkSelection ) );
		if ( instance && instance->GetParent() == m_owner )
		{			
			instance->SetBaseMaterial( resourceMaterial );
		}
		else
		{
			// Set new material
			SetMaterial( chunkSelection, resourceMaterial );
		}

		m_properties->Get().RefreshValues();

		// Update material list
		UpdateMaterialList();

		// Update proxies
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
}

void CEdMaterialListManager::OnUseShader( wxCommandEvent& event )
{	
	ApplyShader( m_selected );
}

void CEdMaterialListManager::OnUseAllShader( wxCommandEvent& event )
{
	for( Int32 i=0; i<m_materialList->GetItemCount(); i++ )
	{
		ApplyShader( i );
	}	
}

namespace
{
	template < typename TestFunc >
	String MakeNameUnique( const String& baseName, const String& suffix, TestFunc uniqnessTest )
	{
		String newNameBase = baseName;
		if ( !suffix.Empty() && !newNameBase.EndsWith( suffix ) )
		{
			newNameBase +=suffix;
		}

		String newName = newNameBase;
		Int32 idx=1;
		while ( !uniqnessTest( newName ) )
		{
			newName = newNameBase + TXT("(") + ToString( idx++ ) + TXT(")");
		}

		return newName;
	}
}

void CEdMaterialListManager::OnAddShader( wxCommandEvent& event )
{
	String materialName;
	if ( IMaterial* resourceMaterial = AcquireSelectedStdShader( &materialName ) )
	{
		if ( CMaterialInstance* newMaterialInst = new CMaterialInstance( m_owner, resourceMaterial ) )
		{
			String newName = MakeNameUnique( materialName, TXT(""), 
											 [this]( const String& s ) { return IsNameUnique( s ); } );

			Int32 newIdx = AddMaterial( newMaterialInst, newName );

			if ( newIdx != -1 )
			{
				UpdateMaterialList();
				m_materialList->EditLabel( newIdx ); // Give the user the chance to alter the name
			}
			else
			{
				newMaterialInst->Discard();
			}
		}
	}
}

void CEdMaterialListManager::UpdateSelectedShaderDesc()
{
	Int32 stdListSelection = m_stdShadersList->GetSelection();

	if ( stdListSelection >= 0 )
	{
		CDiskFile* shaderFile = m_stdShadersFiles[ stdListSelection ];
		
		shaderFile->Load();

		// Get the resource
		IMaterial* resourceMaterial = Cast< IMaterial > ( shaderFile->GetResource() );
		ASSERT( resourceMaterial );

		if ( CMaterialGraph* def = Cast< CMaterialGraph > ( resourceMaterial->GetMaterialDefinition() ) )
		{
			for ( CGraphBlock* block : def->GraphGetBlocks() )
			{			
				if ( CDescriptionGraphBlock* dgb = Cast< CDescriptionGraphBlock >( block ) )
				{
					if ( dgb->GetCaption() == TXT("ShaderDescription") )
					{
						m_shaderDescription->SetValue( dgb->GetDescriptionText().AsChar() );
						return;
					}
				}
			}
		}
	}

	// not found
	m_shaderDescription->SetValue( TXT("") );
}

void CEdMaterialListManager::OnStdShaderListSelection( wxCommandEvent& event )
{
	UpdateSelectedShaderDesc();

	if ( m_selected >= 0 )
	{
		ApplyShader( m_selected );
	}
}

void CEdMaterialListManager::OnCreateInstance( wxCommandEvent& event )
{
	// Get shader
	IMaterial* currentMaterial = GetMaterial( m_selected );
	if ( currentMaterial && currentMaterial->GetParent() == NULL && currentMaterial->GetFile() != NULL )
	{
		// Create instance inside owner space
		CMaterialInstance* instance = new CMaterialInstance( m_owner, currentMaterial );

		// Set as new material
		SetMaterial( m_selected, instance );

		if ( m_undoManager )
		{
			// Store on the undo queue
			CUndoMeshMaterialInstanceExistance::CreateCreationStep( *m_undoManager, this, m_selected );
		}

		// Update list
		UpdateMaterialList();
	}
}

void CEdMaterialListManager::OnDestroyInstance( wxCommandEvent& event )
{
	// Get instance
	CMaterialInstance* instance = SafeCast< CMaterialInstance >( GetMaterial( m_selected ) );
	if ( instance && instance->GetParent() == m_owner )
	{
		if ( m_undoManager )
		{
			// Store on the undo queue
			CUndoMeshMaterialInstanceExistance::CreateDeletionStep( *m_undoManager, this, m_selected );
		}

		// Revert to shader
		SetMaterial( m_selected, instance->GetBaseMaterial() );

		// Update list
		UpdateMaterialList();
	}
}

CMaterialInstance* CEdMaterialListManager::CreateNewMaterialInstance( CObject* parent )
{
	IMaterial* selected = GetMaterial( m_selected );

	if ( selected->GetMaterialDefinition() == nullptr )
	{ // broken material, do not copy it
		return nullptr;
	}

	// if a local instance is selected, make its parent as a base for a new material
	IMaterial* base = ( selected->GetParent() == m_owner ) ? selected->GetBaseMaterial() : selected;
	CMaterialInstance* newMaterialInst = nullptr;

	if ( newMaterialInst = new CMaterialInstance( parent, base ) )
	{
		if ( CMaterialInstance* selectedAsInst = Cast< CMaterialInstance >( selected ) )
		{
			// copy parameters if the selected material is an instance
			for ( const MaterialParameterInstance& param : selectedAsInst->GetParameters() )
			{
				newMaterialInst->WriteParameterRaw( param.GetName(), param.GetData() );
			}
		}
	}
	return newMaterialInst;
}

void CEdMaterialListManager::OnCopyInstance( wxCommandEvent& event )
{
	if ( CMaterialInstance* newMaterialInst = CreateNewMaterialInstance( m_owner ) )
	{
		String newName = MakeNameUnique( GetMaterialName( m_selected ), TXT("_copy"), 
										 [this]( const String& s ) { return IsNameUnique( s ); } );

		Int32 newIdx = AddMaterial( newMaterialInst, newName );

		if ( newIdx != -1 )
		{
			UpdateMaterialList();
			m_materialList->EditLabel( newIdx ); // Give the user the chance to alter the name
		}
		else
		{
			newMaterialInst->Discard();
		}
	}
}

void CEdMaterialListManager::OnRemoveUnused( wxCommandEvent& event )
{
	if ( RemoveUnusedMaterials() )
	{
		UpdateMaterialList();
	}
}

void CEdMaterialListManager::OnRemap( wxCommandEvent& event )
{
	if ( RemapMaterials() )
	{
		UpdateMaterialList();
	}
}

wxDragResult CEdMaterialListManager::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
    const TDynArray< CResource* > &resources = GetDraggedResources();

    if (resources.Size() == 0)
        return wxDragNone;

    // Try to load
    IMaterial* baseMaterial = Cast< IMaterial >(  resources[0] );
    if ( baseMaterial )
    {
		Int32 flags;
		Int32 idx = m_materialList->HitTest( wxPoint( x, y ), flags );

        if ( idx < 0 || !( flags & wxLIST_HITTEST_ONITEM ) )
		{
			return wxDragNone;
		}

		m_materialList->SetHighlight( idx );

        return def;
    }
    
    return wxDragNone;
}

Bool CEdMaterialListManager::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
    if ( m_materialList->GetItemCount() == 0 || resources.Size() == 0 )
	{
        return false;
	}


	Int32 flags;
	Int32 idx = m_materialList->HitTest( wxPoint( x, y ), flags );

    if ( idx < 0 || !( flags & wxLIST_HITTEST_ONITEM ) )
	{
		return false;
	}

    // Try to load
    if ( IMaterial* baseMaterial = Cast< IMaterial >( resources[0] ) )
    {
        // Get instance
        CMaterialInstance* instance = Cast< CMaterialInstance >( GetMaterial( idx ) );
        if ( instance && instance->GetParent() == m_owner )
        {
            instance->SetBaseMaterial( baseMaterial );
        }
        else
        {
            // Set new material
            SetMaterial( idx, baseMaterial );
        }

        // Update material list
        UpdateMaterialList();
    }

    return true;
}

void CEdMaterialListManager::OnHighlight( wxCommandEvent& event )
{
	if ( m_selected >= 0 )
	{
		HighlightMaterial( m_selected, m_highlightCheck->GetValue() );
	}
}

void CEdMaterialListManager::OnLocalInstanceChecked( wxCommandEvent& event )
{
	if ( m_instanceCheck->GetValue() )
	{
		OnCreateInstance( event );
		m_saveInstanceBtn->Enable();
	}
	else
	{
		OnDestroyInstance( event );
		m_saveInstanceBtn->Disable();
	}
}

void CEdMaterialListManager::OnSaveMaterialInstanceToFile( wxCommandEvent& event )
{
	if ( CMaterialInstance* newMaterial = CreateNewMaterialInstance( nullptr ) )
	{
		wxFileDialog saveFileDialog( this, wxT("Save file"), "", "", ".w2mi files (*.w2mi)|*.w2mi", wxFD_SAVE|wxFD_OVERWRITE_PROMPT );
		if ( saveFileDialog.ShowModal() == wxID_OK )
		{
			String depotAbsolutePath;
			GDepot->GetAbsolutePath( depotAbsolutePath );

			String path = saveFileDialog.GetDirectory() + TXT("\\");
			String filename = saveFileDialog.GetFilename();

			CDirectory* saveDir = GDepot->FindPath( path.StringAfter( depotAbsolutePath ) );
			if ( saveDir && newMaterial->SaveAs( saveDir, filename ) )
			{
				GFeedback->ShowMsg( TXT("Success"), TXT("Material instance saved.") );
				return;
			}
		}
		newMaterial->Discard();
	}
	GFeedback->ShowError( TXT("Saving material instance failed.") );
}

void CEdMaterialListManager::OnUpdateUI( wxUpdateUIEvent& event )
{
	Bool isSelected = ( m_selected >= 0 );
	IMaterial* selectedMaterial = isSelected ? GetMaterial( m_selected ) : nullptr;

	m_applyToAll    ->Enable( selectedMaterial != nullptr && m_stdShadersList->GetSelection() != wxNOT_FOUND );
	m_copyBtn       ->Enable( selectedMaterial != nullptr );
	m_instanceCheck ->Enable( selectedMaterial != nullptr );
	m_stdShadersList->Enable( isSelected ); // NOTE: allow to select a shader even if selectedMaterial is null (broken)
}

void CEdMaterialListManager::EnableMaterialHighlighting( Bool state )
{
	if ( state )
	{
		m_highlightCheck->Show();
		if ( m_selected >= 0 )
		{
			HighlightMaterial( m_selected, m_highlightCheck->GetValue() );
		}
	}
	else
	{
		if ( m_selected >= 0 )
		{
			HighlightMaterial( m_selected, false );
		}
		m_highlightCheck->Hide();
	}
}

void CEdMaterialListManager::EnableListModification( Bool state )
{
	m_copyBtn->Show( state );
	m_removeBtn->Show( state );

	m_copyBtn->GetParent()->Layout();
}

void CEdMaterialListManager::EnableMaterialRemapping( Bool state )
{
	m_remapBtn->Show( state );
	m_remapBtn->GetParent()->Layout();
}

void CEdMaterialListManager::ExpandRootItems()
{
    if ( CBasePropItem *basePropItem = m_properties->Get().GetRootItem() )
    {
        if ( !basePropItem->IsExpanded() )
        {
            basePropItem->Expand();
        }

        TDynArray< CBasePropItem * > children = basePropItem->GetChildren();
        for ( Uint32 i = 0; i < children.Size(); ++i )
        {
            if ( !children[i]->IsExpanded() )
            {
                children[i]->Expand();
            }
        }
    }
}

void CEdMaterialListManager::OnMaterialPropertiesChanged( wxCommandEvent& event )
{
	auto eventData = static_cast< CEdPropertiesPage::SPropertyEventData* >( event.GetClientData() );

	if ( eventData->m_propertyName == TXT("baseMaterial") )
	{
		UpdateMaterialList();
	}
}

void CEdMaterialListManager::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( EditorPropertyPostChange ) || name == RED_NAME( EditorPropertyChanging ) || name == RED_NAME( EditorPropertyPostTransaction ) )
	{
		const CEdPropertiesPage::SPropertyEventData& ped = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( ped.m_page == &m_properties->Get() )
		{
			Bool finished = ( name == RED_NAME( EditorPropertyPostTransaction ) );
			MaterialPropertyChanged( ped.m_propertyName, finished );
		}
	}
}

void CEdMaterialListManager::ValidateAndFixMaterialInstanceParameters( CMaterialInstance* matInstance )
{
	THashMap< CName, CProperty > baseParameters;

	// Get base material graph and extract parameters
	if ( CMaterialGraph* graph = Cast< CMaterialGraph >( matInstance->GetMaterialDefinition() ) )
	{
		for ( CGraphBlock* block : graph->GraphGetBlocks() )
		{
			if ( CMaterialParameter* param = Cast< CMaterialParameter >( block ) )
			{
				if ( param->GetParameterName() && param->GetParameterProperty() )
				{
					baseParameters.Insert( param->GetParameterName(), *(param->GetParameterProperty() ) );
				}
			}
		}

		TMaterialParameters instanceParameters = matInstance->GetParameters();
		TDynArray< Bool > validationResult;
		Uint32 mismatchedTypesCount = 0;

		// check if instance parameters have same type as base parameters
		for ( Uint32 i = 0; i < instanceParameters.Size(); ++i )
		{
			MaterialParameterInstance param = instanceParameters[i];
			const CProperty* baseProp = baseParameters.FindPtr( param.GetName() );
			if ( baseProp && baseProp->GetType() != param.GetType() )
			{
				validationResult.PushBack( false );
				mismatchedTypesCount++;
			}
			else
			{
				validationResult.PushBack( true );
			}
		}

		// oops, found parameters types mismatch
		if ( mismatchedTypesCount > 0 )
		{
			String info = String::Printf( TXT("Found %d parameters types mismatch between material instance and base. Click 'Yes' to default their values."), mismatchedTypesCount );
			if ( GFeedback->AskYesNo( info.AsChar() ) )
			{
				// we're gonna modify the resource - mark it as such
				if ( CResource* ownerRes = Cast< CResource >( m_owner ) )
				{
					ownerRes->MarkModified();
				}

				// clear all instance parameters and write them back only if they have proper type
				matInstance->ClearInstanceParameters();
				for ( Uint32 i = 0; i < instanceParameters.Size(); ++i )
				{
					if ( validationResult[i] )
					{
						MaterialParameterInstance paramInstance = instanceParameters[ i ];
						matInstance->WriteParameterRaw( paramInstance.GetName(), paramInstance.GetData() );
					}
				}
				GFeedback->ShowMsg( TXT("Parameters changed"), TXT("Parameters were updated. Note that the asset may change its look.") );
			}
			else
			{
				GFeedback->ShowWarn( TXT("Crashes possible.") );
			}
		}
	}
}