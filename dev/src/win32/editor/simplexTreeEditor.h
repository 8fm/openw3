
#pragma once

#include "../../common/core/simplexTree.h"

class SimplexTreeEditorCanvas;

struct Float2
{
	Float2(){	x = y = 0.0f;	}
	Float2( Float xx, Float yy ){ x = xx; y = yy; }
	Float2( Int32 xx, Int32 yy ){ x = (Float)xx; y = (Float)yy; }
	void operator += ( const Float2 & p ){ x += p.x; y += p.y; }
	void pull( const Float2 & p, Float mul )
	{
		Float dx = (p.x-x)*mul;
		Float dy = (p.y-y)*mul;
		{
			x += dx;
			y += dy;
		}
	}
	Float bsk( const Float2 & p )
	{
		return (pow( x-p.x, 2.0f ) + pow( y-p.y, 2.0f ));
	}
	Float x;
	Float y;
};

inline Float2 operator- ( const Float2 & a, const Float2 & b ){ return Float2( a.x-b.x, a.y-b.y ); }

class CSimplexTreeNodeVis : public CSimplexTreeNode // visual version
{
public:
	CSimplexTreeNodeVis();
	Int32 CreatePositiveStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy );
	Int32 CreateNegativeStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy );
	void RemoveNode( Int32 ind );
	void ResetSelection();
	void GenerateShapes();
	Int32 FindNodeAtPoint( Float x, Float y );
	void Draw( SimplexTreeEditorCanvas* vp );
	void DrawSel( SimplexTreeEditorCanvas* vp, Int32 sel );
	//void AutoGeneration();
	void AutoGeneration2();
	void Create( Int32 dep );
	void SetDimensions( Float dim );

	TDynArray<bool> & GetSelection() { return m_selected; }
	TDynArray< TDynArray<Float2> > & GetShapes() { return m_shapes; }
	TDynArray<Float2> & GetBox() { return m_bbox; }
	TDynArray< Float2 > & GetPoints() { return m_points; }
	TDynArray< TDynArray<Float2> > & GetAreas() { return m_areas; }

private:
	void GenerateShapePositive( const SSimplexTreeStruct & nod, const TDynArray<Float2> & arr, TDynArray<Float2> & out  );
	void GenerateShapeNegative( const SSimplexTreeStruct & nod, const TDynArray<Float2> & arr, TDynArray<Float2> & out  );

	Int32 CreateP( Int32 par );
	Int32 CreateN( Int32 par );
	void Create( Int32 par, Int32 dep );

	TDynArray<bool> m_selected;
	TDynArray< TDynArray<Float2> > m_shapes;
	TDynArray<Float2> m_bbox; // should be world size
	TDynArray< Float2 > m_points;

	TDynArray< TDynArray<Float2> > m_areas;
};

class Bitmap;
class SimplexTreeEditorCanvas : public CEdCanvas
{
	DECLARE_EVENT_TABLE()
public:
	SimplexTreeEditorCanvas( wxWindow* parent );
	virtual ~SimplexTreeEditorCanvas();
	CSimplexTreeNodeVis & GetTree() { return m_tree; }
	void DrawPolyMatrix( const Float2* arr, Int32 num, const wxColour & col, Float wid );
	void FillPolyMatrix( const Float2* arr, Int32 num, const wxColour & col );
	void DrawCircleCenteredMatrix( const Float2 & cen, Float rad, const wxColour & col, Float wid );
	Float* GetMatrix(){ return m_matrix; }
	void LoadImageFromFile();
	Float worldSize;
private:
	virtual void PaintCanvas( Int32 width, Int32 height );
	virtual void MouseClick( wxMouseEvent& event );
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );
	void OnKey( wxKeyEvent& event );

	Int32 m_sel;
	Float m_gradient;
	CSimplexTreeNodeVis m_tree;
	Float2 m_start;
	wxPoint m_click;
	Float m_matrix[6];

	Float2 m_view;
	Float m_zoom;
	void CreateMatrix();

	
	Gdiplus::Bitmap* map;

};


class CEdSimplexTreeEditor 
	: public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

public:

	CEdSimplexTreeEditor( wxWindow* parent, CResourceSimplexTree* res );
	~CEdSimplexTreeEditor();


private:
	// Events
	void OnSave( wxCommandEvent& event );
	void OnGenerate( wxCommandEvent& event );
	void OnGetFrmWorld( wxCommandEvent& event );
	void OnImportImage( wxCommandEvent& event );

	void GeneratePoints();
	SimplexTreeEditorCanvas* m_view;
	CResourceSimplexTree* m_resource;
	
};