/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshComponent.h"

#ifdef USE_UMBRA
#include "umbraScene.h"
#endif

#include "mesh.h"
#include "meshDataBuilder.h"
#include "../../common/core/gatheredResource.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "meshSkinningAttachment.h"	
#include "renderVertices.h"
#include "renderFrame.h"
#include "world.h"
#include "../core/events.h"
#include "../core/resourceUsage.h"
#include "meshTypeResource.h"
#include "entity.h"
#include "tickManager.h"
#include "bitmapTexture.h"
#include "baseEngine.h"
#include "layer.h"

IMPLEMENT_ENGINE_CLASS( CMeshComponent );


const Float CMeshComponent::AUTOHIDE_MARGIN = 5.0f;


CGatheredResource resMeshIcon( TXT("engine\\textures\\icons\\meshicon.xbm"), RGF_NotCooked );

CMeshComponent::CMeshComponent()
{
}

CMeshComponent::~CMeshComponent()
{
}

CBitmapTexture* CMeshComponent::GetSpriteIcon() const
{
	return resMeshIcon.LoadAndGet< CBitmapTexture >();
}

Color CMeshComponent::CalcSpriteColor() const
{
	if ( IsSelected() )
	{
		return Color::GREEN;
	}
	else
	{
		return Color::WHITE;
	}
}

Float CMeshComponent::CalcSpriteSize() const
{
    return 0.25f;
}

void CMeshComponent::SetResource( CResource* resource )
{
	RED_ASSERT( !resource || resource->IsA< CMesh >(), TXT("Cannot set '%ls' to '%ls' component."), resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() ); 
	CMesh* mesh = Cast< CMesh >( resource );
	if ( (mesh != nullptr || resource == nullptr) && mesh != m_mesh )
	{
		// Set new mesh
		m_mesh = mesh;

		// Update bounding
		OnUpdateBounds();

		// Full recreation
		PerformFullRecreation();
	}
}

void CMeshComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	resources.PushBack( TryGetMesh() );
}

CMesh* CMeshComponent::TryGetMesh() const
{
	return m_mesh;
}

CMesh* CMeshComponent::GetMeshNow() const
{
	// Note, we don't stop the streaming here, it just means we will get a callback later where we can update bounds, etc, rather than doing it here
	return m_mesh;
}

const String CMeshComponent::GetMeshResourcePath() const
{
	if ( m_mesh )
		return m_mesh->GetDepotPath();

	return TXT("UnknownMesh");
}

void CMeshComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// We need to update bounds if mesh is changed
	if ( property->GetName() == TXT("mesh") )
	{
		// Update bounds
		ScheduleUpdateTransformNode();

		// Recreate so the mesh is shown immediately
		PerformFullRecreation();
	}
}

void CMeshComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_TBN );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_MeshComponent );
	if ( world->GetPreviewWorldFlag() )
	{
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NavObstacles );
	}
	
#if !defined( NO_RESOURCE_IMPORT ) && !defined( NO_EDITOR )
	if ( TryGetMesh() != nullptr && TryGetMesh()->GetAutoHideDistance() <= 0.f )
	{
		DATA_HALT( DES_Major, GetMeshNow(), TXT("Mesh properties"), TXT("No autohide distance set in mesh. Imported by: %ls"), GetMeshNow()->GetAuthorName().AsChar() );
	}
#endif
}

void CMeshComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_TBN );
	if ( world->GetPreviewWorldFlag() )
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NavObstacles );
	}
}

#ifdef USE_UMBRA
Bool CMeshComponent::ShouldBeCookedAsOcclusionData() const
{
	// if meshComponent is attached to entity via HardAttachment it will be transformed along with the parent, then do not include it in occlusion data
	return !GetTransformParent();
}

Bool CMeshComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	Uint8 flags = 0;
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this, &flags ) )
	{
		return umbraScene->AddMesh( this, bounds, flags );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif // USE_UMBRA

void CMeshComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Get the mesh (if loaded yet)
	CMesh* mesh = TryGetMesh();
    if ( mesh == nullptr )
	{
        return;
    }

	// Draw only if visible
	if ( IsVisible() && IsAttached() && flag == SHOW_MeshComponent )
	{
		// Sprite icon
		CBitmapTexture* icon = GetSpriteIcon();
		if ( icon )
		{
			// Draw editor icons
			Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
			const Float size = 0.25f*screenScale;
#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), GetHitProxyID(), icon, false );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), CHitProxyID(), icon, false );
#endif
		}
	}

#ifndef NO_RESOURCE_IMPORT
	if ( flag == SHOW_TBN )
	{
		const Matrix& localToWorld = this->GetLocalToWorld();

		const CMeshData meshData( mesh );
		const auto& chunkArr = meshData.GetChunks();

		Int32 lod = GetForcedLODLevel();
		lod = Max( lod, 0 );
		
		const CMesh::TChunkArray& meshChunks = mesh->GetMeshLODLevels()[lod].m_chunks;
		const Uint32 numCh = meshChunks.Size();

		// Precalculate how big a buffer we need, so we don't need to re-alloc.
		Uint32 totalLinesSize = 0;
		for ( Uint32 ch = 0; ch < numCh; ++ch )
		{
			// Each vertex has three lines (6 points).
			totalLinesSize += chunkArr[meshChunks[ch]].m_numVertices * 6;
		}
		TDynArray< DebugVertex > lines;
		lines.Reserve( totalLinesSize );

		for ( Uint32 ch = 0; ch < numCh; ++ch )
		{
			Uint16 chunkId = meshChunks[ch];

			const TDynArray< SMeshVertex, MC_BufferMesh >& chunkVertices = chunkArr[chunkId].m_vertices;
			const Uint32 numVerts = chunkArr[chunkId].m_numVertices;

			Vector tran;
			Matrix rot = Matrix::IDENTITY;
			for ( Uint32 v=0; v <numVerts; ++v )
			{
				const SMeshVertex& vertex = chunkVertices[v];

				rot.V[0].Set3( vertex.m_tangent );
				rot.V[0].W = 0.0f;

				rot.V[1].Set3( vertex.m_binormal );
				rot.V[1].W = 0.0f;

				rot.V[2].Set3( vertex.m_normal );
				rot.V[2].W = 0.0f;

				tran.Set3( vertex.m_position );
				tran.W =  1.0f;

				Vector worldPos = localToWorld.TransformPoint( tran );

				const Float scale = 0.2f * frame->GetFrameInfo().CalcScreenSpaceScale( worldPos );

				Vector tEnd = tran+rot.V[0]*scale;
				Vector bEnd = tran+rot.V[1]*scale;
				Vector nEnd = tran+rot.V[2]*scale;

				tEnd = localToWorld.TransformPoint( tEnd );
				bEnd = localToWorld.TransformPoint( bEnd );
				nEnd = localToWorld.TransformPoint( nEnd );

				lines.PushBack( DebugVertex( worldPos, Color::RED.ToUint32() ) );
				lines.PushBack( DebugVertex( tEnd, Color::RED.ToUint32() ) );
				lines.PushBack( DebugVertex( worldPos, Color::GREEN.ToUint32() ) );
				lines.PushBack( DebugVertex( bEnd, Color::GREEN.ToUint32() ) );
				lines.PushBack( DebugVertex( worldPos, Color::BLUE.ToUint32() ) );
				lines.PushBack( DebugVertex( nEnd, Color::BLUE.ToUint32() ) );

				// If adding another set of lines pushes us over some threshold, flush lines. Keeps us from sending
				// too many at once.
				// Ultimately, the buffer used by GpuApi for rendering these should be bigger than this, but let's
				// just be safe :)
				if ( lines.Size() + 6 > 65536 )
				{
					frame->AddDebugLines( &lines[0], lines.Size() );
					lines.ClearFast();
				}
			}
		}

		if ( !lines.Empty() )
		{
			frame->AddDebugLines( &lines[0], lines.Size() );
		}
	}
	else 
#endif
		
	if ( flag == SHOW_NavObstacles )
	{
#ifndef NO_OBSTACLE_MESH_DATA
		const auto& obstaclesDef = mesh->GetNavigationObstacle();
		if ( !obstaclesDef.IsEmpty() )
		{
			Matrix m;
			GetLocalToWorld( m );

			static const Uint32 MAX_VERTS = 128;
			TStaticArray< DebugVertex, MAX_VERTS*2 > outputVerts;
			TStaticArray< Uint16, MAX_VERTS*6 > outputIndices;

			const Color COLOR_FLOOR( 0, 190, 20, 128 );
			const Color COLOR_CEIL( 0, 220, 0, 128 );

			const auto& shapeList = obstaclesDef.GetShapes();
			for ( auto itShapes = shapeList.Begin(), endShapes = shapeList.End(); itShapes != endShapes; ++itShapes )
			{
				const auto& verts = itShapes->m_verts;
				const Box& bounds = itShapes->m_bbox;

				if ( verts.Size() > 128 || verts.Size() < 3 )
				{
					continue;
				}
				outputVerts.Clear();
				outputIndices.Clear();

				for ( Uint32 i = 0, n = verts.Size(); i < n; ++i )
				{
					const Vector2& v = verts[ i ];
					Vector pos1( v.X, v.Y, bounds.Min.Z );
					Vector pos2( v.X, v.Y, bounds.Max.Z );

					outputVerts.PushBack( DebugVertex( pos1, COLOR_FLOOR ) );
					outputVerts.PushBack( DebugVertex( pos2, COLOR_CEIL ) );

					// add indices
					if ( i > 0 )
					{
						Uint16 baseInd = Uint16( outputVerts.Size() );
						// wall tri1
						outputIndices.PushBack( baseInd-1 );
						outputIndices.PushBack( baseInd-2 );
						outputIndices.PushBack( baseInd-3 );
						// wall tri2
						outputIndices.PushBack( baseInd-4 );
						outputIndices.PushBack( baseInd-3 );
						outputIndices.PushBack( baseInd-2 );
					}
				}

				Uint16 indiceLimit = Uint16( outputVerts.Size() );
				outputIndices.PushBack( 0 );
				outputIndices.PushBack( indiceLimit-2 );
				outputIndices.PushBack( indiceLimit-1 );

				outputIndices.PushBack( indiceLimit-1 );
				outputIndices.PushBack( 1 );
				outputIndices.PushBack( 0 );

				frame->AddDebugTriangles( m, outputVerts.TypedData(), outputVerts.Size(), outputIndices.TypedData(), outputIndices.Size(), COLOR_FLOOR, false, true );
			}
		}
#endif
	}
}


#ifndef NO_DATA_VALIDATION
void CMeshComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	if ( GetMeshNow() != nullptr )
	{		
		// distance of LOD[n+1] should be greater than distance of LOD[n]
		if ( GetMeshNow()->GetNumLODLevels() > 0 )
		{
			for ( Uint32 i = 0; i < GetMeshNow()->GetNumLODLevels() - 1; ++i )
			{
				Float thisLodDistance = GetMeshNow()->GetLODLevel( i ).GetDistance();
				Float nextLodDistance = GetMeshNow()->GetLODLevel( i + 1 ).GetDistance();
				if( thisLodDistance >= nextLodDistance )
				{
					DATA_HALT( DES_Major, TryGetMesh(), TXT("Rendering"), TXT("MeshComponent '%ls' in entity has invalid LOD distances: %1.3f in LOD%d and %1.3f in LOD%d!"), GetName().AsChar(), thisLodDistance, i, nextLodDistance, i + 1 );
				}
			}
		}
		else
		{
			DATA_HALT( DES_Major, TryGetMesh(), TXT("Rendering"), TXT("MeshComponent '%ls' uses mesh with no LODs!"), GetName().AsChar() );
		}

		if( GetMeshNow()->GetAutoHideDistance() < 0.01f ) 
		{
			DATA_HALT( DES_Major, TryGetMesh(), TXT("Rendering"), TXT("MeshComponent '%ls' in entity has no AutoHideDistance!"), GetName().AsChar() );
		}

		if(GetMeshNow()->HasFlag( DF_ForceNoAutohide ) ) 
		{
			DATA_HALT( DES_Minor, TryGetMesh(), TXT("Rendering"), TXT("MeshComponent '%ls' in entity is forcing NO AUTOHIDE!"), GetName().AsChar() );
		}
		
		// There are legitimate reasons for that (collision-only entities,
		// hidden-by-default entities shown by scripts, etc) so i commented
		// it out.  Contact me if there is a reason this is a bad idea  -bs
		/*
		if( !(GetMeshNow()->HasFlag( DF_IsVisible )) && !(GetMeshNow()->HasFlag( DF_CastShadowsWhenNotVisible )) && !(DF_Col) 
		{
			DATA_ASSERT( false, DES_Minor, GetEntity(), TXT("Rendering"), TXT("MeshComponent '%ls' in entity is not visible and not casting any shadows, is it intentional?"), GetName().AsChar() );
		}
		*/
		
		if( m_forceLODLevel != -1 )
		{
			DATA_HALT( DES_Minor, TryGetMesh(), TXT("Rendering"), TXT("MeshComponent '%ls' in entity is forcing LOD level, is it intentional?"), GetName().AsChar() );
		}
	}

}

#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO

void CMeshComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	TBaseClass::CollectResourceUsage( collector, isStremable );

	if ( m_mesh )
		collector.ReportResourceUsage( m_mesh );
}

#endif

// Overriding this to handle the type change for mesh
Bool CMeshComponent::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue ) )
		return true;

	
	if ( propertyName == TXT("mesh") && readValue.GetRTTIType() )
	{
		if ( readValue.GetRTTIType() == GetTypeObject< TSoftHandle< CMesh > >() )
		{
			const auto& meshHandle = *(const TSoftHandle< CMesh > *) readValue.GetData();
			m_mesh = meshHandle.Get();
			return true;
		}
		else if ( readValue.GetRTTIType() == GetTypeObject< THandle< CMesh > >() )
		{
			m_mesh = *(const THandle< CMesh > *) readValue.GetData();
			return true;
		}
		else if ( readValue.GetRTTIType() == GetTypeObject< CMesh* >() )
		{
			m_mesh = *(CMesh** ) readValue.GetData();
			return true;
		}
		else
		{
			ERR_ENGINE( TXT("Invalid type of mesh handle: %ls in prop %ls in %ls"), 
				readValue.GetRTTIType()->GetName().AsChar(), propertyName.AsChar(), GetFriendlyName().AsChar() );
		}
	}

	// not matched
	return false;
}

void CMeshComponent::OnAppearanceChanged( Bool added )
{
	// Force a full load before passing the event to the superclass
	if ( added )
	{
		GetMeshNow();
	}

	TBaseClass::OnAppearanceChanged( added );
}

void CMeshComponent::SetAsCloneOf( const CMeshComponent* otherMeshComponent )
{
	m_transform						= otherMeshComponent->m_transform;
	m_tags							= otherMeshComponent->m_tags;
	m_drawableFlags					= otherMeshComponent->m_drawableFlags;	
#ifndef NO_COMPONENT_GRAPH
	m_graphPositionX				= otherMeshComponent->m_graphPositionX;
	m_graphPositionY				= otherMeshComponent->m_graphPositionY;
#endif
	
	// This function should only be used in the editor
	SetResource( otherMeshComponent->GetMeshNow() );

	OnPostLoad();
}

Bool CMeshComponent::TryUsingResource( CResource * resource )
{
	if ( CMesh* mesh = Cast< CMesh >( resource ) )
	{
		SetResource( mesh );
		return true;
	}
	return false;
}

CMeshTypeResource* CMeshComponent::GetMeshTypeResource() const
{
	return TryGetMesh();
}

Uint32 CMeshComponent::GetOcclusionId() const
{
	RED_ASSERT( GetMeshNow() != nullptr, TXT("Mesh not available") );
	return GetMeshNow() ? GetHash( GetMeshNow()->GetDepotPath() ) : 0;
}

Bool CMeshComponent::IsMergableIntoWorldGeometry() const
{
	// entity is not mergable
	if ( !GetEntity()->CheckStaticFlag( ESF_NoCompMerge ) )
		return false;

	// no mesh or mesh not mergable ?
	if ( !m_mesh || !m_mesh->CanMergeIntoGlobalShadowMesh() )
		return false;

	// in general we are mergable
	return true;
}
