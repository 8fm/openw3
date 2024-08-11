#pragma once 

struct chunkVerts
{
	TDynArray<Int32> verts;
};

/// Editor for vertex painting
class CEdVertexPaintTool : public wxPanel
{

public:
	CEdVertexPaintTool() {};
	CEdVertexPaintTool( wxWindow* parent, CMesh* mesh );
	~CEdVertexPaintTool();

	wxCheckBox*								m_VPChRed;
	wxCheckBox*								m_VPChGreen;
	wxCheckBox*								m_VPChBlue;
	wxCheckBox*								m_VPChAlpha;
	
	wxCheckBox*								m_VPToggleSelection;
	wxCheckBox*								m_VPSelectElement;
	wxCheckBox*								m_VPUseBlend;

	wxSlider*								m_VPAlpha;

	wxSlider*								m_VPBSize;
	wxSlider*								m_VPBPower;
	wxChoice*								m_VPBIndex;

	wxButton*								m_VPFill;
	wxButton*								m_VPClear;

	wxColourPickerCtrl*						m_VPColorPicker;

	bool									m_showVertices;	

	TDynArray<chunkVerts>					m_currentVerts;	

	DebugVertex								m_VPBrushVerts[32];

	Float									m_VPBrushDistance;
	Vector									m_VPBrushDir;
	Float									m_VPBrushLocalScale;

	wxButton*								m_savevc;
	wxButton*								m_loadvc;

	wxCheckBox*								m_paintAllLods;
	wxSpinCtrl*								m_lod;

	CMesh*									m_mesh;
	class CMeshData*						m_meshData;

	void OnSaveVC( wxCommandEvent& event );
	void OnLoadVC( wxCommandEvent& event );

	void TogglePaint( void );
	void ToggleSelection( wxCommandEvent& event );

	Bool PaintVertex( void );
	void SelectVertex( void );

	void OnFill( wxCommandEvent& event );
	void OnBrushBlendPowerChanged( wxScrollEvent& event );
	void OnBrushSizeChanged( wxScrollEvent& event );

	void OnAlphaChanged( wxScrollEvent& event );

	Bool Edit( void );
	void SetBrushSize( int diffSize );
	void InvertBrushPower( void );

	Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame, const Vector& origin, const Vector& dir, bool displaySelection );	

	// Call when the LOD list changes.
	void LodsChanged();

private:

	Vector		mOrigin;
	Vector		mDir;
	Vector		m_SelectedElementCenter;
	Vector		m_SelectedElementSize;	

	//Vector3		m_ClosestVertPos;
	//Vector3		m_ClosestVertNorm;	

	Uint32 GetFinalColor( Uint32 vertexColor, wxColour wC, Bool useTextureBlendingMode, Uint8 blendModeColor );
	Bool CheckVertexHit( Vector3 vPos, Vector3 vNorm, Float& distToClosestVert, Float brushSize );
	void SelectAllVerticesInElement( Int32 chunkIndex, Int32 startingIndex );
	void DeselectAllVertices( void );
	void CalculateBoundsOfCurrentSelection( void );

	void CollectChunksToCheck( TDynArray<Uint16>& chunksToCheck );
};

