
#include "build.h"
#include "SimplexTreeEditor.h"
#include "../../common/engine/simplexTreeResource.h"
#include "../../common/engine/windAreaComponent.h"
#include "../../common/engine/worldIterators.h"
#include "../../common/engine/world.h"
#include "../../common/engine/clipMap.h"

CSimplexTreeNodeVis::CSimplexTreeNodeVis()
	: CSimplexTreeNode()
{
	m_bbox.PushBack( Float2(-4000,-4000) );
	m_bbox.PushBack( Float2(-4000, 4000) );
	m_bbox.PushBack( Float2( 4000, 4000) );
	m_bbox.PushBack( Float2( 4000,-4000) );

	ResetSelection();
	GenerateShapes();
}
Int32 CSimplexTreeNodeVis::CreatePositiveStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy )
{
	Int32 ind = CSimplexTreeNode::CreatePositiveStruct( par, id, px, py, dx, dy );
	ResetSelection();
	GenerateShapes();
	return ind;
}
Int32 CSimplexTreeNodeVis::CreateNegativeStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy )
{
	Int32 ind = CSimplexTreeNode::CreateNegativeStruct( par, id, px, py, dx, dy );
	ResetSelection();
	GenerateShapes();
	return ind;
}
void CSimplexTreeNodeVis::RemoveNode( Int32 ind )
{
	CSimplexTreeNode::RemoveNode( ind );
	ResetSelection();
	GenerateShapes();
}
void CSimplexTreeNodeVis::ResetSelection()
{
	m_selected.ClearFast();
	const Int32 numn = m_nodes.Size();
	m_selected.Reserve( numn );
	Int32 i;
	for( i=0;i<numn;++i )
	{
		m_selected.PushBack( false );
	}
}
void CSimplexTreeNodeVis::GenerateShapes()
{
	m_shapes.ClearFast();
	const Int32 numn = m_nodes.Size();
	m_shapes.Reserve( numn );
	Int32 i;
	for( i=0;i<numn*2;++i ){ m_shapes.PushBack( TDynArray<Float2>() ); }
	for( i=0;i<numn;++i )
	{
		Int32 pos = m_nodes[i].m_positiveStruct;
		Int32 neg = m_nodes[i].m_negativeStruct;
		Int32 par = m_nodes[i].m_parent;
		if( par==-1 )
		{
			GenerateShapePositive( m_nodes[i], m_bbox, m_shapes[(i*2)+0] );
			GenerateShapeNegative( m_nodes[i], m_bbox, m_shapes[(i*2)+1] );
		}
		else
		{
			if( m_nodes[par].m_positiveStruct == i ) // if positive
			{
				GenerateShapePositive( m_nodes[i], m_shapes[(par*2)+0], m_shapes[(i*2)+0] );
				GenerateShapeNegative( m_nodes[i], m_shapes[(par*2)+0], m_shapes[(i*2)+1] );
			}
			else if( m_nodes[par].m_negativeStruct == i )// if negative
			{
				GenerateShapePositive( m_nodes[i], m_shapes[(par*2)+1], m_shapes[(i*2)+0] );
				GenerateShapeNegative( m_nodes[i], m_shapes[(par*2)+1], m_shapes[(i*2)+1] );
			}
		}
	}
}
Int32 CSimplexTreeNodeVis::FindNodeAtPoint( Float x, Float y )
{
	Int32 ind = CSimplexTreeNode::FindNodeAtPoint( x, y );
	ResetSelection();
	m_selected[ind] = true;
	return ind;
}

void CSimplexTreeNodeVis::GenerateShapePositive( const SSimplexTreeStruct & nod, const TDynArray<Float2> & arr, TDynArray<Float2> & out  )
{
	for( Uint32 i=0;i<arr.Size();i++ )
	{
		Float2 curr = arr[i];
		Float gn = nod.Gradient( (Float)curr.x, (Float)curr.y );
		Float2 prev = i==0 ? arr[arr.Size()-1] : arr[i-1];
		Float gp = nod.Gradient( (Float)prev.x, (Float)prev.y );
		if( (gn<0.0f && gp>0.0f) || (gn>0.0f && gp<0.0f) )
		{
			Float sum = fabs(gn)+fabs(gp);
			if( sum>0.0f )
			{
				Float w = fabs(gp)/sum;
				Float x = (Float)prev.x + ( (Float)curr.x - (Float)prev.x ) * w;
				Float y = (Float)prev.y + ( (Float)curr.y - (Float)prev.y ) * w;
				out.PushBack( Float2( x, y ) );
			}
		}
		if( gn>0.0f )
		{
			out.PushBack( curr );
		}
	}
}

void CSimplexTreeNodeVis::GenerateShapeNegative( const SSimplexTreeStruct & nod, const TDynArray<Float2> & arr, TDynArray<Float2> & out  )
{
	for( Uint32 i=0;i<arr.Size();i++ )
	{
		Float2 curr = arr[i];
		Float gn = nod.Gradient( (Float)curr.x, (Float)curr.y );
		Float2 prev = i==0 ? arr[arr.Size()-1] : arr[i-1];
		Float gp = nod.Gradient( (Float)prev.x, (Float)prev.y );
		if( (gn<0.0f && gp>0.0f) || (gn>0.0f && gp<0.0f) )
		{
			Float sum = fabs(gn)+fabs(gp);
			if( sum>0.0f )
			{
				Float w = fabs(gp)/sum;
				Float x = (Float)prev.x + ( (Float)curr.x - (Float)prev.x ) * w;
				Float y = (Float)prev.y + ( (Float)curr.y - (Float)prev.y ) * w;
				out.PushBack( Float2( x, y ) );
			}
		}
		if( gn<0.0f )
		{
			out.PushBack( curr );
		}
	}
}

void SimplexTreeEditorCanvas::DrawPolyMatrix( const Float2* arr, Int32 num, const wxColour & col, Float wid )
{
	TDynArray<wxPoint> points;
	Int32 i;
	points.Reserve( num );

	for( i=0;i<num;++i )
	{
		Float2 p( m_matrix[0]*Float(arr[i].x) + m_matrix[2]*Float(arr[i].x) + m_matrix[4],
				  m_matrix[1]*Float(arr[i].y) + m_matrix[3]*Float(arr[i].y) + m_matrix[5] );
		points.PushBack( wxPoint( Int32(p.x), Int32(p.y) ) );
	}
	DrawPoly( &points[0], num, col, wid );
}
void SimplexTreeEditorCanvas::FillPolyMatrix( const Float2* arr, Int32 num, const wxColour & col )
{
	TDynArray<wxPoint> points;
	Int32 i;
	points.Reserve( num );

	for( i=0;i<num;++i )
	{
		Float2 p( m_matrix[0]*Float(arr[i].x) + m_matrix[2]*Float(arr[i].x) + m_matrix[4],
			m_matrix[1]*Float(arr[i].y) + m_matrix[3]*Float(arr[i].y) + m_matrix[5] );
		points.PushBack( wxPoint( Int32(p.x), Int32(p.y) ) );
	}
	FillPoly( &points[0], num, col );
}

void SimplexTreeEditorCanvas::DrawCircleCenteredMatrix( const Float2 & cen, Float rad, const wxColour & col, Float wid )
{
	Float r = rad*m_matrix[0];
	r = r<1.0f ? 1.0f : r;
	// radius is scaled with hack
	Float2 p( m_matrix[0]*Float(cen.x) + m_matrix[2]*Float(cen.x) + m_matrix[4],
			  m_matrix[1]*Float(cen.y) + m_matrix[3]*Float(cen.y) + m_matrix[5] );
	DrawCircleCentered( wxPoint( (Int32)p.x, (Int32)p.y ), r, col, wid );
}

void CSimplexTreeNodeVis::Draw( SimplexTreeEditorCanvas* vp )
{
	const Int32 numn = m_nodes.Size();
	Int32 i;

	for( i=0;i<numn;++i )
	{
		if( m_nodes[i].m_positiveID >=0 && m_shapes[(i*2)+0].Size()>0 )
		{
			vp->FillPolyMatrix( &m_shapes[(i*2)+0][0], m_shapes[(i*2)+0].Size(), wxColor(200,0,0) ); 
		}
		if( m_nodes[i].m_negativeID >=0 && m_shapes[(i*2)+1].Size()>0 )
		{
			vp->FillPolyMatrix( &m_shapes[(i*2)+1][0], m_shapes[(i*2)+1].Size(), wxColor(0,0,200) ); 
		}
		
		/*
		if( m_selected[i] )
		{
			if(  m_shapes[(i*2)+0].Size() )
			{
				//vp->FillPolyMatrix( &m_shapes[(i*2)+0][0], m_shapes[(i*2)+0].Size(), wxColor(200,0,0) ); 
				vp->DrawPolyMatrix( &m_shapes[(i*2)+0][0], m_shapes[(i*2)+0].Size(), wxColor(255,255,0), 1.0f );
			}
			if( m_shapes[(i*2)+1].Size() )
			{
				//vp->FillPolyMatrix( &m_shapes[(i*2)+1][0], m_shapes[(i*2)+1].Size(), wxColor(0,0,200) ); 
				vp->DrawPolyMatrix( &m_shapes[(i*2)+1][0], m_shapes[(i*2)+1].Size(), wxColor(255,255,0), 1.0f );
			}
		}
		else
		*/
		{
			if(  m_shapes[(i*2)+0].Size() )
			{
				vp->DrawPolyMatrix( &m_shapes[(i*2)+0][0], m_shapes[(i*2)+0].Size(), wxColor(0,0,0), 1.0f );
			}
			if( m_shapes[(i*2)+1].Size() )
			{
				vp->DrawPolyMatrix( &m_shapes[(i*2)+1][0], m_shapes[(i*2)+1].Size(), wxColor(0,0,0), 1.0f );
			}
		}
		
		if( m_nodes[i].m_positiveStruct==-1 )
		{
			TDynArray<Float2> & pts = m_shapes[(i*2)+0];
			const Int32 numpoin  = pts.Size();
			if(numpoin && m_nodes[i].m_positiveID>=0 )
			{
				Int32 j;
				Float2 cen(0,0);
				for( j=0;j<numpoin;++j )
				{
					cen += pts[j];
				}
				cen.x /= numpoin;
				cen.y /= numpoin;

				Float2 cenp( vp->GetMatrix()[0]*Float(cen.x) + vp->GetMatrix()[2]*Float(cen.x) + vp->GetMatrix()[4],
					vp->GetMatrix()[1]*Float(cen.y) + vp->GetMatrix()[3]*Float(cen.y) + vp->GetMatrix()[5] );

				char buf[16];
				sprintf_s( (char*)buf, 16, "%d", m_nodes[i].m_positiveID );
				vp->DrawText( wxPoint( (Int32)cenp.x, (Int32)cenp.y ), vp->GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );
			}
		}
		if( m_nodes[i].m_negativeStruct==-1 )
		{
			TDynArray<Float2> & pts = m_shapes[(i*2)+1];
			const Int32 numpoin  = pts.Size();
			if(numpoin && m_nodes[i].m_negativeID>=0)
			{
				Int32 j;
				Float2 cen(0,0);
				for( j=0;j<numpoin;++j )
				{
					cen += pts[j];
				}
				cen.x /= numpoin;
				cen.y /= numpoin;

				Float2 cenp( vp->GetMatrix()[0]*Float(cen.x) + vp->GetMatrix()[2]*Float(cen.x) + vp->GetMatrix()[4],
					vp->GetMatrix()[1]*Float(cen.y) + vp->GetMatrix()[3]*Float(cen.y) + vp->GetMatrix()[5] );

				char buf[16];
				sprintf_s( (char*)buf, 16, "%d", m_nodes[i].m_negativeID );
				vp->DrawText( wxPoint( (Int32)cenp.x, (Int32)cenp.y ), vp->GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );
			}
		}
		
	}

	const Int32 nump = m_points.Size();

	for( i=0;i<nump;++i )
	{
		vp->DrawCircleCenteredMatrix( Float2( m_points[i].x, m_points[i].y ), 5.0f, wxColor(255,255,0), 1.0f );
	}


	const Int32 numa = m_areas.Size();

	for( i=0;i<numa;++i )
	{
		vp->DrawPolyMatrix( &m_areas[i][0], m_areas[i].Size(), wxColor(200,0,0), 1.0f ); 
	}

	// debug draw selection helper
	CWorld* world = GGame->GetActiveWorld();
	if( world && world->GetSelectionManager()->GetSelectionCount() > 0 )
	{
		TDynArray< CNode* > nodes = world->GetSelectionManager()->GetSelectedNodes();
		for( Uint32 i=0; i<nodes.Size(); ++i ) 
		{				
			vp->DrawCircleCenteredMatrix( Float2( nodes[i]->GetWorldPosition().X, nodes[i]->GetWorldPosition().Y ), 5.0f, wxColor(255,255,255), 1.0f );
		}
	}

}

void CSimplexTreeNodeVis::DrawSel( SimplexTreeEditorCanvas* vp, Int32 sel )
{
	char buf[256] = {0};
	if( sel>=0 )
	{
		Int32 ps = m_nodes[sel].m_positiveStruct;
		Int32 ns = m_nodes[sel].m_negativeStruct;

		Bool possible_edit = false;
		if( ps==-1 && ns==-1 )
		{
			possible_edit = true;
			sprintf_s( (char*)buf, 256, "Possible to edit area division with mouse: ctrl + right drag in green area." );
		}

		if( m_shapes[(sel*2)+0].Size() >=0 )
		{
			vp->DrawPolyMatrix( &m_shapes[(sel*2)+0][0], m_shapes[(sel*2)+0].Size(), possible_edit ? wxColor(0,200,0) : wxColor(200,200,0), 2.0f ); 
		}
		if( m_shapes[(sel*2)+1].Size() >=0 )
		{
			vp->DrawPolyMatrix( &m_shapes[(sel*2)+1][0], m_shapes[(sel*2)+1].Size(), possible_edit ? wxColor(0,200,0) : wxColor(200,200,0), 2.0f ); 
		}
	}
	if( strlen(buf)>0 )
	{
		vp->DrawText( wxPoint( (Int32)10, (Int32)10 ), vp->GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(0,255,0) );
	}
}


BEGIN_EVENT_TABLE( CEdSimplexTreeEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "file_save" ), CEdSimplexTreeEditor::OnSave )
	EVT_MENU( XRCID( "m_autoGen" ), CEdSimplexTreeEditor::OnGenerate )
	EVT_MENU( XRCID( "m_loadfromworld" ), CEdSimplexTreeEditor::OnGetFrmWorld )
	EVT_MENU( XRCID( "m_loadmap" ), CEdSimplexTreeEditor::OnImportImage)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( SimplexTreeEditorCanvas, CEdCanvas )
	EVT_ERASE_BACKGROUND( SimplexTreeEditorCanvas::OnEraseBackground )
	EVT_LEFT_DOWN( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_LEFT_UP( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_MIDDLE_DOWN( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_MIDDLE_UP( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_MOTION( SimplexTreeEditorCanvas::OnMouseMove )
	EVT_MOUSEWHEEL( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_PAINT( SimplexTreeEditorCanvas::OnPaint )
	EVT_RIGHT_DOWN( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_RIGHT_UP( SimplexTreeEditorCanvas::OnMouseEvent )
	EVT_SIZE( SimplexTreeEditorCanvas::OnSize )
	EVT_KEY_DOWN( SimplexTreeEditorCanvas::OnKey )
END_EVENT_TABLE()

SimplexTreeEditorCanvas::SimplexTreeEditorCanvas( wxWindow* parent )
	: CEdCanvas( parent )
{
	m_sel = -1;
	m_view = Float2( 0.0f, 0.0f );
	m_zoom = 1.0f;
	worldSize = 8000.0f;
	map = NULL;
}

void SimplexTreeEditorCanvas::CreateMatrix()
{
	Int32 w;
	Int32 h;
	GetSize( &w, &h );

	Float wid = Float(w);
	Float hei = Float(h);
	Float ratio = hei/wid;

	Float2 offs( (-400.0f/m_zoom) + m_view.x, ((400.0f/m_zoom)*ratio)+m_view.y );
	Float zasx = (800.0f/m_zoom)/wid;
	Float zasy = -((800.0f/m_zoom)*ratio)/hei;

	Float inv_matrix[6] = { 0.0f };
	inv_matrix[0] = zasx;
	inv_matrix[1] = 0.0f;
	inv_matrix[2] = 0.0f;
	inv_matrix[3] = zasy;
	inv_matrix[4] = offs.x;
	inv_matrix[5] = offs.y;

	
	m_matrix[0] = 1.0f/inv_matrix[0];
	m_matrix[3] = 1.0f/inv_matrix[3];

	m_matrix[4] = -inv_matrix[4]*m_matrix[0];
	m_matrix[5] = -inv_matrix[5]*m_matrix[3];
}

void SimplexTreeEditorCanvas::LoadImageFromFile()
{
	wxFileDialog loadFileDialog( this, wxT("Load Image"), wxT( "" ), wxT( "" ), wxT( "*.*" ), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		if( map ){ delete map; map = NULL; }
		wxString ppp = loadFileDialog.GetPath();
		map = new Gdiplus::Bitmap( ppp );
	}
}

SimplexTreeEditorCanvas::~SimplexTreeEditorCanvas()
{
	if( map ){ delete map; map = NULL; }
}

void SimplexTreeEditorCanvas::OnKey( wxKeyEvent& event )
{
	if( event.GetKeyCode()==WXK_DELETE )
	{
		if( m_sel )
		{
			if( m_sel>0 ) // root can not be deleted
			{
				m_tree.RemoveNode( m_sel );
				m_tree.GenerateShapes();
				m_tree.ResetSelection();
			}
			m_sel = -1;
		}
	}

	if( event.GetKeyCode()=='W' )
	{
		if( m_sel )
		{
			if( m_sel>0 ) // root can not be deleted
			{
				Int32 par = m_tree.GetNodes()[m_sel].m_parent;
				if( par>=0 )
				{
					m_tree.ResetSelection();
					m_tree.GetSelection()[par] = true;
					m_sel = par;
				}
			}
		}
	}

	if( event.GetKeyCode()=='D' )
	{
		if( m_sel )
		{
			if( m_sel>0 ) // root can not be deleted
			{
				Int32 par = m_tree.GetNodes()[m_sel].m_positiveStruct;
				if( par>=0 )
				{
					m_tree.ResetSelection();
					m_tree.GetSelection()[par] = true;
					m_sel = par;
				}
			}
		}
	}

	if( event.GetKeyCode()=='A' )
	{
		if( m_sel )
		{
			if( m_sel>0 ) // root can not be deleted
			{
				Int32 par = m_tree.GetNodes()[m_sel].m_negativeStruct;
				if( par>=0 )
				{
					m_tree.ResetSelection();
					m_tree.GetSelection()[par] = true;
					m_sel = par;
				}
			}
		}
	}

	if( event.GetKeyCode()=='Q' )
	{
		Float inv_matrix[6] = { 0.0f };
		inv_matrix[0] = 1.0f/m_matrix[0];
		inv_matrix[3] = 1.0f/m_matrix[3];

		inv_matrix[4] = -m_matrix[4]*inv_matrix[0];
		inv_matrix[5] = -m_matrix[5]*inv_matrix[3];

		Float2 p(	inv_matrix[0]*Float(m_click.x) + inv_matrix[2]*Float(m_click.x) + inv_matrix[4],
					inv_matrix[1]*Float(m_click.y) + inv_matrix[3]*Float(m_click.y) + inv_matrix[5] );

		Int32 ind = m_tree.FindNodeAtPoint( p.x, p.y );
		Float gr = m_tree.GetNodes()[ind].Gradient( p.x, p.y );

		if( gr>=0.0f )
		{
			m_tree.GetNodes()[ind].m_positiveID = 0;
		}
		else
		{
			m_tree.GetNodes()[ind].m_negativeID = 0;
		}
	}

	if( event.GetKeyCode()=='E' )
	{
		Float inv_matrix[6] = { 0.0f };
		inv_matrix[0] = 1.0f/m_matrix[0];
		inv_matrix[3] = 1.0f/m_matrix[3];

		inv_matrix[4] = -m_matrix[4]*inv_matrix[0];
		inv_matrix[5] = -m_matrix[5]*inv_matrix[3];

		Float2 p(	inv_matrix[0]*Float(m_click.x) + inv_matrix[2]*Float(m_click.x) + inv_matrix[4],
			inv_matrix[1]*Float(m_click.y) + inv_matrix[3]*Float(m_click.y) + inv_matrix[5] );

		Int32 ind = m_tree.FindNodeAtPoint( p.x, p.y );
		Float gr = m_tree.GetNodes()[ind].Gradient( p.x, p.y );

		if( gr>=0.0f )
		{
			m_tree.GetNodes()[ind].m_positiveID = -1;
		}
		else
		{
			m_tree.GetNodes()[ind].m_negativeID = -1;
		}
	}

	Repaint();
}

void SimplexTreeEditorCanvas::PaintCanvas( Int32 width, Int32 height )
{
	CreateMatrix();
	Clear( wxColour(30,30,30) );

	Float siz = worldSize*0.5f;
	if( map )
	{
		Float2 p1(	m_matrix[0]*Float(-siz) + m_matrix[2]*Float(-siz) + m_matrix[4],
					m_matrix[1]*Float(siz) + m_matrix[3]*Float(siz) + m_matrix[5] );

		Float2 p2(	m_matrix[0]*Float(siz) + m_matrix[2]*Float(siz) + m_matrix[4],
			m_matrix[1]*Float(-siz) + m_matrix[3]*Float(-siz) + m_matrix[5] );

		Float w = fabs(p2.x - p1.x);
		Float h = fabs(p2.y - p1.y);

		DrawImage( map, Int32(p1.x), Int32(p1.y), Int32(w), Int32(h) );
	}

	m_tree.Draw( this );

	if( m_sel>=0 )
	{
		m_tree.DrawSel( this, m_sel );
	}



	char buf[256];
	sprintf_s( (char*)buf, 256, "DELETE - deletes selected shapes." );
	DrawText( wxPoint( (Int32)10, (Int32)30 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "W - selects parent shape." );
	DrawText( wxPoint( (Int32)10, (Int32)50 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "D - selects positive shape." );
	DrawText( wxPoint( (Int32)10, (Int32)70 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "A - selects negative shape." );
	DrawText( wxPoint( (Int32)10, (Int32)90 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "Q - marks cursorover area." );
	DrawText( wxPoint( (Int32)10, (Int32)110 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "E - clears cursorover area." );
	DrawText( wxPoint( (Int32)10, (Int32)130 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );

	sprintf_s( (char*)buf, 256, "Ctrl + left drag - creates new level of division on area." );
	DrawText( wxPoint( (Int32)10, (Int32)150 ), GetGdiDrawFont(), ANSI_TO_UNICODE(buf), wxColor(255,255,255) );
}


void SimplexTreeEditorCanvas::MouseClick( wxMouseEvent& event )
{
	Float2 pp( (Float)event.GetX(), (Float)event.GetY() );

	m_click = wxPoint( event.GetX(), event.GetY() );

	Float inv_matrix[6] = { 0.0f };
	inv_matrix[0] = 1.0f/m_matrix[0];
	inv_matrix[3] = 1.0f/m_matrix[3];

	inv_matrix[4] = -m_matrix[4]*inv_matrix[0];
	inv_matrix[5] = -m_matrix[5]*inv_matrix[3];

	Float2 p( inv_matrix[0]*Float(pp.x) + inv_matrix[2]*Float(pp.x) + inv_matrix[4],
		      inv_matrix[1]*Float(pp.y) + inv_matrix[3]*Float(pp.y) + inv_matrix[5] );


	if( event.ButtonDown( wxMOUSE_BTN_MIDDLE ) )
	{
		m_start = p;
	}

	if( event.ButtonDown( wxMOUSE_BTN_LEFT ) && !event.ControlDown() )
	{
		m_sel = m_tree.FindNodeAtPoint( p.x, p.y );
		if( m_sel>=0 ){ m_gradient = m_tree.GetNodes()[m_sel].Gradient( p.x, p.y ); }
		m_start = p;
	}

	if( event.ButtonDown( wxMOUSE_BTN_LEFT ) && event.ControlDown() )
	{
		m_sel = m_tree.FindNodeAtPoint( p.x, p.y );
		if( m_sel>=0 ){ m_gradient = m_tree.GetNodes()[m_sel].Gradient( p.x, p.y ); }
		m_start = p;
	}
	if( event.ButtonUp( wxMOUSE_BTN_LEFT ) && event.ControlDown() )
	{
		Float2 end( p.x, p.y );
		Float2 delta = end - m_start;
		Float len = sqrt( delta.x*delta.x + delta.y*delta.y );
		if( len > 0.0f )
		{
			if( m_sel>=0 )
			{
				Float x = Float(delta.x)/len;
				Float y = Float(delta.y)/len;
				if( m_gradient>=0.0f )
				{
					m_tree.CreatePositiveStruct( m_sel, -1, m_start.x, m_start.y, x, y );
					m_tree.GetSelection()[m_sel] = true;
				}
				else
				{
					m_tree.CreateNegativeStruct( m_sel, -1, m_start.x, m_start.y, x, y );
					m_tree.GetSelection()[m_sel] = true;
				}
			}
		}
	}
	if( event.ButtonDown( wxMOUSE_BTN_RIGHT ) && event.ControlDown() )
	{
		m_sel = m_tree.FindNodeAtPoint( p.x, p.y );
		if( m_sel>=0 ){ m_gradient = m_tree.GetNodes()[m_sel].Gradient( p.x, p.y ); }
		m_start = p;
	}
	if( event.ButtonUp( wxMOUSE_BTN_RIGHT ) && event.ControlDown() )
	{
		Float2 end( p.x, p.y );
		Float2 delta = end - m_start;
		Float len = sqrt( (Float)delta.x*(Float)delta.x + (Float)delta.y*(Float)delta.y );
		if( len > 0.0f )
		{
			Int32 ps = m_tree.GetNodes()[m_sel].m_positiveStruct;
			Int32 ns = m_tree.GetNodes()[m_sel].m_negativeStruct;
			if( m_sel>=0 && ps==-1 && ns==-1 )
			{
				Float x = Float(delta.x)/len;
				Float y = Float(delta.y)/len;
				m_tree.GetNodes()[m_sel].Set( m_start.x, m_start.y, x, y );
				m_tree.GenerateShapes();
			}
		}
	}
	if( event.GetWheelRotation() )
	{
		Int32 dir = event.GetWheelRotation();
		dir = dir>1 ? 1 : dir;
		dir = dir<-1 ? -1 : dir;
		m_zoom += 0.05f*Float( dir );
		m_zoom = m_zoom<=0.0f ? 0.1f : m_zoom;
	}
	Repaint();
}
void SimplexTreeEditorCanvas::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	Float2 pp( (Float)event.GetX(), (Float)event.GetY() );
	wxPoint del = wxPoint( event.GetX(), event.GetY() ) - m_click;
	m_click = wxPoint( event.GetX(), event.GetY() );

	Float inv_matrix[6] = { 0.0f };
	inv_matrix[0] = 1.0f/m_matrix[0];
	inv_matrix[3] = 1.0f/m_matrix[3];

	inv_matrix[4] = -m_matrix[4]*inv_matrix[0];
	inv_matrix[5] = -m_matrix[5]*inv_matrix[3];

	Float2 p( inv_matrix[0]*Float(pp.x) + inv_matrix[2]*Float(pp.x) + inv_matrix[4],
		inv_matrix[1]*Float(pp.y) + inv_matrix[3]*Float(pp.y) + inv_matrix[5] );

	if( event.ButtonIsDown( wxMOUSE_BTN_MIDDLE ) )
	{
		m_view.x += Float( -del.x )/m_zoom;
		m_view.y += Float(  del.y )/m_zoom;
	}
	Repaint();
}


CEdSimplexTreeEditor::CEdSimplexTreeEditor( wxWindow* parent, CResourceSimplexTree* res )
	: wxSmartLayoutPanel( parent, TXT("SimplexTree"), false )
{
	// Create Editor and place it
	{
		m_resource = res;
		m_resource->AddToRootSet();
		wxPanel* rp = XRCCTRL( *this, "m_simplexTreePanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_view = new SimplexTreeEditorCanvas( rp );
		sizer1->Add( m_view, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		const Int32 num = res->GetNodes().Size();
		Int32 i;
		m_view->GetTree().GetNodes().Clear();

		if( num )
		{
			for( i=0;i<num;++i )
			{
				m_view->GetTree().GetNodes().PushBack( res->GetNodes()[i] );
			}
		}
		else
		{
			SSimplexTreeStruct base;
			base.Set( 0.0f, 0.0f, 1.0f, 0.0f );
			m_view->GetTree().GetNodes().PushBack( base );
		}
		m_view->GetTree().GenerateShapes();
		m_view->GetTree().ResetSelection();

		//GeneratePoints();
	}
}

CEdSimplexTreeEditor::~CEdSimplexTreeEditor()
{
	if ( m_resource )
	{
		m_resource->RemoveFromRootSet();
	}
}

void CEdSimplexTreeEditor::OnSave( wxCommandEvent& event )
{
	if( m_resource )
	{
		m_resource->GetNodes().ClearFast();
		const Int32 num = m_view->GetTree().GetNodes().Size();
		Int32 i;
		m_resource->GetNodes().Reserve( num );
		for( i=0;i<num;++i )
		{
			m_resource->GetNodes().PushBack(  m_view->GetTree().GetNodes()[i] );
		}
		m_resource->Save();
	}
}

void CEdSimplexTreeEditor::GeneratePoints()
{
	m_view->GetTree().GetPoints().Clear();

	const Int32 nump = 299;
	Int32 i;

	CStandardRand& r = GEngine->GetRandomNumberGenerator();

	Float dim = m_view->GetTree().GetBox()[2].x;
	for( i=0;i<nump;++i )
	{
		Float px = ((r.Get<Float>(1.f)-0.5f)*2.0f)*dim;
		Float py = ((r.Get<Float>(1.f)-0.5f)*2.0f)*dim;
		m_view->GetTree().GetPoints().PushBack( Float2( px, py ) );
	}
}

void CEdSimplexTreeEditor::OnGenerate( wxCommandEvent& event )
{
	m_view->GetTree().AutoGeneration2();
}

void CEdSimplexTreeEditor::OnImportImage( wxCommandEvent& event )
{
		m_view->LoadImageFromFile();
}
#pragma optimize("",off)
void CEdSimplexTreeEditor::OnGetFrmWorld( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();

	if( world )
	{
		Float dim = world->GetWorldDimensions();
		if( world->GetTerrain() )
		{
			dim = world->GetTerrain()->GetTerrainSize();
		}
		m_view->GetTree().SetDimensions( dim );
		m_view->worldSize = dim;
	}
	else
	{
		Float dim = 1000.0f;
		m_view->GetTree().SetDimensions( dim );
	}

	if( world )
	{
		m_view->GetTree().GetPoints().Clear();
		
		m_view->GetTree().GetAreas().Clear();

		Int32 ind = 0;
		for ( WorldAttachedComponentsIterator it( world ); it; ++it )
		{
			CWindAreaComponent *com = Cast< CWindAreaComponent > ( *it );
			if ( com )
			{
				const Matrix & mat = com->GetLocalToWorld();

				m_view->GetTree().GetAreas().PushBack( TDynArray<Float2>() );
				const CAreaComponent::TAreaPoints& arr = com->m_localPoints;

				const Int32 num = arr.Size();
				Int32 j;
				for( j=0;j<num;++j )
				{
					Vector gp = mat.TransformPoint( arr[j] );
					m_view->GetTree().GetAreas()[ind].PushBack( Float2( gp.X, gp.Y ) );
				}
				ind++;
			}
		}	

		//GeneratePoints();
	}
	else
	{
		m_view->GetTree().GetPoints().Clear();
		//GeneratePoints();
	}
}
#pragma optimize("",on)
void CSimplexTreeNodeVis::Create( Int32 dep )
{
	m_nodes.Clear();
	SSimplexTreeStruct root;
	root.Set( 0, 0, 1, 0 );
	m_nodes.PushBack( root );
	Create( 0, dep );
}

void CSimplexTreeNodeVis::SetDimensions( Float dim )
{
	Float pdim = dim*0.5f;
	Float mdim = dim*-0.5f;
	m_bbox[0] = ( Float2(mdim,mdim) );
	m_bbox[1] = ( Float2(mdim,pdim) );
	m_bbox[2] = ( Float2(pdim,pdim) );
	m_bbox[3] = ( Float2(pdim,mdim) );

	ResetSelection();
	GenerateShapes();
}

void CSimplexTreeNodeVis::Create( Int32 par, Int32 dep )
{
	Int32 pn = CreateP( par );
	Int32 nn = CreateN( par );
	if( dep>0 )
	{
		Create( pn, dep-1 );
		Create( nn, dep-1 );
	}
}

Int32 CSimplexTreeNodeVis::CreateP( Int32 par )
{
	Int32 ind = m_nodes.Size();
	m_nodes[ par ].m_positiveStruct = ind;
	SSimplexTreeStruct nod;
	nod.m_parent = par;
	m_nodes.PushBack( nod );
	return ind;
}
Int32 CSimplexTreeNodeVis::CreateN( Int32 par )
{
	Int32 ind = m_nodes.Size();
	m_nodes[ par ].m_negativeStruct = ind;
	SSimplexTreeStruct nod;
	nod.m_parent = par;
	m_nodes.PushBack( nod );
	return ind;
}

void CSimplexTreeNodeVis::AutoGeneration2()
{
	Int32 dep = 12;
	Int32 numi = 1000;
	Float lr = 0.2f;
	Float lr2 = 0.02f;

	Create( dep );

	const Int32 nump = m_points.Size();
	const Int32 numn = m_nodes.Size();
	Int32 i;
	Int32 j;
	Int32 k;

	TDynArray< TDynArray<Int32> > points;

	for( i=0;i<numn;++i ){ points.PushBack( TDynArray<Int32>() ); }

	for( i=0;i<nump;++i )
	{
		points[0].PushBack( i );
	}

	for( i=0;i<numn;++i )
	{
		Int32 numcp = points[i].Size();
		
		if( points[i].Size()<=0 ){ m_nodes[i].m_reserved = -666; }
		if( m_nodes[i].m_parent>=0 )
		{
			if( m_nodes[ m_nodes[i].m_parent ].m_reserved == -666 )
			{
				 m_nodes[i].m_reserved = -666;
			}
			if( numcp<=0 )
			{
				m_nodes[m_nodes[i].m_parent].m_reserved = -666;
			}
		}
		if( m_nodes[i].m_reserved == -666 ){ continue; }
		

		Float2 cen( 0.0f, 0.0f );
		
		for( j=0;j<numcp;++j )
		{
			cen += m_points[ points[i][j] ];
		}
		if( nump )
		{
			cen.x /= Float(nump);
			cen.y /= Float(nump);
		}

		Float2 patA = cen;
		Float2 patB = cen;


		for( k=0;k<numi;++k )
		{
			Float mul = 1.0f - (Float(k)/Float(numi));
			for( j=0;j<numcp;++j )
			{
				Float2 val = m_points[ points[i][j] ];
				Float ea = patA.bsk( val );
				Float eb = patB.bsk( val );

				if( ea<eb )
				{
					patA.pull(val, lr*mul);
					patB.pull(val, lr2*mul);

				}
				else
				{
					patB.pull(val, lr*mul);
					patA.pull(val, lr2*mul);
				}
				
			}
		}

		Float2 pa = patA;
		Float2 pb = patB;
		pa.pull( pb, 0.5f );

		Float dx = pb.x - pa.x;
		Float dy = pb.y - pa.y;
		Float len = sqrt( dx*dx + dy*dy );
		if( len > 0.0f ){ dx/=len; dy/=len; }else{ dx = 1.0f; dy = 0.0f; }

		m_nodes[i].Set( pa.x, pa.y, -dy, dx );

		for( j=0;j<numcp;++j )
		{
			Int32 ind = points[i][j];
			Float2 val = m_points[ ind ];
			Float gr = m_nodes[i].Gradient( val.x, val.y );
			if( gr >= 0.0f  )
			{
				if( m_nodes[i].m_positiveStruct>=0 )
				{
					points[m_nodes[i].m_positiveStruct].PushBack( ind );
				}
			}
			else
			{
				if( m_nodes[i].m_negativeStruct>=0 )
				{
					points[m_nodes[i].m_negativeStruct].PushBack( ind );
				}
			}
		}

		points[i].Clear();

		if( m_nodes[i].m_positiveStruct>=0 )
		{
			Int32 num = points[m_nodes[i].m_positiveStruct].Size();
			if( num <= 1 )
			{
				m_nodes[ m_nodes[i].m_positiveStruct ].m_reserved = -666;
				m_nodes[i].m_positiveStruct = -1;
			}
		}
		if( m_nodes[i].m_negativeStruct>=0 )
		{
			Int32 num = points[m_nodes[i].m_negativeStruct].Size();
			if( num <= 1 )
			{
				m_nodes[ m_nodes[i].m_negativeStruct ].m_reserved = -666;
				m_nodes[i].m_negativeStruct = -1;
			}
		}
	}

	RemoveNodes666();
	ResetSelection();
	GenerateShapes();
}
