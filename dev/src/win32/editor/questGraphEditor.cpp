#include "build.h"
#include "questEditor.h"
#include "questGraphEditor.h"
#include "versionControlIconsPainter.h"
#include "questDebugInfo.h"
#include "questGraphSearcher.h"
#include "dialogEditorUtils.h"
#include "editorExternalResources.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/questVariedInputsBlock.h"
#include "../../common/game/questSceneBlock.h"
#include "../../common/game/questStartBlock.h"
#include "../../common/game/questEndBlock.h"
#include "../../common/game/questCutControlBlock.h"
#include "../../common/game/questPhaseInputBlock.h"
#include "../../common/game/questPhaseOutputBlock.h"
#include "../../common/game/questScriptedActionsBlock.h"
#include "../../common/game/questTestBlock.h"
#include "../../games/r4/questBehaviorEventBlock.h"
#include "../../common/game/questDeletionMarkerBlock.h"
#include "../../games/r4/questBehaviorSocket.h"
#include "../../common/game/questRandomBlock.h"
#include "../../common/engine/descriptionGraphBlock.h"
#include "../../common/engine/commentGraphBlock.h"
#include "../../common/engine/graphConnection.h"
#include "../../common/game/questDependencyInfo.h"
#include "../../common/core/gatheredResource.h"

enum
{
	QUESTEDITOR_MAX_SPECIAL_OPTIONS = 128,

	wxID_QUESTEDITOR_ADDBLOCK = 4401,

	wxID_QUESTEDITOR_REMOVEBLOCK = 4500,
	wxID_QUESTEDITOR_CONVERTORESOURCE,
	wxID_QUESTEDITOR_EMBEDFROMRESOURCE,
	wxID_QUESTEDITOR_ADDINPUT,
	wxID_QUESTEDITOR_REMOVEINPUT,
	wxID_QUESTEDITOR_ADDOUTPUT,
	wxID_QUESTEDITOR_REMOVEOUTPUT,
	wxID_QUESTEDITOR_CHECKOUT,
	wxID_QUESTEDITOR_SUBMIT,
	wxID_QUESTEDITOR_TOOLSMENUS,
	wxID_QUESTEDITOR_SAVE_SCOPE_BLOCK,
	wxID_QUESTEDITOR_ADDPATCHOUTPUT,
	wxID_QUESTEDITOR_REMOVEPATCHOUTPUT,
	wxID_QUESTEDITOR_ADDTERMINATIONINPUT,
	wxID_QUESTEDITOR_REMOVETERMINATIONINPUT,
	wxID_QUESTEDITOR_PASTEHERE,
	wxID_QUESTEDITOR_RUNSPECIALOPTION_0,
	wxID_QUESTEDITOR_RUNSPECIALOPTION_LIMIT = wxID_QUESTEDITOR_RUNSPECIALOPTION_0 + QUESTEDITOR_MAX_SPECIAL_OPTIONS,
	wxID_QUESTEDITOR_REBUILDSOCKETS
};

namespace // anonymous
{
	class CQuestObjectWrapper : public wxObject
	{
	private:
		wxPoint m_position;
		CClass *m_class;

	public:
		CQuestObjectWrapper( CClass *blockClass, wxPoint position )
			: m_class( blockClass )
			, m_position( position )
		{ }

		RED_INLINE wxPoint GetPosition() const { return m_position; }
		RED_INLINE CClass *GetClass() const { return m_class; }

	};

	///////////////////////////////////////////////////////////////////////////

	class IVersionControlCommand : public wxObject
	{
	public:
		virtual ~IVersionControlCommand() {}

		virtual void Execute() = 0;
	};

	///////////////////////////////////////////////////////////////////////////

	class CVCSubmit : public IVersionControlCommand
	{
	private:
		CDiskFile& m_file;

	public:
		CVCSubmit( CDiskFile& file ) : m_file( file ) {}

		void Execute()
		{
			if( m_file.IsEdited() )
			{
				m_file.Save();
			}
			m_file.Submit();
		}
	};

	///////////////////////////////////////////////////////////////////////////

	class CVCCheckout : public IVersionControlCommand
	{
	private:
		CDiskFile& m_file;

	public:
		CVCCheckout( CDiskFile& file ) : m_file( file ) {}

		void Execute()
		{
			m_file.CheckOut();
		}
	};

	///////////////////////////////////////////////////////////////////////////

	class NullGraph : public IGraphContainer
	{
	private:
		TDynArray< CGraphBlock* >	m_noBlocks;
	public:
		virtual CObject *GraphGetOwner() { return NULL; }
		virtual TDynArray< CGraphBlock* >& GraphGetBlocks() { return m_noBlocks; }
		virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const { return m_noBlocks; }
	};

	///////////////////////////////////////////////////////////////////////////

	class IToolCommandExecutor : public wxObject
	{
	public:
		virtual ~IToolCommandExecutor() {}

		virtual void Execute() = 0;
	};

	///////////////////////////////////////////////////////////////////////////

	class CToolBlockCommandExecutor : public IToolCommandExecutor
	{
	private:
		CQuestGraphBlock*		m_block;
		IToolCommandWrapper*	m_command;

	public:
		CToolBlockCommandExecutor( CQuestGraphBlock* block,
			IToolCommandWrapper* command )
			: m_block( block )
			, m_command( command )
		{}

		void Execute()
		{
			m_command->OnBlock( m_block );
		}
	};

	///////////////////////////////////////////////////////////////////////////

	class CToolSocketCommandExecutor : public IToolCommandExecutor
	{
	private:
		CQuestGraphBlock*				m_block;
		const CQuestGraphSocket*	m_socket;
		IToolCommandWrapper*			m_command;

	public:
		CToolSocketCommandExecutor( CQuestGraphBlock* block,
			const CQuestGraphSocket* socket,
			IToolCommandWrapper* command )
			: m_block( block )
			, m_socket( socket )
			, m_command( command )
		{}

		void Execute()
		{
			m_command->OnSocket( m_block, m_socket );
		}
	};
}

///////////////////////////////////////////////////////////////////////////////

CGatheredResource resDlcWithEnabledDeletionMarkers( TXT("quests\\dlcWithEnabledDeletionMarkers.csv"), RGF_Startup );

BEGIN_EVENT_TABLE( CEdQuestGraphEditor, CEdGraphEditor )
	EVT_LEFT_DCLICK( CEdQuestGraphEditor::OnDoubleClick )
	EVT_LEFT_UP( CEdQuestGraphEditor::OnLeftClick )
	EVT_KEY_UP( CEdQuestGraphEditor::OnKeyUp )
	EVT_KILL_FOCUS( CEdQuestGraphEditor::OnKillFocus )
END_EVENT_TABLE()

CEdQuestGraphEditor::CEdQuestGraphEditor( wxWindow* parent, CEdQuestEditor *editor )
	: CEdGraphEditor( parent, false )
	, m_nullGraph( new NullGraph() )
	, m_editor( editor )
	, m_vciPainter( NULL )
	, m_showFlowSequence( false )
	, m_pressedKeyCode( -1 )
	, m_unusedBlocks( 0 )
	, m_checkedForErrors( false )
{
	ASSERT( m_editor );
	m_vciPainter = new CVersionControlIconsPainter( *this );
	SetGraph( m_nullGraph );
	Repaint( false );

	LoadKeyboardShortcuts();
}

CEdQuestGraphEditor::~CEdQuestGraphEditor()
{
	delete m_vciPainter; m_vciPainter = NULL;
	delete m_nullGraph;	m_nullGraph = NULL;
}

CQuestGraph* CEdQuestGraphEditor::GetTopLevelGraph()
{
	if ( m_phasesStack.Empty() )
	{
		return NULL;
	}
	else
	{
		return m_phasesStack[ 0 ];
	}
}

CObject* CEdQuestGraphEditor::GetCurrentPhaseRoot()
{
	if( m_phasesStack.Size() > 0 )
	{
		return m_phasesStack.Back()->GetRoot();
	}

	return nullptr;
}

void CEdQuestGraphEditor::SetQuestGraph( CQuestGraph* graph )
{
	m_phasesStack.Clear();

	if ( graph )
	{
		SetGraph( graph );
		m_phasesStack.PushBack( graph );
	}
	else
	{
		SetGraph( m_nullGraph );
	}

	CheckAndUpdateGUIDs( false );
}

void CEdQuestGraphEditor::GetAllPhases( CQuestGraph *graph, TDynArray< CQuestPhase* >& phasesArray )
{
	if ( graph )
	{
		TDynArray< CGraphBlock * > &blocks = graph->GraphGetBlocks();
		for ( Uint32 i = 0; i < blocks.Size(); ++i )
		{
			// Find phase block
			if ( !blocks[i]->IsA< CQuestScopeBlock >() )
			{
				continue;
			}
			
			// If phase block contains a phase resource rather than embedded graph then collect it
			CQuestScopeBlock *scopeBlock = static_cast< CQuestScopeBlock * >( blocks[i] );
			if ( !scopeBlock->IsResource() )
			{
				continue;
			}

			CQuestPhase* phase = scopeBlock->GetPhase();
			if ( phasesArray.Exist( phase ) )
			{
				// the phase was already collected
				continue;
			}

			phasesArray.PushBack( phase );
			GetAllPhases( scopeBlock->GetGraph(), phasesArray );
		}
	}
}

void CEdQuestGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	CEdGraphEditor::PaintCanvas( width, height );

	Uint32 levelsCount = m_phasesStack.Size();
	if ( levelsCount == 0 )
	{
		return;
	}

	const Int32 levelWidth = 20;
	const Int32 levelHeight = 8;
	const Int32 levelSpace = 5;

    const Float levelLineWidth = ClientToCanvas( wxRect( 0, 0, 2.0f, 1.0f ) ).GetWidth();
    const Float borderLineWidth = ClientToCanvas( wxRect( 0, 0, 15.0f, 1.0f ) ).GetWidth();

    wxRect rect( 15, 0, levelWidth, levelHeight );
	for ( Uint32 i = 0; i < levelsCount; ++i )
	{
		Int32 y = i * ( levelHeight + levelSpace );
        rect.SetTop( y + 15 );
        wxRect canvasRect = ClientToCanvas( rect );
		if ( i == ( levelsCount - 1 ) )
		{
            FillRect( canvasRect, wxColour(255, 255, 0, 40) );
            DrawRect( canvasRect, wxColour(255, 0, 0, 140), levelLineWidth );
		}
		else
		{
            FillRect( canvasRect, wxColour(255, 255, 0, 40) );
            DrawRect( canvasRect, wxColour(255, 255, 0, 140), levelLineWidth );
		}
	}

	// draw a rectangle around the canvas, if we're viewing an embedded phase graph
	if ( CQuestScopeBlock *scopeBlock = GetParentScopeBlock( m_phasesStack.Back()) )
	{
		Color phaseBlockColor = scopeBlock->GetClientColor();
		DrawRoundedRect( ClientToCanvas( GetRect() ), wxColour(phaseBlockColor.R, phaseBlockColor.G, phaseBlockColor.B, phaseBlockColor.A), (Int32)borderLineWidth, borderLineWidth );
	}

	// draw phases descriptions
	String desc;
	String desc2;
	for ( Uint32 i = 0; i < m_phasesStack.Size(); ++i )
	{
		CQuestGraphBlock* block = Cast< CQuestGraphBlock >( m_phasesStack[ i ]->GetParent() );
		desc += String::Printf( TXT("[%s]   "), block? block->GetCaption().AsChar() : TXT(".") );
	}
	if ( !desc.Empty() )
	{
		Float originalScale = GetScale();
		SetScale( 1, false );

		wxPoint canvasTextPos = ClientToCanvas( wxPoint( 2 * rect.GetLeft() + rect.GetWidth(), 15 ) );
		DrawText( canvasTextPos, *m_drawFont, desc, wxColor( 0xFFFFFFFF) );

		// Write the amount of nodes that can be deleted
		if ( !m_checkedForErrors )
		{
			m_unusedBlocks = CountUnusedBlocks();
		}
		if ( m_unusedBlocks != 0 )
		{
			desc2 = String::Printf( TXT("There are more than [%s] unused nodes in this graph"), ToString( m_unusedBlocks ).AsChar() );
			wxPoint canvasTextPos2 = ClientToCanvas( wxPoint( 2 * rect.GetLeft() + rect.GetWidth(), 30 ) );
			DrawText( canvasTextPos2, *m_drawFont, desc2, wxColor( 0xFFFFFFFF) );
		}
		
		SetScale( originalScale, false );
	}
}

wxColor CEdQuestGraphEditor::GetCanvasColor() const 
{ 
	wxColor bgColor = DEFAULT_EDITOR_BACKGROUND; 

	// adjust the color if we're viewing an actively observed thread
	IQuestDebugInfo* debugInfo = NULL;
	if ( m_editor 
		&& m_editor->GetDebugInfo() 
		&& !m_phasesStack.Empty() 
		&& m_editor->GetDebugInfo()->IsGraphInActive( m_phasesStack.Back() ) )
	{
		bgColor = wxColor( 170, 80, 80 );
	}

	return bgColor;
}


void CEdQuestGraphEditor::CalcTitleIconArea( CGraphBlock* block, wxSize &size )
{
	if ( block )
	{
		if ( block->IsA<CQuestScopeBlock>() )
		{
			if ( static_cast<CQuestScopeBlock *>( block )->IsResource() )
			{
				size.x = 15;
				size.y = 15;
			}
			else
			{
				size.x = 0;
				size.y = 0;
			}
		}
	}
}

void CEdQuestGraphEditor::DrawTitleIconArea( CGraphBlock* block, const wxRect& rect )
{
	if ( block )
	{
		if ( block->IsA<CQuestScopeBlock>() )
		{
			CQuestScopeBlock* scopeBlock = static_cast<CQuestScopeBlock *>( block );
			if ( scopeBlock->IsResource() )
			{
				m_vciPainter->PaintVersionControlIcon( scopeBlock->GetPhase()->GetFile(), rect );
			}
		}
	}
}

void CEdQuestGraphEditor::CalcBlockInnerArea( CGraphBlock* block, wxSize& size )
{
	if ( block )
	{
		if ( block->IsExactlyA<CQuestCutControlBlock>() || block->IsExactlyA<CQuestBehaviorEventBlock>() )
		{
			size.x = 40;
			size.y = 40;
		}
		else if ( block->IsExactlyA<CQuestStartBlock>() || 
			block->IsExactlyA<CQuestEndBlock>() ||
			block->IsExactlyA<CQuestPhaseInputBlock>() ||
			block->IsExactlyA<CQuestPhaseOutputBlock>() )
		{
			size.x = 50;
			size.y = 10;
		}
		else
		{
			CEdGraphEditor::CalcBlockInnerArea( block, size );
		}
	}
}

void CEdQuestGraphEditor::DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect )
{
	if ( block )
	{
		wxColour clientColor;
		wxColour borderColor;
		Bool drawThunder = false;
		if ( block->IsExactlyA<CQuestCutControlBlock>() )
		{
			CQuestCutControlBlock* ccBlock = Cast< CQuestCutControlBlock >( block );
			if ( ccBlock->IsPermanent() )
			{
				clientColor = wxColour(192, 0, 0);
				borderColor = wxColour(255, 0, 0);
			}
			else
			{
				clientColor = wxColour(192, 192, 0);
				borderColor = wxColour(255, 255, 0);
			}
			drawThunder = true;
		}
		else if ( block->IsExactlyA<CQuestBehaviorEventBlock>() )
		{
			clientColor = wxColour(115, 255, 56);
			borderColor = wxColour(75, 165, 36);
			drawThunder = true;
		}

		if ( drawThunder )
		{
			const Int32 thunderWidth = 40;
			const Int32 thunderHeight = 40;
			const Int32 thunderPosX = rect.GetLeft() + (rect.GetWidth() - thunderWidth) / 2;
			const Int32 thunderPosY = rect.GetTop() + (rect.GetHeight() - thunderHeight) / 2;

			wxPoint points[11] = 
			{
				wxPoint(thunderPosX + 15, thunderPosY + 0), 
				wxPoint(thunderPosX + 22, thunderPosY + 10),
				wxPoint(thunderPosX + 19, thunderPosY + 12),
				wxPoint(thunderPosX + 28, thunderPosY + 20),
				wxPoint(thunderPosX + 25, thunderPosY + 22),
				wxPoint(thunderPosX + 37, thunderPosY + 37),
				wxPoint(thunderPosX + 17, thunderPosY + 25),
				wxPoint(thunderPosX + 21, thunderPosY + 24),
				wxPoint(thunderPosX + 9, thunderPosY + 16), 
				wxPoint(thunderPosX + 13, thunderPosY + 14),
				wxPoint(thunderPosX + 0, thunderPosY + 6),
			};

			FillPoly( &points[0], sizeof(points) / sizeof(wxPoint), clientColor );
			DrawPoly( &points[0], sizeof(points) / sizeof(wxPoint), borderColor, 1.f );
		}
	}
}

void CEdQuestGraphEditor::AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth )
{
	if (( source && source->GetClass() == ClassID<CQuestCutControlGraphSocket>() ) ||	( destination && destination->GetClass() == ClassID<CQuestCutControlGraphSocket>() )
		|| ( source && source->GetClass() == ClassID<CQuestBehaviorSyncGraphSocket>() ) ||	( destination && destination->GetClass() == ClassID<CQuestBehaviorSyncGraphSocket>() ))
	{
		linkWidth = 2.f;
	}
	else
	{
		linkWidth = 1.f;
	}
}

void CEdQuestGraphEditor::AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow )
{
	if ( source && source->GetPlacement() == LSP_Center && destination && destination->GetPlacement() == LSP_Center )
	{
		if ( ( source->GetClass() == ClassID<CQuestCutControlGraphSocket>() || source->GetClass() == ClassID<CQuestBehaviorSyncGraphSocket>() ) && destination->GetClass() == ClassID<CQuestGraphSocket>() )
		{
			srcCapArrow = false;
			destCapArrow = true;
		}
		else if ( source->GetClass() == ClassID<CQuestGraphSocket>() && ( destination->GetClass() == ClassID<CQuestCutControlGraphSocket>() || source->GetClass() == ClassID<CQuestBehaviorSyncGraphSocket>() ) )
		{
			srcCapArrow = true;
			destCapArrow = false;
		}
	}
	else
	{
		srcCapArrow = false;
		destCapArrow = false;
	}
}

void CEdQuestGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
	m_mousePosition = wxGetMousePosition();

	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );	
	menu.Append( wxID_QUESTEDITOR_REMOVEBLOCK, wxT( "Remove selected" ) );
	menu.Connect( wxID_QUESTEDITOR_REMOVEBLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRemoveSelectedBlocks ), NULL, this );
	menu.Enable( wxID_QUESTEDITOR_REMOVEBLOCK, selectedBlocks.Empty() ? false : true );
	menu.AppendSeparator();

	if ( block->IsA< CQuestScopeBlock >() )
	{
		CQuestScopeBlock* scopeBlock = SafeCast< CQuestScopeBlock >( block );
		// customized menu for the phase block
		AddScopeResourceManagementMenuEntries( scopeBlock, menu );
		AddVersionControlMenuEntries( scopeBlock, menu );

		// Show save option
		if ( scopeBlock->GetPhase() )
		{
			menu.Append( wxID_QUESTEDITOR_SAVE_SCOPE_BLOCK, wxT( "Save block (recursive)" ) );
			menu.Connect( wxID_QUESTEDITOR_SAVE_SCOPE_BLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnSaveScopeBlock ), NULL, this );
			menu.AppendSeparator();
		}
	}
	else if ( block->IsA< CQuestVariedInputsBlock >() )
	{
		menu.Append( wxID_QUESTEDITOR_ADDINPUT, wxT( "Add input" ) );
		menu.Connect( wxID_QUESTEDITOR_ADDINPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnAddInput ), NULL, this );

		if ( SafeCast< CQuestVariedInputsBlock >( block )->CanRemoveInput() )
		{
			menu.Append( wxID_QUESTEDITOR_REMOVEINPUT, wxT( "Remove input" ) );
			menu.Connect( wxID_QUESTEDITOR_REMOVEINPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRemoveInput ), NULL, this );
		}

		menu.AppendSeparator();
	}
	else if ( block->IsA< CQuestRandomBlock >() )
	{
		CQuestRandomBlock* randomBlock = Cast< CQuestRandomBlock >( block );
		if ( randomBlock->CanAddOutput() == true )
		{
			menu.Append( wxID_QUESTEDITOR_ADDOUTPUT, wxT( "Add output" ) );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdQuestGraphEditor::OnAddOutput, this, wxID_QUESTEDITOR_ADDOUTPUT );
		}
		if ( randomBlock->CanRemoveOutput() == true )
		{
			menu.Append( wxID_QUESTEDITOR_REMOVEOUTPUT, wxT( "Remove output" ) );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdQuestGraphEditor::OnRemoveOutput, this, wxID_QUESTEDITOR_REMOVEOUTPUT );
		}
	}
	else if ( block->IsA< CBaseQuestScriptedActionsBlock >() )
	{
		CBaseQuestScriptedActionsBlock* scriptedAction = static_cast< CBaseQuestScriptedActionsBlock* >( block );
		if ( scriptedAction->IsHandlingBehaviorOutcome() )
		{
			menu.Append( wxID_QUESTEDITOR_ADDOUTPUT, wxT( "Do not handle behavior outcome" ) );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdQuestGraphEditor::OnDontHandleBehaviorOutcome, this, wxID_QUESTEDITOR_ADDOUTPUT );
		}
		else
		{
			menu.Append( wxID_QUESTEDITOR_ADDOUTPUT, wxT( "Handle behavior outcome" ) );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdQuestGraphEditor::OnHandleBehaviorOutcome, this, wxID_QUESTEDITOR_ADDOUTPUT );
		}
	}

	// append menus from tools
	CQuestGraphBlock *questBlock = Cast< CQuestGraphBlock >( block );

	if ( questBlock )
	{
		CQuestGraphBlock::SpecialOptionsList specialOptions;
		questBlock->GetContextMenuSpecialOptions( specialOptions );

		// patch outputs
		if ( questBlock->HasPatchOutput() )
		{
			menu.Append( wxID_QUESTEDITOR_REMOVEPATCHOUTPUT, wxT( "Remove patch output" ) );
			menu.Connect( wxID_QUESTEDITOR_REMOVEPATCHOUTPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRemovePatchOutputBlock ), NULL, this );
		}
		else
		{
			menu.Append( wxID_QUESTEDITOR_ADDPATCHOUTPUT, wxT( "Add patch output" ) );
			menu.Connect( wxID_QUESTEDITOR_ADDPATCHOUTPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnAddPatchOutputBlock ), NULL, this );
		}
		menu.AppendSeparator();

		if ( !specialOptions.Empty() )
		{
			for ( Uint32 i = 0, n = specialOptions.Size(); i != n; ++i )
			{
				menu.Append( wxID_QUESTEDITOR_RUNSPECIALOPTION_0 + i, specialOptions[ i ].AsChar() );
				menu.Connect( wxID_QUESTEDITOR_RUNSPECIALOPTION_0 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRunBlockSpecialOption ), NULL, this );
			}
			
			menu.AppendSeparator();
		}

		
		
		if ( questBlock->HasTerminationInput() )
		{
			menu.Append( wxID_QUESTEDITOR_REMOVETERMINATIONINPUT, wxT( "Remove termination input" ) );
			menu.Connect( wxID_QUESTEDITOR_REMOVETERMINATIONINPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRemoveTerminationInputBlock ), NULL, this );
		}
		else
		{
			menu.Append( wxID_QUESTEDITOR_ADDTERMINATIONINPUT, wxT( "Add termination input" ) );
			menu.Connect( wxID_QUESTEDITOR_ADDTERMINATIONINPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnAddTerminationInputBlock ), NULL, this );
		}

		menu.AppendSeparator();
		// submenus 
		TDynArray< SToolMenu > subMenus;
		m_editor->OnCreateBlockContextMenu( subMenus, Cast<CQuestGraphBlock>( block )  );
		Uint32 count = subMenus.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( subMenus[i].activationType & SToolMenu::AT_BLOCK )
			{
				menu.Append( wxID_QUESTEDITOR_TOOLSMENUS + i, subMenus[ i ].name.AsChar() );
				menu.Connect( wxID_QUESTEDITOR_TOOLSMENUS + i, 
					wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( CEdQuestGraphEditor::OnToolMenu ),
					new CToolBlockCommandExecutor( questBlock, subMenus[ i ].command ),
					this );
			}
		}

		if ( count > 0 )
		{
			menu.AppendSeparator();
		}

		menu.AppendSeparator();
		menu.Append( wxID_QUESTEDITOR_REBUILDSOCKETS, wxT( "Rebuild sockets" ) );
		menu.Connect( wxID_QUESTEDITOR_REBUILDSOCKETS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRebuildSockets ), NULL, this );
	}

	CEdGraphEditor::InitLinkedBlockContextMenu( block, menu );
}

void CEdQuestGraphEditor::OnToolMenu( wxCommandEvent& event )
{
	if ( IToolCommandExecutor *command = dynamic_cast< IToolCommandExecutor* >( event.m_callbackUserData ) )
	{
		command->Execute();
	}
}

void CEdQuestGraphEditor::AddScopeResourceManagementMenuEntries( CQuestScopeBlock* block, wxMenu& menu )
{
	if ( block->CanConvertToResource() )
	{
		if ( block->IsEmbeddedGraph() )
		{
			menu.Append( wxID_QUESTEDITOR_CONVERTORESOURCE, wxT( "Convert to resource" ) );
			menu.Connect( wxID_QUESTEDITOR_CONVERTORESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnConvertToResource ), NULL, this );
		}
		else
		{
			menu.Append( wxID_QUESTEDITOR_EMBEDFROMRESOURCE, wxT( "Embed graph from resource" ) );
			menu.Connect( wxID_QUESTEDITOR_EMBEDFROMRESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnEmbedGraphFromResource ), NULL, this );
		}

		menu.AppendSeparator();
	}
}

void CEdQuestGraphEditor::AddVersionControlMenuEntries( CQuestScopeBlock* block, wxMenu& menu )
{
	if ( block->IsResource() == false )
	{
		return;
	}

	CDiskFile* file = block->GetPhase()->GetFile();
	ASSERT( file );
	if ( file == NULL )
	{
		ERR_EDITOR( TXT( "Quest Editor error: miscreated resource" ) );
		return;
	}

	Bool vcOptionDisplayed = true;

	if ( file->IsCheckedOut() || file->IsAdded() )
	{
		menu.Append( wxID_QUESTEDITOR_SUBMIT, wxT( "Submit" ) );
		menu.Connect( wxID_QUESTEDITOR_SUBMIT, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdQuestGraphEditor::OnVersionControlCommand ), 
			new CVCSubmit( *file ), 
			this );
	}
	else if ( file->IsCheckedIn() )
	{
		menu.Append( wxID_QUESTEDITOR_CHECKOUT, wxT( "Checkout" ) );
		menu.Connect( wxID_QUESTEDITOR_CHECKOUT, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdQuestGraphEditor::OnVersionControlCommand ), 
			new CVCCheckout( *file ),
			this );
	}
	else
	{
		vcOptionDisplayed = false;
	}

	if ( vcOptionDisplayed )
	{
		menu.AppendSeparator();
	}
}

void CEdQuestGraphEditor::InitLinkedSocketContextMenu( CGraphSocket *socket, wxMenu &menu )
{
	m_mousePosition = wxGetMousePosition();
	CEdGraphEditor::InitLinkedSocketContextMenu( socket, menu );

	// append menus from tools
	CQuestGraphSocket* questSocket = Cast< CQuestGraphSocket >( socket );
	if ( !questSocket )
	{
		return;
	}
	CQuestGraphBlock *questBlock = Cast< CQuestGraphBlock >( socket->GetBlock() );
	if ( !questBlock )
	{
		return;
	}

	TDynArray< SToolMenu > subMenus;
	m_editor->OnCreateBlockContextMenu( subMenus, Cast<CQuestGraphBlock>( socket->GetBlock() ) );
	Uint32 count = subMenus.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( ( questSocket->GetDirection() == LSD_Input ) && ( subMenus[i].activationType & SToolMenu::AT_IN_SOCKET ) )
		{
			menu.Append( wxID_QUESTEDITOR_TOOLSMENUS + i, subMenus[ i ].name.AsChar() );
			menu.Connect( wxID_QUESTEDITOR_TOOLSMENUS + i, 
				wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler( CEdQuestGraphEditor::OnToolMenu ),
				new CToolSocketCommandExecutor( questBlock, questSocket, subMenus[ i ].command ),
				this );
		}
		else if ( ( questSocket->GetDirection() == LSD_Output ) && ( subMenus[i].activationType & SToolMenu::AT_OUT_SOCKET )  )
		{
			menu.Append( wxID_QUESTEDITOR_TOOLSMENUS + i, subMenus[ i ].name.AsChar() );
			menu.Connect( wxID_QUESTEDITOR_TOOLSMENUS + i, 
				wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler( CEdQuestGraphEditor::OnToolMenu ),
				new CToolSocketCommandExecutor( questBlock, questSocket, subMenus[ i ].command ),
				this );
		}
	}
}

void CEdQuestGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	if ( m_phasesStack.Empty() ) { return; }

	m_mousePosition = wxGetMousePosition();
	CQuestGraph *currentGraph = m_phasesStack.Back();

	wxMenu* subMenu = &menu;

	static TDynArray< CClass* >	blockClasses;
	blockClasses.ClearFast();
	CEdQuestEditor::GetEditableBlockClasses( currentGraph, blockClasses );

	TSortedMap< String, wxMenu* >	groups;
	Uint32 blockIdx = 0;
	for ( TDynArray< CClass* >::const_iterator it = blockClasses.Begin(); it != blockClasses.End(); ++it )
	{
		CGraphBlock* defaultBlock = (*it)->GetDefaultObject< CGraphBlock >();

		wxMenu* menuGroup = NULL;
		String groupId = defaultBlock->GetBlockCategory();
		if ( groupId == String::EMPTY )
		{
			menuGroup = &menu;
		}
		else
		{
			TSortedMap< String, wxMenu* >::iterator groupIt = groups.Find( groupId );
			if ( groupIt == groups.End() )
			{
				menuGroup = new wxMenu();
				groups.Insert( groupId, menuGroup );
			}
			else
			{
				menuGroup = groupIt->m_second;
			}
		}

		String blockName = defaultBlock->GetBlockName();
		menuGroup->Append( wxID_QUESTEDITOR_ADDBLOCK + blockIdx, blockName.AsChar() );
		menu.Connect( wxID_QUESTEDITOR_ADDBLOCK + blockIdx, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdQuestGraphEditor::OnAddQuestBlock ), 
			new CQuestObjectWrapper( *it, m_mousePosition ), 
			this );

		++blockIdx;
	}

	for ( TSortedMap< String, wxMenu* >::const_iterator it = groups.Begin(); it != groups.End(); ++it )
	{
		subMenu->Append( wxID_ANY, it->m_first.AsChar(), it->m_second );
	}

	menu.AppendSeparator();

	// Paste menu
	Bool canPaste = false;
	if ( wxTheClipboard->Open() )
	{
		CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
		canPaste = wxTheClipboard->IsSupported( data.GetDataFormat() );
		wxTheClipboard->Close();
	}
	menu.Append( wxID_QUESTEDITOR_PASTEHERE, wxT( "Paste here" ) );
	menu.Connect( wxID_QUESTEDITOR_PASTEHERE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnPasteHere ), NULL, this );
	menu.Enable( wxID_QUESTEDITOR_PASTEHERE, canPaste );

	TDynArray<CGraphBlock *> selectedBlocks;
	GetSelectedBlocks( selectedBlocks );	
	menu.Append( wxID_QUESTEDITOR_REMOVEBLOCK, wxT( "Remove selected" ) );
	menu.Connect( wxID_QUESTEDITOR_REMOVEBLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdQuestGraphEditor::OnRemoveSelectedBlocks ), NULL, this );
	menu.Enable( wxID_QUESTEDITOR_REMOVEBLOCK, selectedBlocks.Empty() ? false : true );
}

void CEdQuestGraphEditor::OnAddQuestBlock( wxCommandEvent& event )
{
	if ( ModifyGraphStructure() )
	{
		if ( CQuestObjectWrapper *questObjectWrapper = dynamic_cast< CQuestObjectWrapper * >( event.m_callbackUserData ) )
		{
			// Create new step
			CGraphBlock* newBlock = SpawnBlock( questObjectWrapper->GetClass(), ScreenToClient( m_mousePosition ) );

			if ( newBlock )
			{
				if ( newBlock->IsA< CQuestTestBlock >() )
				{
					m_editor->ShowTestQuestWarning();
				}
				if ( newBlock->IsA< CQuestPhaseInputBlock >() || newBlock->IsA< CQuestPhaseOutputBlock >() )
				{
					m_editor->RepairDuplicateInputs();
				}
			}
		}
		
		GraphStructureModified();
	}
}

Bool CEdQuestGraphEditor::ShouldUseDeletionMarkers( const String& depotPath )
{
	if ( depotPath.BeginsWith( TXT("qa") ) )
	{
		// don't use deletion markers in qa folder
		return false;
	}

	if ( depotPath.BeginsWith( TXT("dlc") ) )
	{
		C2dArray* collisionTypes = resDlcWithEnabledDeletionMarkers.LoadAndGet< C2dArray >();
		if ( collisionTypes )
		{
			Uint32 rowCount = collisionTypes->GetNumberOfRows();
			for ( Uint32 row = 0; row < rowCount; ++row )
			{
				String rowValue = collisionTypes->GetValue( 0, row );
				if ( depotPath.BeginsWith( TXT("dlc\\") + rowValue ) )
				{
					return true;
				}
			}
		}
		//don't use deletion markers in dlc folder by default
		return false;
	}

	// if not in qa or dlc folder, we want to use deletion markers
	return true;
}

void CEdQuestGraphEditor::OnRemoveSelectedBlocks( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	//in current production stage deletion markers are unnecessary obstacle - they should be reintroduced when appropriate times
	String depotPath = m_editor->GetQuestPhase() ? m_editor->GetQuestPhase()->GetDepotPath() : String::EMPTY;
	Bool dontUseDeletionMarkers	= !ShouldUseDeletionMarkers( depotPath );

	// let's check all phases' files
	if ( dontUseDeletionMarkers )
	{
		for ( const CGraphBlock* gb : selectedBlocks )
		{
			if ( CQuestPhase* phase = gb->FindParent< CQuestPhase >() )
			{
				depotPath = phase->GetFile() ? phase->GetFile()->GetDepotPath() : String::EMPTY;
				if ( ShouldUseDeletionMarkers( depotPath ) )
				{
					dontUseDeletionMarkers = false;
					break;
				}
			}
		}
	}

	{
		// remove all the blocks that should be 'just removed', without being replaced with deletion markers
		TDynArray< CGraphBlock* > blocksToRemove = selectedBlocks;
		for ( Uint32 i=0; i<blocksToRemove.Size(); ++i )
		{
			CGraphBlock* block = blocksToRemove[i];

			if ( dontUseDeletionMarkers || block->IsA< CGraphHelperBlock >() )
			{
				CUndoGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, block );
				if ( GetGraph()->GraphRemoveBlock( block ) == false )
				{
					ERR_EDITOR( TXT("Unable to remove block %s"), block->GetCaption() );
				}
				// remove from the selection to skip it while analyzing later
				selectedBlocks.Remove( block );
			}
		}
		CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
	}

	// if we don't use selection markers, all selected block should be removed already
	ASSERT ( selectedBlocks.Empty() || !dontUseDeletionMarkers, TXT("Not all selected blocks are processed") );

	// We want to replace the selected blocks with blocks that will mark 
	// the fact that something was here and was deleted, so that
	// we can keep track of changes in the quest structure in terms of
	// being able to apply game patches.
	// Each replacer block has a single input and a single output, 
	// and can replace a selection of blocks that are:
	//  1.) connected together
	//  2.) have a single input and a single output
	// If that's not the case, we need to identify the chunks of blocks that
	// we can replace using a single block like that.


	// Analyze the selection in terms of orthogonal graphs
	CEdUndoManager::Transaction transactionScope( *m_undoManager, TXT("DeletingWithDeletionMarkers") );
	while( selectedBlocks.Size() > 0 )
	{
		CQuestDeletionMarkerBlock* subGraph = Cast< CQuestDeletionMarkerBlock >( SpawnBlock( CQuestDeletionMarkerBlock::GetStaticClass(), wxPoint( 0, 0 ) ) );

		// select one of the start blocks
		TDynArray< CGraphBlock* > blocksQueue;

		CGraphBlock* analyzedBlock = selectedBlocks.Back();
		blocksQueue.PushBack( analyzedBlock );

		// flood fill
		while ( blocksQueue.Empty() == false )
		{
			// store the block in the subgraph
			CQuestGraphBlock* block = Cast< CQuestGraphBlock >( blocksQueue.PopBack() );
			selectedBlocks.Remove( block );
			if ( !subGraph )
			{
				continue;
			}

			subGraph->AddBlock( block );

			// analyze the connections
			const TDynArray< CGraphSocket* >& sockets = block->GetSockets();
			for ( TDynArray< CGraphSocket* >::const_iterator it = sockets.Begin(); it != sockets.End(); ++it )
			{
				CGraphSocket* socket = *it;
				ELinkedSocketPlacement socketPlacement = socket->GetPlacement();
				if ( socketPlacement != LSP_Left && socketPlacement != LSP_Right )
				{
					continue;
				}

				const TDynArray< CGraphConnection* >& connections = socket->GetConnections();
				for ( TDynArray< CGraphConnection* >::const_iterator connIt = connections.Begin(); connIt != connections.End(); ++connIt )
				{
					if ( !(*connIt)->IsActive() )
					{
						continue;
					}

					CGraphSocket* destSocket = (*connIt)->GetDestination();
					CQuestGraphBlock* adjacentBlock = Cast< CQuestGraphBlock >( destSocket->GetBlock() );
					if ( !adjacentBlock )
					{
						continue;
					}

					Bool isBlockSelected = selectedBlocks.Exist( adjacentBlock );
					if ( isBlockSelected )
					{
						blocksQueue.PushBackUnique( adjacentBlock );
					}
					else
					{
						// this is an external block - reroute the connection to the graph
						subGraph->AddConnection( socket, *connIt );
					}
				}
			}

			// remove the analyzed block from the graph
			CUndoGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, block );
			if ( GetGraph()->GraphRemoveBlock( block ) == false)
			{
				ERR_EDITOR( TXT("Unable to remove block %s"), block->GetCaption() );
			}
			CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
		}
	}

	GraphStructureModified();
	DeselectAllBlocks();
}

void  CEdQuestGraphEditor::OnRebuildSockets( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( GetActiveItem( position ).Get() ) )
	{
		scopeBlock->OnRebuildSockets();
	}
}

void CEdQuestGraphEditor::OnConvertToResource( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestScopeBlock *scopeBlock = dynamic_cast<CQuestScopeBlock *>( GetActiveItem( position ).Get() ) )
	{
		scopeBlock->ConvertToResource();

		// Save newly created resource
		CQuestPhase *resource = scopeBlock->GetPhase();
		if ( !resource )
		{
			ERR_EDITOR( TXT("Error when converting a scope graph into a resource. No resource created.") );
			return;
		}

		SaveResource< CQuestPhase >( TXT("defaultPhase"), TXT( "Quest phase files" ), resource );
	}
}

void CEdQuestGraphEditor::OnEmbedGraphFromResource( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if (CQuestScopeBlock *scopeBlock = SafeCast< CQuestScopeBlock >( GetActiveItem( position ).Get() ) )
	{
		scopeBlock->EmbedGraphFromResource();
	}
}

void CEdQuestGraphEditor::OnVersionControlCommand( wxCommandEvent& event )
{
	IVersionControlCommand* command = dynamic_cast< IVersionControlCommand * >( event.m_callbackUserData );
	if ( command )
	{
		command->Execute();
	}
}

void CEdQuestGraphEditor::OnAddInput( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestVariedInputsBlock *block = SafeCast< CQuestVariedInputsBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphVariedInputBlock::CreateAddingStep( *m_undoManager, this, block );
		block->AddInput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnRemoveInput( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestVariedInputsBlock *block = SafeCast< CQuestVariedInputsBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphVariedInputBlock::CreateRemovingStep( *m_undoManager, this, block );
		block->RemoveInput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnAddOutput( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestRandomBlock *block = SafeCast< CQuestRandomBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphRandomBlockOutput::CreateAddingStep( *m_undoManager, this, block );
		block->AddOutput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnRemoveOutput( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestRandomBlock *block = SafeCast< CQuestRandomBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphRandomBlockOutput::CreateRemovingStep( *m_undoManager, this, block );
		block->RemoveOutput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnHandleBehaviorOutcome( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CBaseQuestScriptedActionsBlock *block = SafeCast< CBaseQuestScriptedActionsBlock >( GetActiveItem( position ).Get() ) )
	{
		block->SetHandleBahviorOutcome( true );
	}
}
void CEdQuestGraphEditor::OnDontHandleBehaviorOutcome( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CBaseQuestScriptedActionsBlock *block = SafeCast< CBaseQuestScriptedActionsBlock >( GetActiveItem( position ).Get() ) )
	{
		block->SetHandleBahviorOutcome( false );
	}
}

void CEdQuestGraphEditor::OnAddTerminationInputBlock( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestGraphBlock *block = SafeCast< CQuestGraphBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphBlockIO::CreateAddingStep( *m_undoManager, this, block, CUndoQuestGraphBlockIO::TERMINATION_INPUT );
		block->AddTerminationInput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnRemoveTerminationInputBlock( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestGraphBlock *block = SafeCast< CQuestGraphBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphBlockIO::CreateRemovingStep( *m_undoManager, this, block, CUndoQuestGraphBlockIO::TERMINATION_INPUT );
		block->RemoveTerminationInput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnAddPatchOutputBlock( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestGraphBlock *block = SafeCast< CQuestGraphBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphBlockIO::CreateAddingStep( *m_undoManager, this, block, CUndoQuestGraphBlockIO::PATCH_OUTPUT );
		block->AddPatchOutput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnRemovePatchOutputBlock( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestGraphBlock *block = SafeCast< CQuestGraphBlock >( GetActiveItem( position ).Get() ) )
	{
		CUndoQuestGraphBlockIO::CreateRemovingStep( *m_undoManager, this, block, CUndoQuestGraphBlockIO::PATCH_OUTPUT );
		block->RemovePatchOutput();
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnRunBlockSpecialOption( wxCommandEvent& event )
{
	wxPoint position = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	if ( CQuestGraphBlock *block = SafeCast< CQuestGraphBlock >( GetActiveItem( position ).Get() ) )
	{
		Int32 option = event.GetId() - wxID_QUESTEDITOR_RUNSPECIALOPTION_0;
		ASSERT( option >= 0 && option < QUESTEDITOR_MAX_SPECIAL_OPTIONS );
		block->RunSpecialOption( option );
		GraphStructureModified();
	}
}

void CEdQuestGraphEditor::OnSaveScopeBlock( wxCommandEvent& event )
{
	if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CQuestScopeBlock >() )
	{
		CQuestScopeBlock* scopeBlock = static_cast< CQuestScopeBlock* >( m_activeItem.Get() );

		CQuestPhase *questPhase = scopeBlock->GetPhase();
		if ( questPhase )
		{
			// Update GUIDs
			CheckAndUpdateGUIDs( true );

			// Get all included phasses
			TDynArray<CQuestPhase *> questPhases;
			questPhases.PushBack( questPhase );
			GetAllPhases( questPhase->GetGraph(), questPhases );
			ASSERT( questPhases.Exist( questPhase ) );

			// Save them all
			for ( Uint32 i = 0; i < questPhases.Size(); ++i )
			{
				questPhases[i]->Save();
			}
		}
	}
}

void CEdQuestGraphEditor::OnLeftClick( wxMouseEvent& event )
{
	event.Skip();
}

void CEdQuestGraphEditor::OnDoubleClick( wxMouseEvent& event )
{
	if ( event.m_leftDown )
	{
		wxPoint position = ClientToCanvas( event.GetPosition() );

		ISerializable* selectedItem = GetActiveItem( position ).Get();
		if ( selectedItem && selectedItem->IsA< CQuestGraphBlock >() )
		{
			CQuestGraphBlock* block = SafeCast< CQuestGraphBlock >( selectedItem );
			OpenBlock( block );


			IQuestBlockWithScene* sceneBlock = dynamic_cast< IQuestBlockWithScene* >( block );
			if ( sceneBlock )
			{
				// clicking on a quest scene block invokes the scene editor
				EditScene( sceneBlock );
			}
		}
		else
		{
			OpenBlock( NULL );
		}
	}
	
	event.Skip();
}

void CEdQuestGraphEditor::OnPasteHere( wxCommandEvent& event )
{
	wxPoint point = ClientToCanvas( ScreenToClient( m_mousePosition ) );
	Vector pos( point.x, point.y, 0 );

	Paste( &pos, true );
}

Bool CEdQuestGraphEditor::OpenBlock( const CGUID& blockGuid, Bool enterPhase )
{
	for ( CGraphBlock* block : GetGraph()->GraphGetBlocks() )
	{
		CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block );
		if ( questBlock )
		{
			if ( questBlock->GetGUID() == blockGuid )
			{
				return OpenBlock( questBlock, enterPhase );
			}
		}
	}
	return false;
}

Bool CEdQuestGraphEditor::OpenBlock( CQuestGraphBlock* block, Bool enterPhase )
{
	Bool ret = false;

	if ( block != NULL )
	{
		if ( block->IsA< CQuestScopeBlock >() && enterPhase )
		{
			// clicking on a quest phase block makes the editor display
			// the graph of that phase
			CQuestGraph * subGraph = SafeCast< CQuestScopeBlock >( block )->GetGraph();
			ASSERT( subGraph );
			if ( subGraph )
			{
				PushGraph( subGraph );
				ret = true;
			}
		}
		else
		{
			FocusOnBlock( block );
		}
	}
	else
	{
		// clicking somewhere where there are no blocks makes the editor
		// go one level up in the displayed graphs hierarchies
		PopGraph( );
	}

	// inform the editor about the graph we're viewing, so that it can pass
	// the news on to the tools
	if ( m_editor && m_phasesStack.Size() > 0 )
	{
		CQuestGraph* viewedGraph = m_phasesStack.Back();

		// notify that the viewed graph has changed
		m_editor->OnGraphSet( *viewedGraph );
	}

	return ret;
}

void CEdQuestGraphEditor::OpenBlock( TDynArray< CQuestGraphBlock* >& stack )
{
	// go to the topmost graph
	while ( PopGraph() != NULL ) {}

	if ( stack.Empty() )
	{
		return;
	}

	// start going down
	Uint32 count = stack.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		OpenBlock( stack[ i ], i < ( count - 1 ) );
	}
}

void CEdQuestGraphEditor::EditScene( IQuestBlockWithScene* sceneBlock )
{
	ASSERT( sceneBlock );
	if ( sceneBlock == NULL )
	{
		return;
	}

	CStoryScene* scene = sceneBlock->GetScene();
	if ( scene == NULL )
	{
		scene = CreateObject< CStoryScene >();
		Bool result = SaveResource< CStoryScene >( TXT("defaultDialog"), TXT( "Dialog files" ), scene );

		if ( result )
		{
			sceneBlock->SetScene( scene );
		}
		else
		{
			ERR_EDITOR( TXT( "Could not create a new dialog resource." ) );
			scene->Discard();
			scene = NULL;
		}
	}

	if ( scene )
	{
		if ( StorySceneEditorUtils::StartStorySceneDebug( scene ) )
		{
			return;
		}
		
		wxTheFrame->GetAssetBrowser()->OpenFile( scene->GetFile()->GetDepotPath() );
	}
}

void CEdQuestGraphEditor::LoadKeyboardShortcuts()
{
	C2dArray* list = LoadResource< C2dArray >( EDITOR_QUEST_SHORTCUTS_CSV );

	if ( list )
	{
		for ( Uint32 i = 0; i < list->GetNumberOfRows(); ++i )
		{
			String keyCodeName		= list->GetValue( TXT("KeyCode"), i );
			String blockClass1Name	= list->GetValue( TXT("BlockClass"), i );
			String blockClass2Name	= list->GetValue( TXT("AltBlockClass"), i );

			if ( keyCodeName.GetLength() != 1 )
			{
				continue;
			}
			Char keyCode = keyCodeName[ 0 ];
			if ( !iswascii( keyCode ) )
			{
				continue;
			}
			CClass* blockClass1 = SRTTI::GetInstance().FindClass( CName( blockClass1Name ) );
			if ( !blockClass1 )
			{
				continue;
			}
			CClass* blockClass2 = nullptr;
			if ( !blockClass2Name.Empty() )
			{
				// alternative class is optional
				blockClass2 = SRTTI::GetInstance().FindClass( CName( blockClass2Name ) );
			}
			m_shortcuts[ keyCode ] = TPair< CClass*, CClass* >( blockClass1, blockClass2 );
		}
	}
}

void CEdQuestGraphEditor::OnEditorClosed( wxWindowDestroyEvent& event )
{
	// Continue evaluating this event at higher level
	event.Skip();
}

void CEdQuestGraphEditor::OnKeyDown( wxKeyEvent& event )
{
	Int32 keyCode = event.GetKeyCode();
	if ( keyCode == WXK_DELETE )
	{
		wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
		commandEvent.SetId( wxID_QUESTEDITOR_REMOVEBLOCK );
		OnRemoveSelectedBlocks( commandEvent );
	}	
	else if ( keyCode == WXK_BACK )
	{
		OpenBlock( nullptr );
	}

	if ( iswascii( keyCode ) )
	{
		if ( m_action == MA_None )
		{
			// register pressed key only if there's no ongoing action
			m_pressedKeyCode = keyCode;
		}
	}

	CEdGraphEditor::OnKeyDown( event );
}

void CEdQuestGraphEditor::OnKeyUp( wxKeyEvent& event )
{
	Int32 keyCode = event.GetKeyCode();
	if ( iswascii( keyCode ) )
	{
		m_pressedKeyCode = -1;
	}
}

void CEdQuestGraphEditor::OnKillFocus( wxFocusEvent& event )
{
	m_pressedKeyCode = -1;
}

void CEdQuestGraphEditor::PushGraph( CQuestGraph* graph )
{
	if ( ( m_phasesStack.Size() > 0 ) && ( graph == m_phasesStack.Back() ) )
	{
		// don't allow to stack the same graph over and over again
		return;
	}

	m_phasesStack.PushBack( graph );
	CUndoQuestGraphPushPop::CreatePushingStep( *m_undoManager, this, &m_phasesStack );
	SetGraph( graph );
	ScrollBackgroundOffset( wxPoint(0, 0) );	// refresh offset
	CheckBlocksVisibility();
}

CQuestGraph* CEdQuestGraphEditor::PopGraph()
{
	if ( m_phasesStack.Size() <= 1 )
	{
		// there always has to be at least one graph remaining on the stack
		return NULL;
	}

	CUndoQuestGraphPushPop::CreatePoppingStep( *m_undoManager, this, &m_phasesStack );
	CQuestGraph* lastViewedGraph = m_phasesStack.PopBack();
	CQuestGraph* currentlyViewedGraph = m_phasesStack.Back();
	SetGraph( m_phasesStack.Back() );
	m_shouldZoom = false; // avoid zooming
	CheckBlocksVisibility();
	
	// focus on the block we exited
	auto blocks = currentlyViewedGraph->GraphGetBlocks();
	for( auto block : blocks )
	{
		CQuestScopeBlock* scopeBlock = dynamic_cast<CQuestScopeBlock*>( block );
		if( scopeBlock != nullptr && scopeBlock->GetGraph() == lastViewedGraph )
		{
			FocusOnBlock( scopeBlock );
		}
	}

	return lastViewedGraph;
}

CQuestScopeBlock *CEdQuestGraphEditor::GetParentScopeBlock( CQuestGraph *graph )
{
	if ( graph )
	{
		// Check if graph is stored in a phase block
		if ( CQuestScopeBlock *scopeBlock = dynamic_cast<CQuestScopeBlock *>( graph->GetParent() ) )
		{
			return scopeBlock;
		}
	}

	return NULL;
}

void CEdQuestGraphEditor::AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth )
{
	IQuestDebugInfo* debugInfo = NULL;
	if ( m_editor )
	{
		debugInfo = m_editor->GetDebugInfo();
	}

	if ( !debugInfo )
	{
		return;
	}

	CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block );

	if ( !questBlock )
	{
		return;
	}

	if ( debugInfo->IsBreakpointToggled( questBlock ) )
	{
		borderColor = Color::RED;
		borderWidth = 5.0f;
		return;
	}

	if ( debugInfo->IsBlockActive( questBlock ) )
	{
		borderColor = Color::YELLOW;
		borderWidth = 3.0f;
		return;
	}

	if ( debugInfo->IsBlockVisited( questBlock ) )
	{
		if ( !IsBlockSelected( questBlock ) )
		{
			borderColor = Color::DARK_YELLOW;
			borderWidth = 3.0f;
			return;
		}
	}
}

void CEdQuestGraphEditor::DrawBlockLayout( CGraphBlock* block )
{
	CEdGraphEditor::DrawBlockLayout( block );

	CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block );
	if ( questBlock == NULL )
	{
		return;
	}

	// Find layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( block );
	if ( layout )
	{
		// Calculate window offset
		wxPoint location( block->GetPosition().X, block->GetPosition().Y );
		wxPoint corner( location + layout->m_windowSize );

		Uint32 yOffset = 0;

		String altName = questBlock->GetBlockAltName();
		if ( !altName.Empty() )
		{
			wxColour borderColor( 0, 0, 0 );
			wxColour commentColor( 255, 255, 255 );
			wxPoint textSize = TextExtents( *m_boldFont, altName );

			Uint32 textX = ( questBlock->GetBlockShape() == GBS_LargeCircle || questBlock->GetBlockShape() == GBS_DoubleCircle ) ? ( location.x + 24 - ( textSize.x / 2 ) ) : ( location.x + 5 );

			yOffset = -textSize.y - 2;
			if ( questBlock->GetBlockShape() == GBS_Triangle )
			{
				yOffset -= textSize.y + 2;
			}
			else if ( ( questBlock->GetBlockShape() == GBS_Octagon ) || 
				( questBlock->GetBlockShape() == GBS_Arrow ) )
			{
				yOffset -= 2 * textSize.y + 2;
			}

			Uint32 textY = location.y + yOffset;

			DrawText( wxPoint( textX, textY ), *m_boldFont, altName, borderColor );
			DrawText( wxPoint( textX - 1, textY - 1 ), *m_boldFont, altName, commentColor );

		}

		// Draw comment
		String comment = questBlock->GetComment();
		if ( comment.Empty() == false )
		{
			wxColour borderColor( 64,64,128 );
			wxColour commentColor( 128, 128, 255 );
			wxPoint textSize = TextExtents( *m_boldFont, comment );
			
			Uint32 textX = ( questBlock->GetBlockShape() == GBS_LargeCircle || questBlock->GetBlockShape() == GBS_DoubleCircle ) ? ( location.x + 24 - ( textSize.x / 2 ) ) : ( location.x + 5 );

			Uint32 textY = location.y - textSize.y - 2 + yOffset;
			if ( questBlock->GetBlockShape() == GBS_Triangle )
			{
				textY -= textSize.y + 2;
			}
			else if ( ( questBlock->GetBlockShape() == GBS_Octagon ) || 
				( questBlock->GetBlockShape() == GBS_Arrow ) )
			{
				textY -= 2 * textSize.y + 2;
			}
			else if ( ( questBlock->GetBlockShape() == GBS_DoubleCircle ) ||
				( questBlock->GetBlockShape() == GBS_LargeCircle ) )
			{
				textY -= textSize.y + 5;
			}

			DrawText( wxPoint( textX, textY ), *m_boldFont, comment, borderColor );
			DrawText( wxPoint( textX - 1, textY - 1 ), *m_boldFont, comment, commentColor );
		}
	}
}

void CEdQuestGraphEditor::AnnotateConnection( const CGraphConnection* con, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width )
{
	CEdGraphEditor::AnnotateConnection( con, src, srcDir, dest, destDir, width );

	if ( !m_showFlowSequence || !con || !con->IsActive() )
	{
		return;
	}

	const CGraphSocket* startSocket = con->GetSource();
	if ( !startSocket )
	{
		return;
	}

	// check the index of the connection, not taking into account the inactive links

	const TDynArray< CGraphConnection* >& connections = startSocket->GetConnections();
	Int32 index = -1;
	Uint32 count = connections.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( connections[ i ]->IsActive() )
		{
			++index;
		}

		if ( connections[ i ] == con )
		{
			break;
		}
	}

	// draw the index on the link
	wxPoint midPoint = CalculateLinkMidpoint( src, srcDir, dest, destDir, 1.f );

	static const Char* strNum[] = { TXT("1"), TXT("2"), TXT("3"), TXT("4"), 
		TXT("5"), TXT("6"), TXT("7"), TXT("8"), TXT("9"), TXT("10"), TXT("11"), 
		TXT("12"),TXT("13"), TXT("14"), TXT("15"), TXT("16") };
	static wxColour white( 255, 255, 255 );
	if ( index < 16 )
	{
		DrawText( midPoint, GetGdiDrawFont(), strNum[ index ], white );
	}
	else
	{
		DrawText( midPoint, GetGdiDrawFont(), String::Printf( TXT( "%d" ), index ).AsChar(), white );
	}
}

void CEdQuestGraphEditor::MouseClick( wxMouseEvent& event )
{
	if ( !iswascii( m_pressedKeyCode ) )
	{
		CEdGraphEditor::MouseClick( event );
	}
	else
	{
		CEdCanvas::MouseClick( event );

		if ( !event.LeftDown() )
		{
			return;
		}
		THashMap< Char, TPair< CClass*, CClass* > >::const_iterator shortcut = m_shortcuts.Find( m_pressedKeyCode );
		if ( shortcut == m_shortcuts.End() )
		{
			return;
		}
		CQuestGraph *currentGraph = m_phasesStack.Back();
		if ( !currentGraph )
		{
			return;
		}

		TStaticArray< CClass*, 2 > m_classes;
		m_classes.PushBack( shortcut->m_second.m_first );
		m_classes.PushBack( shortcut->m_second.m_second );

		for ( Uint32 i = 0; i < m_classes.Size(); i++ )
		{
			if ( !m_classes[ i ] )
			{
				continue;
			}

			if ( m_classes[ i ] != ClassID< CDescriptionGraphBlock >() &&
				 m_classes[ i ] != ClassID< CCommentGraphBlock >() )
			{
				// do not check these conditions for CDescriptionGraphBlock & CCommentGraphBlock because they don't inherit from CQuestGraphBlock (see CEdQuestEditor::GetEditableBlockClasses)
				CQuestGraphBlock* defaultBlock = m_classes[ i ]->GetDefaultObject< CQuestGraphBlock >();
				if ( !defaultBlock )
				{
					continue;
				}
				if ( !defaultBlock->CanBeAddedToGraph( currentGraph ) )
				{
					continue;
				}
			}
			if ( ModifyGraphStructure() )
			{
				SpawnBlock( m_classes[ i ], ScreenToClient( wxGetMousePosition() ) );
				GraphStructureModified();
				break;
			}
		}
	}
}

void CEdQuestGraphEditor::ToggleFlowSequence()
{
	m_showFlowSequence = !m_showFlowSequence;
}

void CEdQuestGraphEditor::FindBlock( CQuestGraphBlock* block, TDynArray< CQuestGraphBlock* >& stack ) const
{
	if ( m_phasesStack.Empty() )
	{
		return;
	}

	struct SData
	{
		CQuestGraph* graph;
		TDynArray< CQuestGraphBlock* > stack;

		SData( CQuestGraph* _graph = NULL, 
			const TDynArray< CQuestGraphBlock* >& _stack = TDynArray< CQuestGraphBlock* >() ) 
			: graph( _graph )
			, stack( _stack ) 
		{}
	};

	stack.Clear();

	TDynArray< SData > graphsStack;
	graphsStack.PushBack( SData( m_phasesStack[ 0 ] ) );

	// analyze the graph
	while ( !graphsStack.Empty() )
	{
		SData currGraph = graphsStack.PopBack();

		TDynArray< CGraphBlock* >& blocks = currGraph.graph->GraphGetBlocks();
		Uint32 count = blocks.Size();

		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( blocks[ i ] == block )
			{
				stack = currGraph.stack;
				stack.PushBack( block );
				return;
			}

			if ( blocks[ i ]->IsA< CQuestScopeBlock >() )
			{
				CQuestScopeBlock* scope = Cast< CQuestScopeBlock >( blocks[ i ] );
				graphsStack.PushBack( SData( scope->GetGraph(), currGraph.stack ) );
				graphsStack.Back().stack.PushBack( scope );
			}
		}
	}
}

void CEdQuestGraphEditor::FindInGraph()
{
	CQuestGraphSearcher* searcher = new CQuestGraphSearcher( *this );
	searcher->Show();
}

void CollectBlocksRecursive( CQuestGraph* graph, TDynArray< CQuestGraphBlock* >& outBlocks )
{
	// Graph should be valid
	if ( graph )
	{
		// Collect blocks from graph
		TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
		for ( Uint32 i=0; i<blocks.Size(); ++i )
		{
			CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( blocks[ i ] );
			if ( questBlock && !outBlocks.Exist( questBlock ) )
			{
				// Add to list
				outBlocks.PushBack( questBlock );

				// Recurse to sub graph
				if ( questBlock->IsA< CQuestScopeBlock >() )
				{
					CQuestScopeBlock* scope = static_cast< CQuestScopeBlock* >( questBlock );
					CollectBlocksRecursive( scope->GetGraph(), outBlocks );
				}
			}
		}
	}
}

void CollectBlocksFiles( const TDynArray< CQuestGraphBlock* >& blocks, TDynArray< CDiskFile* >& outFiles )
{
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		CQuestPhase* phase = blocks[i]->FindParent< CQuestPhase >();
		if ( phase && phase->GetFile() )
		{
			outFiles.PushBackUnique( phase->GetFile() );			
		}
	}
}

namespace 
{
	String GetDuplicatedGUIDsMessage( const CQuestGraph::DuplicateMap& duplicates )
	{
		String msg;
		for ( const auto& entry : duplicates )
		{
			if ( entry.m_second.Size() > 1 )
			{
				msg += TXT("Blocks sharing GUID: ") + ToString( entry.m_first ) + TXT("\n");
				for ( const CQuestGraph::GUIDCheckEntry& e : entry.m_second )
				{
					String scopePath;
					for ( const CQuestScopeBlock* scopeBlock : e.m_scope )
					{
						scopePath += scopeBlock->GetBlockName() + TXT("/");
					}
					msg += TXT("   ") + scopePath + e.m_block->GetBlockName() + TXT("\n");
				}
				msg += TXT("\n");
			}
		}

		return msg;
	}
}

void CEdQuestGraphEditor::CheckAndUpdateGUIDs( Bool performUpdate )
{
	// Get all blocks
	Bool duplicateFound = false;
	CQuestGraph::DuplicateMap duplicatedGUIDs;
	if ( !m_phasesStack.Empty() && m_phasesStack[0] != NULL )
	{
		m_phasesStack[0]->CheckGUIDs( duplicatedGUIDs );

		for( auto guidIt = duplicatedGUIDs.Begin(); guidIt != duplicatedGUIDs.End(); ++guidIt )
		{
			if( guidIt.Value().Size() > 1 )
			{
				duplicateFound = true;
				break;
			}
		}
	}

	if ( duplicateFound )
	{
		String msg = TXT("Quest blocks have DUPLICATED GUIDS and are invalid.\n\n") + GetDuplicatedGUIDsMessage( duplicatedGUIDs );
		GFeedback->ShowError( msg.AsChar() );

		if ( performUpdate )
		{
			// Duplicated shit
			if ( GFeedback->AskYesNo( TXT("Duplicate GUIDs prevent save games from working with this quest. Regenerate GUIDs?") ) )
			{
				UpdateGUIDs();
			}
		}
	}
}

void CEdQuestGraphEditor::UpdateGUIDs()
{
	// Make sure user knows what he's doing
	if ( wxNO == wxMessageBox( wxT("This will update all GUIDs for all blocks in this quest and all linked quests. Files that were changed will be left opened. After this operation all previous save games will stop working. Are you sure ?"), wxT("Update Quest GUID"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT ) )
	{
		return;
	}

	// Get all blocks
	TDynArray< CQuestGraphBlock* > blocks;
	CollectBlocksRecursive( m_phasesStack[0], blocks );

	// Get files from blocks
	TDynArray< CDiskFile* > filesFromBlocks;
	CollectBlocksFiles( blocks, filesFromBlocks );

	// HACK!!! blocking quest guids regeneration for r4data/quests as it would break player's save from before given patch (it's not blocked for dlcs though)
	if ( ( m_editor->GetQuestPhase() && m_editor->GetQuestPhase()->GetDepotPath().BeginsWith( TXT("quests") ) ) 
		|| FindIf( filesFromBlocks.Begin(), filesFromBlocks.End(), []( CDiskFile* df ) { return df->GetDepotPath().BeginsWith( TXT("quests") ); } ) != filesFromBlocks.End() )
	{
		wxMessageBox( wxT("Updating all GUIDs for all blocks in this quest and all linked quests is blocked for r4data/quests as it will break game saves."), wxT("Update Quest GUID"), wxICON_INFORMATION | wxOK );
		return;
	}

	// Checkout files
	if ( !filesFromBlocks.Empty() )
	{
		// Make sure user knows what he's doing
		String msg = String::Printf( TEXT("%i file(s) need to be checked out. Proceed ?"), filesFromBlocks.Size() );
		if ( wxNO == wxMessageBox( msg.AsChar(), wxT("Update Quest GUID"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT ) )
		{
			return;
		}

		// Checkout files
		for ( Uint32 i=0; i<filesFromBlocks.Size(); i++ )
		{
			CDiskFile* file = filesFromBlocks[i];
			if ( !file->MarkModified() )
			{
				String msg = String::Printf( TEXT("File '%s' was not checked out. Unable to update quest GUIDS."), file->GetDepotPath().AsChar() );
				wxMessageBox( msg.AsChar(), wxT("Update Quest GUID"), wxICON_ERROR | wxOK );
				return;
			}
		}
	}

	// Update blocks
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		CQuestGraphBlock* block = blocks[i];
		block->UpdateGUID();
	}

	// Save all files
	for ( Uint32 i=0; i<filesFromBlocks.Size(); i++ )
	{
		CDiskFile* file = filesFromBlocks[i];
		file->Save();
	}

	// Stats
	String msg = String::Printf( TEXT("Quest GUIDs were updated for %i block(s) in %i file(s). Make sure you submit them all. Please!"), blocks.Size(), filesFromBlocks.Size() );
	wxMessageBox( msg.AsChar(), wxT("Update Quest GUID"), wxICON_INFORMATION | wxOK );
}

void CEdQuestGraphEditor::OnUpdateCommunityGUIDs()
{
	// find all spawnsets
	TDynArray< CDiskFile* > foundFiles;
	GDepot->Search( TXT(".w2comm"), foundFiles );

	// analyze them one by one, memorizing those that contain the phase name we're looking for
	for ( TDynArray< CDiskFile* >::iterator it = foundFiles.Begin(); it != foundFiles.End(); ++it )
	{
		CDiskFile* file = *it;
		if ( !file->IsLoaded() )
		{
			ResourceLoadingContext context;
			file->Load( context );
		}

		CCommunity* spawnset = Cast< CCommunity >( file->GetResource() );
		if ( spawnset )
		{
			spawnset->UpdateGUIDS();
			file->MarkModified();
			file->Save();
		}
	}
}

void CEdQuestGraphEditor::OnUpgrade()
{
}

void CEdQuestGraphEditor::CreateDepFromGraph()
{
	CQuestGraph* cGraph = m_phasesStack[0];
	TDynArray< CQuestGraphBlock* > blocks;
	CollectBlocksRecursive( m_phasesStack[0], blocks );

	String outputFolder = GFileManager->GetDataDirectory() +TXT("dep_files\\");
	QuestDependencyInfo::Start( cGraph, blocks, outputFolder );
}

void CEdQuestGraphEditor::GetUnusedBlocks( TDynArray< CQuestGraphBlock* >& unusedBlocks )
{
	TDynArray< CQuestGraphBlock* > blocks;
	CollectBlocksRecursive( m_phasesStack[0], blocks );

	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		CQuestGraphBlock* block = blocks[i];
		TDynArray< CGraphSocket* > socks;
		socks = block->GetSockets();
		Bool allEmpty = true;
		for ( Uint32 i = 0; i < socks.Size(); ++i )
		{
			if ( socks[i]->GetDirection() == LSD_Input )
			{
				TDynArray< CGraphConnection* > conns;
				conns = socks[i]->GetConnections();
				if ( !conns.Empty() )
				{
					allEmpty = false;
					break;
				}
			}
		}
		if (allEmpty)
		{
			if ( block->IsExactlyA< CQuestPhaseInputBlock >() || block->GetBlockName() == TXT("Start") || block->GetBlockName() == TXT("Out") || block->GetBlockName() == TXT("End") )
			{
				continue;
			}
			else
			{
				unusedBlocks.PushBackUnique( block );
			}
		}
	}
}

void CEdQuestGraphEditor::DeleteUnusedBlocks()
{
	if ( GFeedback->AskYesNo( TXT("Do you want to Delete Unused Blocks?") ) )
	{
		TDynArray< CQuestGraphBlock* > unusedBlocks;
		GFeedback->BeginTask( TXT("Deleting Blocks"), false);
		
		Uint32 deletedTotalCount = 0;
		Bool deletedSomething = true;
		while ( deletedSomething )
		{
			Uint32 lastCount = deletedTotalCount;
			GetUnusedBlocks( unusedBlocks );

			TDynArray< CDiskFile* > filesFromBlocks;
			CollectBlocksFiles( unusedBlocks, filesFromBlocks );

			if ( m_editor->GetQuestPhase() && m_editor->GetQuestPhase()->GetDepotPath().BeginsWith( TXT("quests") ) 
				|| FindIf( filesFromBlocks.Begin(), filesFromBlocks.End(), []( CDiskFile* df ) { return df->GetDepotPath().BeginsWith( TXT("quests") ); } ) != filesFromBlocks.End() )
			{
				wxMessageBox( wxT("Deleting unused blocks blocked for r4data/quests to prevent problems with saves made before given patch"), wxT("Deleting unused blocks"), wxICON_INFORMATION | wxOK );
				break;
			}

			for ( CQuestGraphBlock* block : unusedBlocks )
			{
				if ( DeleteUnusedBlock( block ) )
				{
					++deletedTotalCount;
				}
			}
			if ( lastCount == deletedTotalCount )
			{
				deletedSomething = false;
			}
		}
		
		GFeedback->EndTask();
		GFeedback->ShowMsg( TXT("Deleted Nodes"), TXT("You have deleted %i Nodes. Look in the log to see the names of them."), deletedTotalCount );	
		m_checkedForErrors = false;
	}
}

Bool CEdQuestGraphEditor::DeleteUnusedBlock( CQuestGraphBlock* block )
{
	// If it is the CQuestPhaseInputBlock,, Start or End or Out then we don't remove it.
	if ( block->IsExactlyA< CQuestPhaseInputBlock >() || block->GetBlockName() == TXT("Start") || block->GetBlockName() == TXT("Out") || block->GetBlockName() == TXT("End") )
	{
		return false;
	}
	CQuestGraph *qg = Cast<CQuestGraph>( block->GetParent() );
	if (qg)
	{
		if( qg->GraphRemoveBlock( block ) )					
		{
			WARN_EDITOR( TXT("node_removing %s"), block->GetCaption().AsChar());
			return true;
		}	
	}
	return false;
}

Uint32 CEdQuestGraphEditor::CountUnusedBlocks()
{	
	TDynArray< CQuestGraphBlock* > unusedBlocks;
	GFeedback->BeginTask( TXT("Counting Blocks"), false);
	GetUnusedBlocks( unusedBlocks );
	GFeedback->EndTask();
	m_checkedForErrors = true;
	return unusedBlocks.Size(); 
}

void CEdQuestGraphEditor::SetScaleAndOffset( Float scale, const wxPoint& offset )
{
	m_shouldZoom = false;

	SetScale( scale );
	m_desiredScale = scale;

	SetBackgroundOffset( offset );

	CheckBlocksVisibility();
	Repaint();
}