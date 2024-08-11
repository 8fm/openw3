#include "build.h"
#include "dialogGraphEditor.h"

#include "dialogEditorActions.h"
#include "dialogEditorPage.h"
#include "dialogEditorSection.h"
#include "undoDialogEditor.h"
#include "cutsceneEditor.h"
#include "editorExternalResources.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneGraph.h"
#include "../../common/game/storySceneFlowCondition.h"
#include "../../common/game/storySceneFlowConditionBlock.h"
#include "../../common/game/storySceneFlowSwitch.h"
#include "../../common/game/storySceneFlowSwitchBlock.h"
#include "../../common/game/storySceneRandomBlock.h"
#include "../../common/game/storySceneInputBlock.h"
#include "../../common/game/storySceneOutputBlock.h"
#include "../../common/game/storySceneSectionBlock.h"
#include "../../common/game/storySceneScriptingBlock.h"
#include "../../common/game/storySceneCutsceneBlock.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneGraphSocket.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneVideo.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/graphConnectionRebuilder.h"

namespace // anonymous
{
	class CSceneObjectWrapper : public wxObject
	{
	private:
		const CClass*	m_blockClass;
		String			m_templateName;
		wxPoint			m_position;

	public:
		CSceneObjectWrapper( const CClass *blockClass,const wxString& templateName, wxPoint position )
			: m_blockClass( blockClass )
			, m_templateName( templateName )
			, m_position( position )
		{ }

		RED_INLINE const CClass *GetBlockClass() const { return m_blockClass; }
		RED_INLINE const String& GetTemplateName() const { return m_templateName; }
		RED_INLINE wxPoint GetPosition() const { return m_position; }
	};
}

BEGIN_EVENT_TABLE( CEdSceneGraphEditor, CEdGraphEditor )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHECKCONSISTENCY, CEdSceneGraphEditor::OnCheckConsistency )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTEHERE, CEdSceneGraphEditor::OnPasteHere )
	EVT_MENU( wxID_STORYSCENEEDITOR_DELETESECTION, CEdSceneGraphEditor::OnDeleteBlock )
	EVT_MENU( wxID_STORYSCENEEDITOR_OPENCUTSCENEPREVIEW, CEdSceneGraphEditor::OnOpenCutscenePreview )
	EVT_LEFT_DCLICK( CEdSceneGraphEditor::OnDoubleClick )
	EVT_LEFT_UP( CEdSceneGraphEditor::OnLeftClick )
	EVT_KEY_DOWN( CEdSceneGraphEditor::OnKeyDown )
	EVT_KEY_UP( CEdSceneGraphEditor::OnKeyUp )
	EVT_KILL_FOCUS( CEdSceneGraphEditor::OnKillFocus )
	EVT_MENU( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_NEW, CEdSceneGraphEditor::OnCreateDialogsetNew )
	EVT_MENU( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMFILE, CEdSceneGraphEditor::OnCreateDialogsetFromFile )
	EVT_MENU( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMPREVSECTION, CEdSceneGraphEditor::OnCreateDialogsetFromPreviousSection )
	EVT_MENU( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMSELSECTION, CEdSceneGraphEditor::OnCreateDialogsetFromSelectedSection )
	EVT_MENU_RANGE( wxID_STORYSCENEEDITOR_CHANGEDIALOGSET, wxID_STORYSCENEEDITOR_CHANGEDIALOGSET_LIMIT, CEdSceneGraphEditor::OnChangeDialogset )
	EVT_MENU( wxID_STORYSCENEEDITOR_VERIFYSECTION, CEdSceneGraphEditor::OnVerifySection )
	EVT_MENU( wxID_STORYSCENEEDITOR_CLONESECTION, CEdSceneGraphEditor::OnEditCopy )
END_EVENT_TABLE()

CEdSceneGraphEditor::CEdSceneGraphEditor( wxWindow* parent, CEdSceneEditor* sceneEditor )
	: CEdGraphEditor( parent, false )
	, m_sceneEditor( sceneEditor )
	, m_mediator( sceneEditor )
	, m_pressedKeyCode( -1 )
{
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionRemoved ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionAdded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionNameChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneChoiceLineChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ScenePartBlockSocketsChanged ), this );

	m_shouldRepaint = true;
	m_shouldZoom = true;

	FillBlockAndTemplateInfo();
	LoadKeyboardShortcuts();
}

CEdSceneGraphEditor::~CEdSceneGraphEditor(void)
{
	SEvents::GetInstance().UnregisterListener( this );
}

CEdSceneEditorScreenplayPanel* CEdSceneGraphEditor::HACK_GetScreenplayPanel() const
{
	return GetSceneEditor()->HACK_GetScreenplayPanel();
}

CEdSceneEditor* CEdSceneGraphEditor::GetSceneEditor() const
{
	return m_sceneEditor;
}

CStoryScene* CEdSceneGraphEditor::HACK_GetStoryScene() const
{
	return GetSceneEditor()->HACK_GetStoryScene();
}

void CEdSceneGraphEditor::UpdateBlockLayout( CGraphBlock* block )
{
	if ( m_layout.FindPtr( block ) == NULL && block->GetPosition().Mag2() == 0.0f )
	{
		SetBlockPositionAtMouseCursor( block );
	}

	__super::UpdateBlockLayout( block );
}

void CEdSceneGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
	menu.Append( wxID_STORYSCENEEDITOR_DELETESECTION, wxT( "Delete block" ) );

	CEdGraphEditor::InitLinkedBlockContextMenu( block, menu );

	// custom context menu items
	if ( block->IsA< CStorySceneCutsceneSectionBlock >() )
	{
		menu.Append( wxID_STORYSCENEEDITOR_OPENCUTSCENEPREVIEW, wxT( "Open Cutscene Preview" ) );
	}
	else if ( block->IsA< CStorySceneSectionBlock >() )
	{
		CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( block );

		menu.AppendSeparator();
		menu.Append( wxID_STORYSCENEEDITOR_VERIFYSECTION, wxT( "Verify section" ) )->Enable( false );

		wxMenu* dialogsetMenu = new wxMenu;
		TDynArray< CName > dialogsetNames;
		HACK_GetStoryScene()->GetDialogsetInstancesNames( dialogsetNames );
		for ( Uint32 i = 0; i < dialogsetNames.Size(); ++i )
		{
			dialogsetMenu->AppendRadioItem( wxID_STORYSCENEEDITOR_CHANGEDIALOGSET + i, dialogsetNames[ i ].AsString().AsChar() )->Check( sectionBlock->GetSection()->GetDialogsetChange() == dialogsetNames[ i ] );
		}
		dialogsetMenu->AppendSeparator();
		dialogsetMenu->Append( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_NEW, wxT( "Create new" ) );
		dialogsetMenu->Append( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMFILE, wxT( "Create from file" ) );
		dialogsetMenu->Append( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMPREVSECTION, wxT( "Create from previous section" ) )->Enable( false );
		dialogsetMenu->Append( wxID_STORYSCENEEDITOR_CREATEDIALOGSET_FROMSELSECTION, wxT( "Create from this section" ) );

		menu.AppendSubMenu( dialogsetMenu, wxT( "Change dialogset") );

		wxMenu* cloneMenu = new wxMenu;
		cloneMenu->Append( wxID_STORYSCENEEDITOR_CLONESECTION, wxT(  "(HACK)I know what i am doing and i understand the implications of cloning IDs" ) );
		menu.AppendSubMenu( cloneMenu, wxT( "Clone (Preserve string ids)" ) );
	}
}

void CEdSceneGraphEditor::InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu )
{
	CEdGraphEditor::InitLinkedSocketContextMenu( block, menu );
}

void CEdSceneGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	m_mousePosition = wxGetMousePosition();

	for ( Uint32 idx = 0; idx < m_sceneMenuInfo.Size(); idx++ )
	{
		const CSceneMenuInfo& info = m_sceneMenuInfo[ idx ];
		if ( info.IsBlock() )
		{
			menu.Append( wxID_STORYSCENEEDITOR_ADDSCENEBLOCK + idx, info.GetMenuText() );
			menu.Connect( wxID_STORYSCENEEDITOR_ADDSCENEBLOCK + idx, 
				wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler( CEdSceneGraphEditor::OnAddSceneBlock ), 
				new CSceneObjectWrapper( info.GetBlockClass(), wxT(""), m_mousePosition ), 
				this );
		}
	}

	menu.AppendSeparator();

	wxMenu* subMenu = new wxMenu();

	for ( Uint32 idx = 0; idx < m_sceneMenuInfo.Size(); idx++ )
	{
		const CSceneMenuInfo& info = m_sceneMenuInfo[ idx ];
		if ( info.IsTemplate() )
		{
			subMenu->Append( wxID_STORYSCENEEDITOR_ADDSCENETEMPLATE + idx, info.GetMenuText() );
			menu.Connect( wxID_STORYSCENEEDITOR_ADDSCENETEMPLATE + idx, 
				wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler( CEdSceneGraphEditor::OnAddSceneTemplate ), 
				new CSceneObjectWrapper( nullptr, info.GetTemplateName(), m_mousePosition ), 
				this );
		}
	}
	menu.AppendSubMenu( subMenu, wxT("Basic scene blocks") );

	Bool canPaste = false;
	if ( wxTheClipboard->Open() )
	{
		CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
		canPaste = wxTheClipboard->IsSupported( data.GetDataFormat() );
		wxTheClipboard->Close();
	}
	menu.AppendSeparator();
	menu.Append( wxID_STORYSCENEEDITOR_PASTEHERE, wxT( "Paste here" ) );
	menu.Enable( wxID_STORYSCENEEDITOR_PASTEHERE, canPaste );

	menu.AppendSeparator();
	menu.Append( wxID_STORYSCENEEDITOR_CHECKCONSISTENCY, wxT( "Check consistency" ) );
}

void CEdSceneGraphEditor::MouseClick( wxMouseEvent& event )
{
	THashMap< Char, TPair< CClass*, String > >::const_iterator shortcut = m_shortcuts.Find( m_pressedKeyCode );
	if ( shortcut != m_shortcuts.End() && event.LeftDown() )
	{
		CEdCanvas::MouseClick( event );

		const CClass* blockClass = shortcut->m_second.m_first;
		const String& templateName = shortcut->m_second.m_second;
		if ( blockClass )
		{
			AddSceneBlock( blockClass, &wxGetMousePosition() );
			SetFocus(); // needed because of CEdSceneSectionPanel::SetFocus() which caused spamming section name property with pressed key
		}
		else if ( !templateName.Empty() )
		{
			AddSceneTemplate( templateName, &wxGetMousePosition() );
			SetFocus(); // needed because of CEdSceneSectionPanel::SetFocus() which caused spamming section name property with pressed key
		}
	}
	else
	{
		CEdGraphEditor::MouseClick( event );
	}
}

void CEdSceneGraphEditor::OnAddSceneBlock( wxCommandEvent& event )
{
	CSceneObjectWrapper *sceneObjectWrapper = dynamic_cast< CSceneObjectWrapper * >( event.m_callbackUserData );
	if ( !sceneObjectWrapper )
	{
		return;
	}
	const CClass* blockClass = sceneObjectWrapper->GetBlockClass();
	if ( !blockClass )
	{
		return;
	}
	const wxPoint& mousePosition = sceneObjectWrapper->GetPosition();

	AddSceneBlock( blockClass, &mousePosition );
}

void CEdSceneGraphEditor::AddSceneBlock( const CClass* blockClass, const wxPoint* mousePosition /*= nullptr*/ )
{
	const CClass* objectClass = nullptr;

	// look for object class by block class
	for ( Uint32 i = 0; i < m_sceneMenuInfo.Size(); i++ )
	{
		const CSceneMenuInfo& info = m_sceneMenuInfo[ i ];
		if ( info.IsBlock() )
		{
			if ( blockClass == info.GetBlockClass() )
			{
				// get object class based on block class
				objectClass = info.GetObjectClass();
				break;
			}
		}
	}
	if ( !objectClass )
	{
		// return if no object class was found
		return;
	}

	CStoryScene* scene = HACK_GetStoryScene();
	if ( scene == NULL )
	{
		return;
	}

	CStorySceneGraphBlock* createdBlock = scene->CreateAndAddSceneBlock( const_cast< CClass* >( blockClass ), const_cast< CClass* >( objectClass ) );

	SetBlockPositionAtMouseCursor( createdBlock, mousePosition );
	CreateAddBlockUndoStep( createdBlock );

	// block specific initialization
	if ( blockClass == ClassID< CStorySceneInputBlock >() )
	{
		ASSERT( objectClass == ClassID< CStorySceneInput >() );
		HACK_GetScreenplayPanel()->GetSceneEditor()->ReloadSceneInputsTable();
	}

	m_mediator->OnGraph_AddSceneBlock( createdBlock );

	GraphStructureModified();
}

void CEdSceneGraphEditor::OnAddSceneTemplate( wxCommandEvent& event )
{
	CSceneObjectWrapper *sceneObjectWrapper = dynamic_cast< CSceneObjectWrapper * >( event.m_callbackUserData );
	if ( !sceneObjectWrapper )
	{
		return;
	}
	const String& templateName = sceneObjectWrapper->GetTemplateName();
	if ( templateName.Empty() )
	{
		return;
	}
	const wxPoint& mousePosition = sceneObjectWrapper->GetPosition();

	AddSceneTemplate( templateName, &mousePosition );
}

void CEdSceneGraphEditor::AddSceneTemplate( const String& templateName, const wxPoint* mousePosition /*= nullptr*/ )
{
	ESceneTemplateType templateType = STT_None;

	// look for event id by template name
	for ( Uint32 i = 0; i < m_sceneMenuInfo.Size(); i++ )
	{
		const CSceneMenuInfo& info = m_sceneMenuInfo[ i ];
		if ( info.IsTemplate() )
		{
			if ( templateName == info.GetTemplateName() )
			{
				// get event id based on template name
				templateType = info.GetTemplateType();
				break;
			}
		}
	}
	if ( templateType == STT_None )
	{
		// return if no event id was found
		return;
	}

	AddSceneTemplate( templateType, mousePosition );
}

void CEdSceneGraphEditor::AddSceneTemplate( ESceneTemplateType templateType, const wxPoint* mousePosition /*= nullptr*/ )
{
	Bool gameplay  =    ( templateType == STT_Gameplay );
	Bool fullTemplate = ( templateType == STT_FullVoiceSet );

	wxPoint blockPos;
	if ( mousePosition )
	{
		blockPos = ClientToCanvas( this->ScreenToClient( *mousePosition ) );
	}
	else
	{
		blockPos = ClientToCanvas( this->ScreenToClient( m_mousePosition ) );
	}

	if ( fullTemplate )
	{
		const Char* strings[] = 
		{
			TXT("rain"), TXT("gossip"),	TXT("gossip_reply"), TXT("question"), TXT("answer"), TXT("greeting"), TXT("greeting_reply"), TXT("greeting_geralt"), TXT("reaction_to_geralt"),
			TXT("bump"), TXT("sound_reaction"),	TXT("dead_body"), TXT("afraid"), TXT("fear"), TXT("prayers"), TXT("barter_question"), TXT("barter_answer"), TXT("warning"), TXT("combat_comment"),
			TXT("battlecry_geralt"), TXT("battlecry"), TXT("battlecry_group_order"), TXT("battlecry_group_order_answer"), TXT("battlecry_group_signs"), TXT("battlecry_group_death") 
		};
		Int32 itemsCount = sizeof(strings)/sizeof(Char*);
		for( Int32 i = 0; i < itemsCount; i++ )
		{
			AddSceneTemplate( true, blockPos + wxPoint( 0, i*100 ), strings[i] );
		}
	}
	else
	{
		AddSceneTemplate( gameplay, blockPos );
	}

	CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );
	HACK_GetScreenplayPanel()->GetSceneEditor()->ReloadSceneInputsTable();
	GraphStructureModified();	

	m_mediator->OnGraph_AddSceneBlock( NULL );
}

void CEdSceneGraphEditor::AddSceneTemplate( Bool gameplay, wxPoint blockPos, const Char* title )
{
	CStoryScene* scene = HACK_GetStoryScene();
	if ( scene == NULL )
	{
		return;
	}

	CStorySceneGraphBlock* inputBlock = scene->CreateAndAddSceneBlock( ClassID< CStorySceneInputBlock >(), ClassID< CStorySceneInput >() );
	inputBlock->SetPosition( Vector( blockPos.x -120 , blockPos.y, 0.f ) );
	CreateAddBlockUndoStep(inputBlock);

	CStorySceneGraphBlock* sectionBlock = scene->CreateAndAddSceneBlock( ClassID< CStorySceneSectionBlock >(), ClassID< CStorySceneSection >() );		
	sectionBlock->SetPosition( Vector( blockPos.x, blockPos.y, 0.f ) );
	CreateAddBlockUndoStep(sectionBlock);

	CStorySceneSection* section = static_cast<CStorySceneSection*>( sectionBlock->GetControlPart() );
	ConnectSockets( inputBlock->FindSocket( CNAME( Out ) ), sectionBlock->FindSocket( CNAME( In ) ) );
	if( title )
	{
		SafeCast<CStorySceneInput>( inputBlock->GetControlPart() )->SetName( title );
		SafeCast<CStorySceneSection>( sectionBlock->GetControlPart() )->SetName( title );
	}
	if ( gameplay )
	{
		section->SetIsGameplay( true );
	}
	else
	{
		CStorySceneGraphBlock* outputBlock = scene->CreateAndAddSceneBlock( ClassID< CStorySceneOutputBlock >(), ClassID< CStorySceneOutput >() );			
		outputBlock->SetPosition( Vector( blockPos.x + 120, blockPos.y, 0.f ) );
		CreateAddBlockUndoStep(outputBlock);
		ConnectSockets( sectionBlock->FindSocket( CNAME( Out ) ), outputBlock->FindSocket( CNAME( In ) ) );
	}
}

void CEdSceneGraphEditor::OnCheckConsistency( Bool doNotShowInfoIfItsOK, Bool doNotShowInfoIfItsNotOK )
{
	CStoryScene* scene = HACK_GetStoryScene();
	if ( scene == NULL )
	{
		return;
	}
	CStorySceneGraph* graph = scene->GetGraph();
	if ( graph == NULL )
	{
	    return;
	}
	graph->CheckConsistency( NULL, doNotShowInfoIfItsOK, doNotShowInfoIfItsNotOK );
}

void CEdSceneGraphEditor::OnCheckConsistency( wxCommandEvent& event )
{
	OnCheckConsistency();
}

void CEdSceneGraphEditor::OnPasteHere( wxCommandEvent& event )
{
	wxPoint point = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	Vector pos( point.x, point.y, 0 );

	OnPaste( &pos );
}

void CEdSceneGraphEditor::OnAddSection( wxCommandEvent& event )
{
	AddSceneBlock( ClassID< CStorySceneSectionBlock >() );
}

void CEdSceneGraphEditor::OnAddCutscene( wxCommandEvent& event )
{
	AddSceneBlock( ClassID< CStorySceneCutsceneSectionBlock >() );
}

void CEdSceneGraphEditor::OnKeyDown( wxKeyEvent& event )
{
	Int32 keyCode = event.GetKeyCode();
	if ( keyCode == WXK_DELETE )
	{
		wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
		OnDeleteBlock( commandEvent );
	}	

	if ( iswascii( keyCode ) )
	{
		if ( m_action == MA_None )
		{
			// register pressed key only if there's no ongoing action
			m_pressedKeyCode = keyCode;
		}
	}

}

void CEdSceneGraphEditor::OnKeyUp( wxKeyEvent& event )
{
	Int32 keyCode = event.GetKeyCode();
	if ( iswascii( keyCode ) )
	{
		m_pressedKeyCode = -1;
	}
}

void CEdSceneGraphEditor::OnKillFocus( wxFocusEvent& event )
{
	m_pressedKeyCode = -1;
}

void CEdSceneGraphEditor::OnDeleteBlock( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	if ( blocks.Empty() == true )
	{
		return;
	}

	Int32 inputsToRemove = 0;
	Int32 outputsToRemove = 0;
	Uint32 numSectionsToRemove = 0;
	for ( Uint32 i=0; i<blocks.Size(); ++i )
	{
		if( blocks[ i ] && blocks[ i ]->IsA< CStorySceneSectionBlock >() )
		{
			++numSectionsToRemove;
		}
		if ( blocks[ i ] && blocks[ i ]->IsA< CStorySceneInputBlock >() )
		{
			inputsToRemove++;
		}
		else if ( blocks[ i ] && blocks[ i ]->IsA< CStorySceneOutputBlock >() )
		{
			outputsToRemove++;
		}
	}

	if( numSectionsToRemove > 0 )
	{
		const Uint32 numSections = m_mediator->OnGraph_GetNumBlocks( ClassID< CStorySceneSectionBlock >() );
		if( numSections - numSectionsToRemove < 1 )
		{
			GFeedback->ShowMsg( TXT("Message"), TXT("Can't delete section(s) - scene has to have at least one section.") );
			return;
		}
	}

	if ( inputsToRemove > 0 )
	{
		const Int32 numInputs = m_mediator->OnGraph_GetNumBlocks( ClassID< CStorySceneInputBlock >() );
		if ( numInputs - inputsToRemove < 1 )
		{
			GFeedback->ShowMsg( TXT("Message"), TXT("Story scene must have at least one input.\n\nYou want to remove [%d] input(s), story scene has only [%d] input(s)."), inputsToRemove, numInputs );
			return;
		}
	}

	if ( outputsToRemove > 0 )
	{
		const Int32 numOutputs = m_mediator->OnGraph_GetNumBlocks( ClassID< CStorySceneOutputBlock >() );
		if ( numOutputs - outputsToRemove < 1 )
		{
			GFeedback->ShowMsg( TXT("Message"), TXT("Story scene must have at least one output.\n\nYou want to remove [%d] output(s), story scene has only [%d] output(s)."), outputsToRemove, numOutputs );
			return;
		}
	}

	Int32 blockDeletionConfirmationResponse = false;

	if ( blocks.Size() == 1 )
	{
		blockDeletionConfirmationResponse = wxMessageBox( 
			wxT( "Are you sure you want to delete this story scene block?" ), 
			wxT( "Confirm operation" ), wxYES_NO | wxCENTRE );
	}
	else
	{
		blockDeletionConfirmationResponse = wxMessageBox( 
			wxT( "Are you sure you want to delete these story scene blocks?" ), 
			wxT( "Confirm operation" ), wxYES_NO | wxCENTRE );
	}

	if ( blockDeletionConfirmationResponse != wxYES )
	{
		return;
	}

	CEdSceneEditorScreenplayPanel* sceneEditorPage = HACK_GetScreenplayPanel();

	sceneEditorPage->GetSceneEditor()->ResetPropertiesBrowserObject();

	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		// Preparing the step does the actual deletion (to correctly handle some call order issues)
		CUndoDialogGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, blocks[i], CUndoDialogGraphBlockExistance::EDITOR_DELETE_BLOCK );
	}

	CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );	

	m_mediator->OnGraph_RemoveSceneBlocks( ( TDynArray< CStorySceneGraphBlock* >& )( blocks ) );

	GraphStructureModified();
}

void CEdSceneGraphEditor::DeleteBlock( CGraphBlock* block )
{
	block->BreakAllLinks();

	if ( block->IsA< CStorySceneSectionBlock >() == true )
	{
		CStorySceneSectionBlock* sectionBlockToDelete = Cast< CStorySceneSectionBlock >( block );

		CStorySceneSection* sectionToDelete = sectionBlockToDelete->GetSection();
	}
	else if ( block->IsA< CStorySceneInputBlock >() == true )
	{
		CStorySceneInputBlock* inputBlockToDelete = Cast< CStorySceneInputBlock >( block );

		CStorySceneInput* inputToDelete = inputBlockToDelete->GetInput();
		HACK_GetScreenplayPanel()->GetSceneEditor()->ReloadSceneInputsTable();
	}

	m_graph->GraphRemoveBlock( block );

	GraphStructureModified();
}

void CEdSceneGraphEditor::OnDoubleClick( wxMouseEvent& event )
{
	if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CStorySceneGraphBlock >() )
	{
		CStorySceneGraphBlock* sceneBlock = static_cast< CStorySceneGraphBlock* >( m_activeItem.Get() );

		CStorySceneControlPart* cp = sceneBlock->GetControlPart();
		if ( cp && cp->SupportsInputSelection() )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( sceneBlock );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{			
				wxRect rect = GetInputSelectionRect( sceneBlock );				
				rect = CanvasToClient( rect );
				if ( rect.Contains( event.GetPosition() ) )
				{
					cp->ToggleSelectedInputLinkElement();
					m_mediator->OnGraph_ToggleSelectedInputLinkElement();
					return;
				}
			}
		}

		if ( cp && cp->SupportsOutputSelection() )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( sceneBlock );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{			
				wxRect rect = GetOutputSelectionRect( sceneBlock );
				rect = CanvasToClient( rect );
				if ( rect.Contains( event.GetPosition() ) )
				{
					cp->ToggleSelectedOutputLinkElement();
					m_mediator->OnGraph_ToggleSelectedOutputLinkElement();
					return;
				}
			}
		}

		if ( sceneBlock->IsA< CStorySceneSectionBlock >() )
		{
			CStorySceneSectionBlock* sectionBlock = static_cast< CStorySceneSectionBlock* >( sceneBlock );

			CEdSceneEditorScreenplayPanel* sceneEditorPage = HACK_GetScreenplayPanel();
			CEdSceneSectionPanel* sectionPanel = sceneEditorPage->FindSectionPanel( sectionBlock->GetSection() );

			if ( sectionPanel != NULL )
			{
				sectionPanel->SetFocus();
				sceneEditorPage->HandleSectionSelection( sectionPanel );
			}
		}
	}
}

void CEdSceneGraphEditor::OnOpenCutscenePreview( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	if ( blocks.Empty() == true )
	{
		return;
	}

	CStorySceneCutsceneSectionBlock *cutsceneBlock = SafeCast< CStorySceneCutsceneSectionBlock >( blocks[0] );
	OpenCutscenePreview( cutsceneBlock );
}

void CEdSceneGraphEditor::OnLeftClick( wxMouseEvent& event )
{
	CEdSceneEditorScreenplayPanel* sceneEditorPage = HACK_GetScreenplayPanel();

	if ( m_activeItem.Get() == NULL )
	{
		GetSceneEditor()->SetPropertiesBrowserObject( sceneEditorPage->HACK_GetStoryScene(), nullptr );
		event.Skip();
		return;
	}

	const CStorySceneCutsceneSectionBlock* cutScene = Cast<CStorySceneCutsceneSectionBlock>( m_activeItem.Get() );
	bool shouldCameraManipulationBeEnabled = cutScene==NULL;
	CEdSceneEditor *sceneEditor= GetSceneEditor();
	ASSERT( sceneEditor );
	wxToolBar* previewControlToolbar = XRCCTRL( *sceneEditor, "PreviewControlToolbar", wxToolBar );
	ASSERT( previewControlToolbar );
	previewControlToolbar->EnableTool( XRCID( "DefaultEventsButton" ), shouldCameraManipulationBeEnabled );
	previewControlToolbar->EnableTool( XRCID( "DefaultLookAtsButton" ), shouldCameraManipulationBeEnabled );
	previewControlToolbar->EnableTool( XRCID( "ClearEventsButton" ), shouldCameraManipulationBeEnabled );

	if ( m_activeItem.Get()->IsA< CStorySceneGraphBlock >() )
	{
		CStorySceneGraphBlock* sceneGraphBlock = Cast< CStorySceneGraphBlock >( m_activeItem.Get() );
		sceneEditor->SetPropertiesBrowserObject( sceneGraphBlock->GetControlPart(), nullptr );
	}
	else if ( m_activeItem.Get()->IsA< CStorySceneGraphSocket >() )
	{
		CStorySceneGraphSocket *storySceneGraphSocket = Cast<CStorySceneGraphSocket>( m_activeItem.Get() );
		sceneEditor->SetPropertiesBrowserObject( storySceneGraphSocket->GetLinkElement(), nullptr );
	}

	event.Skip();
}


void CEdSceneGraphEditor::CreateAddBlockUndoStep( CGraphBlock* createdBlock )
{
	CUndoDialogGraphBlockExistance::PrepareCreationStep( *m_undoManager, this, createdBlock );
	CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );
}

void CEdSceneGraphEditor::SetBlockPositionAtMouseCursor( CGraphBlock* createdBlock, const wxPoint* mousePosition /*= nullptr*/ )
{
	wxPoint mousePos;
	if ( mousePosition )
	{
		mousePos = ClientToCanvas( this->ScreenToClient( *mousePosition ) );
	}
	else
	{
		mousePos = ClientToCanvas( this->ScreenToClient( m_mousePosition ) );
	}
	createdBlock->SetPosition( Vector( mousePos.x, mousePos.y, 0.f ) );
}

void CEdSceneGraphEditor::ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket )
{
	CEdGraphEditor::ConnectSockets( srcSocket, destSocket );

	OnCheckConsistency( true, false );
}

Bool CEdSceneGraphEditor::IsBlockActivated( CGraphBlock* block ) const
{
	if ( const CStorySceneGraphBlock* sceneBlock = Cast< const CStorySceneGraphBlock >( block ) )
	{
		if ( const CStorySceneControlPart* cp = sceneBlock->GetControlPart() )
		{
			return m_mediator->OnGraph_IsBlockActivated( cp );
		}
	}

	return __super::IsBlockActivated( block );
}

Float CEdSceneGraphEditor::GetBlockActivationAlpha( CGraphBlock* block ) const
{
	if ( const CStorySceneGraphBlock* sceneBlock = Cast< const CStorySceneGraphBlock >( block ) )
	{
		if ( const CStorySceneControlPart* cp = sceneBlock->GetControlPart() )
		{
			return m_mediator->OnGraph_GetBlockActivationAlpha( cp );
		}
	}

	return __super::GetBlockActivationAlpha( block );
}

wxRect CEdSceneGraphEditor::GetOutputSelectionRect( const CGraphBlock* block ) const
{
	wxRect rect;			
	if( Int32 size = block->GetSize().X > 0 ) //rectangular block 
	{
		rect.x		= block->GetPosition().X + block->GetSize().X - 14;
		rect.y		= block->GetPosition().Y + 6;
	}
	else // triangular block
	{
		rect.x		= block->GetPosition().X + 4;
		rect.y		= block->GetPosition().Y + 26;
	}

	rect.width	= 10;
	rect.height	= 10;
	return rect;
}

wxRect CEdSceneGraphEditor::GetInputSelectionRect( const CGraphBlock* block ) const
{
	wxRect rect;
	rect.x		= block->GetPosition().X + 4;
	rect.y		= block->GetPosition().Y + 6;
	rect.width	= 10;
	rect.height	= 10;
	return rect;
}

void CEdSceneGraphEditor::DrawBlockLayout( CGraphBlock* block )
{
	CEdGraphEditor::DrawBlockLayout( block );

	wxPoint location( block->GetPosition().X, block->GetPosition().Y );

	CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( block );
	const CStorySceneControlPart* cp = sceneBlock ? sceneBlock->GetControlPart() : nullptr;

	if ( sceneBlock && sceneBlock->GetComment().Empty() == false )
	{
		wxColour borderColor( 64,64,128 );
		wxColour commentColor( 128, 128, 255 );
		wxPoint textSize = TextExtents( *m_boldFont, sceneBlock->GetComment() );
		Uint32 textX = ( sceneBlock->GetBlockShape() == GBS_LargeCircle || sceneBlock->GetBlockShape() == GBS_DoubleCircle ) ? ( location.x + 24 - ( textSize.x / 2 ) ) : ( location.x + 5 );
		DrawText( wxPoint( textX, location.y - textSize.y - 2 ), *m_boldFont, sceneBlock->GetComment(), borderColor );
		DrawText( wxPoint( textX - 1, location.y - textSize.y - 1 ), *m_boldFont, sceneBlock->GetComment(), commentColor );
	}
	else if ( cp && cp->GetLinkedElements().Size() > 1 && !cp->GetClass()->IsA< CStorySceneLinkHub >() )
	{
		const String text = String::Printf( TXT("Number of inputs: %d"), sceneBlock->GetControlPart()->GetLinkedElements().Size() );
		wxColour borderColor( 64,64,128 );
		wxColour commentColor( 255, 128, 128 );
		wxPoint textSize = TextExtents( *m_boldFont, text );
		Uint32 textX = ( sceneBlock->GetBlockShape() == GBS_LargeCircle || sceneBlock->GetBlockShape() == GBS_DoubleCircle ) ? ( location.x + 24 - ( textSize.x / 2 ) ) : ( location.x + 5 );
		DrawText( wxPoint( textX, location.y - textSize.y - 2 ), *m_boldFont, text, borderColor );
		DrawText( wxPoint( textX - 1, location.y - textSize.y - 1 ), *m_boldFont, text, commentColor );
	}

	if ( cp && cp->SupportsInputSelection() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{			
			wxRect rect = GetInputSelectionRect( sceneBlock );				

			FillRect( rect, wxColour( 128, 128, 168, 255 ) );
			DrawRect( rect, wxColour( 0, 0, 0, 255 ) );
		}
	}

	if ( cp && cp->SupportsOutputSelection() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{			
			wxRect rect = GetOutputSelectionRect( block );	

			FillRect( rect, wxColour( 168, 128, 128, 255 ) );
			DrawRect( rect, wxColour( 0, 0, 0, 255 ) );
		}
	}

	if ( block->IsActivated() && cp->IsA< CStorySceneSection >() )
	{
		const CStorySceneSection* section = static_cast< const CStorySceneSection* >( cp );

		const CName dialogsetName = m_mediator->OnGraph_GetCurrentDialogsetName();

		const BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			String text = String::Printf( TXT("Dialogset: %ls"), dialogsetName.AsString().AsChar() );
			wxColour borderColor( 64,64,128 );
			wxColour commentColor( 128, 128, 128 );

			if ( !dialogsetName )
			{
				commentColor = wxColour( 255, 50, 50 );
				text = TXT("Error - ") + text;
			}

			wxPoint textSize = TextExtents( *m_boldFont, text );
			Uint32 textX = ( sceneBlock->GetBlockShape() == GBS_LargeCircle || sceneBlock->GetBlockShape() == GBS_DoubleCircle ) ? ( location.x + 24 - ( textSize.x / 2 ) ) : ( location.x + 5 );
			DrawText( wxPoint( textX, location.y + 2 + layout->m_windowSize.GetHeight() ), *m_boldFont, text, borderColor );
			DrawText( wxPoint( textX - 1, location.y + 3 + layout->m_windowSize.GetHeight() ), *m_boldFont, text, commentColor );
		}
	}
}

namespace
{
	Int32 GetSocketNumerByType( const CGraphSocket* socket, ELinkedSocketDirection dir  )
	{
		Int32 counter = -1;

		const CGraphBlock* block = socket->GetBlock();
		const TDynArray< CGraphSocket* >& sockets = block->GetSockets();
		const Uint32 numSockets = sockets.Size();
		for ( Uint32 i=0; i<numSockets; ++i )
		{
			const CGraphSocket* s = sockets[ i ];
			if ( s && s->GetDirection() == dir )
			{
				++counter;
			}

			if ( s == socket )
			{
				break;
			}
		}

		return counter;
	}
}

void CEdSceneGraphEditor::AdjustSocketCaption( CGraphSocket* socket, String& caption, wxColor& color )
{
	if ( socket->GetDirection() == LSD_Input )
	{
		if ( CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( socket->GetBlock() ) )
		{
			if ( CStorySceneControlPart* cp = sceneBlock->GetControlPart() )
			{
				if ( cp->SupportsInputSelection() )
				{
					const Int32 selInput = (Int32)cp->GetSelectedInputLinkElement();
					if ( selInput == GetSocketNumerByType( socket, LSD_Input ) )
					{
						caption += TXT(" <");
					}
				}
			}
		}
	}
	else if ( socket->GetDirection() == LSD_Output )
	{
		if ( CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( socket->GetBlock() ) )
		{
			if ( CStorySceneControlPart* cp = sceneBlock->GetControlPart() )
			{
				if ( cp->SupportsOutputSelection() )
				{
					const Int32 selInput = (Int32)cp->GetSelectedOutputLinkElement();
					if ( selInput == GetSocketNumerByType( socket, LSD_Output ) )
					{
						caption = TXT("> ") + caption;
					}
				}
			}
		}
	}
}

void CEdSceneGraphEditor::AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth )
{
	/*Bool forceToDefault = false;

	CStorySceneGraphBlock* sceneSrcBlock = Cast< CStorySceneGraphBlock >( source->GetBlock() );
	CStorySceneGraphBlock* sceneDestBlock = Cast< CStorySceneGraphBlock >( destination->GetBlock() );
	if ( sceneSrcBlock && sceneDestBlock && IsBlockActivated( sceneSrcBlock ) && IsBlockActivated( sceneDestBlock ) )
	{
		const CStorySceneControlPart* cpSrc = sceneSrcBlock->GetControlPart();
		const CStorySceneControlPart* cpDest = sceneDestBlock->GetControlPart();

		if ( cpSrc && cpDest )
		{
			if ( cpSrc->SupportsInputSelection() )
			{
				const Uint32 socketIdx = (Uint32)sceneSrcBlock->GetSockets().GetIndex( source );
				if ( socketIdx != cpSrc->GetSelectedInputLinkElement() )
				{
					forceToDefault = true;
				}
			}
			if ( cpDest->SupportsInputSelection() )
			{
				const Uint32 socketIdx = (Uint32)sceneDestBlock->GetSockets().GetIndex( destination );
				if ( socketIdx != cpDest->GetSelectedInputLinkElement() )
				{
					forceToDefault = true;
				}
			}
		}
	}

	if ( forceToDefault )
	{
		linkWidth = 3;
		linkColor = Color( 0,0,0 );
	}
	else*/
	{
		__super::AdjustLinkColors( source, destination, linkColor, linkWidth );
	}
}

void CEdSceneGraphEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	CEdGraphEditor::DispatchEditorEvent( name, data );
	
	if ( name == CNAME( SceneSectionRemoved ) )
	{
		CStorySceneControlPart* controlPart = GetEventData< CStorySceneControlPart* >( data );
		CStorySceneGraphBlock* controlPartBlock = FindControlPartBlock( controlPart );

		if ( controlPartBlock == NULL )
		{
			return;
		}

		CUndoDialogGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, controlPartBlock, CUndoDialogGraphBlockExistance::GRAPH_REMOVE_BLOCK );
		CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );	
	}
	else if ( name == CNAME( SceneSectionNameChanged ) )
	{
		CStorySceneControlPart* controlPart = GetEventData< CStorySceneControlPart* >( data );
		CStorySceneGraphBlock* controlPartBlock = FindControlPartBlock( controlPart );

		if ( controlPartBlock == NULL )
		{
			return;
		}

		controlPartBlock->InvalidateLayout();
		Repaint();
	}
	else if ( name == CNAME( ScenePartBlockSocketsChanged ) )
	{
		CStorySceneControlPart* controlPart = GetEventData< CStorySceneControlPart* >( data );
		CStorySceneGraphBlock* controlPartBlock = FindControlPartBlock( controlPart );

		if ( controlPartBlock == NULL )
		{
			return;
		}

		controlPartBlock->OnRebuildSockets();
		controlPartBlock->InvalidateLayout();
		Repaint();
	}
	else if ( name == CNAME( SceneChoiceLineChanged ) )
	{
		CStorySceneChoiceLine* choiceLine = GetEventData< CStorySceneChoiceLine* >( data );
		CStorySceneSection* section = choiceLine->FindParent< CStorySceneSection >();
		CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( FindControlPartBlock( section ) );

		if ( sectionBlock == NULL )
		{
			return;
		}

		sectionBlock->OnChoiceLineChanged( choiceLine );

		Repaint();
	}
}

void CEdSceneGraphEditor::OnEditCopy( wxCommandEvent& event )
{
	TDynArray< CStorySceneGraphBlock* > selectedBlocks;
	GetSelectedSceneBlocks( selectedBlocks );

	for ( TDynArray< CStorySceneGraphBlock* >::iterator selectedIter = selectedBlocks.Begin();
		selectedIter != selectedBlocks.End(); ++selectedIter )
	{
		CStorySceneControlPart* controlPart = (*selectedIter)->GetControlPart();
		if ( controlPart != NULL )
		{
			controlPart->SetParent( (*selectedIter) );
		}
	}


	Bool copy = ( event.GetId() != wxID_STORYSCENEEDITOR_CLONESECTION );
	CopySelection( copy );

	for ( TDynArray< CStorySceneGraphBlock* >::iterator selectedIter = selectedBlocks.Begin();
		selectedIter != selectedBlocks.End(); ++selectedIter )
	{
		CStorySceneControlPart* controlPart = (*selectedIter)->GetControlPart();
		if ( controlPart != NULL )
		{
			controlPart->SetParent( HACK_GetStoryScene() );
		}
	}
}

void CEdSceneGraphEditor::OnEditCut( wxCommandEvent& event )
{
	// Copy blocks to buffer

	TDynArray< CStorySceneGraphBlock* > selectedBlocks;
	GetSelectedSceneBlocks( selectedBlocks );

	for ( TDynArray< CStorySceneGraphBlock* >::iterator selectedIter = selectedBlocks.Begin();
		selectedIter != selectedBlocks.End(); ++selectedIter )
	{
		CStorySceneControlPart* controlPart = (*selectedIter)->GetControlPart();
		if ( controlPart != NULL )
		{
			controlPart->SetParent( (*selectedIter) );
		}
	}

	CopySelection( false );

	for ( TDynArray< CStorySceneGraphBlock* >::iterator selectedIter = selectedBlocks.Begin();
		selectedIter != selectedBlocks.End(); ++selectedIter )
	{
		CStorySceneControlPart* controlPart = (*selectedIter)->GetControlPart();
		if ( controlPart != NULL )
		{
			controlPart->SetParent( HACK_GetStoryScene() );
		}
	}

	// Delete cut blocks

	for ( auto selectedIter = selectedBlocks.Begin(); selectedIter != selectedBlocks.End(); ++selectedIter )
	{
		CUndoDialogGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, *selectedIter, CUndoDialogGraphBlockExistance::GRAPH_REMOVE_BLOCK );
	}

	CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );

	m_mediator->OnGraph_RemoveSceneBlocks( selectedBlocks );
}

void CEdSceneGraphEditor::OnEditPaste( wxCommandEvent& event )
{
	OnPaste( nullptr );
}

CStorySceneGraphBlock* CEdSceneGraphEditor::FindControlPartBlock( CStorySceneControlPart* controlPart )
{
	if ( m_graph == NULL || controlPart->GetScene() != HACK_GetStoryScene() )
	{
		return NULL;
	}

	TDynArray< CGraphBlock* > blocks = m_graph->GraphGetBlocks();

	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		CStorySceneGraphBlock* sectionBlock = Cast< CStorySceneGraphBlock >( blocks[ i ] );
		if ( sectionBlock && sectionBlock->GetControlPart() == controlPart )
		{
			return sectionBlock;
		}
	}
	return NULL;
}

void CEdSceneGraphEditor::SelectSectionBlock( CStorySceneSection* section, Bool select )
{
	CGraphBlock* sectionBlock = FindControlPartBlock( section );

	if ( sectionBlock != NULL )
	{
		SelectBlock( sectionBlock, select );
	}
	
	if ( select == true )
	{
		SetContolPartActive( section );
	}
	
}

void CEdSceneGraphEditor::CenterOnSectionBlock( CStorySceneSection* section )
{
	CGraphBlock* sectionBlock = FindControlPartBlock( section );
	if ( section != NULL )
	{
		FocusOnBlock( sectionBlock );
	}
}

void CEdSceneGraphEditor::GetSelectedSceneBlocks( TDynArray< CStorySceneGraphBlock* >& selectedSceneBlocks )
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );
	for ( Uint32 i = 0; i < selectedBlocks.Size(); ++i )
	{
		CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( selectedBlocks[ i ] );
		if ( sceneBlock != NULL )
		{
			selectedSceneBlocks.PushBack( sceneBlock );
		}
	}
}

void CEdSceneGraphEditor::OpenCutscenePreview( CStorySceneCutsceneSectionBlock* csBlock )
{
	CStorySceneCutsceneSection* csSection = Cast< CStorySceneCutsceneSection >( csBlock->GetSection() );
	if ( csSection )
	{
		CCutsceneTemplate* csTempl = csSection->GetCsTemplate();
		if ( csTempl )
		{
			TDynArray< SCutsceneActorLine > lines;
			csSection->GetDialogLines( lines );

			CEdCutsceneEditor* editor = new CEdCutsceneEditor( 0, csTempl, &lines );
			editor->Show();
		}
	}
}

void CEdSceneGraphEditor::CopyBlockConnections( CGraphBlock* from, CGraphBlock* to )
{
	GraphConnectionRelinker relinker( from, to );
	from->BreakAllLinks();
}

void CEdSceneGraphEditor::OnPaste( const Vector* pos )
{
	TDynArray< CGraphBlock* > pastedBlocks = Paste( pos, true );

	CEdSceneEditorScreenplayPanel* sceneEditorPage = HACK_GetScreenplayPanel();

	for ( TDynArray< CGraphBlock* >::iterator pastedIter = pastedBlocks.Begin();
		pastedIter != pastedBlocks.End(); ++pastedIter )
	{
		if ( CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( *pastedIter ) )
		{
			CStorySceneControlPart* controlPart = sceneBlock->GetControlPart();
			if ( controlPart != NULL )
			{
				controlPart->SetParent( HACK_GetStoryScene() );
				HACK_GetStoryScene()->AddControlPart( controlPart );
			}

			GraphBlockSpawnInfo spawnInfo( sceneBlock->GetClass() );
			spawnInfo.m_position = (*pastedIter)->GetPosition();
			sceneBlock->OnSpawned( spawnInfo );
		}

		CUndoDialogGraphBlockExistance::PrepareCreationStep( *m_undoManager, this, *pastedIter );
	}

	CUndoDialogGraphBlockExistance::FinalizeStep( *m_undoManager );

	m_mediator->OnGraph_AddSceneBlocks( ( TDynArray< CStorySceneGraphBlock* >& )( pastedBlocks ) );

	GraphStructureModified();
}

void CEdSceneGraphEditor::MarkDebugControlPart( const CStorySceneControlPart* controlPart )
{
	SetContolPartActive( controlPart );
}

void CEdSceneGraphEditor::SetContolPartActive( const CStorySceneControlPart* controlPart )
{
	if ( m_graph == NULL )
	{
		return;
	}

	Bool shouldRepaint = true;

	TDynArray< CGraphBlock* > blocks = m_graph->GraphGetBlocks();

	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( blocks[ i ] );
		if ( sceneBlock == NULL )
		{
			continue;
		}

		if ( sceneBlock->GetControlPart() == controlPart )
		{
			if ( !sceneBlock->IsActivated() )
			{
				sceneBlock->SetActive( true );
			}
			else
			{
				shouldRepaint = false;
			}
		}
		else
		{
			sceneBlock->SetActive( false );
		}
	}

	m_shouldRepaint = shouldRepaint;
}

void CEdSceneGraphEditor::OnCreateDialogsetNew( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* dialogset = HACK_GetScreenplayPanel()->GetSceneEditor()->CreateNewDialogset();

	if ( dialogset != NULL )
	{
		TDynArray< CGraphBlock* > selectedBlocks;
		GetSelectedBlocks( selectedBlocks );

		for ( Uint32 i = 0; i < selectedBlocks.Size(); ++i )
		{
			if ( selectedBlocks[ i ]->IsExactlyA< CStorySceneSectionBlock >() == false )
			{
				continue;
			}

			CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( selectedBlocks[ i ] );
			sectionBlock->GetSection()->SetDialogsetChange( dialogset->GetName() );
		}
	}

	HACK_GetScreenplayPanel()->GetSceneEditor()->RefreshPropertiesPage();
}

void CEdSceneGraphEditor::OnCreateDialogsetFromFile( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* dialogset = HACK_GetScreenplayPanel()->GetSceneEditor()->CreateDialogsetFromFile();

	if ( dialogset != NULL )
	{
		TDynArray< CGraphBlock* > selectedBlocks;
		GetSelectedBlocks( selectedBlocks );

		for ( Uint32 i = 0; i < selectedBlocks.Size(); ++i )
		{
			if ( selectedBlocks[ i ]->IsExactlyA< CStorySceneSectionBlock >() == false )
			{
				continue;
			}

			CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( selectedBlocks[ i ] );
			sectionBlock->GetSection()->SetDialogsetChange( dialogset->GetName() );
		}
	}

	HACK_GetScreenplayPanel()->GetSceneEditor()->RefreshPropertiesPage();
}

void CEdSceneGraphEditor::OnCreateDialogsetFromPreviousSection( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	if ( selectedBlocks.Size() != 1 )
	{
		return;
	}
	
	if ( selectedBlocks[ 0 ]->IsExactlyA< CStorySceneSectionBlock >() == false )
	{
		return;
	}

	CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( selectedBlocks[ 0 ] );

	CStorySceneDialogsetInstance* dialogset = HACK_GetScreenplayPanel()->GetSceneEditor()->CreateDialogsetFromPreviousSection( sectionBlock->GetSection() );

	sectionBlock->GetSection()->SetDialogsetChange( dialogset->GetName() );

	HACK_GetScreenplayPanel()->GetSceneEditor()->RefreshPropertiesPage();
}

void CEdSceneGraphEditor::OnCreateDialogsetFromSelectedSection( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	if ( selectedBlocks.Size() != 1 )
	{
		return;
	}

	if ( selectedBlocks[ 0 ]->IsExactlyA< CStorySceneSectionBlock >() == false )
	{
		return;
	}

	CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( selectedBlocks[ 0 ] );

	CStorySceneDialogsetInstance* dialogset = HACK_GetScreenplayPanel()->GetSceneEditor()->CreateDialogsetFromCurrentSection( sectionBlock->GetSection() );

	sectionBlock->GetSection()->SetDialogsetChange( dialogset->GetName() );

	HACK_GetScreenplayPanel()->GetSceneEditor()->RefreshPropertiesPage();
}

void CEdSceneGraphEditor::OnChangeDialogset( wxCommandEvent& event )
{
	Int32 dialogsetIndex = event.GetId() - wxID_STORYSCENEEDITOR_CHANGEDIALOGSET;
	if ( dialogsetIndex < 0 )
	{
		return;
	}
	TDynArray< CName > allDialogsetInstancesNames;
	GetSceneEditor()->HACK_GetStoryScene()->GetDialogsetInstancesNames( allDialogsetInstancesNames );

	if ( (Uint32 ) dialogsetIndex >= allDialogsetInstancesNames.Size() )
	{
		return;
	}

	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	if ( selectedBlocks.Size() != 1 )
	{
		return;
	}

	if ( selectedBlocks[ 0 ]->IsExactlyA< CStorySceneSectionBlock >() == false )
	{
		return;
	}

	CStorySceneSectionBlock* sectionBlock = Cast< CStorySceneSectionBlock >( selectedBlocks[ 0 ] );
	sectionBlock->GetSection()->SetDialogsetChange( allDialogsetInstancesNames[ dialogsetIndex ] );

	HACK_GetScreenplayPanel()->GetSceneEditor()->RefreshPropertiesPage();

}

void CEdSceneGraphEditor::OnVerifySection( wxCommandEvent& event )
{

}

void CEdSceneGraphEditor::FillBlockAndTemplateInfo()
{
	// uses ClassID, no static array here
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add input" ),				ClassID< CStorySceneInputBlock >(),				ClassID< CStorySceneInput >(),				wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add output" ),				ClassID< CStorySceneOutputBlock >(),			ClassID< CStorySceneOutput >(),				wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add section" ),				ClassID< CStorySceneSectionBlock >(),			ClassID< CStorySceneSection >(),			wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add cutscene" ),			ClassID< CStorySceneCutsceneSectionBlock >(),	ClassID< CStorySceneCutsceneSection >(),	wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add flow condition" ),		ClassID< CStorySceneFlowConditionBlock >(),		ClassID< CStorySceneFlowCondition >(),		wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add flow switch" ),			ClassID< CStorySceneFlowSwitchBlock >(),		ClassID< CStorySceneFlowSwitch >(),			wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add randomizer" ),			ClassID< CStorySceneRandomBlock >(),			ClassID< CStorySceneRandomizer >(),			wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add scripting block" ),		ClassID< CStorySceneScriptingBlock >(),			ClassID< CStorySceneScript >(),				wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add video block" ),			ClassID< CStorySceneVideoBlock >(),				ClassID< CStorySceneVideoSection >(),		wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Add link hub" ),			ClassID< CStorySceneLinkHubBlock >(),			ClassID< CStorySceneLinkHub >(),			wxT( "" ),						STT_None	) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Gameplay scene" ),			nullptr,										nullptr,									wxT( "GameplaySceneTemplate" ),	STT_Gameplay		) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Normal scene" ),			nullptr,										nullptr,									wxT( "NormalSceneTemplate" ),	STT_Normal		) );
	m_sceneMenuInfo.PushBack( CSceneMenuInfo( wxT( "Full voiceset template" ),	nullptr,										nullptr,									wxT( "FullVoicesetTemplate" ),	STT_FullVoiceSet	) );
}

void CEdSceneGraphEditor::LoadKeyboardShortcuts()
{
	C2dArray* list = LoadResource< C2dArray >( EDITOR_SCENE_SHORTCUTS_CSV );

	if ( list )
	{
		for ( Uint32 i = 0; i < list->GetNumberOfRows(); ++i )
		{
			String keyCodeName		= list->GetValue( TXT("KeyCode"), i );
			String blockClassName	= list->GetValue( TXT("BlockClass"), i );
			String templateName		= list->GetValue( TXT("Template"), i );

			if ( keyCodeName.GetLength() != 1 )
			{
				continue;
			}
			Char keyCode = keyCodeName[ 0 ];
			if ( !iswascii( keyCode ) )
			{
				continue;
			}
			CClass* blockClass = SRTTI::GetInstance().FindClass( CName( blockClassName ) );
			if ( !blockClass && templateName.Empty() )
			{
				continue;
			}
			m_shortcuts[ keyCode ] = TPair< CClass*, String >( blockClass, templateName );
		}
	}
}
