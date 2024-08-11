/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "dynamicObstacle.h"
#include "../physics/physicsWrapper.h"
#include "physicsTileWrapper.h"
#include "mesh.h"
#include "../physics/physicsWorld.h"
#include "collisionCache.h"
#include "collisionMesh.h"
#include "../physics/compiledCollision.h"
#include "attachment.h"
#include "animatedComponent.h"
#include "game.h"
#include "staticMeshComponent.h"
#include "clipMap.h"
#include "terrainTile.h"
#include "world.h"
#include "layer.h"
#include "entity.h"
#include "../core/depot.h"
#include "viewport.h"
#include "renderFrame.h"
#include "../physics/physicsSettings.h"
#include "renderFragment.h"

#include "mesh.h"
#include "meshDataBuilder.h"
#include "renderer.h"
#include "layerInfo.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CStaticMeshComponent );	
IMPLEMENT_RTTI_ENUM( EPathEngineCollision );

CStaticMeshComponent::CStaticMeshComponent()
	: m_pathLibCollisionType( PLC_Static )
	, m_fadeOnCameraCollision( false )
	, m_physicsBodyIndex( -1 )
	, m_physicalCollisionType( CNAME( Static ) )
	, m_debugClimbableMesh( NULL )
{
}

CStaticMeshComponent::~CStaticMeshComponent()
{
	SAFE_RELEASE( m_debugClimbableMesh );
}

void CStaticMeshComponent::SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup )
{
	m_pathLibCollisionType = collisionGroup;
}
EPathLibCollision CStaticMeshComponent::GetPathLibCollisionGroup() const
{
	return m_pathLibCollisionType;
}
CComponent* CStaticMeshComponent::AsEngineComponent()
{
	return this;
}
PathLib::IComponent* CStaticMeshComponent::AsPathLibComponent()
{
	return this;
}

CPhysicsWrapperInterface* CStaticMeshComponent::GetPhysicsRigidBodyWrapper() const
{
	CWorld* world = GetWorld();
	if( !world ) return 0;
	CPhysicsWorld* physicsWorld = nullptr;;
	if( !world->GetPhysicsWorld( physicsWorld ) ) return 0;
	CPhysicsTileWrapper* tileWrapper = physicsWorld->GetTerrainTileWrapper( GetLocalToWorld().GetTranslationRef() );
	if( !tileWrapper ) return 0;

	if( m_physicsBodyIndex != -1 )
	{
		CStaticMeshComponent* object = nullptr;
		if( IPhysicsWrapperParentProvider* provider = tileWrapper->GetParentProvider( m_physicsBodyIndex ) )
		{
			provider->GetParent( object );
			if( object == this )
			{
				return tileWrapper;
			}
		}
	}
	return 0;
}

Uint32 CStaticMeshComponent::CalculatePathLibObstacleHash()
{
	const Vector& pos = GetPosition();
	const EulerAngles& rot = GetRotation();
	Uint32 h[7];
	// NOTICE: Hashing might get destroyed by floating point precision issues. We try to minimize 
	// risk by decreasing precision.
	h[ 0 ] = GetHash( Int32( pos.X * 512.f ) );
	h[ 1 ] = GetHash( Int32( pos.Y * 512.f ) );
	h[ 2 ] = GetHash( Int32( pos.Z * 512.f ) );
	h[ 3 ] = GetHash( Int32( rot.Yaw * 512.f ) );
	h[ 4 ] = GetHash( Int32( rot.Pitch * 512.f ) );
	h[ 5 ] = GetHash( Int32( rot.Roll * 512.f ) );
	h[ 6 ] = GetHash( GetMeshResourcePath() );

	return GetArrayHash( h, 7 );
}

Bool CStaticMeshComponent::IsLayerBasedGrouping() const
{
	return true;
}

Bool CStaticMeshComponent::IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const
{
	return false;
}

#ifndef NO_RESOURCE_COOKING
void CStaticMeshComponent::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	if ( GetTransformParent() )
	{
		CObject* parent = GetTransformParent()->GetParent();
		CAnimatedComponent* animComponent = Cast<CAnimatedComponent>( parent );
		if ( animComponent )
		{
			animComponent->ForcePoseAndStuffDuringCook();
		}
	}
}
#endif // NO_RESOURCE_COOKING

CPhysicalCollision CStaticMeshComponent::GetCollisionWithClimbFlags( ) const
{
	CPhysicalCollision outCollision = GetPhysicalCollision( );
	outCollision.ResolveCollisionMasks( );

	if ( m_drawableFlags & DF_ClimbabUnlock || IsEntityMarkedWithTag( CNAME( climb ) ) )
	{
		CPhysicsEngine::CollisionMask newMask = GPhysicEngine->GetCollisionTypeBit( CNAME( UnlockClimb ) );
		outCollision.m_collisionTypeMask |= newMask;
		outCollision.m_collisionTypeNames.PushBackUnique( CNAME( UnlockClimb ) );
	}
	else if ( m_drawableFlags & DF_ClimbBlock || IsEntityMarkedWithTag( CNAME( no_climb ) ) )
	{
		CPhysicsEngine::CollisionMask newMask = GPhysicEngine->GetCollisionTypeBit( CNAME( LockClimb ) );
		outCollision.m_collisionTypeMask |= newMask;
		outCollision.m_collisionTypeNames.PushBackUnique( CNAME( LockClimb ) );
	}

	return outCollision;
}

Bool CStaticMeshComponent::IsEntityMarkedWithTag( CName tag ) const
{
	if ( const CEntity* ent = this->GetEntity() )
	{
		if( ent->GetTags().HasTag( tag ) )
		{
			return true;
		}
	}

	return false;
}

void CStaticMeshComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Collision layer changed
	if ( property->GetName().AsString() == TXT("layerType") )
	{
		PerformFullRecreation();
	}

	if ( property->GetName().AsString() == TXT("pathLibCollisionType") )
	{
		if ( m_pathLibCollisionType == PLC_Immediate )
		{
			m_pathLibCollisionType = PLC_Dynamic;
		}
	}
}

Bool CStaticMeshComponent::UsesAutoUpdateTransform()
{
	Bool result = TBaseClass::UsesAutoUpdateTransform();
#ifndef NO_EDITOR
	// Special case for editor - if we missed update transform because we're invisible
	// then update the transform anyway since the collision mesh position depends on
	// the transform
	if ( GIsEditor && !GIsEditorGame && ( m_drawableFlags & DF_MissedUpdateTransform ) == DF_MissedUpdateTransform && m_renderProxy == nullptr )
	{
		m_drawableFlags &= ~DF_MissedUpdateTransform;
		result = true;
	}
#endif
	return result;
}

Bool CStaticMeshComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( TBaseClass::OnPropertyMissing( propertyName, readValue ) )
	{
		return true;
	}

	static CName pathEngineCollisionType( TXT("pathEngineCollisionType") );

	if ( propertyName == pathEngineCollisionType )
	{
		Uint32 val = *static_cast< const EPathEngineCollision* > ( readValue.GetData() );
		m_pathLibCollisionType = EPathLibCollision( val );
		return true;
	}

	return false;
}

#ifndef NO_EDITOR
void CStaticMeshComponent::EditorOnTransformChangeStart()
{
	TBaseClass::EditorOnTransformChangeStart();
}

void CStaticMeshComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	ForceUpdateTransformNodeAndCommitChanges();
	if( CPhysicsWrapperInterface* tileWrapper = GetPhysicsRigidBodyWrapper() )
	{
		tileWrapper->SetFlag( PRBW_PoseIsDirty, true );
	}
}

void CStaticMeshComponent::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
	EditorRecreateCollision();
}

void CStaticMeshComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	if ( IsNoticableInEditor( PathLib::CProcessingEvent::TYPE_ATTACHED ) )
	{
		GetMeshNow();
	}
	TBaseClass::OnNavigationCook( world, context );
}

void CStaticMeshComponent::EditorRecreateCollision()
{
	// Destroy the physics object
	if ( CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper() )
	{
		if( m_physicsBodyIndex != -1 )
		{
			wrapper->Release( m_physicsBodyIndex );
			m_physicsBodyIndex = -1;
		}
	}
	CompiledCollisionPtr compiledCollision = GetCompiledCollision( true );
	if( compiledCollision )
	{
		OnCompiledCollisionFound( compiledCollision );
	}
}
#endif		// !NO_EDITOR

CompiledCollisionPtr CStaticMeshComponent::GetCompiledCollision( Bool loadSynchronously ) const
{
	const String meshResourcePath = GetMeshResourcePath();
	CDiskFile* file = GDepot->FindFile( meshResourcePath );
	if( !file )
	{
		return CompiledCollisionPtr();
	}

	Red::System::DateTime time = file->GetFileTime();

	// Test the cache
	{
		CompiledCollisionPtr compiledMesh;
		RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
		GCollisionCache->FindCompiled_Sync( compiledMesh, meshResourcePath, time );

		if ( compiledMesh )
		{
			return compiledMesh;
		}
	}

#ifndef NO_RESOURCE_IMPORT
	CMesh* theMesh = loadSynchronously ? GetMeshNow() : TryGetMesh();
	if( !theMesh )
	{
		return CompiledCollisionPtr();
	}

	// Use the collision shape from the mesh if it is loaded
	const CCollisionMesh* collisionMesh = theMesh->GetCollisionMesh();
	if ( collisionMesh )
	{
		CompiledCollisionPtr compiledMesh;

		// Compile a local version of collision mesh
		GCollisionCache->Compile_Sync( compiledMesh, collisionMesh, meshResourcePath, time );
		return compiledMesh;
	}
#endif

	return CompiledCollisionPtr();

}

void CStaticMeshComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	ForceUpdateTransformNodeAndCommitChanges();

	ScheduleFindCompiledCollision();

	// Register to editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Collision );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NonClimbable );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_CollisionIfNotVisible );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_CollisionSoundOcclusion );
}

void CStaticMeshComponent::OnDetached( CWorld* world )
{
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	// Destroy the physics object
	if ( wrapper )
	{
		CStaticMeshComponent* object = nullptr;
		if( IPhysicsWrapperParentProvider* provider = wrapper->GetParentProvider( m_physicsBodyIndex ) )
		{
			provider->GetParent( object );
			if( object == this)
			{
				wrapper->Release( m_physicsBodyIndex );
				m_physicsBodyIndex = -1;
			}
		}
	}

	CancelFindCompiledCollision();

	// Remove form editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Collision );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NonClimbable );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_CollisionIfNotVisible );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_CollisionSoundOcclusion );

	// Base detach
	TBaseClass::OnDetached( world );
}

Bool CStaticMeshComponent::ShouldGenerateEditorFragments( CRenderFrame* frame ) const
{
	Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	Vector currentPos = GetLocalToWorld().GetTranslation();
	Float distFromCam = frame->GetFrameInfo().m_camera.GetPosition().DistanceSquaredTo( currentPos );
	Float size = GetBoundingBox().CalcSize().SquareMag3();

	if ( distFromCam < dist + size )
	{
		return true;
	}
	else
	{
		return false;
	}

}


void CStaticMeshComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	Bool renderCollisionMesh = false;
	if ( flag == SHOW_Collision )
	{
		renderCollisionMesh = true;
	}

	if ( flag == SHOW_CollisionSoundOcclusion && m_physicsBodyIndex != -1 )
	{
		CWorld* world = GetWorld();
		if ( world )
		{
			CPhysicsWorld* physicsWorld = nullptr;;
			if( world->GetPhysicsWorld( physicsWorld ) )
			{
				CPhysicsTileWrapper* tileWrapper = physicsWorld->GetTerrainTileWrapper( GetLocalToWorld().GetTranslationRef() );
				if( tileWrapper )
				{
					Float occlusionAttenuation = 0.0f;
					Float occlusionDiagonalLimit = 0.0f;
					tileWrapper->GetOcclusionParameters( m_physicsBodyIndex, 0, &occlusionDiagonalLimit );
					if( occlusionAttenuation >= 0.0f && occlusionDiagonalLimit >= 0.0f ) 
					{
						renderCollisionMesh = true;
					}
				}
			}
		}
	}

	if( flag == SHOW_CollisionIfNotVisible )
	{
		if( !IsVisible() )
		{
			renderCollisionMesh = true;
		}
	}	

#ifndef NO_RESOURCE_IMPORT
	// Collision mode, render collision only
	if( renderCollisionMesh )
	{
		// Get the collision mesh
		const CCollisionMesh* mesh = TryGetMesh() ? TryGetMesh()->GetCollisionMesh() : NULL;
		if ( mesh )
		{
			// Setup rendering context
			CCollisionMesh::RenderContext renderContext;
#ifndef NO_COMPONENT_GRAPH
			renderContext.m_hitProxyID = GetHitProxyID();
#else
			renderContext.m_hitProxyID = CHitProxyID();
#endif
			renderContext.m_localToWorld = GetLocalToWorld();
			renderContext.m_selected = IsSelected();
			renderContext.m_solid = true;

			// Generate collision mesh fragments
			mesh->GenerateFragments( frame, renderContext );
		}
	}
#endif

	if ( flag == SHOW_NonClimbable )
	{
		const CPhysicsWrapperInterface* wrapper = this->GetPhysicsRigidBodyWrapper();
		{
			if ( wrapper )
			{
				CPhysicsEngine::CollisionMask colMask = wrapper->GetCollisionTypesBits();
				CPhysicsEngine::CollisionMask climbableMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) 
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) )
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) )
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) )
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Fence ) );


				if ( !IsEntityMarkedWithTag( CName( TXT( "no_climb" ) ) ) && ( colMask & climbableMask ) )
				{
					return;
				}

				if ( !m_debugClimbableMesh )
				{
					GenerateDebugMesh();
				}
			}
			
			if ( m_debugClimbableMesh )				
			{
				// generate fragments
				new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_debugClimbableMesh, true );
			}
		}
	}

	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );
}

void CStaticMeshComponent::GenerateDebugMesh()
{
	if ( CMesh* mesh = TryGetMesh() )
	{
		const CMesh::TLODLevelArray& lodLvlInfo = mesh->GetMeshLODLevels();
		if ( lodLvlInfo.Size() == 0 )
		{
			return;
		}

		CMeshData meshdata( mesh );
		const auto& chunks = meshdata.GetChunks();
		CMesh::LODLevel currentLevel = lodLvlInfo[0];

		Uint32 verticesCount = 0;
		for ( Uint32 i = 0; i < currentLevel.m_chunks.Size(); ++i )
		{
			Uint32 chunk_i = currentLevel.m_chunks[i];
			if ( chunk_i < chunks.Size() )
			{
				verticesCount += chunks[chunk_i].m_vertices.Size();
			}
		}

		if ( verticesCount <= 0 )
		{
			return;
		}

		// generate debug mesh
		TDynArray< DebugVertex > vertices;
		TDynArray< Uint32 > indices;
		Color color = Color::CYAN;
		color.A = 150;

		Uint32 localVertexOffset = 0;
		for ( Uint32 i = 0; i < currentLevel.m_chunks.Size(); ++i )
		{
			Uint32 chunk_i = currentLevel.m_chunks[i];
			if ( chunk_i < chunks.Size() )
			{
				const SMeshChunk& currChunk = chunks[chunk_i];

				for ( Uint32 j = 0; j < currChunk.m_vertices.Size(); ++j )
				{
					new ( vertices ) DebugVertex( Vector( currChunk.m_vertices[j].m_position ) + Vector( currChunk.m_vertices[j].m_normal ) * 0.025f, color );
				}

				for ( Uint32 j = 0; j < currChunk.m_indices.Size(); ++j )
				{
					indices.PushBack( currChunk.m_indices[j] + localVertexOffset );
				}
				localVertexOffset += currChunk.m_vertices.Size();
			}
		}

		// Upload to card
		if ( indices.Size() > 0 )
		{
			m_debugClimbableMesh = GRender->UploadDebugMesh( vertices, indices );
		}
	}
}

void CStaticMeshComponent::ScheduleFindCompiledCollision()
{
	if( ShouldScheduleFindCompiledCollision() )
	{
		if ( GetMeshNow() )
		{
			const String meshResourcePath = GetMeshResourcePath();
			CDiskFile* file = GDepot->FindFile( meshResourcePath );
			if( file )
			{
				GCollisionCache->FindCompiled_Async( this, meshResourcePath, file->GetFileTime() );	
			}
		}
	}
}

Bool CStaticMeshComponent::ShouldScheduleFindCompiledCollision() const
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateStaticMeshGeometry ) return false;
#endif

	if( GetPhysicsRigidBodyWrapper() )
	{
		return false;
	}

	return true;
}

void CStaticMeshComponent::CancelFindCompiledCollision()
{
	if( !m_compiledCollision )
	{
		GCollisionCache->CancelFindCompiled_Async( this );
	}

	m_compiledCollision.Reset();
}

void CStaticMeshComponent::OnCompiledCollisionInvalid()
{
#ifndef NO_RESOURCE_IMPORT
	CMesh* theMesh = GetMeshNow();
	if( !theMesh )
	{
		return;
	}

	// Use the collision shape from the mesh if it is loaded
	const CCollisionMesh* collisionMesh = theMesh->GetCollisionMesh();
	if ( collisionMesh )
	{
		// Compile a local version of collision mesh
		CompiledCollisionPtr collision;

		CDiskFile* file = GDepot->FindFile( GetMeshResourcePath() );

		if( GCollisionCache->Compile_Sync( collision, collisionMesh, GetMeshResourcePath(), file->GetFileTime() ) )
		{
			OnCompiledCollisionFound( collision );
		}
	}
#endif
}

void CStaticMeshComponent::OnCompiledCollisionFound( CompiledCollisionPtr collision )
{
	m_compiledCollision = collision;

	CWorld* world = GetWorld();
	if ( !world ) return ;
	CPhysicsWorld* physicsWorld = nullptr;
	if( !world->GetPhysicsWorld( physicsWorld ) ) return;

	const Matrix& pose = GetLocalToWorld();
	const Vector& position = pose.GetTranslationRef();

	CPhysicsTileWrapper* tileWrapper = physicsWorld->GetTerrainTileWrapper( position );
	if( !tileWrapper )
	{
		CClipMap* clipMap = GetWorld()->GetTerrain();
		if( clipMap && physicsWorld->IsPositionInside( position ) )
		{
			Int32 x = 0;
			Int32 y = 0;
			CTerrainTile* terrainTile = clipMap->GetTileFromPosition( position, x, y );
			if( terrainTile )
			{
				tileWrapper = terrainTile->CreateCollisionPlaceholder( world, clipMap->GetBoxForTile( x, y, 0.0f ), x, y, terrainTile->GetResolution() );
			}
			else return;
		}
		else return;
	}
	if( m_physicsBodyIndex != -1 )
	{
		if( tileWrapper )
		{
			CStaticMeshComponent* object = nullptr;
			if( IPhysicsWrapperParentProvider* provider = tileWrapper->GetParentProvider( m_physicsBodyIndex ) )
			{
				provider->GetParent( object );
				if( object == this )
				{
					return;
				}
			}
		}
		m_physicsBodyIndex = -1;
	}
	if( !tileWrapper) return ;

	CPhysicalCollision upgradedCollision = GetCollisionWithClimbFlags( );

	m_physicsBodyIndex = tileWrapper->AddStaticBody( CPhysicsWrapperParentComponentProvider( this ), pose, m_compiledCollision, upgradedCollision.m_collisionTypeMask, upgradedCollision.m_collisionGroupsMask );
}
