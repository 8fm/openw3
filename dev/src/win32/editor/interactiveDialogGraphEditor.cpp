#include "build.h"
#if 0
#include "interactiveDialogEditor.h"
#include "interactiveDialogGraphEditor.h"
#include "dialogEditorActions.h"

#include "../../games/r6/idResource.h"
#include "../../games/r6/idTopic.h"

#include "../../games/r6/idGraph.h"
#include "../../games/r6/idBasicBlocks.h"
#include "../../games/r6/idGraphBlockOutputTerminate.h"
#include "../../games/r6/idGraphBlockText.h"
#include "../../games/r6/idGraphBlockFlow.h"
#include "../../games/r6/idGraphBlockBranch.h"
#include "../../games/r6/idGraphBlockChoice.h"
#include "../../games/r6/idGraphBlockCondition.h"
#include "../../games/r6/idGraphBlockFact.h"
#include "../../games/r6/idGraphBlockComunicatorSwitch.h"
#include "../../games/r6/idGraphBlockRequestFocus.h"
#include "../../games/r6/idGraphBlockInteraction.h"
#include "../../games/r6/idGraphBlockEvents.h"
#include "../../games/r6/idGraphBlockConnector.h"
#include "../../games/r6/idGraphBlockCheckpoint.h"

enum
{
	wxID_IDEDITOR_ADDINPUT = 7900,	
	wxID_IDEDITOR_ADDOUTPUT,
	wxID_IDEDITOR_ADDOUTPUTTERMINATE,
	wxID_IDEDITOR_ADDSECTION,		
	wxID_IDEDITOR_DELETEBLOCK,
	wxID_IDEDITOR_ADDREQUESTFLOW,
	wxID_IDEDITOR_ADDBRANCH,
	wxID_IDEDITOR_ADDCHOICE,
	wxID_IDEDITOR_ADDFORK,
	wxID_IDEDITOR_ADDCONDITION,
	wxID_IDEDITOR_ADDFACT,
	wxID_IDEDITOR_ADDCOMMUNICATOR,
	wxID_IDEDITOR_ADDREQUESTFOCUS,
	wxID_IDEDITOR_ADDREQUESTINTERACTION,
	wxID_IDEDITOR_ADDEVENTS,
	wxID_IDEDITOR_ADDCONNECTOR,
	wxID_IDEDITOR_ADDCHECKPOINT,
};

BEGIN_EVENT_TABLE( CEdInteractiveDialogGraphEditor, CEdGraphEditor )
	EVT_LEFT_UP( CEdInteractiveDialogGraphEditor::OnLeftClick )
	EVT_SET_FOCUS( CEdInteractiveDialogGraphEditor::OnSetFocus )
	EVT_KILL_FOCUS( CEdInteractiveDialogGraphEditor::OnKillFocus )
END_EVENT_TABLE()


const Vector CEdInteractiveDialogGraphEditor::BLOCK_POS_ADD( 120.f, 20.f, 0.f );

CEdInteractiveDialogGraphEditor::CEdInteractiveDialogGraphEditor( wxWindow* parent, CEdInteractiveDialogEditor* mainEditor )
	: CEdGraphEditor( parent, false )
	, m_mainEditor( mainEditor )
	, m_highlightedBlock( nullptr )
	, m_useMousePositionForNewBlocks( false )
{
	m_shouldRepaint = true;
	m_shouldZoom = true;

	SetHook( mainEditor );
}

CEdInteractiveDialogGraphEditor::~CEdInteractiveDialogGraphEditor(void)
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdInteractiveDialogGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
	menu.Append( wxID_IDEDITOR_DELETEBLOCK, wxT( "Delete selected blocks" ) );
	menu.AppendSeparator();

	CEdGraphEditor::InitLinkedBlockContextMenu( block, menu );
}

void CEdInteractiveDialogGraphEditor::InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu )
{
	CEdGraphEditor::InitLinkedSocketContextMenu( block, menu );
}

void CEdInteractiveDialogGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	m_mousePosition = wxGetMousePosition();
	FillBlockMenu( menu );
}

void CEdInteractiveDialogGraphEditor::FillBlockMenu( wxMenu& menu )
{
	menu.Append( wxID_IDEDITOR_ADDINPUT, wxT( "Add &input\tCtrl-I" ) );
	menu.Connect( wxID_IDEDITOR_ADDINPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddInput ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDOUTPUT, wxT( "Add &output\tCtrl-O" ) );
	menu.Connect( wxID_IDEDITOR_ADDOUTPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddOutput ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDOUTPUTTERMINATE, wxT( "Add output terminate" ) );
	menu.Connect( wxID_IDEDITOR_ADDOUTPUTTERMINATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddOutputTemrinate ), nullptr, this );

	menu.AppendSeparator();

	menu.Append( wxID_IDEDITOR_ADDSECTION, wxT( "Add &text\tCtrl-T" ) );
	menu.Connect( wxID_IDEDITOR_ADDSECTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddText ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDCHOICE, wxT( "Add &choice\tCtrl-Q" ) );
	menu.Connect( wxID_IDEDITOR_ADDCHOICE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddChoice ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDBRANCH, wxT( "Add &branch\tCtrl-B" ) );
	menu.Connect( wxID_IDEDITOR_ADDBRANCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddBranch ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDCONNECTOR, wxT( "Add connector" ) );
	menu.Connect( wxID_IDEDITOR_ADDCONNECTOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddConnector ), nullptr, this );

	menu.AppendSeparator();
	
	menu.Append( wxID_IDEDITOR_ADDCONDITION, wxT( "Add condition" ) );
	menu.Connect( wxID_IDEDITOR_ADDCONDITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddCondition ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDREQUESTFLOW, wxT( "Add flow" ) );
	menu.Connect( wxID_IDEDITOR_ADDREQUESTFLOW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddFlow ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDFORK, wxT( "Add fork" ) );
	menu.Connect( wxID_IDEDITOR_ADDFORK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddFork ), nullptr, this );	

	menu.AppendSeparator();

	menu.Append( wxID_IDEDITOR_ADDFACT, wxT( "Add &fact\tCtrl-F" ) );
	menu.Connect( wxID_IDEDITOR_ADDFACT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddFact ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDREQUESTINTERACTION, wxT( "Add interaction" ) );
	menu.Connect( wxID_IDEDITOR_ADDREQUESTINTERACTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddRequestInteraction ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDEVENTS, wxT( "Add &events" ) );
	menu.Connect( wxID_IDEDITOR_ADDEVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddEvents ), nullptr, this );

	menu.AppendSeparator();

	menu.Append( wxID_IDEDITOR_ADDCOMMUNICATOR, wxT( "Add communicator" ) );
	menu.Connect( wxID_IDEDITOR_ADDCOMMUNICATOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddCommunicator ), nullptr, this );
	menu.Append( wxID_IDEDITOR_ADDREQUESTFOCUS, wxT( "Add &request focus\tCtrl-R" ) );
	menu.Connect( wxID_IDEDITOR_ADDREQUESTFOCUS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdInteractiveDialogGraphEditor::OnAddRequestFocus ), nullptr, this );
}

template < class TBlockType > 
void CEdInteractiveDialogGraphEditor::OnAddBlockFromBlockMenu()
{
	if ( m_graph == NULL )
	{
		return;
	}

	wxPoint blockPosition = m_mousePosition;
	if ( false == m_useMousePositionForNewBlocks )
	{
		CIDGraphBlock* lastBlock = static_cast< CIDGraph* > ( m_graph )->GetLastBlockByXPosition();
		if ( lastBlock )
		{
			Vector pos = lastBlock->GetPosition() + BLOCK_POS_ADD;
			blockPosition = ClientToScreen( CanvasToClient( wxPoint( Int32( pos.X ), Int32( pos.Y ) ) ) );
		}
	}

	TBlockType* block = CreateAndAddDialogBlock< TBlockType > ();
	RepositionBlock( block, blockPosition );

	HiliteBlock( block );
	m_mainEditor->OnBlockSelected( block );
}

template < class TBlockType > 
TBlockType* CEdInteractiveDialogGraphEditor::CreateAndAddDialogBlock()
{
	GraphBlockSpawnInfo info( TBlockType::GetStaticClass() );
	TBlockType* createdBlock = Cast< TBlockType >( m_graph->GraphCreateBlock( info ) );
	createdBlock->OnCreatedInEditor();
	return createdBlock;	
}

void CEdInteractiveDialogGraphEditor::OnAddInput( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockInput >			(); }
void CEdInteractiveDialogGraphEditor::OnAddOutput( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockOutput >			(); }
void CEdInteractiveDialogGraphEditor::OnAddOutputTemrinate( wxCommandEvent& event )		{ OnAddBlockFromBlockMenu < CIDGraphBlockOutputTerminate >	(); }
void CEdInteractiveDialogGraphEditor::OnAddText( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockText >				(); }
void CEdInteractiveDialogGraphEditor::OnAddFlow( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockFlow >				(); }
void CEdInteractiveDialogGraphEditor::OnAddBranch( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockBranch >			(); }
void CEdInteractiveDialogGraphEditor::OnAddChoice( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockChoice >			(); }
void CEdInteractiveDialogGraphEditor::OnAddFork( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockFork >				(); }
void CEdInteractiveDialogGraphEditor::OnAddCondition( wxCommandEvent& event )			{ OnAddBlockFromBlockMenu < CIDGraphBlockCondition >		(); }
void CEdInteractiveDialogGraphEditor::OnAddFact( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockFact >				(); }
void CEdInteractiveDialogGraphEditor::OnAddCommunicator( wxCommandEvent& event )		{ OnAddBlockFromBlockMenu < CIDGraphBlockComunicatorSwitch >(); }
void CEdInteractiveDialogGraphEditor::OnAddRequestFocus( wxCommandEvent& event )		{ OnAddBlockFromBlockMenu < CIDGraphBlockRequestFocus >		(); }
void CEdInteractiveDialogGraphEditor::OnAddRequestInteraction( wxCommandEvent& event )	{ OnAddBlockFromBlockMenu < CIDGraphBlockInteraction >		(); }
void CEdInteractiveDialogGraphEditor::OnAddEvents( wxCommandEvent& event )				{ OnAddBlockFromBlockMenu < CIDGraphBlockEvents >			(); }
void CEdInteractiveDialogGraphEditor::OnAddConnector( wxCommandEvent& event )			{ OnAddBlockFromBlockMenu < CIDGraphBlockConnector >		(); }
void CEdInteractiveDialogGraphEditor::OnAddCheckpoint( wxCommandEvent& event )			{ OnAddBlockFromBlockMenu < CIDGraphBlockCheckpoint >		(); }


void CEdInteractiveDialogGraphEditor::OnDeleteBlock( wxCommandEvent& event )
{
	PerformDeleteSelection();
}

void CEdInteractiveDialogGraphEditor::RepositionBlock( CIDGraphBlock* block, const wxPoint& newPositionInScreenSpace )
{
	wxPoint mousePos = ClientToCanvas( ScreenToClient( newPositionInScreenSpace ) );
	block->SetPosition( Vector( mousePos.x, mousePos.y, 0.f ) );
	Repaint();
}

void CEdInteractiveDialogGraphEditor::DoDeleteBlock( CGraphBlock* block )
{
	block->BreakAllLinks();
	m_graph->GraphRemoveBlock( block );
}

void CEdInteractiveDialogGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	__super::PaintCanvas( width, height );

	if ( m_highlightedBlock )
	{
		const Vector pos = m_highlightedBlock->GetPosition();
		const Vector size = GetSizeFromLayout( const_cast< CIDGraphBlock* > ( m_highlightedBlock ) ); // f*@#!n CEdGraphEditor::GetSizeFromLayout() shuould have const CGraphBlock* as an argument, since it doesn't f*@#!n change it!
		static const wxColour hiliteColor( 255, 192, 127, 127 ); 
		static const Float width = 10.f;

		DrawRect( Int32( MRound( pos.X - width * 0.5f ) ), Int32( MRound( pos.Y - width * 0.5f ) ), Int32( MRound( size.X + width ) ), Int32( MRound( size.Y + width ) ), hiliteColor, width ); 
	}

	const CIDTopic* topic = m_graph ? Cast< const CIDTopic > ( m_graph->GraphGetOwner() ) : NULL;
	const String text = topic ? String::Printf( TXT("Editing topic: %s"), topic->GetName() ? topic->GetName().AsString().AsChar() : TXT("Unnamed topic") ) : TXT("Please select a topic.");

	DrawText( ClientToCanvas( wxPoint( 6,6 ) ), GetGdiBoldFont(), text.AsChar(), wxColour( 20, 20, 20 ) );
	DrawText( ClientToCanvas( wxPoint( 5,5 ) ), GetGdiBoldFont(), text.AsChar(), wxColour( 255, 255, 255 ) );
}

void CEdInteractiveDialogGraphEditor::OnLeftClick( wxMouseEvent& event )
{
	m_mousePosition = wxGetMousePosition();
	
	THandle< ISerializable > clickedObject = GetActiveItem( ClientToCanvas( ScreenToClient( m_mousePosition ) ) ); 
	m_mainEditor->SetPropertiesObject( clickedObject.Get() );

	CIDGraphBlock* block = Cast< CIDGraphBlock >( clickedObject.Get() );
	if ( block )
	{
		m_mainEditor->OnBlockSelected( block );
		HiliteBlock( block );
	}

	event.Skip();
}

void CEdInteractiveDialogGraphEditor::OnCopySelection()
{
	CopySelection();
	RED_LOG( Dialog, TXT("Editor: Copy selection.") );
	wxMessageBox( TXT("Please remember, that if you COPY a localized string, you actually COPY it's stringdb id. This means if you edit COPIED string, you also edit ORIGINAL (strings in both COPIED and ORIGINAL blocks change)."), TXT("Reminder") );
}

void CEdInteractiveDialogGraphEditor::OnPaste()
{
	const wxPoint mousePos = ClientToCanvas( ScreenToClient( wxGetMousePosition() ) );
	Paste( &Vector( mousePos.x, mousePos.y, 0.f ) );
	RED_LOG( Dialog, TXT("Editlor: Paste.") );
}

void CEdInteractiveDialogGraphEditor::OnCutSelection()
{
	CutSelection();
	RED_LOG( Dialog, TXT("Editor: Cut selection.") );
}

void CEdInteractiveDialogGraphEditor::OnDeleteSelection()
{
	PerformDeleteSelection();
	RED_LOG( Dialog, TXT("Editor: Delete selection.") );
}

void CEdInteractiveDialogGraphEditor::PerformDeleteSelection()
{
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	if ( blocks.Empty() )
	{
		return;
	}

	String txt = TXT("Do you really want to delete these blocks:\n");
	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		txt += blocks[ i ]->GetBlockName();
		txt += TXT("\n");
	}

	if ( wxMessageBox( txt.AsChar(), wxT("Are you sure?"), wxYES_NO | wxCENTRE ) != wxYES )
	{
		return;
	}

	Freeze();
	{
		for ( Uint32 i = 0; i < blocks.Size(); ++i )
		{
			DoDeleteBlock( blocks[ i ] );
		}

		if ( blocks.Exist( const_cast< CIDGraphBlock* > ( m_highlightedBlock ) ) )
		{
			HiliteBlock( nullptr );
		}

		m_mainEditor->OnGraphStructureModified( m_graph );
		Repaint();
	}
	Thaw();
}

void CEdInteractiveDialogGraphEditor::HiliteBlock( CIDGraphBlock* block )
{
	const Bool repaint( m_highlightedBlock != block );
	
	m_highlightedBlock = block;

	if ( repaint )
	{
		Freeze();
		{
			if ( block && false == IsBlockVisible( block ) )	
			{
				FocusOnBlock( block );
			}

			Repaint();
		}
		Thaw();
	}
}

void CEdInteractiveDialogGraphEditor::OnSetFocus( wxFocusEvent& event )
{
	m_useMousePositionForNewBlocks = true;
	event.Skip();
}

void CEdInteractiveDialogGraphEditor::OnKillFocus( wxFocusEvent& event )
{
	m_useMousePositionForNewBlocks = false;
	event.Skip();
}
#endif