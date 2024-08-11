#include "build.h"
#include "../../common/game/communityData.h"
#include "../../common/game/actionPointComponent.h"
#include "communityEditor.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/game/actionPoint.h"


namespace // anonymous
{
	CGatheredResource resGridStorySubPhases( TXT("gameplay\\globals\\storysubphases.csv"), 0 );
}

// Event table
BEGIN_EVENT_TABLE( CEdCommunityEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemSave" ), CEdCommunityEditor::OnSave )
	EVT_MENU( XRCID( "menuItemExit" ), CEdCommunityEditor::OnExit )
	EVT_MENU( XRCID( "menuItemValidate" ), CEdCommunityEditor::OnValidate )
	//EVT_MOUSE_CAPTURE_LOST( CEdSpawnSetEditor::OnMouseCaptureLost )
	EVT_COMMAND( wxID_ANY, wxEVT_GRID_VALUE_CHANGED, CEdCommunityEditor::OnGridValueChanged )
	EVT_CHOICE( XRCID( "spawnsetTypeChoice" ), CEdCommunityEditor::OnTypeChanged )
END_EVENT_TABLE()

CEdCommunityEditor::CEdCommunityEditor(wxWindow* parent, CCommunity* community)
: wxSmartLayoutPanel(parent, TXT("SpawnSetEditor"), false)
, m_communityTableProperty( TXT("communityTable") )
, m_storyPhaseTimetableProperty( TXT("storyPhaseTimetable") )
, m_sceneTableProperty( TXT("sceneTable") )
, m_reactionTableProperty( TXT("reactionTable") )
, m_layersProperty( TXT("layers") )
, m_community(community)
, m_communityTableGrid(NULL)
, m_storyPhaseTimetableGrid(NULL)
, m_sceneTableGrid(NULL)
, m_layoutsGrid(NULL)
, m_invalidData(true)
{
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

	// Add the reference to resources
	m_community->AddToRootSet();

	if( !m_community->InlinedDuplicateTest() )
	{
		wxMessageBox( wxT("Duplicated initializers, check the log!!!" ) );
	}

	CheckAndUpdateGUIDs();

	m_guiGridPanel = XRCCTRL( *this, "panelGrid", wxPanel );
	m_notebook = new wxNotebook(m_guiGridPanel, wxID_ANY);

	m_typeChoice = XRCCTRL( *this, "spawnsetTypeChoice", wxChoice );
	const TDynArray< CName >& typeOptions = SRTTI::GetInstance().FindEnum( CNAME( ECommunitySpawnsetType ) )->GetOptions();
	for ( Uint32 i = 0; i < typeOptions.Size(); ++i )
	{
		m_typeChoice->AppendString( typeOptions[ i ].AsString().AsChar() );
	}
	m_typeChoice->SetSelection( community->GetSpawnsetType() );


	// Add tabs
	CClass *classPtr = m_community->GetClass();

	// tab 1
	if (m_communityTableGrid = CreateGridFromProperty(m_notebook, classPtr, TXT("communityTable")))
	{
		m_communityTableGrid->SetDefaultObjectParent( m_community );
		m_notebook->AddPage(m_communityTableGrid, TXT("Spawnset"), true);
	}

	// tab 2
	if (m_storyPhaseTimetableGrid = CreateGridFromProperty(m_notebook, classPtr, TXT("storyPhaseTimetable")))
	{
		m_notebook->AddPage(m_storyPhaseTimetableGrid, TXT("Timetables"), false);
	}

	//////////////////////////////////////////////////////////////////////////

	//m_timetablePanel = new wxPanel( m_notebook );
	//wxSizer* timetableSizer = new wxBoxSizer( wxHORIZONTAL );
	//m_timetablePanel->SetSizer( timetableSizer );
	//PropertiesPageSettings propertySettings;
	//m_timetableProperties = new CEdPropertiesPage( m_timetablePanel, propertySettings );
	//m_timetablePanel->GetSizer()->Add( m_timetableProperties, 1, wxALL | wxEXPAND );

	//m_notebook->AddPage( m_timetablePanel, TXT( "New timetable" ), false );
	//if ( m_community->GetStoryPhaseTimetable().Empty() == false )
	//{
	//	m_timetableProperties->SetObject( &m_community->GetStoryPhaseTimetable()[ 0 ] );
	//}


	//////////////////////////////////////////////////////////////////////////

	wxBoxSizer* sizerGrid = new wxBoxSizer(wxVERTICAL);
	sizerGrid->Add(m_notebook, 1, wxEXPAND);
	m_guiGridPanel->SetSizer( sizerGrid );
	m_guiGridPanel->Layout();

	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();
	
	wxString titleString(m_community->GetFile()->GetFileName().AsChar());
	titleString += wxT( " - Community Editor" );
	SetTitle( titleString );
	Show();
}

CEdCommunityEditor::~CEdCommunityEditor()
{
	SEvents::GetInstance().UnregisterListener( this );

	// Remove the reference from resources
	m_community->RemoveFromRootSet();

	SaveOptionsToConfig();
}

void CEdCommunityEditor::CheckAndUpdateGUIDs()
{
	// Do we have any duplicated GUIDs ?
	if( !m_community->CheckGUIDs() )
	{
		// Duplicated shit
		if ( wxYES == wxMessageBox( wxT("Community have DUPLICATED GUIDS and are invalid. Please regenerate GUIDs."), wxT("Duplicated GUIDs found"), wxYES_NO | wxYES_DEFAULT | wxICON_WARNING ) )
		{
			m_community->UpdateGUIDS();
		}
	}
}

void CEdCommunityEditor::CacheMapPinPositions()
{
    if ( ! GGame->GetActiveWorld() )
    {
        return;
    }

    TDynArray< CSTableEntry >& commTable = m_community->GetCommunityTable();

    for ( TDynArray< CSTableEntry >::iterator it = commTable.Begin(); it != commTable.End(); ++it )
    {
        CSTableEntry& tableEntry = *it;

        for ( TDynArray< CSStoryPhaseEntry >::iterator jt = tableEntry.m_storyPhases.Begin(); jt != tableEntry.m_storyPhases.End(); ++jt )
        {
            CSStoryPhaseEntry& storyPhase = *jt;

            if ( storyPhase.m_spawnPointTags.Empty() || storyPhase.m_startInAP )
            {
                continue;
            }

            Vector pos;
            Float yaw;
            CLayerInfo* layer;

            if ( storyPhase.AllocateSpawnPoint(pos, yaw, layer) ) 
            {
                storyPhase.m_cachedMapPinPosition = pos;
                m_invalidData = true;
            }
        }
    }

    Vector apPos( 0.f, 0.f, 0.f );

    CActionPointManager* apMan = CreateObject<CActionPointManager>();

    TDynArray< CActionPointComponent* > apComponents;
    CollectAllComponents( GGame->GetActiveWorld(), apComponents );

    TDynArray< CSStoryPhaseTimetableEntry >& timetable = m_community->GetStoryPhaseTimetable();

    for ( TDynArray< CActionPointComponent* >::iterator it = apComponents.Begin(); it != apComponents.End(); ++it )
    {
        apMan->RegisterActionPoint( *it );
    }

    for ( TDynArray< CSStoryPhaseTimetableEntry >::iterator it = timetable.Begin(); it != timetable.End(); ++it )
    {
        CSStoryPhaseTimetableEntry& ttEntry = *it;
        for ( TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >::iterator jt = ttEntry.m_actionCategies.Begin(); jt != ttEntry.m_actionCategies.End(); ++jt )
        {
            CSStoryPhaseTimetableACategoriesTimetableEntry& spTTEntry = *jt;
            for ( TDynArray< CSStoryPhaseTimetableActionEntry >::iterator kt = spTTEntry.m_actions.Begin(); kt != spTTEntry.m_actions.End(); ++kt )
            {
                for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::iterator lt = (*kt).m_actionCategories.Begin(); lt != (*kt).m_actionCategories.End(); ++lt )
                {
                    CSStoryPhaseTimetableACategoriesEntry& entry = *lt;

                    TActionPointID actionPoint;                 

                    CLayerInfo* layer = (*kt).m_layerName.GetCachedLayer();
					
					SActionPointFilter apFilter;
					apFilter.m_actionPointTags = entry.m_apTags;
					apFilter.m_category = entry.m_name;
					apFilter.m_layerGuid = (layer != NULL ) ? layer->GetGUID() : CGUID::ZERO;

                    if ( layer && apMan->FindActionPoint( actionPoint, apFilter ) == FAPR_Success )
                    {
                        if ( apMan->GetGoToPosition( actionPoint, &apPos, NULL ) )
                        {
                            m_invalidData = true;
                        }
                    }
                }
            }
        }
    }
}

void CEdCommunityEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPropertyPreChange ) )
	{
		m_invalidData = true;
	}
	
	if ( name == CNAME( EditorPropertyPostChange ) )
	{
		m_invalidData = true;
	}
}

void CEdCommunityEditor::OnInternalIdle()
{
	if (m_invalidData)
	{
		CClass *classPtr = m_community->GetClass();
		if ( CProperty *propertyPtr = classPtr->FindProperty( m_communityTableProperty ) )
			SetGridObject( m_communityTableGrid, propertyPtr );

		if ( CProperty *propertyPtr = classPtr->FindProperty( m_storyPhaseTimetableProperty ) )
			SetGridObject( m_storyPhaseTimetableGrid, propertyPtr );

		if ( CProperty *propertyPtr = classPtr->FindProperty( m_sceneTableProperty ) )
			SetGridObject( m_sceneTableGrid, propertyPtr );

		if ( CProperty *propertyPtr = classPtr->FindProperty( m_layersProperty ) )
			SetGridObject( m_layoutsGrid, propertyPtr );

		m_invalidData = false;
	}
}

void CEdCommunityEditor::SaveOptionsToConfig()
{
	String identifier = TXT("/Frames/CommunityEditor");
	SaveLayout(identifier);
	
	// save current page
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.Write(identifier + TXT("/CurrentPage"), m_notebook->GetSelection());

	if (m_communityTableGrid)
		m_communityTableGrid->SaveLayout(identifier + TXT("/CommunityTable/"));
	if (m_storyPhaseTimetableGrid)
		m_storyPhaseTimetableGrid->SaveLayout(identifier + TXT("/StoryPhaseTimetable/"));
	if (m_sceneTableGrid)
		m_sceneTableGrid->SaveLayout(identifier + TXT("/SceneTable/"));
	if (m_layoutsGrid)
		m_layoutsGrid->SaveLayout(identifier + TXT("/Layouts/"));
}

void CEdCommunityEditor::LoadOptionsFromConfig()
{
	String identifier = TXT("/Frames/CommunityEditor");
	LoadLayout(identifier);

	// Load current page
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	Int32 currentPage = config.Read(identifier + TXT("/CurrentPage"), 0);
	if (currentPage >= 0 && currentPage < (Int32)m_notebook->GetPageCount())
		m_notebook->SetSelection(currentPage);

	if (m_communityTableGrid)
		m_communityTableGrid->LoadLayout(identifier + TXT("/CommunityTable/"));
	if (m_storyPhaseTimetableGrid)
		m_storyPhaseTimetableGrid->LoadLayout(identifier + TXT("/StoryPhaseTimetable/"));
	if (m_sceneTableGrid)
		m_sceneTableGrid->LoadLayout(identifier + TXT("/SceneTable/"));
	if (m_layoutsGrid)
		m_layoutsGrid->LoadLayout(identifier + TXT("/Layouts/"));
}

void CEdCommunityEditor::SaveModifiedResources()
{
	// save edited story phases resource
	C2dArray* arr = resGridStorySubPhases.LoadAndGet< C2dArray >();
	if ( arr && arr->GetFile() )
	{
		if ( arr->GetFile()->IsModified() )
		{
			arr->Save();
		}
	}
	else
	{
		ERR_EDITOR( TXT( "Can't save globals/storySubPhases.csv file - invalid resource" ) );
	}
}

void CEdCommunityEditor::OnSave( wxCommandEvent& event )
{
	CheckAndUpdateGUIDs();
    CacheMapPinPositions();
	m_community->Save();
	SaveModifiedResources();
}

void CEdCommunityEditor::OnExit( wxCommandEvent& event )
{
	SaveModifiedResources();
	ClosePanel();
}

void CEdCommunityEditor::OnValidate( wxCommandEvent& event )
{
	String errMsg;
	if ( !m_communityValidator.Validate( m_community, errMsg ) )
	{
		wxMessageBox( errMsg.AsChar(), TXT("Error in community data") );
	}
	else
	{
		wxMessageBox( TXT("Validation successful."), TXT("Community data valid") );
	}
}

void CEdCommunityEditor::OnGridValueChanged( wxCommandEvent& event )
{
	m_community->MarkModified();
}

CGridEditor *CEdCommunityEditor::CreateGridFromProperty(wxWindow *parent, CClass *classPtr, String propertyName)
{
	if (CProperty *propertyPtr = classPtr->FindProperty(CName(propertyName.AsChar())))
	{
		CGridEditor *gridEditor = new CGridEditor(parent);
		gridEditor->RegisterCustomType( new CGridTagListDesc );
		gridEditor->RegisterCustomType( new CGridGameTimeDesc );
		gridEditor->RegisterCustomType( new CGridSpawnTypeDesc );
		gridEditor->RegisterCustomType( new CGridLayerNameDesc );
        gridEditor->RegisterCustomType( new CGridVectorTypeDesc );

		if ( propertyName == TXT("communityTable") )
		{
			CGridStoryPhaseTypeDesc* storyPhasesEd = new CGridStoryPhaseTypeDesc;
			storyPhasesEd->AddSource( 12, resGridStorySubPhases );
			gridEditor->RegisterCustomType( storyPhasesEd );
			gridEditor->RegisterCustomColumnDesc( TXT( "Appearances" ), new CGridAppearanceColumnDesc( 4 ) );
		}
		else if ( propertyName == TXT("sceneTable") )
		{
			CGridStoryPhaseTypeDesc* storyPhasesEd = new CGridStoryPhaseTypeDesc;
			storyPhasesEd->AddSource( 1, resGridStorySubPhases );
			gridEditor->RegisterCustomType( storyPhasesEd );
		}
		else if ( propertyName == TXT("storyPhaseTimetable") )
		{
			gridEditor->RegisterCustomType( new CArrayEditorTypeDescBase(7, &SActionPointResourcesManager::GetInstance().Get2dArray(), true) );
		}

		SetGridObject(gridEditor, propertyPtr);
		return gridEditor;
	}
	else
	{
		WARN_EDITOR( TXT( "Cannot find property %s in the class %s." ), propertyName.AsChar(), classPtr->GetName().AsString().AsChar() );
	}

	return NULL;
}

void CEdCommunityEditor::SetGridObject(CGridEditor *grid, CProperty *prop)
{
	if (grid)
	{
		void *data = prop->GetOffsetPtr(m_community);
		grid->SetObject(data, prop);
	}
}

void CEdCommunityEditor::OnTypeChanged( wxCommandEvent& event )
{
	m_community->SetSpawnsetType( static_cast< ECommunitySpawnsetType >( m_typeChoice->GetSelection() ) );
	m_community->MarkModified();
}
