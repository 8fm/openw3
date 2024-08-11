/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "toolsPanel.h"
#include "sceneExplorer.h"
#include "../../common/game/actionAreaVertex.h"
#include "stripeEdit.h"
#include "undoCreate.h"
#include "undoVertex.h"
#include "../../common/core/depot.h"
#define ONLY_RENDER_PROXY_STRIPE_PROPERTIES
#include "../../common/renderer/renderProxyStripe.h"
#include "undoStripe.h"
#include "undoTerrain.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/bitmapTexture.h"
#include "presetsBox.h"


wxIMPLEMENT_CLASS( CEdStripeToolPanel, wxPanel );

enum {
	ID_CREATE_STRIPE=1000,
	ID_ALIGN_TERRAIN_TO_STRIPES,
	ID_ALIGN_STRIPES_TO_TERRAIN,
	ID_SPLIT_STRIPE,
	ID_CLOSE_STRIPE_EDITOR
};

IMPLEMENT_ENGINE_CLASS( CEdStripeEdit );

RED_DECLARE_NAME( StripeDeleted );

static Bool ViewAllRoadLines = false;
static Bool ViewAIRoadLines = true;

//////////////////////////////////////////////////////////////////////////

class CEdStripeEditPresetsBoxHook : public IEdPresetsBoxHook
{
public:
	CEdStripeEdit*	m_tool;

	// Saves the preset files
	void SavePresets( CEdPresets* presets )
	{
		CDirectory* presetsDir = GDepot->FindPath( TXT("engine\\presets\\stripes\\") );
		if ( presetsDir == nullptr )
		{
			GDepot->CreatePath( TXT("engine\\presets\\stripes\\") );
			presetsDir = GDepot->FindPath( TXT("engine\\presets\\stripes\\") );
		}
		if ( presetsDir != nullptr )
		{
			presets->SavePresetFiles( presetsDir->GetAbsolutePath(), true );
		}
	}

	//! Called to confirm that a preset can be applied
	virtual Bool OnCanApplyPreset( CEdPresetsBox* /* source */, const String& /* presetName */ )
	{
		// Try to mark all edited stripes as modified
		for ( auto it=m_tool->m_stripes.Begin(); it != m_tool->m_stripes.End(); ++it )
		{
			CEdStripeEdit::CEditedStripe* editedStripe = *it;
			if ( !editedStripe->m_stripe->MarkModified() )
			{
				return false;
			}
		}

		return true;
	}

	//! Called to apply a preset
	virtual void OnApplyPreset( CEdPresetsBox* source, const String& presetName )
	{
		// Go over all edited stripes
		for ( auto it=m_tool->m_stripes.Begin(); it != m_tool->m_stripes.End(); ++it )
		{
			CEdStripeEdit::CEditedStripe* editedStripe = *it;

			// Make sure we can modify this stripe
			if ( !editedStripe->m_stripe->MarkModified() )
			{
				continue;
			}

			// Apply the preset
			source->GetPresets()->ApplyPresetToObject( presetName, editedStripe->m_stripe, TXT("stripe") );
		}

		// Update all stripes
		m_tool->UpdateAllStripes();
	}

	//! Called to save a preset
	virtual void OnSavePreset( CEdPresetsBox* source, const String& presetName )
	{
		// Go over all edited stripes
		for ( auto it=m_tool->m_stripes.Begin(); it != m_tool->m_stripes.End(); ++it )
		{
			CEdStripeEdit::CEditedStripe* editedStripe = *it;

			// Save the preset
			source->GetPresets()->SetPresetFromObject( presetName, editedStripe->m_stripe, TXT("stripe") );

			// We only care about the first stripe we find usable
			break;
		}

		// Save preset files
		SavePresets( source->GetPresets() );
	}

	//! Called after a preset has been deleted
	virtual void OnPresetDeleted( CEdPresetsBox* source, const String& presetName )
	{
		// Just save the new presets
		SavePresets( source->GetPresets() );
	}
};

//////////////////////////////////////////////////////////////////////////

void CEdStripeEdit::CEditedStripe::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	m_editor->OnStripePointMoved( this, vertexIndex, oldPosition, wishedPosition, allowedPosition );
}

void CEdStripeEdit::CEditedStripe::OnEditorNodeRotated( Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation )
{
	m_editor->OnStripePointRotated( this, vertexIndex, oldRotation, wishedRotation, allowedRotation );
}

//////////////////////////////////////////////////////////////////////////

CEdStripeEdit::CEdStripeEdit()
	: m_world( nullptr )
	, m_viewport( nullptr )
	, m_nearbyStripe( nullptr )
	, m_nearbyVertex( 0 )
{
}

void CEdStripeEdit::OnStripePointMoved( CEditedStripe* editedStripe, Int32 index, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	if ( editedStripe->m_stripe->MarkModified() )
	{
		// Make sure the point is inside the terrain
		Box terrainBox;
		terrainBox.Min = m_world->GetTerrain()->GetTerrainCorner();
		terrainBox.Max = terrainBox.Min + Vector( m_world->GetTerrain()->GetTerrainSize(), m_world->GetTerrain()->GetTerrainSize(), 0.0f );
		terrainBox.Min.Z = -10000000;
		terrainBox.Max.Z = 10000000;
		if ( !terrainBox.Contains( wishedPosition ) )
		{
			allowedPosition = oldPosition;
			return;
		}

		// Move it
		Matrix worldToLocal;
		editedStripe->m_stripe->GetWorldToLocal( worldToLocal );
		editedStripe->m_stripe->m_points[index].m_position = worldToLocal.TransformPoint( wishedPosition );
		allowedPosition = wishedPosition;
		UpdateStripe( editedStripe );

		// Inform the panel
		m_panel->PointModified( index );
	}
	else
	{
		allowedPosition = oldPosition;
	}
}

void CEdStripeEdit::OnStripePointRotated( CEditedStripe* editedStripe, Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation )
{
	if ( editedStripe->m_stripe->MarkModified() )
	{
		editedStripe->m_stripe->m_points[vertexIndex].m_rotation = wishedRotation;
		m_panel->PointModified( vertexIndex );
		allowedRotation = wishedRotation;
	}
	else
	{
		allowedRotation = oldRotation;
	}
}

void CEdStripeEdit::UpdateStripe( CEditedStripe* editedStripe )
{
	editedStripe->m_stripe->UpdateStripeProxy();
}

void CEdStripeEdit::UpdateAllStripes()
{
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		UpdateStripe( *it );
	}
}

void CEdStripeEdit::DestroyVertexEntities()
{
	m_nearbyStripe = nullptr;
	m_nearbyVertex = 0;
	
	// Destroy vertex entities
	for ( auto it=m_allVertexEntities.Begin(); it != m_allVertexEntities.End(); ++it )
	{
		if ( !(*it)->IsDestroyed() )
		{
			(*it)->Destroy();
		}
	}
	m_allVertexEntities.Clear();

	// Clean vertex entity arrays from active edited stripes
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		CEditedStripe* editedStripe = *it;
		editedStripe->m_vertices.Clear();
	}
}

void CEdStripeEdit::RebuildVertexEntities()
{
	// Destroy previous entities (if any)
	DestroyVertexEntities();

	// Build new entities
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		CEditedStripe* editedStripe = *it;
		CStripeComponent* stripe = editedStripe->m_stripe;

		// Create vertex entities for the stripe's control points
		for ( Uint32 i=0; i < stripe->m_points.Size(); ++i )
		{
			EntitySpawnInfo info;
			info.m_entityClass = CVertexEditorEntity::GetStaticClass();
			info.m_spawnPosition = stripe->GetLocalToWorld().TransformPoint( stripe->m_points[i].m_position );

			CVertexEditorEntity* vertexEntity = Cast< CVertexEditorEntity >( m_world->GetDynamicLayer()->CreateEntitySync( info ) );
			ASSERT( vertexEntity, TXT("Failed to create the vertex entity for the stripe's control point") );
			if ( !vertexEntity )
			{
				continue;
			}

			vertexEntity->m_index = i;
			vertexEntity->m_owner = editedStripe;
			vertexEntity->m_rotatable = true;
			vertexEntity->m_drawSmallBox = true;
			vertexEntity->SetRotation( stripe->m_points[i].m_rotation );

			SComponentSpawnInfo spawnInfo;
			spawnInfo.m_name = TXT("Vertex");
			vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );

			editedStripe->m_vertices.PushBack( vertexEntity );
			m_allVertexEntities.PushBack( vertexEntity );
		}
	}
}

void CEdStripeEdit::SelectPointByIndex( CStripeComponent* stripe, Uint32 vertexIndex )
{
	// Find edited stripe
	CEditedStripe* editedStripe = nullptr;
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		if ( (*it)->m_stripe == stripe )
		{
			editedStripe = *it;
			break;
		}
	}
	ASSERT( editedStripe, TXT("Invalid stripe given in SelectPointByIndex") );
	ASSERT( vertexIndex < stripe->m_points.Size(), TXT("Invalid stripe index given in SelectPointByIndex") );

	// Select point vertex entity
	m_viewport->GetSelectionManager()->DeselectAll();
	m_viewport->GetSelectionManager()->Select( editedStripe->m_vertices[vertexIndex] );

	// Inform panel
	m_panel->SetStripe( stripe );
	m_panel->PointSelected( vertexIndex );
}

void CEdStripeEdit::DestroyEditedStripes()
{
	DestroyVertexEntities();
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		delete *it;
	}
	m_stripes.Clear();
}

void CEdStripeEdit::AddEditedStripe( CStripeComponent* stripeComponent )
{
	CEditedStripe* editedStripe = new CEditedStripe();
	m_stripes.PushBack( editedStripe );
	editedStripe->m_stripe = stripeComponent;
	editedStripe->m_editor = this;
	editedStripe->m_lastKnownMatrix = stripeComponent->GetLocalToWorld();
	RebuildVertexEntities();

	// Inform panel
	if ( m_stripes.Size() == 1 )
	{
		m_panel->SetStripe( m_stripes[0]->m_stripe );
	}
}

void CEdStripeEdit::AlignTerrainToStripe( CEditedStripe* editedStripe, Float falloff, Float heightOffset )
{
	CStripeComponent* stripe = editedStripe->m_stripe;
	Matrix localToWorld = stripe->GetLocalToWorld();
	const Float* falloffMap = NULL;
	TDynArray< Float > falloffMapData;
	Int32 fmWidth, fmHeight;
	Float fmRepeat;
	Bool useFalloffMap = false;

	struct Triangle
	{
		Plane p;
		Vector a, b, c;
	};

	// Get stripe geometry
	SRenderProxyStripeProperties fakeProperties;
	TDynArray< Triangle > triangles;
	Float previousWidth = stripe->m_width;
	stripe->GenerateStripeGeometry( &fakeProperties );
	triangles.Reserve( fakeProperties.m_indices.Size()/3*2 );
	for ( Uint32 idx=0; idx < fakeProperties.m_indices.Size(); idx += 3 )
	{
		Triangle t;

		// Triangle vertices
		auto ia = fakeProperties.m_vertices[fakeProperties.m_indices[idx]];
		auto ib = fakeProperties.m_vertices[fakeProperties.m_indices[idx + 1]];
		auto ic = fakeProperties.m_vertices[fakeProperties.m_indices[idx + 2]];
		Vector a( ia.x, ia.y, ia.z ), b( ib.x, ib.y, ib.z ), c( ic.x, ic.y, ic.z );

		// Bring vertices to world space
		t.a = localToWorld.TransformPoint( a );
		t.b = localToWorld.TransformPoint( b );
		t.c = localToWorld.TransformPoint( c );

		// Store plane
		t.p = Plane( t.a, t.b, t.c );

		// Store triangle
		triangles.PushBack( t );
	}

	// Add extra triangles for falloff
	if ( falloff > 0.001f )
	{
		stripe->GenerateStripeGeometry( &fakeProperties, falloff*2.0f );
		for ( Uint32 idx=0; idx < fakeProperties.m_indices.Size(); idx += 3 )
		{
			Triangle t;

			// Triangle vertices
			auto ia = fakeProperties.m_vertices[fakeProperties.m_indices[idx]];
			auto ib = fakeProperties.m_vertices[fakeProperties.m_indices[idx + 1]];
			auto ic = fakeProperties.m_vertices[fakeProperties.m_indices[idx + 2]];
			Vector a( ia.x, ia.y, ia.z ), b( ib.x, ib.y, ib.z ), c( ic.x, ic.y, ic.z );

			// Bring vertices to world space
			t.a = localToWorld.TransformPoint( a );
			t.b = localToWorld.TransformPoint( b );
			t.c = localToWorld.TransformPoint( c );

			// Store plane
			t.p = Plane( t.a, t.b, t.c );

			// Store triangle
			triangles.PushBack( t );
		}

		// Prepare falloff map data (if specified)
		if ( m_panel->m_falloffCheck->GetValue() && m_panel->m_falloffMap.IsOk() && m_panel->m_falloffMap.GetWidth() > 1 )
		{
			fmWidth = m_panel->m_falloffMap.GetWidth();
			fmHeight = m_panel->m_falloffMap.GetHeight();
			fmRepeat = (Float)fmWidth/(Float)fmHeight;
			falloffMapData.Resize( fmWidth*fmHeight );
			for ( int y=0; y < fmHeight; ++y )
			{
				for ( int x=0; x < fmWidth; ++x )
				{
					falloffMapData[ y*fmWidth + x ] = (Float)( m_panel->m_falloffMap.GetRed( x, y ) )/255.0f;
				}
			}
			falloffMap = falloffMapData.TypedData();
			useFalloffMap = true;
		}
	}

	// Grab terrain area the stripe is in
	CClipMap* terrain = m_world->GetTerrain();
	Float lowestElevation = terrain->GetLowestElevation();
	Float elevationRange = terrain->GetHeighestElevation() - lowestElevation;
	Vector terrainCorner = terrain->GetTerrainCorner();
	Float tileSize = terrain->GetTileSize();
	Float cellSize = tileSize/((Float)terrain->GetTilesMaxResolution());
	Rect rect;

	terrain->GetTilesInWorldArea( fakeProperties.m_boundingBox.Min, fakeProperties.m_boundingBox.Max, rect );
	fakeProperties.m_boundingBox.Min.Z = -10000000;
	fakeProperties.m_boundingBox.Max.Z = 10000000;

	// Iterate through all the parts and try to mark them as modified
	for ( Int32 row=rect.m_top; row <= rect.m_bottom; ++row )
	{
		for ( Int32 col=rect.m_left; col <= rect.m_right; ++col )
		{
			CTerrainTile* tile = terrain->GetTile( col, row );
			if ( !tile->MarkModified() )
			{
				return;
			}
		}
	}

	CUndoTerrain::CreateStep( wxTheFrame->GetUndoManager(), TXT("align terrain to stripes"), terrain, true, false, false, false );

	// Main iteration for the alignment
	Vector direction( 0.0f, 0.0f, 1.0f );
	TDynArray< Vector > tmpvtx( 3 );
	Vector* vtx = tmpvtx.TypedData();
	Double startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	Bool progressShown = false;
	Int32 twidth = rect.m_right - rect.m_left + 1;
	Int32 theight = rect.m_bottom - rect.m_top + 1;
	Int32 prgr = 0, prgc = 0; // progress tracking row/col
	for ( Int32 trow=rect.m_top; trow <= rect.m_bottom; ++trow, ++prgr )
	{
		prgc = 0;
		for ( Int32 tcol=rect.m_left; tcol <= rect.m_right; ++tcol, ++prgc )
		{
			CTerrainTile* tile = terrain->GetTile( tcol, trow );
			Uint16* heightmap = tile->GetLevelWriteSyncHM(0);
			Rect affected( INT_MAX, INT_MIN, INT_MAX, INT_MIN );

			// Add the empty stroke just to initialize the data from the tile BEFORE it gets modified
			CUndoTerrain::AddStroke( wxTheFrame->GetUndoManager(), tile, trow, tcol, Rect::EMPTY, Rect::EMPTY );

			for ( Uint32 row = 0; row < terrain->GetTilesMaxResolution(); ++row )
			{
				Uint16* cell = heightmap;
				Double currentTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
				if ( !progressShown && currentTime - startTime > 1.5 )
				{
					progressShown = true;
					GFeedback->BeginTask( TXT("Aligning terrain..."), false );
				}
				if ( progressShown )
				{
					GFeedback->UpdateTaskProgress( row + terrain->GetTilesMaxResolution()*( prgr*twidth + prgc ),
						terrain->GetTilesMaxResolution()*( twidth*theight + 1 ) );
				}
				for ( Uint32 col = 0; col < terrain->GetTilesMaxResolution(); ++col, ++cell )
				{
					Vector cellWorldPos;
					cellWorldPos.X = terrainCorner.X + ((Float)tcol)*tileSize + ((Float)col)*cellSize;
					cellWorldPos.Y = terrainCorner.Y + ((Float)trow)*tileSize + ((Float)row)*cellSize;
					cellWorldPos.Z = lowestElevation + ((Float)(*cell)/65536.f)*elevationRange;
					cellWorldPos.W = 1.0f;

					// Skip cell if it is outside the bounding box
					if ( !fakeProperties.m_boundingBox.Contains( cellWorldPos ) )
					{
						continue;
					}

					// Find the distance from the point to the stripe's center curve
					const Vector flattenedCellPos( cellWorldPos.X, cellWorldPos.Y, 0.0f );
					Bool found = false;
					Float distanceFromStripe = 10000000;
					Float positionOnLine = 0.0f;
					Uint32 pointIndex = 0;
					for ( Uint32 i=0; i + 1 < stripe->m_centerPoints.Size(); ++i )
					{
						const Vector a = stripe->GetLocalToWorld().TransformPoint( stripe->m_centerPoints[i] );
						const Vector b = stripe->GetLocalToWorld().TransformPoint( stripe->m_centerPoints[i + 1] );
						const Vector flattenedPoint1Pos( a.X, a.Y, 0.0f );
						const Vector flattenedPoint2Pos( b.X, b.Y, 0.0f );
						const Float lineLength = flattenedPoint1Pos.DistanceTo( flattenedPoint2Pos );
						const Vector direction = ( flattenedPoint2Pos - flattenedPoint1Pos )/lineLength;
						Float alpha;
						const Float distanceFromLine = MSqrt( PointToLineDistanceSquared3( flattenedPoint1Pos, direction, flattenedCellPos, alpha ) );
						if ( alpha >= 0.0f && alpha < lineLength && distanceFromLine < distanceFromStripe )
						{
							distanceFromStripe = distanceFromLine;
							positionOnLine = alpha/lineLength;
							pointIndex = i;
							found = true;
						}
					}

					// Check the points in addition to lines
					{
						for ( Uint32 i=0; i < stripe->m_centerPoints.Size(); ++i )
						{
							const Vector a = stripe->GetLocalToWorld().TransformPoint( stripe->m_centerPoints[i] );
							const Vector flattenedPointPos( a.X, a.Y, 0.0f );
							const Float distanceFromPoint = MSqrt( flattenedCellPos.DistanceSquaredTo( flattenedPointPos ) );
							if ( distanceFromPoint < distanceFromStripe )
							{
								distanceFromStripe = distanceFromPoint;
								pointIndex = i;
								found = true;
							}
						}
					}

					// Failed to find any distance or the distance is greater than the stripe width and falloff -> ignore cell
					if ( !found || distanceFromStripe > stripe->m_width*0.5f + falloff )
					{
						continue;
					}

					// Check for collision with the stripe geometry
					Bool collision = false;
					Vector collisionPoint;
					Bool half = false;
					for ( auto it=triangles.Begin(); it != triangles.End(); ++it )
					{
						const Triangle& triangle = *it;

						// Do a plane check
						Float hitDistance;
						Vector hitPoint;
						if ( triangle.p.IntersectRay( cellWorldPos, direction, hitPoint, hitDistance ) )
						{
							vtx[0] = triangle.a;
							vtx[1] = triangle.b;
							vtx[2] = triangle.c;
							if ( IsPointInsideConvexShape( hitPoint, tmpvtx ) )
							{
								collisionPoint = hitPoint;
								collision = true;
								break;
							}
						}
					}

					// If we have a collision, move the terrain height to the collision point
					if ( !collision )
					{
						continue;
					}

					// if reached here, the cell will be affected
					affected.m_left   = Min< Int32 >( col, affected.m_left );
					affected.m_top    = Min< Int32 >( row, affected.m_top );
					affected.m_right  = Max< Int32 >( col, affected.m_right );
					affected.m_bottom = Max< Int32 >( row, affected.m_bottom );

					// Point is inside the stripe
					if ( distanceFromStripe <= stripe->m_width*0.5f )
					{
						Float height = collisionPoint.Z + heightOffset;
						*cell = (Uint16)( (((height - 0.02f) - lowestElevation)/elevationRange)*65536.f );
					}
					else if ( falloff > 0.001f && distanceFromStripe < stripe->m_width*0.5f + falloff ) // Point is in fallof range
					{
						Float t = Clamp( ( distanceFromStripe - stripe->m_width*0.5f )/falloff, 0.0f, 1.0f );
						Float height;
						
						if ( useFalloffMap )
						{
							Float u = ( pointIndex % (Int32)MFloor( fmRepeat ) ) * ( 1.0f / fmRepeat ) + ( positionOnLine / fmRepeat );
							Float v = 1.0f - t;

							height = Lerp( falloffMap[ (Int32)(v*( (Float)( fmHeight - 1 ) ) )*fmWidth + (Int32)( u*( (Float)( fmWidth - 1 ) ) ) ], cellWorldPos.Z, collisionPoint.Z ) + heightOffset;
						}
						else
						{
							height = Lerp( Clamp( t*t*t*(t*(t*6.f - 15.f) + 10.f), 0.f, 1.f ), collisionPoint.Z, cellWorldPos.Z ) + heightOffset;
						}
						*cell = (Uint16)( (((height - 0.02f) - lowestElevation)/elevationRange)*65536.f );
					}
				}

				heightmap += terrain->GetTilesMaxResolution();
			}

			if ( !affected.IsEmpty() )
			{
				++affected.m_right;
				++affected.m_bottom;
				CUndoTerrain::AddStroke( wxTheFrame->GetUndoManager(), tile, trow, tcol, affected, Rect::EMPTY );
			}

			// Update tile
			tile->SetDirty( true );
			tile->LoadAllMipmapsSync();
			tile->RebuildMipmaps();
			tile->InvalidateCollision();
		}
	}

	CUndoTerrain::FinishStep( wxTheFrame->GetUndoManager() );

	// Hide the feedback dialog if it was visible
	if ( progressShown )
	{
		GFeedback->EndTask();
	}
}

void CEdStripeEdit::AlignTerrainToStripes( Float falloff, Float heightOffset )
{
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		AlignTerrainToStripe( *it, falloff, heightOffset );
	}
}

void CEdStripeEdit::AlignStripeToTerrain( CEditedStripe* editedStripe, Float blend )
{
	CStripeComponent* stripe = editedStripe->m_stripe;
	Matrix worldToLocal;
	stripe->GetWorldToLocal( worldToLocal );

	CUndoStripePoint::CreateStep( *wxTheFrame->GetUndoManager(), this, stripe, TXT("align stripe to terrain") );

	// Adjust points
	for ( auto it=stripe->m_points.Begin(); it != stripe->m_points.End(); ++it )
	{
		SStripeControlPoint& point = *it;

		// Calculate new position
		Float height;
		Vector worldPosition = stripe->GetLocalToWorld().TransformPoint( point.m_position );
		m_world->GetTerrain()->GetHeightForWorldPositionSync( worldPosition, 0, height );
		worldPosition.Z = height;
		point.m_position = worldToLocal.TransformPoint( worldPosition );

		// Calculate new rotation
		Vector originalZAxis = point.m_rotation.ToMatrix().TransformPoint( Vector( 0, 0, 1 ) );
		Vector zAxis = ( originalZAxis*( 1.0f - blend ) + m_world->GetTerrain()->GetNormalForWorldPosition( worldPosition )*blend ).Normalized3();
		Vector xAxis = Vector::Cross( zAxis, Vector( 0, 1, 0 ) ).Normalized3();
		Vector yAxis = Vector::Cross( zAxis, xAxis ).Normalized3();
		xAxis.W = yAxis.W = zAxis.W = 0;
		Matrix theMatrix( xAxis, yAxis, zAxis, Vector( 0, 0, 0, 1 ) );
		point.m_rotation = theMatrix.ToEulerAnglesFull();
	}

	// Rebuild vertices
	RebuildVertexEntities();
	stripe->UpdateStripeProxy();
}

void CEdStripeEdit::AlignStripesToTerrain( Float blend )
{
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		AlignStripeToTerrain( *it, blend );
	}
}

void CEdStripeEdit::HandleInvalidObject( CObject* invalidObject )
{
	// Pass to the panel
	m_panel->HandleInvalidObject( invalidObject );

	// Check if the nearby stripe was deleted
	if ( m_nearbyStripe && m_nearbyStripe->m_stripe == invalidObject )
	{
		m_nearbyStripe = NULL;
	}

	// Check if any of the stripes were deleted
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		CEditedStripe* editedStripe = *it;
		if ( editedStripe->m_stripe == invalidObject )
		{
			m_stripes.Remove( editedStripe );
			delete editedStripe;
			RebuildVertexEntities();
			return;
		}
	}
}

void CEdStripeEdit::OnCreateStripe( wxCommandEvent& event )
{
	// Make sure we have an active layer
	if ( !wxTheFrame->GetSceneExplorer()->GetActiveLayer() )
	{
		wxMessageBox( wxT("Please activate a layer first"), wxT("No active layer"), wxICON_WARNING|wxOK );
		return;
	}

	// Create entity
	EntitySpawnInfo sinfo;
	sinfo.m_spawnPosition = m_contextMenuVector;
	CEntity* stripeEntity = wxTheFrame->GetSceneExplorer()->GetActiveLayer()->CreateEntitySync( sinfo );
	ASSERT( stripeEntity, TXT("Failed to create the stripe entity, let's crash!") );
	stripeEntity->SetStreamed( false );

	// Create component
	SComponentSpawnInfo csinfo;
	CStripeComponent* stripeComponent = Cast< CStripeComponent >( stripeEntity->CreateComponent( CStripeComponent::GetStaticClass(), csinfo ) );
	ASSERT( stripeComponent, TXT("Failed to create the stripe component inside the stripe entity, crash imminent!") );
	stripeEntity->ForceUpdateTransformNodeAndCommitChanges();
	stripeComponent->ForceUpdateTransformNodeAndCommitChanges();

	// Add a couple of points
	stripeComponent->m_points.Grow( 2 );
	stripeComponent->m_points.Back().m_position.X += 2.0f;

	// Set diffuse texture, if selected
	String resourcePath;
	if ( GetActiveResource( resourcePath, CBitmapTexture::GetStaticClass() ) )
	{
		CBitmapTexture* diffuseTexture = Cast< CBitmapTexture >( GDepot->LoadResource( resourcePath ) );
		if ( diffuseTexture )
		{
			stripeComponent->m_diffuseTexture = diffuseTexture;
		}

		// Set normal texture, if exists
		CFilePath normalPath( resourcePath );
		normalPath.SetFileName( normalPath.GetFileName() + TXT("_n") );

		CBitmapTexture* normalTexture = Cast< CBitmapTexture >( GDepot->LoadResource( normalPath.ToString() ) );
		if ( normalTexture )
		{
			stripeComponent->m_normalTexture = normalTexture;
		}
	}

	// Update stripe proxy
	stripeComponent->UpdateStripeProxy();

	// Add undo step
	CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), stripeEntity, true );
	CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
}

void CEdStripeEdit::OnAlignTerrainToStripes( wxCommandEvent& event )
{
	Int32 cm = (Int32)m_panel->m_falloffRangeSpin->GetValue();
	if ( FormattedDialogBox( wxT("Align terrain to stripe"), wxT("'Falloff in centimeters:'I|H{~B@'OK'|B'Cancel'}"), &cm ) == 0 )
	{
		AlignTerrainToStripes( ((Float)cm)*0.01f, 0.0f );
	}
}

void CEdStripeEdit::OnAlignStripesToTerrain( wxCommandEvent& event )
{
	AlignStripesToTerrain( 1.0f );
}

void CEdStripeEdit::OnSplitStripe( wxCommandEvent& event )
{
	if ( m_nearbyStripe == nullptr ) return;

	// Check vertex
	if ( m_nearbyVertex == 0 || m_nearbyVertex + 1 == m_nearbyStripe->m_vertices.Size() )
	{
		wxMessageBox( wxT("Cannot split from the first or last vertex"), wxT("Error"), wxOK|wxCENTRE|wxICON_ERROR, wxTheFrame );
		return;
	}
	
	// Create entity
	EntitySpawnInfo sinfo;
	sinfo.m_spawnPosition = m_nearbyStripe->m_stripe->GetEntity()->GetWorldPosition();
	CEntity* stripeEntity = m_nearbyStripe->m_stripe->GetLayer()->CreateEntitySync( sinfo );
	ASSERT( stripeEntity, TXT("Failed to create the stripe entity, let's crash!") );
	stripeEntity->SetStreamed( false );

	// Create component
	SComponentSpawnInfo csinfo;
	CStripeComponent* stripeComponent = Cast< CStripeComponent >( stripeEntity->CreateComponent( CStripeComponent::GetStaticClass(), csinfo ) );
	ASSERT( stripeComponent, TXT("Failed to create the stripe component inside the stripe entity, crash imminent!") );
	stripeEntity->ForceUpdateTransformNodeAndCommitChanges();
	stripeComponent->ForceUpdateTransformNodeAndCommitChanges();

	// Copy all properties
	TDynArray< CProperty* > properties;
	m_nearbyStripe->m_stripe->GetClass()->GetProperties( properties );
	for ( auto it=properties.Begin(); it != properties.End(); ++it )
	{
		CProperty* prop = *it;
		// Skip uneditable properties... and name
		if ( prop->GetName() == CNAME( name ) || !prop->IsEditable() || !prop->IsSerializable() )
		{
			continue;
		}

		// Copy the value
		prop->GetType()->Copy( prop->GetOffsetPtr( stripeComponent ), prop->GetOffsetPtr( m_nearbyStripe->m_stripe ) );
	}

	// Move the new stripe at the first point's origin
	stripeEntity->SetPosition( m_nearbyStripe->m_stripe->GetLocalToWorld().TransformPoint( m_nearbyStripe->m_stripe->m_points[m_nearbyVertex].m_position ) );
	stripeEntity->ForceUpdateTransformNodeAndCommitChanges();
	
	// Copy the points in the new stripe
	Matrix worldToLocal;
	stripeComponent->GetWorldToLocal( worldToLocal );
	stripeComponent->m_points.Clear();
	for ( Uint32 i=m_nearbyVertex; i < m_nearbyStripe->m_stripe->m_points.Size(); ++i )
	{
		stripeComponent->m_points.PushBack( m_nearbyStripe->m_stripe->m_points[i] );

		Vector pointInWorldSpace = m_nearbyStripe->m_stripe->GetLocalToWorld().TransformPoint( stripeComponent->m_points.Back().m_position );
		stripeComponent->m_points.Back().m_position = worldToLocal.TransformPoint( pointInWorldSpace );
	}

	// Remove the extra points
	m_nearbyStripe->m_stripe->m_points.Resize( m_nearbyVertex + 1 );

	// Update stripe proxies
	RebuildVertexEntities();
	UpdateAllStripes();
	stripeComponent->UpdateStripeProxy();
}

void CEdStripeEdit::OnCloseStripeEditor( wxCommandEvent& event )
{
	wxTheFrame->GetToolsPanel()->CancelTool();
}

String CEdStripeEdit::GetCaption() const
{
	return TXT("Stripe edit");
}

Bool CEdStripeEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Inform stripe components we're in stripe editing mode
	GStripeComponentEditorFlags |= SCEF_STRIPE_TOOL_ACTIVE;
	if ( ViewAllRoadLines ) GStripeComponentEditorFlags |= SCEF_SHOW_CENTER_LINES;
	if ( ViewAIRoadLines ) GStripeComponentEditorFlags |= SCEF_SHOW_AIROAD_LINES;

	// Save world and viewport
	m_world = world;
	m_viewport = viewport;

	// Create panel
	m_panel = new CEdStripeToolPanel( this, panel );
	m_panelSizer->Add( m_panel, 1, wxALL|wxEXPAND, 0 );
	m_panel->SetStripe( nullptr, true );

	// Set selection granularity to entities
	viewport->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Extract stripes from selection
	m_stripes.Clear();
	for ( Uint32 i = 0; i < selection.Size(); i++ )
	{
		CStripeComponent* stripe = Cast< CStripeComponent >( selection[i] );
		if ( stripe )
		{
			AddEditedStripe( stripe );
		}
	}

	// Do what the function's name says
	RebuildVertexEntities();

	// We need to know about object destruction events
	SEvents::GetInstance().RegisterListener( CNAME( OnObjectDiscarded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( StripeDeleted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameStarting ), this );

	return true;
}

void CEdStripeEdit::End()
{
	// Remove from events
	SEvents::GetInstance().UnregisterListener( CNAME( GameStarting ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( StripeDeleted ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( OnObjectDiscarded ), this );

	// Destroy edited stripes info
	DestroyEditedStripes();

	// Destroy panel
	m_panel->Destroy();
	m_panel = nullptr;

	// Inform stripe components we're not anymore in stripe editing mode
	GStripeComponentEditorFlags &= ~( SCEF_STRIPE_TOOL_ACTIVE | SCEF_SHOW_AIROAD_LINES | SCEF_SHOW_CENTER_LINES | SCEF_VISUALIZE_BLEND | SCEF_HIDE_CONTROLS );

	// Remove undo steps referencing the editor
	wxTheFrame->GetUndoManager()->NotifyObjectRemoved( this );
}

Bool CEdStripeEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager* selectionMgr = m_viewport->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction( *selectionMgr );

	// Scan the selection for vertices and stripes
	CStripeComponent* selectedStripe = nullptr;
	TDynArray< CVertexComponent* > selectedVertices;
	for ( Uint32 i=0; i < objects.Size(); ++i )
	{
		CComponent* component = Cast< CComponent >( objects[i]->GetHitObject() );
		if ( component->IsA< CStripeComponent >() )
		{
			selectedStripe = static_cast< CStripeComponent* >( component );
			break;
		}
		else if ( component->IsA< CVertexComponent >() )
		{
			selectedVertices.PushBack( static_cast< CVertexComponent* >( component ) );
		}
	}
	
	// If we have a new stripe, disregard everything and switch to it
	// TODO: add multistripe selection
	if ( selectedStripe )
	{
		m_panel->PointSelected( -1 );
		DestroyEditedStripes();
		AddEditedStripe( selectedStripe );
		selectionMgr->DeselectAll();
		selectionMgr->Select( selectedStripe );
		return true;
	}

	// Remove current selection (TODO: allow ctrl+shift for multiple selection)
	selectionMgr->DeselectAll();

	// Select vertices
	for ( auto it=selectedVertices.Begin(); it != selectedVertices.End(); ++it )
	{
		selectionMgr->Select( (*it)->GetEntity() );
	}

	// Inform the panel
	if ( selectedVertices.Size() == 1 )
	{
		m_panel->PointSelected( (Int32)selectedVertices[0]->GetObjectIndex() );
	}
	else
	{
		m_panel->PointSelected( -1 );
	}

	return true;
}

Bool CEdStripeEdit::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	wxMenu menu;

	m_contextMenuVector = collision;

	menu.Append( ID_CREATE_STRIPE, TXT("Create stripe") );
	menu.Connect( ID_CREATE_STRIPE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStripeEdit::OnCreateStripe ), nullptr, this );
	menu.AppendSeparator();
	menu.Append( ID_ALIGN_TERRAIN_TO_STRIPES, TXT("Align terrain stripes") );
	menu.Connect( ID_ALIGN_TERRAIN_TO_STRIPES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStripeEdit::OnAlignTerrainToStripes ), nullptr, this );
	menu.Append( ID_ALIGN_STRIPES_TO_TERRAIN, TXT("Align stripes to terrain") );
	menu.Connect( ID_ALIGN_STRIPES_TO_TERRAIN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStripeEdit::OnAlignStripesToTerrain ), nullptr, this );
	menu.AppendSeparator();
	menu.Append( ID_SPLIT_STRIPE, TXT("Split stripe at nearby vertex") );
	menu.Connect( ID_SPLIT_STRIPE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStripeEdit::OnSplitStripe ), nullptr, this );
	menu.AppendSeparator();
	menu.Append( ID_CLOSE_STRIPE_EDITOR, TXT("Close stripe editor tool") );
	menu.Connect( ID_CLOSE_STRIPE_EDITOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStripeEdit::OnCloseStripeEditor ), nullptr, this );

	wxTheFrame->PopupMenu( &menu );

	return true;
}

Bool CEdStripeEdit::HandleActionClick( Int32 x, Int32 y )
{
	// No stripe nearby
	if ( !m_nearbyStripe )
	{
		return false;
	}

	// Ctrl+click or Ctrl+Alt+click to add new point
	// (Ctrl+click => add at the end, Ctrl+Alt+click => add at the front)
	if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		// Calculate position
		Vector worldPosition;
		if ( !m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), x, y, worldPosition ) )
		{
			return true;
		}

		// Make sure the point is inside the terrain
		Box terrainBox;
		terrainBox.Min = m_world->GetTerrain()->GetTerrainCorner();
		terrainBox.Max = terrainBox.Min + Vector( m_world->GetTerrain()->GetTerrainSize(), m_world->GetTerrain()->GetTerrainSize(), 0.0f );
		terrainBox.Min.Z = -10000000;
		terrainBox.Max.Z = 10000000;
		if ( !terrainBox.Contains( worldPosition ) )
		{
			return true;
		}

		// Create point
		Matrix worldToLocal;
		m_nearbyStripe->m_stripe->GetWorldToLocal( worldToLocal );
		SStripeControlPoint* newPoint = nullptr;
		Int32 newPointIndex;
		if ( RIM_IS_KEY_DOWN( IK_Alt ) )
		{
			CUndoStripePoint::CreateStep( *wxTheFrame->GetUndoManager(), this, m_nearbyStripe->m_stripe );
			m_nearbyStripe->m_stripe->m_points.Insert( 0, SStripeControlPoint() );
			newPoint = &m_nearbyStripe->m_stripe->m_points[0];
			newPointIndex = 0;
		}
		else
		{
			CUndoStripePoint::CreateStep( *wxTheFrame->GetUndoManager(), this, m_nearbyStripe->m_stripe );
			m_nearbyStripe->m_stripe->m_points.Grow();
			newPoint = &m_nearbyStripe->m_stripe->m_points.Back();		
			newPointIndex = m_nearbyStripe->m_stripe->m_points.SizeInt() - 1;
		}
		CEdStripeEdit::CEditedStripe* nearbyStripe = m_nearbyStripe;
		newPoint->m_position = worldToLocal.TransformPoint( worldPosition );
		UpdateStripe( m_nearbyStripe );
		RebuildVertexEntities();
		m_panel->PointSelected( -1 );
		m_nearbyStripe = nearbyStripe;
		m_nearbyVertex = newPointIndex;
		{
			CSelectionManager::CSelectionTransaction transaction( *m_viewport->GetSelectionManager() );
			m_viewport->GetSelectionManager()->DeselectAll();
			m_viewport->GetSelectionManager()->Select( m_nearbyStripe->m_vertices[m_nearbyVertex] );
		}
		m_panel->PointSelected( m_nearbyVertex );
		return true;
	}
	
	// Alt+click to split the segment that begins from the current point
	if ( !RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		// Ignore last vertex
		if ( m_nearbyVertex + 1 == m_nearbyStripe->m_vertices.Size() )
		{
			return false;
		}

		// Segment vertices
		Vector startVertex = m_nearbyStripe->m_stripe->m_points[m_nearbyVertex].m_position;
		Vector endVertex = m_nearbyStripe->m_stripe->m_points[m_nearbyVertex + 1].m_position;
		Vector middleVertex = ( startVertex + endVertex )*0.5f;

		// Add new point
		CUndoStripePoint::CreateStep( *wxTheFrame->GetUndoManager(), this, m_nearbyStripe->m_stripe );
		SStripeControlPoint newPoint( middleVertex );
		m_nearbyStripe->m_stripe->m_points.Insert( m_nearbyVertex + 1, newPoint );
		UpdateStripe( m_nearbyStripe );
		RebuildVertexEntities();
		m_panel->PointSelected( -1 );
		return true;
	}

	// Plain old click, check if we're near a point
	if ( !m_nearbyStripe->m_vertices[m_nearbyVertex]->IsSelected() )
	{
		Float activePointRadius = 16.0f; // this is 4 squared
		if ( m_nearbyVertex + 1 < m_nearbyStripe->m_vertices.Size() )
		{
			activePointRadius = m_nearbyStripe->m_vertices[m_nearbyVertex]->GetWorldPositionRef().DistanceSquaredTo( m_nearbyStripe->m_vertices[m_nearbyVertex + 1]->GetWorldPositionRef() )/3.0f;
		}
		if ( activePointRadius < m_nearbyVertexDistance )
		{
			CSelectionManager::CSelectionTransaction transaction( *m_viewport->GetSelectionManager() );
			m_viewport->GetSelectionManager()->DeselectAll();
			m_viewport->GetSelectionManager()->Select( m_nearbyStripe->m_vertices[m_nearbyVertex] );
			m_panel->PointSelected( m_nearbyVertex );
			return true;
		}
	}

	return false;
}

Bool CEdStripeEdit::OnDelete()
{
	// Collect vertex entities to delete
	TDynArray< CVertexEditorEntity* > vertexEntities;
	TDynArray< CEntity* > entities;
	m_viewport->GetSelectionManager()->GetSelectedEntities( entities );
	m_viewport->GetSelectionManager()->DeselectAll();
	for ( auto it=entities.Begin(); it != entities.End(); ++it )
	{
		CVertexEditorEntity* vertexEntity = Cast< CVertexEditorEntity >( *it );
		if ( vertexEntity )
		{
			vertexEntities.PushBack( vertexEntity );
		}
	}

	// With two points remaining in the stripe, deleting one should delete the stripe - keep a list of them!
	TDynArray< CEditedStripe* > stripesToDelete;

	// Delete the points these entities represent
	while ( !vertexEntities.Empty() )
	{
		CVertexEditorEntity* entity = vertexEntities.PopBack();
		CEditedStripe* editedStripe = dynamic_cast< CEditedStripe* >( entity->m_owner );
		if ( editedStripe )
		{
			ptrdiff_t index = editedStripe->m_vertices.GetIndex( entity );
			if ( index >= 0 )
			{
				if ( editedStripe->m_vertices.Size() <= 2 ) // paranoid
				{
					stripesToDelete.PushBackUnique( editedStripe );
				}
				else
				{
					CUndoStripePoint::CreateStep( *wxTheFrame->GetUndoManager(), this, editedStripe->m_stripe, TXT("deleting stripe point") );
					editedStripe->m_stripe->m_points.RemoveAt( (size_t)index );
				}
			}
		}
	}

	// Delete pointless stripes
	if ( !stripesToDelete.Empty() )
	{
		for ( auto it=stripesToDelete.Begin(); it != stripesToDelete.End(); ++it )
		{
			CEditedStripe* editedStripe = *it;
			// Note: the CreateStep method will cause the CStripeComponent to raise a StripeDeleted event which will
			//       be handled by DispatchEditorEvent, remove the edited stripe from m_stripes and recreate the
			//		 temporary vertex entities
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), editedStripe->m_stripe, false, true ); 
		}
		CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
	}

	// Rebuild vertex entities and update stripes
	RebuildVertexEntities();
	UpdateAllStripes();

	return true;
}

Bool CEdStripeEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		return HandleActionClick( x, y );
	}
	return false;
}

Bool CEdStripeEdit::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Do not render anything special when the view controls is unchecked
	if ( !m_panel->m_viewControlsCheck->GetValue() )
	{
		GStripeComponentEditorFlags |= SCEF_HIDE_CONTROLS;
		return false;
	}
	else
	{
		GStripeComponentEditorFlags &= ~SCEF_HIDE_CONTROLS;
	}

	// Render stripes
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		CEditedStripe*		editedStripe = *it;
		CStripeComponent*	stripe = editedStripe->m_stripe;
		Matrix				localToWorld = stripe->GetLocalToWorld();

		// Skip empty stripes
		if ( stripe->m_points.Size() < 2 )
		{
			continue;
		}

		// Check if the stripe has moved/rotated/etc since the last time and update the vertices if so
		if ( stripe->GetLocalToWorld() != editedStripe->m_lastKnownMatrix )
		{
			editedStripe->m_lastKnownMatrix = stripe->GetLocalToWorld();
			for ( Uint32 i=0; i < editedStripe->m_vertices.Size(); ++i )
			{
				Vector position = editedStripe->m_lastKnownMatrix.TransformPoint( stripe->m_points[i].m_position );
				editedStripe->m_vertices[i]->SetPosition( position );
			}
		}

		// If the align terrain panel is open, render the stripe geometry and falloff range overlay
		if ( m_panel->m_alignTerrainToStripeControls->IsShown() )
		{
			Float falloff = m_panel->m_falloffCheck->GetValue() ? ((Float)m_panel->m_falloffRangeSpin->GetValue())*0.01f : 0.0f;
			Float heightOffset = m_panel->m_heightOffsetCheck->GetValue() ? ((Float)m_panel->m_heightOffsetSpin->GetValue())*0.01f : 0.0f;

			// Generate geometry
			SRenderProxyStripeProperties fakeProperties;
			stripe->GenerateStripeGeometry( &fakeProperties );

			// Convert stripe vertices to debug vertices
			TDynArray< DebugVertex > vertices;
			vertices.Grow( fakeProperties.m_vertices.Size() );
			for ( Uint32 i=0; i < fakeProperties.m_vertices.Size(); ++i )
			{
				Vector worldPosition = stripe->GetLocalToWorld().TransformPoint( Vector( fakeProperties.m_vertices[i].x, fakeProperties.m_vertices[i].y, fakeProperties.m_vertices[i].z + 0.07f ) );
				worldPosition.Z += heightOffset;
				vertices[i] = DebugVertex( worldPosition, Color( 0, 200, 0, 64 ) );
			}

			// Render vertices
			frame->AddDebugTriangles( Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), fakeProperties.m_indices.TypedData(), fakeProperties.m_indices.Size(), Color::BLACK );

			// Render falloff range
			if ( falloff > 0.001f )
			{
				// Generate geometry
				SRenderProxyStripeProperties fakeProperties;
				stripe->GenerateStripeGeometry( &fakeProperties, falloff*2.0f );

				// Convert stripe vertices to debug vertices
				TDynArray< DebugVertex > vertices;
				vertices.Grow( fakeProperties.m_vertices.Size() );
				for ( Uint32 i=0; i < fakeProperties.m_vertices.Size(); ++i )
				{
					Vector worldPosition = stripe->GetLocalToWorld().TransformPoint( Vector( fakeProperties.m_vertices[i].x, fakeProperties.m_vertices[i].y, fakeProperties.m_vertices[i].z + 0.07f ) );
					worldPosition.Z += heightOffset;
					vertices[i] = DebugVertex( worldPosition, Color( 200, 32, 0, 64 ) );
				}

				// Render vertices
				frame->AddDebugTriangles( Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), fakeProperties.m_indices.TypedData(), fakeProperties.m_indices.Size(), Color::BLACK );
			}
		}

		// Render stripe line at the center
		for ( Uint32 i=0; i < stripe->m_centerPoints.Size() - 1; ++i )
		{
			const Vector& startPoint = localToWorld.TransformPoint( stripe->m_centerPoints[i] );
			const Vector& endPoint = localToWorld.TransformPoint( stripe->m_centerPoints[i + 1] );
			frame->AddDebugLine( startPoint, endPoint, Color::YELLOW, true );
			
			// Render terrain above/below line if the stripe is perpendicular to the ground
			if ( stripe->m_localToWorld.GetAxisZ().Dot3( Vector( 0.0f, 0.0f, 1.0f ) ) > 0.9f )
			{
				Float height;
				m_world->GetTerrain()->GetHeightForWorldPosition( startPoint, height );
				// Start point line
				if ( startPoint.Z > height + 0.1f )
				{
					frame->AddDebugLine( startPoint, Vector( startPoint.X, startPoint.Y, height ), Color::GREEN, true );
				}
				else if ( startPoint.Z < height - 0.1f )
				{
					frame->AddDebugLine( startPoint, Vector( startPoint.X, startPoint.Y, height ), Color::RED, true );
				}
				// End point line
				if ( i + 2 == stripe->m_centerPoints.Size() )
				{
					m_world->GetTerrain()->GetHeightForWorldPosition( endPoint, height );
					if ( fabs( endPoint.Z - height ) > 0.1f )
					{
						if ( endPoint.Z > height )
						{
							frame->AddDebugLine( startPoint, Vector( endPoint.X, endPoint.Y, height ), Color::GREEN, true );
						}
						else if ( endPoint.Z < height )
						{
							frame->AddDebugLine( startPoint, Vector( endPoint.X, endPoint.Y, height ), Color::RED, true );
						}
					}
				}
			}
		}

		// Render control points
		for ( auto it=stripe->m_points.Begin(); it != stripe->m_points.End(); ++it )
		{
			const SStripeControlPoint& point = *it;
			frame->AddDebugSphere( localToWorld.TransformPoint( point.m_position ), 0.125f*point.m_scale, Matrix::IDENTITY, point.m_color );
		}

		// Render stripe control point line under cursor
		if ( m_nearbyStripe && m_nearbyStripe->m_stripe == stripe && m_nearbyVertex + 1 < stripe->m_points.Size() )
		{
			const Vector& startPoint = localToWorld.TransformPoint( stripe->m_points[m_nearbyVertex].m_position );
			const Vector& endPoint = localToWorld.TransformPoint( stripe->m_points[m_nearbyVertex + 1].m_position );
			frame->AddDebugLine( startPoint, endPoint, Color::RED, true );
		}

		// Render new control point location when alt or ctrl is pressed
		if ( m_nearbyStripe && m_nearbyStripe->m_vertices.Size() > 1 && ( RIM_IS_KEY_DOWN( IK_Ctrl ) || RIM_IS_KEY_DOWN( IK_Alt ) ) )
		{
			Vector start;
			Vector point;
			Bool render = true;
			POINT cursorPos;

			// Get cursor coordinates
			::GetCursorPos( &cursorPos );
			::ScreenToClient( (HWND)m_viewport->GetHandle(), &cursorPos );
			
			// Calculate starting position of the line and new control point position
			if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Alt ) ) // new point at the end
			{
				start = m_nearbyStripe->m_vertices.Back()->GetWorldPositionRef();
				if ( !m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), cursorPos.x, cursorPos.y, point ) )
				{
					render = false;
				}
			}
			else if ( !RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) ) // new point at the middle
			{
				if ( m_nearbyVertex + 1 == m_nearbyStripe->m_vertices.Size() )
				{
					render = false;
				}
				else
				{
					start = m_nearbyStripe->m_vertices[m_nearbyVertex]->GetWorldPositionRef();
					point = ( start + m_nearbyStripe->m_vertices[m_nearbyVertex + 1]->GetWorldPositionRef() )*0.5f;
				}
			}
			else if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) ) // new point at the beginning
			{
				start = m_nearbyStripe->m_vertices[0]->GetWorldPositionRef();
				if ( !m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), cursorPos.x, cursorPos.y, point ) )
				{
					render = false;
				}
			}

			// Render line and sphere
			if ( render )
			{
				frame->AddDebugLine( start, point, Color::RED, true );
				frame->AddDebugSphere( point, 0.125f, Matrix::IDENTITY, Color::LIGHT_RED );
			}
		}
	}

	return false;
}

Bool CEdStripeEdit::OnViewportTrack( const CMousePacket& packet )
{
	// TODO: project to screenspace instead

	// Create ray
	Vector origin, direction;
	packet.m_viewport->CalcRay( packet.m_x, packet.m_y, origin, direction );
	
	// Find closest intersection with the vertex entities
	// (if this looks slow, it is because it is slow, but then i don't expect more than ~30-40 active control points)
	CEditedStripe* closestEditedStripe = nullptr;
	Uint32 closestEditedVertex = 0;
	Float closestDistance = 0.0f;
	for ( auto it=m_stripes.Begin(); it != m_stripes.End(); ++it )
	{
		CEditedStripe* editedStripe = *it;
		for ( Uint32 i=0; i < editedStripe->m_vertices.Size(); ++i )
		{
			Vector intersection, unused;
			Sphere vertexSphere( editedStripe->m_vertices[i]->GetWorldPositionRef(), 1.0f );
			if ( vertexSphere.IntersectRay( origin, direction, intersection, unused ) != 0 )
			{
				Float distanceFromIntersection = intersection.DistanceSquaredTo( origin );
				if ( closestEditedStripe == nullptr || distanceFromIntersection < closestDistance )
				{
					closestEditedStripe = editedStripe;
					closestEditedVertex = i;
					closestDistance = distanceFromIntersection;
				}
			}
		}
	}

	// No stripe vertex found
	if ( !closestEditedStripe )
	{
		return false;
	}

	// Set members
	m_nearbyStripe = closestEditedStripe;
	m_nearbyVertex = closestEditedVertex;
	m_nearbyVertexDistance = closestDistance;

	return true;
}

Bool CEdStripeEdit::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	// Alt+C -> toggle controls
	if ( key == IK_C && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
	{
		m_panel->m_viewControlsCheck->SetValue( !m_panel->m_viewControlsCheck->GetValue() );
	}
	return false;
};

void CEdStripeEdit::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( OnObjectDiscarded ) )
	{
		CObject *discardedObject = GetEventData< CObject* >( data );
		HandleInvalidObject( discardedObject );
	}
	else if ( name == CNAME( StripeDeleted ) )
	{
		CStripeComponent* deletedStripe = GetEventData< CStripeComponent* >( data );
		HandleInvalidObject( deletedStripe );
	}
	else if ( name == CNAME( GameStarting ) )
	{
		DestroyEditedStripes();
	}
}

void CEdStripeEdit::Reset()
{
	m_stripes.Clear();
	m_world = nullptr;
	m_viewport = nullptr;
}

//////////////////////////////////////////////////////////////////////////

CEdStripeToolPanel::CEdStripeToolPanel( CEdStripeEdit* tool, wxWindow* parent )
{
	m_tool = tool;

	// Load panel from resource
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("StripesPanel") );

	// Get controls
	m_viewAllCheck					= XRCCTRL( *this, "ViewAllCheck", wxCheckBox );
	m_viewAICheck					= XRCCTRL( *this, "ViewAICheck", wxCheckBox );
	m_viewBlendMaskCheck			= XRCCTRL( *this, "BlendMaskCheck", wxCheckBox );
	m_viewControlsCheck				= XRCCTRL( *this, "ViewControlsCheck", wxCheckBox );
	m_regularStripe					= XRCCTRL( *this, "RegularStripeToggle", wxToggleButton );
	m_terrainStripe					= XRCCTRL( *this, "TerrainStripeToggle", wxToggleButton );
	m_aiRoad						= XRCCTRL( *this, "AIRoadToggle", wxToggleButton );
	m_alignTerrainToStripe		    = XRCCTRL( *this, "AlignTerrainToStripeToggle", wxToggleButton );
	m_alignStripeToTerrain		    = XRCCTRL( *this, "AlignStripeToTerrainToggle", wxToggleButton );
	m_alignTerrainToStripeControls  = XRCCTRL( *this, "AlignTerrainToStripeControlsPanel", wxPanel );
	m_alignStripeToTerrainControls  = XRCCTRL( *this, "AlignStripeToTerrainControlsPanel", wxPanel );
	m_rotationBlendSlider			= XRCCTRL( *this, "RotationBlendSlider", wxSlider );
	wxButton* applyTerrainToStripe  = XRCCTRL( *this, "ApplyAlignToStripeButton", wxButton );
	wxButton* applyStripeToTerrain  = XRCCTRL( *this, "ApplyStripeToTerrainButton", wxButton );
	m_controlPointPanel				= XRCCTRL( *this, "ControlPointPanel", wxPanel );
	m_falloffCheck					= XRCCTRL( *this, "FalloffCheck", wxCheckBox );
	m_falloffRangeSpin				= XRCCTRL( *this, "FalloffRangeSpin", wxSpinCtrl );
	m_heightOffsetCheck				= XRCCTRL( *this, "HeightOffsetCheck", wxCheckBox );
	m_heightOffsetSpin				= XRCCTRL( *this, "HeightOffsetSpin", wxSpinCtrl );
	m_falloffMapCheck				= XRCCTRL( *this, "FalloffMapCheck", wxCheckBox );
	m_falloffMapBitmap				= XRCCTRL( *this, "FalloffMapBitmap", wxStaticBitmap );
	wxButton* browseFalloffMap		= XRCCTRL( *this, "BrowseFalloffMapButton", wxButton );
	m_stripePropertiesPanel			= XRCCTRL( *this, "StripePropertiesPanel", wxPanel );
	m_pointSlider				    = XRCCTRL( *this, "PointSlider", wxSlider );
	m_pointLabel				    = XRCCTRL( *this, "PointLabel", wxStaticText );
	m_propertiesPanel			    = XRCCTRL( *this, "PropertiesPanel", wxPanel );

	// Create properties pages
	m_stripePropertiesPage = new CEdPropertiesPage( m_stripePropertiesPanel, PropertiesPageSettings(), wxTheFrame->GetUndoManager() );
	m_stripePropertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_stripePropertiesPanel->GetSizer()->Add( m_stripePropertiesPage, 1, wxEXPAND|wxALL, 0 );
	m_propertiesPage = new CEdPropertiesPage( m_propertiesPanel, PropertiesPageSettings(), wxTheFrame->GetUndoManager() );
	m_propertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_propertiesPanel->GetSizer()->Add( m_propertiesPage, 1, wxEXPAND|wxALL, 0 );

	// Create presets box
	m_presets = new CEdPresets();
	m_presetsBox = new CEdPresetsBox( XRCCTRL( *this, "PresetsPanel", wxPanel ) );
	XRCCTRL( *this, "PresetsPanel", wxPanel )->GetSizer()->Add( m_presetsBox, 0, wxEXPAND );
	m_presetsBoxHook = new CEdStripeEditPresetsBoxHook();
	((CEdStripeEditPresetsBoxHook*)m_presetsBoxHook)->m_tool = tool;
	m_presetsBox->SetPresetsBoxHook( m_presetsBoxHook );
	m_presetsBox->SetPresets( m_presets );

	// Load preset files
	CDirectory* presetsDir = GDepot->FindPath( TXT("engine\\presets\\stripes\\") );
	if ( presetsDir != nullptr )
	{
		m_presets->LoadPresetFiles( presetsDir->GetAbsolutePath() );
	}

	// Setup controls
	m_viewAllCheck->SetValue( ViewAllRoadLines );
	m_viewAICheck->SetValue( ViewAIRoadLines );

	// Assign events
	m_viewAllCheck->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdStripeToolPanel::OnViewAllClick, this );
	m_viewAICheck->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdStripeToolPanel::OnViewAIClick, this );
	m_viewBlendMaskCheck->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdStripeToolPanel::OnViewBlendMaskClick, this );
	m_regularStripe->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdStripeToolPanel::OnRegularStripeClick, this );
	m_terrainStripe->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdStripeToolPanel::OnTerrainStripeClick, this );
	m_aiRoad->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdStripeToolPanel::OnAIRoadClick, this );
	m_alignTerrainToStripe->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdStripeToolPanel::OnAlignTerrainToStripeClick, this );
	browseFalloffMap->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStripeToolPanel::OnBrowseFalloffMapClick, this );
	applyTerrainToStripe->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStripeToolPanel::OnApplyTerrainToStripeClick, this );
	m_alignStripeToTerrain->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdStripeToolPanel::OnAlignStripeToTerrainClick, this );
	applyStripeToTerrain->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStripeToolPanel::OnApplyStripeToTerrainClick, this );
	m_pointSlider->Bind( wxEVT_COMMAND_SLIDER_UPDATED, &CEdStripeToolPanel::OnPointSliderChange, this );
	m_stripePropertiesPage->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdStripeToolPanel::OnPropertyChange, this );
	m_propertiesPage->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdStripeToolPanel::OnPropertyChange, this );
}

CEdStripeToolPanel::~CEdStripeToolPanel()
{
	delete m_presetsBoxHook;
	delete m_presets;
}

void CEdStripeToolPanel::FillFromControlPoint( const SStripeControlPoint& cp )
{
}

void CEdStripeToolPanel::UpdateControlPoint( SStripeControlPoint& cp )
{
}

void CEdStripeToolPanel::UpdateStripeToggleButtons()
{
	m_regularStripe->Enable( m_stripe != nullptr );
	m_terrainStripe->Enable( m_stripe != nullptr );
	m_aiRoad->Enable( m_stripe != nullptr );
	if ( m_stripe != nullptr )
	{
		m_regularStripe->SetValue( !m_stripe->GetProjectToTerrain() );
		m_terrainStripe->SetValue( m_stripe->GetProjectToTerrain() );
		m_aiRoad->SetValue( m_stripe->IsExposedToAI() );
	}
}

void CEdStripeToolPanel::UpdatePointInfo()
{
	m_pointSlider->Enable( m_controlPointIndex != -1 );
	if ( m_controlPointIndex == -1 )
	{
		m_pointLabel->SetLabel( wxT("n/a") );
		m_propertiesPage->SetNoObject();
	}
	else
	{
		m_controlPointIndex = ::Clamp( m_controlPointIndex, 0, (int)( m_stripe->GetControlPointCount() - 1 ) );
		m_pointSlider->SetRange( 0, (int)( m_stripe->GetControlPointCount() - 1 ) );
		m_pointSlider->SetValue( m_controlPointIndex );
		m_pointLabel->SetLabel( wxString::Format( wxT("%d/%d"), (int)( m_controlPointIndex + 1 ), (int)m_stripe->GetControlPointCount() ) );
		m_propertiesPage->SetObject( &m_stripe->GetControlPoint( m_controlPointIndex ) );
	}
}

void CEdStripeToolPanel::PointSelected( Int32 pointIndex )
{
	m_controlPointIndex = pointIndex;
	UpdatePointInfo();
}

void CEdStripeToolPanel::PointModified( Int32 pointIndex )
{
	if ( pointIndex == m_controlPointIndex )
	{
		UpdatePointInfo();
	}
}

void CEdStripeToolPanel::PointDeleted( Int32 pointIndex )
{
	if ( pointIndex == m_controlPointIndex )
	{
		m_controlPointIndex = -1;
		UpdatePointInfo();
	}
}

void CEdStripeToolPanel::HandleInvalidObject( CObject* invalidObject )
{
	// Is the stripe invalid?
	if ( invalidObject == m_stripe )
	{
		SetStripe( nullptr, true );
		return;
	}
}

void CEdStripeToolPanel::OnViewAllClick( wxCommandEvent& event )
{
	ViewAllRoadLines = m_viewAllCheck->GetValue();
	if ( ViewAllRoadLines )
	{
		GStripeComponentEditorFlags |= SCEF_SHOW_CENTER_LINES;
	}
	else
	{
		GStripeComponentEditorFlags &= ~SCEF_SHOW_CENTER_LINES;
	}
}

void CEdStripeToolPanel::OnViewAIClick( wxCommandEvent& event )
{
	ViewAIRoadLines = m_viewAICheck->GetValue();
	if ( ViewAIRoadLines )
	{
		GStripeComponentEditorFlags |= SCEF_SHOW_AIROAD_LINES;
	}
	else
	{
		GStripeComponentEditorFlags &= ~SCEF_SHOW_AIROAD_LINES;
	}
}

void CEdStripeToolPanel::OnViewBlendMaskClick( wxCommandEvent& event )
{
	if ( m_viewBlendMaskCheck->GetValue() )
	{
		GStripeComponentEditorFlags |= SCEF_VISUALIZE_BLEND;
	}
	else
	{
		GStripeComponentEditorFlags &= ~SCEF_VISUALIZE_BLEND;
	}
}

void CEdStripeToolPanel::OnRegularStripeClick( wxCommandEvent& event )
{
	SetPropertyValue( m_stripe, TXT("projectToTerrain"), false, true );
	m_stripe->UpdateStripeProxy();
	UpdateStripeToggleButtons();
}

void CEdStripeToolPanel::OnTerrainStripeClick( wxCommandEvent& event )
{
	SetPropertyValue( m_stripe, TXT("projectToTerrain"), true, true );
	m_stripe->UpdateStripeProxy();
	UpdateStripeToggleButtons();
}

void CEdStripeToolPanel::OnAIRoadClick( wxCommandEvent& event )
{
	Bool value = false;
	GetPropertyValue( m_stripe, TXT("exposedToNavigation"), value );
	value = !value;
	SetPropertyValue( m_stripe, TXT("exposedToNavigation"), value, true );
	m_stripe->UpdateStripeProxy();
	UpdateStripeToggleButtons();
}

void CEdStripeToolPanel::OnAlignTerrainToStripeClick( wxCommandEvent& event )
{
	m_alignTerrainToStripeControls->Show( !m_alignTerrainToStripeControls->IsShown() );
	m_alignTerrainToStripe->SetValue( m_alignTerrainToStripeControls->IsShown() );
	Layout();
}

void CEdStripeToolPanel::OnBrowseFalloffMapClick( wxCommandEvent& event )
{
	wxString selectedFileName = wxFileSelector( wxT("Select Falloff Map"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("PNG files (*.png)|*.png"), wxFD_OPEN|wxFD_FILE_MUST_EXIST, wxTheFrame );
	if ( selectedFileName.IsEmpty() )
	{
		return;
	}

	// Try to load the image
	if ( !m_falloffMap.LoadFile( selectedFileName ) && m_falloffMap.IsOk() )
	{
		m_falloffMap.Resize( wxSize( 1, 1 ), wxPoint( 0, 0 ) );
		m_falloffMap.Clear();
	}

	// Create preview bitmap
	if ( m_falloffMap.IsOk() )
	{
		wxBitmap preview( m_falloffMap );
		m_falloffMapBitmap->SetBitmap( preview );
		m_falloffCheck->SetValue( true );
	}
}

void CEdStripeToolPanel::OnApplyTerrainToStripeClick( wxCommandEvent& event )
{
	m_tool->AlignTerrainToStripes( m_falloffCheck->GetValue() ? ((Float)m_falloffRangeSpin->GetValue())*0.01f : 0.0f,
								   m_heightOffsetCheck->GetValue() ? ((Float)m_heightOffsetSpin->GetValue())*0.01f : 0.0f );
}

void CEdStripeToolPanel::OnAlignStripeToTerrainClick( wxCommandEvent& event )
{
	m_alignStripeToTerrainControls->Show( !m_alignStripeToTerrainControls->IsShown() );
	m_alignStripeToTerrain->SetValue( m_alignStripeToTerrainControls->IsShown() );
	Layout();
}

void CEdStripeToolPanel::OnApplyStripeToTerrainClick( wxCommandEvent& event )
{
	m_tool->AlignStripesToTerrain( ((Float)m_rotationBlendSlider->GetValue())/1000.0f );
}

void CEdStripeToolPanel::OnPointSliderChange( wxCommandEvent& event )
{
	m_tool->SelectPointByIndex( m_stripe, m_pointSlider->GetValue() );
}

void CEdStripeToolPanel::OnPropertyChange( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* clientData = static_cast< CEdPropertiesPage::SPropertyEventData* >( event.GetClientData() );
	ASSERT( m_stripe );
	m_stripe->UpdateStripeProxy();

	// Update the toggle buttons if the user changes the associated properties from the page
	if ( clientData && ( clientData->m_propertyName.AsString() == TXT("projectToTerrain") ||
		                 clientData->m_propertyName.AsString() == TXT("exposeToAI") ) )
	{
		UpdateStripeToggleButtons();
	}
}

void CEdStripeToolPanel::SetStripe( CStripeComponent* stripe, Bool forceRefresh/* =false */ )
{
	// Check if we got the same stripe (and it isn't a forced refresh)
	if ( !forceRefresh && m_stripe == stripe )
	{
		return;
	}

	m_stripe = stripe;
	m_controlPointIndex = -1;

	// Update properties page for stripe
	if ( stripe )
	{
		m_stripePropertiesPage->SetObject( stripe );
	}
	else
	{
		m_stripePropertiesPage->SetNoObject();
	}

	UpdateStripeToggleButtons();
	UpdatePointInfo();

	// Select the first point if there are points available
	if ( stripe && stripe->GetControlPointCount() > 0 )
	{
		m_tool->SelectPointByIndex( stripe, 0 );
	}
}
