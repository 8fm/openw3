/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "versionControlIconsPainter.h"
#include "guiResourceGraphEditor.h"
#include "guiResourceEditor.h"

#include "../../common/game/guiResource.h"

BEGIN_EVENT_TABLE( CEdGuiResourceGraphEditor, CEdGraphEditor )
	EVT_LEFT_UP( CEdGuiResourceGraphEditor::OnLeftClick )
END_EVENT_TABLE()

enum
{
	wxID_GUIEDITOR_ADDBLOCK = 5401,
	wxID_GUIEDITOR_REMOVEBLOCK = 5501,
	wxID_GUIEDITOR_REFRESHPREVIEW,
};

namespace // anonymous
{
	class CGuiGraphBlockWrapper : public wxObject
	{
	private:
		wxPoint m_position;
		CClass *m_class;

	public:
		CGuiGraphBlockWrapper( CClass *blockClass, wxPoint position )
			: m_class( blockClass )
			, m_position( position )
		{ }

		RED_INLINE wxPoint GetPosition() const { return m_position; }
		RED_INLINE CClass *GetClass() const { return m_class; }

	};
}

//////////////////////////////////////////////////////////////////////////

CEdGuiResourceGraphEditor::CEdGuiResourceGraphEditor( wxWindow* parent, CEdGuiResourceEditor* guiResourceEditor )
	: CEdGraphEditor( parent, false )
	, m_guiResourceEditor( guiResourceEditor )
{
	m_vciPainter = new CVersionControlIconsPainter( *this );
}

//////////////////////////////////////////////////////////////////////////

CEdGuiResourceGraphEditor::~CEdGuiResourceGraphEditor()
{
	delete m_vciPainter;
}

//////////////////////////////////////////////////////////////////////////

wxColor CEdGuiResourceGraphEditor::GetCanvasColor() const
{
	wxColor bgColor = DEFAULT_EDITOR_BACKGROUND;

	return bgColor;
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	CEdGraphEditor::PaintCanvas( width, height );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::CalcTitleIconArea( CGraphBlock* block, wxSize &size )
{
	size.x = 15;
	size.y = 15;
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::DrawTitleIconArea( CGraphBlock* block, const wxRect& rect )
{
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::CalcBlockInnerArea( CGraphBlock* block, wxSize& size )
{
	size = wxSize( 64, 32 );
	//CEdGraphEditor::CalcBlockInnerArea( block, size );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect )
{
	if ( block )
	{
		wxColour clientColor;
		wxColour borderColor;
		Bool drawThunder = false;

		// 		if ( block->IsExactlyA< CGuiFlashResourceBlock >() )
		// 		{
		// 			CGuiFlashResourceBlock* resourceBlock = Cast<CGuiFlashResourceBlock>( block );
		// 			clientColor = wxColour(192, 0, 0);
		// 			borderColor = wxColour(255, 0, 0);
		// 			const TDynArray< CGraphSocket* >& sockets = block->GetSockets();
		// 			for ( TDynArray< CGraphSocket* >::const_iterator it = sockets.Begin(); it != sockets.End(); ++it )
		// 			{
		// 				const CGraphSocket* socket = *it;
		// 				if
		// 				( ( socket->IsA< CGuiFlashInputListenerSocket >() || socket->IsA< CGuiFlashEngineValueListenerSocket >() ) &&
		// 					socket->HasConnections()					
		// 				)				
		// 				{
		// 					drawThunder = true;
		// 					break;
		// 				}
		// 			}
		// 		}

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

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth )
{
	linkWidth = 2.f;
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow )
{
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
// 	menu.Append( wxID_GUIEDITOR_REFRESHPREVIEW, wxT( "Refresh UI preview" ) );
// 	menu.Connect( wxID_GUIEDITOR_REFRESHPREVIEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGuiGraphEditor::OnRefreshPreview ), NULL, this );
// 	menu.AppendSeparator();
// 
 	CEdGraphEditor::InitLinkedBlockContextMenu( block, menu );
 
 	m_mousePosition = wxGetMousePosition();
 	TDynArray< CGraphBlock* > selectedBlocks;
 	GetSelectedBlocks( selectedBlocks );
 	menu.Append( wxID_GUIEDITOR_REMOVEBLOCK, wxT( "Remove selected" ) );
 	menu.Connect( wxID_GUIEDITOR_REMOVEBLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGuiResourceGraphEditor::OnRemoveSelectedBlocks ), NULL, this );
 	menu.Enable( wxID_GUIEDITOR_REMOVEBLOCK, CanDeleteSelectedBlocks() ? true : false );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::InitLinkedSocketContextMenu( CGraphSocket *socket, wxMenu &menu )
{
	m_mousePosition = wxGetMousePosition();
	CEdGraphEditor::InitLinkedSocketContextMenu( socket, menu );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
 	m_mousePosition = wxGetMousePosition();
// 
// 	menu.Append( wxID_GUIEDITOR_REFRESHPREVIEW, wxT( "Refresh UI preview" ) );
// 	menu.Connect( wxID_GUIEDITOR_REFRESHPREVIEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGuiGraphEditor::OnRefreshPreview ), NULL, this );
// 	menu.AppendSeparator();
// 
 	static TDynArray< CClass* >	blockClasses;
 	GetEditableBlockClasses( blockClasses );
 
 	Uint32 blockIdx = 0;
 	for ( TDynArray< CClass* >::const_iterator it = blockClasses.Begin(); it != blockClasses.End(); ++it )
 	{
		ASSERT( *it );
		if ( ! *it )
		{
			continue;
		}

		IGuiResourceBlock* defaultBlock = (*it)->GetDefaultObject< IGuiResourceBlock >();
		String blockName = defaultBlock->GetBlockName();
		wxMenuItem* menuItem = menu.Append( wxID_GUIEDITOR_ADDBLOCK + blockIdx, blockName.AsChar() );

		menu.Connect( wxID_GUIEDITOR_ADDBLOCK + blockIdx, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdGuiResourceGraphEditor::OnAddGraphBlock ), 
			new CGuiGraphBlockWrapper( *it, m_mousePosition ), 
			this );

		//const Bool canAddBlock = GetGraph()->CanCreateBlock( *it );
		//if ( ! canAddBlock )
		//{
		//	menu.Enable( wxID_GUIEDITOR_ADDBLOCK + blockIdx, false );
		//}

		++blockIdx;
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth )
{
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::DrawBlockLayout( CGraphBlock* block )
{
	CEdGraphEditor::DrawBlockLayout( block );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::AnnotateConnection( const CGraphConnection* con, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width /*= 1.0f*/ )
{
	CEdGraphEditor::AnnotateConnection( con, src, srcDir, dest, destDir, width );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::OnLeftClick( wxMouseEvent& event )
{
	//m_guiEditor->SetPropertyGraphBaseItem( m_activeItem );

	event.Skip();
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::OnAddGraphBlock( wxCommandEvent& event )
{
	if ( ModifyGraphStructure() )
	{
		if ( CGuiGraphBlockWrapper *blockWrapper = dynamic_cast< CGuiGraphBlockWrapper * >( event.m_callbackUserData ) )
		{
			// Create new step
			wxPoint graphPoint = ClientToCanvas( blockWrapper->GetPosition() );
			SpawnBlock( blockWrapper->GetClass(), ScreenToClient( m_mousePosition ) );
		}
		GraphStructureModified();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::OnRemoveSelectedBlocks( wxCommandEvent& event )
{
	if ( ModifyGraphStructure() )
	{
		DeleteSelectedBlocks();
		GraphStructureModified();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::OnRefreshPreview( wxCommandEvent& event )
{
//	m_guiEditor->RefreshPreview();
}

//////////////////////////////////////////////////////////////////////////

Bool CEdGuiResourceGraphEditor::CanDeleteSelectedBlocks()
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	if ( selectedBlocks.Empty() )
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::GetEditableBlockClasses( TDynArray< CClass* >& outClasses )
{
	outClasses.ClearFast();
	IGuiResource* guiGraph = static_cast< IGuiResource* >( GetGraph() );
	
	if ( ! guiGraph || ! guiGraph->GetResourceBlockClass() )
	{
		return;
	}

	SRTTI::GetInstance().EnumClasses( guiGraph->GetResourceBlockClass(), outClasses );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceGraphEditor::DeleteSelectedBlocks()
{
	TDynArray< CGraphBlock* > selectedBlocks;
	GetSelectedBlocks( selectedBlocks );

	for ( Uint32 i = 0; i < selectedBlocks.Size(); ++i )
	{
		if ( ! GetGraph()->GraphRemoveBlock( selectedBlocks[i] ) )
		{
			ERR_EDITOR( TXT("Unable to remove block %s"), selectedBlocks[i]->GetCaption() );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// CGuiGraph* CEdGuiGraphEditor::GetGraph()
// {
// 	ASSERT( m_guiEditor ); 
// 	return m_guiEditor ? m_guiEditor->GetGraph() : 0;
// }
