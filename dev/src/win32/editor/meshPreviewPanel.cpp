/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshPreviewPanel.h"
#include "meshEditor.h"
#include "meshTypePreviewComponent.h"
#include "voxelization.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionCache.h"
#include "vertexPaintTool.h"
#include "dataError.h"

#include "../../common/physics/compiledCollision.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/fonts.h"


BEGIN_EVENT_TABLE( CEdMeshPreviewPanel, CEdInteractivePreviewPanel )	
END_EVENT_TABLE()


CEdMeshPreviewPanel::CEdMeshPreviewPanel(wxWindow* parent, CEdMeshEditor* editor )
	: CEdInteractivePreviewPanel( parent, true )
	, m_meshEditor( editor )
	, m_showPlaneFloor( false )
	, m_debugPlaneZ(0.0f)
	, m_vertexPaintTool (NULL)
	, m_showCollision( false )
	, m_showNavObstacles( false )
	, m_showWireframe( false )
	, m_showBoundingBox( false )
	, m_textureArraysDataSize( TXT("no texture arrays data loaded"))
	, m_textureDataSize( TXT("no texture data loaded"))
	, m_meshDataSize( TXT("no render data loaded"))
	, m_bbDiagInfo( TXT("no data") )
{
	// Create entity
	EntitySpawnInfo einfo;
	m_entity = m_previewWorld->GetDynamicLayer()->CreateEntitySync( einfo );
	m_entity->SetStreamed( false );
	m_meshTypeComponent = CMeshTypePreviewComponent::Create( m_entity, editor->GetMesh() );

	UpdateBBoxInfo();
}

CEdMeshPreviewPanel::~CEdMeshPreviewPanel()
{
}


EMeshTypePreviewType CEdMeshPreviewPanel::GetMeshType()
{
	if (m_meshTypeComponent )
	{
		return m_meshTypeComponent->GetMeshType();
	}
	return MTPT_Unknown;
}


void CEdMeshPreviewPanel::Reload()
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->Reload( m_meshEditor->GetMesh() );
		UpdateBBoxInfo();
	}
}

void CEdMeshPreviewPanel::Refresh()
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->Refresh();
		UpdateBBoxInfo();
	}
}

void CEdMeshPreviewPanel::ShowWireframe( Bool show )
{
	if ( show )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Wireframe );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Wireframe );
	}
	m_showWireframe = show;
}

void CEdMeshPreviewPanel::ShowBoundingBox( Bool show )
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->ShowBoundingBox( show );
	}
	m_showBoundingBox = show;
}

void CEdMeshPreviewPanel::ShowCollision( Bool show )
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->ShowCollision( show );
	}
	m_showCollision = show;
}

void CEdMeshPreviewPanel::ShowNavObstacles( Bool show )
{
	m_showNavObstacles = show;
}

void CEdMeshPreviewPanel::ShowTBN( Bool show )
{
	m_showTBN = show;
}

void CEdMeshPreviewPanel::UpdateBounds()
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->UpdateBounds();
	}
}

void CEdMeshPreviewPanel::OverrideViewLOD( Int32 lodOverride )
{
	if ( m_meshTypeComponent )
	{
		m_meshTypeComponent->OverrideViewLOD( lodOverride );
		UpdateBBoxInfo();
	}
}

static Float CalculateDistanceToDC( const CDrawableComponent* dc, const Vector& position, Bool shouldCalcToBBoxCenter = false )
{	
	const Box& box = dc->GetBoundingBox();
	if ( !shouldCalcToBBoxCenter )
	{
		return box.Distance( position );
	}
	else
	{
		const Vector center = box.CalcCenter();
		return center.DistanceTo(position);
	}
}

void CEdMeshPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_showCollision )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Collision );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Collision );
	}

	if ( m_showNavObstacles )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_NavObstacles );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_NavObstacles );
	}

	if ( m_showWireframe )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Wireframe );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Wireframe );
	}

	if ( m_showBoundingBox )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Bboxes );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Bboxes );
	}

	if ( m_showTBN )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_TBN );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_TBN );
	}


	// show AO floor plane
	if( m_showPlaneFloor )
	{	
		Matrix mat = Matrix::IDENTITY;

		mat.SetScale33( 10.0f );
		mat.V[3].A[2] = m_debugPlaneZ;

		frame->AddDebugFilledRect(mat, Color::GRAY);		
	}

	// Draw the basic shit
	CEdInteractivePreviewPanel::OnViewportGenerateFragments( view, frame );

	// Draw the stats
	Int32   lineIndex = 1;
	Uint32 	lineHeight = 1.2f*m_font->GetLineDist();

	// Draw the distance from the mesh
	if ( m_meshTypeComponent && m_meshTypeComponent->GetMeshTypeResource() )
	{
		if ( m_meshTypeComponent->GetDrawableComponent() )
		{
			// Show the distance from the camera to the mesh
			const Float distance = CalculateDistanceToDC( m_meshTypeComponent->GetDrawableComponent(), frame->GetFrameInfo().m_camera.GetPosition() );

			frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, String::Printf( TXT("Distance: %1.2fm"), distance ), 0, false, Color(255,255,255), Color(0,0,0) );
			lineIndex++;

			// Show the distance from the camera to bbox center
			const Float distanceToBBox = CalculateDistanceToDC( m_meshTypeComponent->GetDrawableComponent(), frame->GetFrameInfo().m_camera.GetPosition(), true );

			frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, String::Printf( TXT("Distance to bbox center: %1.2fm"), distanceToBBox ), 0, false, Color(255,255,255), Color(0,0,0) );
			lineIndex++;
		}

		// Size
		const Vector size = m_meshTypeComponent->GetMeshTypeResource()->GetBoundingBox().CalcSize();
		const String sizeTxt = String::Printf( TXT("Size: %1.2f x %1.2f x %1.2f"), size.X, size.Y, size.Z );		
		
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, sizeTxt, 0, false, Color(255,255,255), Color(0,0,0) );
		lineIndex++;		

		// Collision
		const CCollisionMesh* colMesh = m_meshTypeComponent->GetCollisionMesh();
		if ( !colMesh )
		{
			// Size	
			frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, TEXT("No collision"), 0, false, Color(255,255,255), Color(0,0,0) ); 
			lineIndex++;					
		}

		// Estimate the memory size
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_textureArraysDataSize.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );
		lineIndex++;
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_textureDataSize.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );
		lineIndex++;
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_meshDataSize.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );	
		lineIndex++;
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_bbDiagInfo.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );	
		lineIndex++;

#ifdef PROFILING_PHYSX
		static CPerfCounter* physicsFetchTimePerfCounter = 0;
		if( !physicsFetchTimePerfCounter ) physicsFetchTimePerfCounter = CProfiler::GetCounter( "Physics scene preview simulation" );

		if( physicsFetchTimePerfCounter )
		{
			const Double freq = Red::System::Clock::GetInstance().GetTimer().GetFrequency();

			static Uint64 previousTime = 0;
			const Uint64 time = physicsFetchTimePerfCounter->GetTotalTime();
			static Uint32 previousHit = 0;
			wxString string = wxString( "Physics simulation: " ) + wxString::Format( wxT("%1.6f ms"), (float)((time - previousTime)/freq)*1000.0f );
			frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, string.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );	
			previousTime = time;
		}
#endif

		Uint32 y = frame->GetFrameOverlayInfo().m_height - 155;
		if ( colMesh )
		{
			CMesh* mesh = m_meshTypeComponent->GetMesh();
			if( mesh )
			{
				CompiledCollisionPtr compiledMesh;

				RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
				GCollisionCache->Compile_Sync( compiledMesh, colMesh, mesh->GetDepotPath(), mesh->GetFileTime() );

				if( compiledMesh )
				{
					Float mass = compiledMesh->GetMassFromResource();
					if( mass > 1000 )
					{
						frame->AddDebugScreenText( 30, y, String::Printf( TXT("Physics mass: %.4f t"), mass/1000 ), 0, false, Color(255,255,255), Color(0,0,0) ); 
					}
					else
					{
						frame->AddDebugScreenText( 30, y, String::Printf( TXT("Physics mass: %.4f kg"), mass ), 0, false, Color(255,255,255), Color(0,0,0) ); 
					}
				}

			}
		}
		if ( CMeshTypePreviewFurComponent* furPrevCmp = Cast< CMeshTypePreviewFurComponent >( m_meshTypeComponent ) )
		{
			if ( !furPrevCmp->IsUsingFur() )
			{
				frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, String::Printf( TXT("Please set platform to Uber to see fur") ), 0, false, Color(255,0,0), Color(0,0,0) ); 
			}
		}
	}

	/* debug this
	{
		if ( ::GetAsyncKeyState( VK_SHIFT) != 0 )
		{
			if ( m_tree )
			{
				if ( m_points.Empty() )
				{
					m_tree->ListVectors( m_tree->m_root, m_points, m_normals, m_colors, 8, m_tree->m_minX, m_tree->m_minY, m_tree->m_minZ, m_tree->m_maxX, m_tree->m_maxY, m_tree->m_maxZ );
				}

				for ( Uint32 i = 0; i < m_points.Size(); ++i )
				{
					Vector origin = m_points[i];
					Vector normal = m_normals[i];
					Vector target = m_points[i] + (normal.Mul3( 0.1f ));

					Uint8 b = (m_colors[i] & 0x000000FF);
					Uint8 r = (m_colors[i] & 0x00FF0000)>> 16;
					Uint8 g = (m_colors[i] & 0x0000FF00 ) >> 8;

					frame->AddDebugLine( origin, target, Color( r*r/256,g*g/256,b*b/256,255 ) );
				}
			}
		}
	}
	*/

	// draw informations from data error reporter
	{
		// few extra lines to prevent from displaying on UMBRA thing information
		lineIndex += 2; 
		TDynArray< String > errors;
		m_meshEditor->GetDataErrors( errors );
		Uint32 errorCount = errors.Size();

		if( errorCount > 0 )
		{
			// draw header
			const String errorsHeader = TXT("DATA ERROR REPORTS:");
			frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, errorsHeader.AsChar(), 0, false, Color(255,0,0), Color(0,0,0) );	
			++lineIndex;
			
			// draw priorities' legend 
			frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, TXT("Priorities: 1 - game will probably not work; 2 - major error; 3 - minor error; 4 - cosmetic thing"), 
				0, false, Color(0,255,0), Color(0,0,0) );
			++lineIndex;

			// draw messages
			for( Uint32 i=0; i<errorCount; ++i )
			{
				frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, errors[i].AsChar(), 0, false, Color(255,0,0), Color(0,0,0) );	
				++lineIndex;
			}
		}
	}

	if( m_vertexPaintTool && m_vertexPaintTool->m_showVertices )
	{
		//Calculate cursor position
		POINT cursorPoint;
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );

		// Calculate ray
		Vector origin, dir;
		GetViewport()->CalcRay( cursorPoint.x, cursorPoint.y, origin, dir );
		dir.Normalize3();

		// cursor over preview window, check if over mesh and mark selected verts
		bool cursorOverPreviewWindow = false;

		if(	cursorPoint.x > 0.0f && cursorPoint.x < (float)GetViewport()->GetWidth()
			&& cursorPoint.y > 0.0f && cursorPoint.y < (float)GetViewport()->GetHeight())
			cursorOverPreviewWindow = true;

		m_vertexPaintTool->OnViewportGenerateFragments( view, frame, origin, dir, cursorOverPreviewWindow );
	}	
}

void CEdMeshPreviewPanel::UpdateBBoxInfo()
{
	if ( m_meshEditor )
	{
		Box bbox = m_meshEditor->GetMesh()->GetBoundingBox();
		Float halfDiag = ( bbox.Max - bbox.Min ).Mag3() * 0.5f;
		m_bbDiagInfo = wxString( "Half of bbox diagonal: " ) + ToString( halfDiag ).AsChar();
	}
}

Bool CEdMeshPreviewPanel::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if( !m_vertexPaintTool || button == 1 )//IK_RightMouse ) 
	{
		if ( m_meshTypeComponent )
		{
			m_meshTypeComponent->OnViewportClick( view, button, state, x, y );
		}

		CEdInteractivePreviewPanel::OnViewportClick( view, button, state, x, y );
	}
	else
	{	
		if( m_vertexPaintTool )
		{			
			if( button == 0 ) m_vertexPaintTool->Edit();
		}	
	}
	return true;
}

Bool CEdMeshPreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{	
	if( m_vertexPaintTool && key != 0 ) // && ( key == IK_LeftMouse ) && action == IACT_Release )
	{
		if( key == IK_LeftBracket || key ==  IK_MouseWheelDown ) 
			m_vertexPaintTool->SetBrushSize(-1);
		if( key == IK_RightBracket || key ==  IK_MouseWheelUp ) 
			m_vertexPaintTool->SetBrushSize(1);
	}

	if ( m_meshTypeComponent && m_meshTypeComponent->OnViewportInput( view, key, action, data ) )
	{
		return true;
	}

	return CEdInteractivePreviewPanel::OnViewportInput( view, key, action, data );
}

Bool CEdMeshPreviewPanel::OnViewportMouseMove( const CMousePacket& packet )
{
	if ( m_meshTypeComponent && m_meshTypeComponent->OnViewportMouseMove( packet ) )
	{
		return true;
	}

	return CEdInteractivePreviewPanel::OnViewportMouseMove( packet );
}

// void CEdMeshPreviewPanel::OnVertexPaint( wxCommandEvent& event )
// {
// 	//	GetViewport()->CaptureInput()
// 
// }
