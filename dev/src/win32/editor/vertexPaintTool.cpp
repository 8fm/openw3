#include "build.h"
#include "vertexPaintTool.h"
#include "toolsPanel.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/renderFrame.h"



CEdVertexPaintTool::CEdVertexPaintTool( wxWindow* parent, CMesh* m )
{	
	m_mesh = m;
	m_meshData = new CMeshData( m );

//	m_ClosestVertPos = Vector3(0.0f, 0.0f, 0.0f);
//	m_ClosestVertNorm = Vector3(0.0f, 0.0f, 0.0f);
	m_SelectedElementCenter = Vector3(0.0f, 0.0f, 0.0f);
	m_SelectedElementSize = Vector3(0.0f, 0.0f, 0.0f);

	m_showVertices = false;
	
	m_VPBrushDistance = 1.0f;	
	m_VPBrushDir = Vector(0.0f, 0.0f, 1.0f);
		
	m_VPChRed = XRCCTRL( *parent, "m_VPChRed", wxCheckBox );
	m_VPChGreen  = XRCCTRL( *parent, "m_VPChGreen", wxCheckBox );
	m_VPChBlue  = XRCCTRL( *parent, "m_VPChBlue", wxCheckBox );
	m_VPChAlpha  = XRCCTRL( *parent, "m_VPChAlpha", wxCheckBox );

	m_VPUseBlend  = XRCCTRL( *parent, "m_VPBlendChkBox", wxCheckBox );
	m_VPUseBlend->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );

	m_VPBSize = XRCCTRL( *parent, "m_VPBrushSizeSlider", wxSlider );
	m_VPBSize->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CEdVertexPaintTool::OnBrushSizeChanged ), NULL, this );

	m_VPBPower = XRCCTRL( *parent, "m_VPBlendPowerSlider", wxSlider );
	m_VPBPower->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CEdVertexPaintTool::OnBrushBlendPowerChanged ), NULL, this );

	m_VPAlpha = XRCCTRL( *parent, "m_VPAlpha", wxSlider );
	m_VPAlpha->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CEdVertexPaintTool::OnAlphaChanged ), NULL, this );

	m_VPBIndex = XRCCTRL( *parent, "m_VPBlendIndex", wxChoice );
	m_VPBIndex->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );

	m_VPFill =  XRCCTRL( *parent, "m_VPFillButton", wxButton );
	m_VPClear = XRCCTRL( *parent, "m_VPClearButton", wxButton );

	m_VPColorPicker = XRCCTRL( *parent, "m_VPBColorPicker", wxColourPickerCtrl );
	m_VPColorPicker->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );
	
	m_VPFill->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::OnFill ), NULL, this );
	m_VPClear->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::OnFill ), NULL, this );
	
	m_VPToggleSelection = XRCCTRL( *parent, "m_VPSelectionChkBox", wxCheckBox );
	m_VPToggleSelection->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );	

	m_VPSelectElement = XRCCTRL( *parent, "m_VPElementChkBox", wxCheckBox );

	XRCCTRL( *parent, "m_VPDeselectButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );

	XRCCTRL( *parent, "m_VPLockButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );
	XRCCTRL( *parent, "m_VPFUnlockAllButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::ToggleSelection ), NULL, this );

	m_savevc =  XRCCTRL( *parent, "m_VPSaveToFile", wxButton );
	m_loadvc =  XRCCTRL( *parent, "m_VPLoadFromFile", wxButton );
	m_savevc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::OnSaveVC ), NULL, this );
	m_loadvc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVertexPaintTool::OnLoadVC ), NULL, this );

	m_VPClear = XRCCTRL( *parent, "m_VPClearButton", wxButton );

	m_VPBrushLocalScale = m_VPBSize->GetValue();

	m_lod = XRCCTRL( *parent, "m_VPLod", wxSpinCtrl );

	m_paintAllLods = XRCCTRL( *parent, "m_VPAllLods", wxCheckBox );
}

CEdVertexPaintTool::~CEdVertexPaintTool()
{
}

void CEdVertexPaintTool::TogglePaint( void )
{
	// get num of chunks 
	if( m_currentVerts.Empty() )
	{
		m_currentVerts.Resize( m_mesh->GetChunks().Size() );

		if( !m_mesh->CanUseExtraStreams() )
		{
			if( GFeedback->AskYesNo(TEXT("This mesh doesn't use vertex color (useExtraStreams)\ndo you want to turn it on?") ) ) 
			{
				m_mesh->SetUseExtractStreams( true );
				m_mesh->Save();

				delete m_meshData;
				m_meshData = new CMeshData( m_mesh );
			}
		}
	}
	else
	{
		m_meshData->FlushChanges();
		m_mesh->Save();
		m_currentVerts.Clear();
	}
	m_showVertices = !m_showVertices;
}

void CEdVertexPaintTool::ToggleSelection( wxCommandEvent& event )
{			
	if( event.GetId() == XRCID("m_VPBlendChkBox") ) m_VPChGreen->SetValue(true);
	if( event.GetId() == XRCID("m_VPBlendIndex") ||  event.GetId() == XRCID("m_VPBColorPicker") ) 
	{
		if( m_VPToggleSelection->IsChecked() )
		{	
			if( PaintVertex() )
			{
				m_mesh->CreateRenderResource();	
				CDrawableComponent::RecreateProxiesOfRenderableComponents();
			}
		}	
	}

	if( event.GetId() == XRCID("m_VPDeselectButton") )
	{
		DeselectAllVertices();		
	}

	if( event.GetId() == XRCID("m_VPLockButton") )
	{

	}

	if( event.GetId() == XRCID("m_VPFUnlockAllButton") )
	{

	}
}

void CEdVertexPaintTool::DeselectAllVertices( void )
{
	for(Uint32 i=0; i<m_currentVerts.Size(); i++)	m_currentVerts[i].verts.Clear();
	m_SelectedElementCenter = Vector3(0.0f, 0.0f, 0.0f);
	m_SelectedElementSize = Vector3(0.0f, 0.0f, 0.0f);
}


void CEdVertexPaintTool::OnAlphaChanged( wxScrollEvent& event )
{
	if( m_VPToggleSelection->IsChecked() )
	{
		if( PaintVertex() )
		{
			m_mesh->CreateRenderResource();
			CDrawableComponent::RecreateProxiesOfRenderableComponents();
		}
	}
}

void CEdVertexPaintTool::OnBrushBlendPowerChanged( wxScrollEvent& event )
{
	if( m_VPToggleSelection->IsChecked() )
	{
		if( PaintVertex() )
		{
			m_mesh->CreateRenderResource();
			CDrawableComponent::RecreateProxiesOfRenderableComponents();
		}
	}
}

void CEdVertexPaintTool::OnBrushSizeChanged( wxScrollEvent& event )
{
	SetBrushSize( 0 );
}

void CEdVertexPaintTool::SetBrushSize( int diffSize )
{
	int bSize = m_VPBSize->GetValue();
	
	if( (bSize + diffSize) <= m_VPBSize->GetMax() && (bSize + diffSize) >= m_VPBSize->GetMin() ) m_VPBSize->SetValue( bSize + diffSize );

	m_VPBrushLocalScale = bSize + diffSize;
}

void CEdVertexPaintTool::InvertBrushPower( void )
{
	m_VPBPower->SetValue( 255 - m_VPBPower->GetValue() );
	m_VPColorPicker->SetColour( wxColour( m_VPBPower->GetValue(), m_VPBPower->GetValue(), m_VPBPower->GetValue(), 255 ) );
}

void CEdVertexPaintTool::OnFill( wxCommandEvent& event )
{
	if( m_VPChRed->IsChecked() || m_VPChGreen->IsChecked() || m_VPChBlue->IsChecked() || m_VPChAlpha->IsChecked() )
	{
		// check if we want to blend texture paint
		bool useTextureBlending = m_VPUseBlend->IsChecked();

		float blendPower = (float)(m_VPBPower->GetValue());
		Uint8 chGreen = Uint8( blendPower + 16.0f*(float)m_VPBIndex->GetSelection() );

		wxColour wC = m_VPColorPicker->GetColour();
		wC.Set( wC.Red(), wC.Green(), wC.Blue(), m_VPAlpha->GetValue() );
		if( event.GetId() ==  XRCID("m_VPClearButton") )
		{
			Color cCol;

			if( m_VPChRed->IsChecked() ) cCol.R = 255;
			if( m_VPChGreen->IsChecked() ) cCol.G = 0;
			if( m_VPChBlue->IsChecked() ) cCol.B = 0;
			if( m_VPChAlpha->IsChecked() ) cCol.A = 255;

			wC.Set( cCol.ToUint32() );
		}

		auto& chunks = m_meshData->GetChunks();

		if ( m_VPToggleSelection->IsChecked() )
		{
			for ( Uint32 t = 0; t < m_currentVerts.Size(); t++ )
			{
				SMeshChunk& chunk = chunks[t];

				const TDynArray< Int32 >& selected = m_currentVerts[t].verts;
				for ( Uint32 c = 0; c < selected.Size(); c++ )
				{
					Uint32 uColor = chunk.m_vertices[ selected[c] ].m_color;
					chunk.m_vertices[ selected[c] ].m_color = GetFinalColor( uColor, wC, useTextureBlending, chGreen );
				}
			}
		}
		else
		{
			TDynArray<Uint16> chunksToCheck;
			CollectChunksToCheck( chunksToCheck );

			for ( Uint32 j=0; j<chunksToCheck.Size(); ++j )
			{
				Uint32 chunkNum = chunksToCheck[j];
				SMeshChunk& chunk = chunks[chunkNum];

				for ( Uint32 i = 0; i < chunk.m_numVertices; i++ )
				{
					Uint32 uColor = chunk.m_vertices[i].m_color;
					chunk.m_vertices[i].m_color = GetFinalColor( uColor, wC, useTextureBlending, chGreen );
				}
			}
		}

		m_meshData->FlushChanges();

		m_mesh->Save();
		m_mesh->CreateRenderResource();
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
	else
	{
		GFeedback->ShowMsg(TXT("Nothing selected!"), TEXT("Select RGB channels first!") );
	}
}

Bool CEdVertexPaintTool::PaintVertex()
{
	bool ret = false;

	auto& chunks = m_meshData->GetChunks();

	Float numOfBlendTextures = 16.0f;
	Float blendPower = (Float)(m_VPBPower->GetValue());
	Uint8 chGreen = Uint8( blendPower + numOfBlendTextures*(Float)m_VPBIndex->GetSelection() );

	// encode texture blends
	bool useTextureBlending = m_VPUseBlend->IsChecked();

	wxColour wC = m_VPColorPicker->GetColour();
	wC.Set( wC.Red(), wC.Green(), wC.Blue(), m_VPAlpha->GetValue() );

	// paint the vertex
	for ( Uint32 t = 0; t<m_currentVerts.Size(); t++ )
	{
		auto& chunk = chunks[t];
		const TDynArray< Int32 >& selected = m_currentVerts[t].verts;
		for ( Uint32 c = 0; c < selected.Size(); c++ )
		{
			Uint32 uColor = chunk.m_vertices[ selected[c] ].m_color;
			chunk.m_vertices[ selected[c] ].m_color = GetFinalColor( uColor, wC, useTextureBlending, chGreen );

			ret = true;
		}
	}
	return ret;
}

void CEdVertexPaintTool::SelectVertex()
{
	auto& chunks = m_meshData->GetChunks();
	TDynArray<Uint16> chunksToCheck;
	CollectChunksToCheck( chunksToCheck );

	Float distToClosestVert = 10000.0f;
	Vector3 closestVertPos = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 closestVertNorm = Vector3(0.0f, 0.0f, 1.0f);

	Vector3 vPos = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 vNorm = Vector3(0.0f, 0.0f, 1.0f);

	Float brushSize = 0.01f*m_VPBSize->GetValue();
	brushSize = pow(brushSize, 2.0f);

	Bool useElement = m_VPSelectElement->IsChecked();
	Bool chunkSelected = false;

	if( useElement )
	{
		Uint32 selectedChunk = 0;
		Uint32 selectedVertex = 0;

		for( Uint32 j=0; j< chunksToCheck.Size(); ++j )
		{
			Uint32 chunkNum = chunksToCheck[j];
			auto& chunk = chunks[chunkNum];
		
			for(Uint32 i=0; i<chunk.m_numVertices; i++)
			{
				vPos = Vector3( chunk.m_vertices[i].m_position );
				vNorm = Vector3( chunk.m_vertices[i].m_normal );

				if( CheckVertexHit( vPos, vNorm, distToClosestVert, brushSize ) )
				{
					selectedChunk = chunkNum;
					selectedVertex = i;
					chunkSelected = true;
					break;
				}
			}
			if( chunkSelected ) break;
		}

		if ( chunkSelected )
		{
			SelectAllVerticesInElement( selectedChunk, selectedVertex );
		}
	}
	else
	{
		for( Uint32 j=0; j< chunksToCheck.Size(); ++j )
		{
			Uint32 chunkNum = chunksToCheck[j];
			const auto& chunk = chunks[chunkNum];

			if ( m_currentVerts.Size() < chunkNum + 1 )
			{
				m_currentVerts.ResizeFast( chunkNum + 1 );
			}

			for(Uint32 i=0; i<chunk.m_numVertices; i++)
			{
				vPos = Vector3( chunk.m_vertices[i].m_position );
				vNorm = Vector3( chunk.m_vertices[i].m_normal );

				if( CheckVertexHit( vPos, vNorm, distToClosestVert, brushSize ) )
				{
					m_currentVerts[ chunkNum ].verts.PushBackUnique( i );
				}
			}
		}
	}

	CalculateBoundsOfCurrentSelection();
}

void CEdVertexPaintTool::SelectAllVerticesInElement( Int32 chunkIndex, Int32 startingIndex )
{
	DeselectAllVertices();

	auto& chunks = m_meshData->GetChunks();
	TDynArray<Uint16> chunksToCheck;
	CollectChunksToCheck( chunksToCheck );

	Uint32 chunkNum = chunksToCheck[ chunkIndex ];
	const SMeshChunk& chunk = chunks[chunkNum];

	if ( m_currentVerts.Size() < chunkNum + 1 )
	{
		m_currentVerts.ResizeFast( chunkNum + 1 );
	}


	TDynArray<Uint16> commonVerts;
	commonVerts.PushBack(startingIndex);

	Bool noMoreTrianglesFound = false;

	while( !noMoreTrianglesFound )
	{
		Bool foundNewIndex = false;

		for(Uint32 j=0; j<chunk.m_numIndices-3; j+=3)
		{
			for(Uint32 v=0; v<commonVerts.Size(); v++)
			{
				if( commonVerts[v] == chunk.m_indices[j] || commonVerts[v] == chunk.m_indices[j+1] || commonVerts[v] == chunk.m_indices[j+2] )
				{
					if( commonVerts.PushBackUnique( chunk.m_indices[j] ) ||
						commonVerts.PushBackUnique( chunk.m_indices[j+1] ) ||
						commonVerts.PushBackUnique( chunk.m_indices[j+2] ) )
					{
						m_currentVerts[chunkNum].verts.PushBackUnique( chunk.m_indices[j] );
						m_currentVerts[chunkNum].verts.PushBackUnique( chunk.m_indices[j+1] );
						m_currentVerts[chunkNum].verts.PushBackUnique( chunk.m_indices[j+2] );

						foundNewIndex = true;
						break;
					}
				}
			}
		}

		if( !foundNewIndex ) 
			noMoreTrianglesFound = true;
	}
	commonVerts.Clear();
}

void CEdVertexPaintTool::CalculateBoundsOfCurrentSelection( void )
{
	auto& chunks = m_meshData->GetChunks();
	Box box( Box::RESET_STATE );

	for ( Uint32 i = 0; i < m_currentVerts.Size(); ++i )
	{
		auto& chunk = chunks[i];
		const TDynArray< Int32 >& selected = m_currentVerts[i].verts;
		for ( Uint32 j = 0; j < selected.Size(); ++j )
		{
			const SMeshVertex& vert = chunk.m_vertices[ selected[j] ];
			box.AddPoint( Vector3( vert.m_position ) );
		}
	}

	m_SelectedElementCenter = box.CalcCenter();
	m_SelectedElementSize = box.CalcExtents();
}

Uint32 CEdVertexPaintTool::GetFinalColor( Uint32 vertexColor, wxColour wC, Bool useTextureBlendingMode, Uint8 blendModeColor  )
{
	Color cColor( vertexColor );

	// paint color
	if( m_VPChRed->IsChecked() )	cColor.R = wC.Red();
	if( m_VPChGreen->IsChecked() )	cColor.G = wC.Green();
	if( m_VPChBlue->IsChecked() )	cColor.B = wC.Blue();
	if( m_VPChAlpha->IsChecked() )	cColor.A = wC.Alpha();

	// check if we want to blend texture paint
	if( useTextureBlendingMode )
	{
		cColor.G = blendModeColor;
	}
	return cColor.ToUint32();
}

Bool CEdVertexPaintTool::Edit( void )
{	
	if( m_VPToggleSelection->IsChecked() )
	{
		SelectVertex();
	}
	else if( PaintVertex() )
	{	
		m_mesh->CreateRenderResource();
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
	return true;
}

Bool CEdVertexPaintTool::CheckVertexHit( Vector3 vPos, Vector3 vNorm, Float& distToClosestVert, Float brushSize )
{
	Vector v = mOrigin - vPos;	
	float vDir = v.Dot3(mDir);
		
	float vRadius = brushSize*brushSize;				

	float currRayToVertSqrDist = ( mDir*( vDir/mDir.Dot3(mDir)) - v).SquareMag3();

	if( distToClosestVert > currRayToVertSqrDist )
	{
		distToClosestVert = currRayToVertSqrDist;
		//m_ClosestVertPos = vPos + vNorm.Normalized()*0.05f;
		//m_ClosestVertNorm = vNorm;
	}

	if( currRayToVertSqrDist < vRadius ) return true;
	else
		return false;
}

Bool CEdVertexPaintTool::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame, const Vector& origin, const Vector& dir, bool displaySelection )
{
	 Bool selectionMode = m_VPToggleSelection->IsChecked();

	Matrix mat = Matrix::IDENTITY;
	
	Box box;
	Float vertDisplaySize = 0.0001f*m_VPBrushLocalScale;
	Color c = Color::RED;

	Float brushSize = 0.01f*m_VPBSize->GetValue();
	brushSize = pow(brushSize, 2.0f);

	Vector worldPos = Vector(0.0f,0.0f,0.0f);	
	bool setVertexSelection = false;

	Float distToClosestVert = 10000.0f;	

	Vector3 vPos = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 vNorm = Vector3(0.0f, 0.0f, 1.0f);

	auto& chunks = m_meshData->GetChunks();
	TDynArray<Uint16> chunksToCheck;
	CollectChunksToCheck( chunksToCheck );

	mOrigin = origin;
	mDir = dir;

	for ( Uint32 j=0; j< chunksToCheck.Size(); ++j )
	{
		Uint32 chunkNum = chunksToCheck[j];
		auto& chunk = chunks[chunkNum];

		// paint mode
		if( !selectionMode )
		{
			if ( m_currentVerts.Size() < chunkNum + 1 )
			{
				m_currentVerts.ResizeFast( chunkNum + 1 );
			}

			m_currentVerts[chunkNum].verts.Clear();

			for(Uint32 i=0; i<chunk.m_numVertices; i++)
			{
				vPos = Vector3( chunk.m_vertices[i].m_position );
				vNorm = Vector3( chunk.m_vertices[i].m_normal );

				if(displaySelection)
				{
					if( CheckVertexHit( vPos, vNorm, distToClosestVert, brushSize ) )
					{
						m_currentVerts[chunkNum].verts.PushBack(i);
						setVertexSelection = true;

						box.Min = vPos + Vector3( -vertDisplaySize, -vertDisplaySize, -vertDisplaySize );
						box.Max = vPos + Vector3( vertDisplaySize, vertDisplaySize, vertDisplaySize );

						frame->AddDebugBox( box, mat, c );
					}
				}
			}
		}
		// selection mode
		else
		{	
			for(Uint32 i=0; i<chunk.m_numVertices; i++)
			{				
				vPos = Vector3( chunk.m_vertices[i].m_position );
				vNorm = Vector3( chunk.m_vertices[i].m_normal );
				Bool vertexInRange = false;

				// show where current brush hit
				if( CheckVertexHit( vPos, vNorm, distToClosestVert, brushSize ) )
				{
					c = Color::GRAY;			

					box.Min = vPos + Vector3( -vertDisplaySize, -vertDisplaySize, -vertDisplaySize );
					box.Max = vPos + Vector3( vertDisplaySize, vertDisplaySize, vertDisplaySize );

					frame->AddDebugBox( box, mat, c );
				}
				/*
				else
				{
					
					for(Uint32 s=0; s<m_currentVerts[j].verts.Size(); s++)
					{			
						if( m_currentVerts[j].verts[s] == i )
						{
							//vPos = Vector3( chunk.m_vertices[ m_currentVerts[j].verts[s] ].m_position[0], chunk.m_vertices[ m_currentVerts[j].verts[s] ].m_position[1], chunk.m_vertices[ m_currentVerts[j].verts[s] ].m_position[2] );
							c = Color::RED;
							break;
						}

						box.Min = vPos + Vector3( -vertDisplaySize, -vertDisplaySize, -vertDisplaySize );
						box.Max = vPos + Vector3( vertDisplaySize, vertDisplaySize, vertDisplaySize );

						frame->AddDebugBox( box, mat, c );
					}					

				}
				*/				
			}
						
			if( m_SelectedElementSize.SquareMag3() > 0.01f )
			{			
				c = Color::RED;

				box.Min = m_SelectedElementCenter + Vector3( -m_SelectedElementSize.X, -m_SelectedElementSize.Y, -m_SelectedElementSize.Z );
				box.Max = m_SelectedElementCenter + Vector3( m_SelectedElementSize.X, m_SelectedElementSize.Y, m_SelectedElementSize.Z );

				// add big box surrounding current selected element
				frame->AddDebugBox( box, mat, c );
			}		
		}
	}	

	return true;

}


void CEdVertexPaintTool::OnSaveVC( wxCommandEvent& event )
{
	auto& chunks = m_meshData->GetChunks();

	TDynArray<Uint16> chunksToCheck;
	CollectChunksToCheck( chunksToCheck );

	Uint32 minJ = 0;
	Uint32 minI = 0;
	Vector3 vPos;
	Vector vCol;

	const Box & aabb = m_mesh->GetBoundingBox();
	Vector min = aabb.Min + Vector(-0.01f,-0.01f,-0.01f);
	Vector max = aabb.Max + Vector( 0.01f, 0.01f, 0.01f);
	//make sure its not flat

	Float gridSize = 0.05f; //grid ma 1cm
	Int32   maxGrid = 100; //max resolution
	Vector os = max - min;

	Int32 gridX = Int32( os.X / gridSize ) + 1;
	Int32 gridY = Int32( os.Y / gridSize ) + 1;
	Int32 gridZ = Int32( os.Z / gridSize ) + 1;

	gridX = gridX > maxGrid ? maxGrid : gridX;
	gridY = gridY > maxGrid ? maxGrid : gridY;
	gridZ = gridZ > maxGrid ? maxGrid : gridZ;

	Int32* data = new Int32[gridX*gridY*gridZ*4];
	Red::System::MemorySet(data, 0, sizeof(Int32)*gridX*gridY*gridZ*4);

	Int32* datanum = new Int32[gridX*gridY*gridZ];
	Red::System::MemorySet(datanum, 0, sizeof(Int32)*gridX*gridY*gridZ);

	TDynArray<Int32> indices;


	for( Uint32 j=0; j< chunksToCheck.Size(); ++j )
	{	
		Uint32 chunkNum = chunksToCheck[j];
		auto& chunk = chunks[chunkNum];		
		for(Uint32 i=0; i<chunk.m_numVertices; i++)
		{
			vPos = Vector3( chunk.m_vertices[i].m_position );
			UINT32 Col = chunk.m_vertices[i].m_color;


			Uint8 r = (Uint8)(Col);
			Uint8 g = (Uint8)(Col >> 8);	
			Uint8 b = (Uint8)(Col >> 16);
			Uint8 a = (Uint8)(Col >> 24);

			Vector delta = vPos - min;
			delta.X /= os.X;
			delta.Y /= os.Y;
			delta.Z /= os.Z;
			delta.X *= gridX;
			delta.Y *= gridY;
			delta.Z *= gridZ;
			Int32 indX = Int32(delta.X);
			Int32 indY = Int32(delta.Y);
			Int32 indZ = Int32(delta.Z);

			if( indX>=0 && indY>=0 && indZ>=0 && indX<gridX && indY<gridY && indZ<gridZ )
			{
				Int32 ind = ((gridX*gridY)*indZ + (gridX)*indY + indX);
				data[ind*4+0] += r;
				data[ind*4+1] += g;
				data[ind*4+2] += b;
				data[ind*4+3] += a;
				datanum[ind] ++;
				if( !indices.FindPtr( ind ) )
				{
					indices.PushBack(ind);
				}
			}

		}

		String path = m_mesh->GetFile()->GetAbsolutePath();
		size_t kropka = -1;
		path.FindCharacter('.', kropka);
		path = path.LeftString( kropka );
		path += TXT(".w2vc");

		//LOG_EDITOR( TXT("%s"), path.AsChar() );


		FILE* f = fopen(UNICODE_TO_ANSI(path.AsChar()),"wb");
		UINT num = indices.Size();
		fwrite(&num, sizeof(Int32),1,f);
		fwrite(&gridX, sizeof(Int32),1,f);
		fwrite(&gridY, sizeof(Int32),1,f);
		fwrite(&gridZ, sizeof(Int32),1,f);

		fwrite(&min.X, sizeof(Float),1,f);
		fwrite(&min.Y, sizeof(Float),1,f);
		fwrite(&min.Z, sizeof(Float),1,f);

		fwrite(&max.X, sizeof(Float),1,f);
		fwrite(&max.Y, sizeof(Float),1,f);
		fwrite(&max.Z, sizeof(Float),1,f);

		for(UINT i=0;i<num;i++)
		{
			Int32 ind = indices[i];
			Int32 indZ = ind / (gridX*gridY);
			Int32 indXY = ind % (gridX*gridY);
			Int32 indY = indXY / (gridX);
			Int32 indX = indXY % (gridX);
			data[ind*4+0] /= datanum[ind];
			data[ind*4+1] /= datanum[ind];
			data[ind*4+2] /= datanum[ind];
			data[ind*4+3] /= datanum[ind];

			fwrite(&indX, sizeof(Int32),1,f);
			fwrite(&indY, sizeof(Int32),1,f);
			fwrite(&indZ, sizeof(Int32),1,f);

			fwrite(&data[ind*4+0], sizeof(Int32),1,f);
			fwrite(&data[ind*4+1], sizeof(Int32),1,f);
			fwrite(&data[ind*4+2], sizeof(Int32),1,f);
			fwrite(&data[ind*4+3], sizeof(Int32),1,f);

		}

		fclose(f);

	}

	delete [] datanum;
	delete [] data;


}
void CEdVertexPaintTool::OnLoadVC( wxCommandEvent& event )
{
	auto& chunks = m_meshData->GetChunks();

	TDynArray<Uint16> chunksToCheck;
	CollectChunksToCheck( chunksToCheck );

	Vector3 vPos;
	Vector vCol;

	Vector min;
	Vector max;

	String path = m_mesh->GetFile()->GetAbsolutePath();
	size_t kropka = -1;
	path.FindCharacter('.', kropka);
	path = path.LeftString( kropka );
	path += TXT(".w2vc");

	FILE* f = fopen(UNICODE_TO_ANSI(path.AsChar()),"rb");
	if(f)
	{
		UINT num = 0;
		Int32 gridX = 0;
		Int32 gridY = 0;
		Int32 gridZ = 0;
		Int32 r = 0;
		Int32 g = 0;
		Int32 b = 0;
		Int32 a = 0;
		fread(&num, sizeof(Int32),1,f);
		fread(&gridX, sizeof(Int32),1,f);
		fread(&gridY, sizeof(Int32),1,f);
		fread(&gridZ, sizeof(Int32),1,f);

		fread(&min.X, sizeof(Float),1,f);
		fread(&min.Y, sizeof(Float),1,f);
		fread(&min.Z, sizeof(Float),1,f);

		fread(&max.X, sizeof(Float),1,f);
		fread(&max.Y, sizeof(Float),1,f);
		fread(&max.Z, sizeof(Float),1,f);

		Vector os = max - min;

		Int32* data = new Int32[gridX*gridY*gridZ*4];
		Red::System::MemorySet(data, 0, sizeof(Int32)*gridX*gridY*gridZ*4);

		for(UINT i=0;i<num;i++)
		{
			Int32 indZ = 0;
			Int32 indY = 0;
			Int32 indX = 0;

			fread(&indX, sizeof(Int32),1,f);
			fread(&indY, sizeof(Int32),1,f);
			fread(&indZ, sizeof(Int32),1,f);

			Int32 ind = ((gridX*gridY)*indZ + (gridX)*indY + indX);

			fread(&r, sizeof(Int32),1,f);
			fread(&g, sizeof(Int32),1,f);
			fread(&b, sizeof(Int32),1,f);
			fread(&a, sizeof(Int32),1,f);

			data[ind*4+0] = r;
			data[ind*4+1] = g;
			data[ind*4+2] = b;
			data[ind*4+3] = a;
		}
		fclose(f);


		for( Uint32 j=0; j< chunksToCheck.Size(); ++j )
		{	
			Uint32 chunkNum = chunksToCheck[j];
			SMeshChunk& chunk = chunks[chunkNum];		
			for(Uint32 i=0; i<chunk.m_numVertices; i++)
			{
				vPos = Vector3( chunk.m_vertices[i].m_position[0], chunk.m_vertices[i].m_position[1], chunk.m_vertices[i].m_position[2] );
				UINT32 Col = chunk.m_vertices[i].m_color;
				Vector delta = vPos - min;
				delta.X /= os.X;
				delta.Y /= os.Y;
				delta.Z /= os.Z;
				delta.X *= gridX;
				delta.Y *= gridY;
				delta.Z *= gridZ;
				Int32 indX = Int32(delta.X);
				Int32 indY = Int32(delta.Y);
				Int32 indZ = Int32(delta.Z);

				Int32 ind = ((gridX*gridY)*indZ + (gridX)*indY + indX);

				r = data[ind*4+0];
				g = data[ind*4+1];
				b = data[ind*4+2];
				a = data[ind*4+3];

				Color cColor;

				cColor.R = (Uint8)r;
				cColor.G = (Uint8)g;
				cColor.B = (Uint8)b;
				cColor.A = (Uint8)a;

				chunk.m_vertices[i].m_color = cColor.ToUint32();
			}
		}

		m_mesh->CreateRenderResource();	
		CDrawableComponent::RecreateProxiesOfRenderableComponents();

		delete [] data;
	}
	else
	{
		LOG_EDITOR( TXT("Cant open: %s"), path.AsChar() );
	}
}

void CEdVertexPaintTool::CollectChunksToCheck( TDynArray<Uint16>& chunksToCheck )
{
	if ( m_paintAllLods->GetValue() )
	{
		for ( Uint32 i = 0; i < m_mesh->GetNumLODLevels(); ++i )
		{
			chunksToCheck.PushBackUnique( m_mesh->GetMeshLODLevels()[i].m_chunks );
		}
	}
	else
	{
		Int32 lod = m_lod->GetValue();
		if ( lod < (Int32)m_mesh->GetNumLODLevels() )
		{
			chunksToCheck = m_mesh->GetMeshLODLevels()[lod].m_chunks;
		}
	}
}

void CEdVertexPaintTool::LodsChanged()
{
	if ( !m_currentVerts.Empty() )
	{
		// First clear, so we deselect any vertices.
		m_currentVerts.ClearFast();
		m_currentVerts.Resize( m_mesh->GetChunks().Size() );
	}

	m_lod->SetMin( 0 );
	m_lod->SetMax( m_mesh->GetNumLODLevels() - 1 );
}
