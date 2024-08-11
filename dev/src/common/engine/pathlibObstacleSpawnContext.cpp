#include "build.h"
#include "pathlibObstacleSpawnContext.h"

#include "../core/heap.h"

#include "collisionMesh.h"
#include "collisionShapeBuilder.h"
#include "collisionShape.h"
#include "component.h"
#include "deniedAreaComponent.h"
#include "destructionSystemComponent.h"
#include "entity.h"
#include "layer.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "mesh.h"
#include "navigationObstacle.h"
#include "pathlibObstacleShape.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibWorld.h"
#include "staticMeshComponent.h"
#include "foliageResource.h"

#ifdef USE_APEX
#include <PxRigidActor.h>
#include "../physics/physXEngine.h"
#include "ApexDestructionWrapper.h"
#endif
#include "destructionComponent.h"


using namespace physx;

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CProcessingEvent
////////////////////////////////////////////////////////////////////////////

Bool CProcessingEvent::GeneralProcessingMechanism::RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )
{
	return false;
}
Bool CProcessingEvent::GeneralProcessingMechanism::ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, IGenerationManagerBase::CAsyncTask::CSynchronousSection& section )
{
	return false;
}
////////////////////////////////////////////////////////////////////////////
// SComponentMapping
////////////////////////////////////////////////////////////////////////////
SComponentMapping::SComponentMapping( CComponent* component )
	: m_entityGuid( component->GetEntity()->GetGUID() )
	, m_componentHash( 0 )
{
	component->GetName().SimpleHash( m_componentHash );
}

IComponent* SComponentMapping::GetComponent( CWorld* world )
{
	CEntity* entity = world->FindEntity( m_entityGuid );
	if ( !entity )
	{
		return nullptr;
	}
	for ( CComponent* component : entity->GetComponents() )
	{
		// firstly check if there is PathLib component inside - because it looks much cheaper than to start from hash checking
		IComponent* o = component->AsPathLibComponent();
		if ( o )
		{
			Uint32 h = 0;
			component->GetName().SimpleHash( h );
			if ( h == m_componentHash )
			{
				return o;
			}
		}
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// SLayerMapping
////////////////////////////////////////////////////////////////////////////
SLayerMapping::SLayerMapping( CComponent* component )
{
	m_layerGroupId = component->GetEntity()->GetLayer()->GetLayerInfo()->GetLayerGroup()->GetLayerGroupId();
}

SLayerMapping::SLayerMapping( CLayerGroup* layerGroup )
{
	m_layerGroupId = layerGroup->GetLayerGroupId();
}

////////////////////////////////////////////////////////////////////////////
// CObstacleSpawnContext
////////////////////////////////////////////////////////////////////////////
void CObstacleSpawnData::Initialize( CStaticMeshComponent* staticMesh, CPathLibWorld& pathlib )
{
	m_collisionType = staticMesh->GetPathLibCollisionType();
	m_bbox.Clear();
	CMesh* mesh = staticMesh->GetMeshNow();		/// ???
	if ( mesh )
	{		
		Matrix localToWorld;
		staticMesh->GetLocalToWorld( localToWorld );
#ifndef NO_OBSTACLE_MESH_DATA
		if( TryProcessNavObstacleData(mesh, localToWorld) )
		{
			return;
		}
#endif

		RED_MESSAGE( "Cooked data DOES NOT HAVE CCollisionMesh - direct access to collision cache is required" );
#ifndef NO_RESOURCE_IMPORT
		TryProcessCollisionCache(mesh, localToWorld);
#endif
	}
}

void CObstacleSpawnData::Initialize( CDestructionComponent* destructionComp, CPathLibWorld& pathlib )
{
	m_collisionType = destructionComp->GetPathLibCollisionType();
	m_bbox.Clear();
	CMesh* mesh = destructionComp->TryGetMesh();	

	if ( mesh )
	{
		Matrix localToWorld;
		destructionComp->GetLocalToWorld( localToWorld );
#ifndef NO_OBSTACLE_MESH_DATA
		if( TryProcessNavObstacleData(mesh, localToWorld) )
		{
			return;
		}
#endif

		RED_MESSAGE( "Cooked data DOES NOT HAVE CCollisionMesh - direct access to collision cache is required" );
#ifndef NO_RESOURCE_IMPORT
		TryProcessCollisionCache(mesh, localToWorld);
#endif
	}
}

void CObstacleSpawnData::Initialize( CDeniedAreaComponent* deniedArea, CPathLibWorld& pathlib )
{
	m_collisionType = deniedArea->GetCollisionType();
	m_bbox = deniedArea->GetBoundingBox();
	CObstacleConvexOccluderData* convexData = new CObstacleConvexOccluderData();
	const auto& worldPoints = deniedArea->GetWorldPoints();
	convexData->m_shapeBBox = deniedArea->GetBoundingBox();
	convexData->m_occluder.Resize( worldPoints.Size() );
	for ( Uint32 i = 0, n = worldPoints.Size(); i != n; ++i )
	{
		convexData->m_occluder[ i ] = worldPoints[ i ].AsVector2();
	}
	if ( MathUtils::GeometryUtils::GetClockwisePolygonArea2D( convexData->m_occluder ) > 0.f )
	{
		Reverse( convexData->m_occluder.Begin(), convexData->m_occluder.End() );
	}
	m_shapes.Resize( 1 );
	m_shapes[ 0 ] = convexData;
}

void CObstacleSpawnData::Initialize( CDestructionSystemComponent* destructionSystem, CPathLibWorld& pathlib )
{
	m_collisionType = PLC_Disabled;
#ifdef USE_APEX

	CApexDestructionWrapper* wrapper = destructionSystem->GetDestructionBodyWrapper();
	
	if ( !wrapper || wrapper->GetActorsCount() == 0 )
	{
		return;
	}

	physx::PxActor* actor = ( physx::PxActor* ) wrapper->GetActor( 0 );
	if( !actor )
	{
		return;
	}

	m_collisionType = destructionSystem->GetPathLibCollisionType();
	m_bbox.Clear();

	physx::PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return;
	Uint32 shapesCount = rigidActor->getNbShapes();

	TDynArray< PxShape* > pxShapeList( shapesCount );
	rigidActor->getShapes( pxShapeList.TypedData(), shapesCount );

	CObstacleIndicedVertsGeometryData shapeData;
	auto& outVerts( shapeData.m_verts );
	auto& outIndices( shapeData.m_indices );
	Bool initialized = false;
	
	for ( Uint32 i = 0; i < shapesCount; ++i )
	{
		PxShape* pxShape = pxShapeList[ i ];
		// this code could be somehow generalized
		switch( pxShape->getGeometryType() )
		{
		case PxGeometryType::eBOX:
			{
				PxBoxGeometry boxGeometry;
				if ( pxShape->getBoxGeometry( boxGeometry ) )
				{
					Uint32 baseVertexIdx = outVerts.Size();
					Uint32 baseIndiceIdx = outIndices.Size();

					Vector halfExtens = TO_VECTOR( boxGeometry.halfExtents );
					outVerts.Grow( 8 );
					outVerts[ baseVertexIdx+0 ] = Vector( halfExtens.X, halfExtens.Y, halfExtens.Z );
					outVerts[ baseVertexIdx+1 ] = Vector( halfExtens.X, -halfExtens.Y, halfExtens.Z );
					outVerts[ baseVertexIdx+2 ] = Vector( -halfExtens.X, halfExtens.Y, halfExtens.Z );
					outVerts[ baseVertexIdx+3 ] = Vector( -halfExtens.X, -halfExtens.Y, halfExtens.Z );
					outVerts[ baseVertexIdx+4 ] = Vector( halfExtens.X, halfExtens.Y, -halfExtens.Z );
					outVerts[ baseVertexIdx+5 ] = Vector( halfExtens.X, -halfExtens.Y, -halfExtens.Z );
					outVerts[ baseVertexIdx+6 ] = Vector( -halfExtens.X, halfExtens.Y, -halfExtens.Z );
					outVerts[ baseVertexIdx+7 ] = Vector( -halfExtens.X, -halfExtens.Y, -halfExtens.Z );

					Uint32 baseInd = baseIndiceIdx;
					outIndices.Grow( 12*3 );
					// walls
					for ( Uint32 i = 0; i < 4; ++i )
					{
						Uint32 ni = (i+1)%4;
						// tri1
						outIndices[ baseInd + 0 ] = baseVertexIdx + i;
						outIndices[ baseInd + 1 ] = baseVertexIdx + ni;
						outIndices[ baseInd + 2 ] = baseVertexIdx + i+4;
						// tri2
						outIndices[ baseInd + 3 ] = baseVertexIdx + ni+4;
						outIndices[ baseInd + 4 ] = baseVertexIdx + i+4;
						outIndices[ baseInd + 5 ] = baseVertexIdx + ni;

						baseInd += 6;
					}
					// ceil & floor
					for ( Uint32 i =0;i < 2; ++i )
					{
						Uint32 floorind = baseVertexIdx + i*4;
						// tri1
						outIndices[ baseInd + 0 ] = floorind+0;
						outIndices[ baseInd + 1 ] = floorind+1;
						outIndices[ baseInd + 2 ] = floorind+2;

						// tri1
						outIndices[ baseInd + 3 ] = floorind+2;
						outIndices[ baseInd + 4 ] = floorind+3;
						outIndices[ baseInd + 5 ] = floorind+0;

						baseInd += 6;
					}
					ASSERT( baseInd == outIndices.Size() );

					initialized = true;
				}
			}
			
			break;
		case PxGeometryType::eCONVEXMESH:
			{
				PxConvexMeshGeometry convexGeometry;
				if ( pxShape->getConvexMeshGeometry( convexGeometry ) && convexGeometry.convexMesh )
				{
					Uint32 baseVertexIdx = outVerts.Size();
					Uint32 baseIndiceIdx = outIndices.Size();

					const auto& transform = convexGeometry.scale;
					const auto* mesh = convexGeometry.convexMesh;

					// copy vertices
					Uint32 verticesCount = mesh->getNbVertices();
					outVerts.Grow( verticesCount );
					const auto* verticesList = mesh->getVertices();
					for ( Uint32 i = 0; i < verticesCount; ++i )
					{
						outVerts[ baseVertexIdx+i ] = TO_VECTOR( transform.transform( verticesList[ i ] ));
					}

					// convert polygond to triangle indices

					// calculate indices count
					Uint32 triCount = 0;
					Uint32 polyCount = mesh->getNbPolygons();
					for ( Uint32 i = 0; i < polyCount; ++i )
					{
						PxHullPolygon polygon;
						mesh->getPolygonData( i, polygon );
						ASSERT( polygon.mNbVerts > 2, TXT( "Assume each polygon consist of minimum one triangle!") );
						triCount += polygon.mNbVerts - 2;
					}

					outIndices.Grow( triCount*3 );

					const physx::PxU8* pxIndices = mesh->getIndexBuffer();

					// fillup indices
					Uint32 baseInd = baseIndiceIdx;

					for ( Uint32 i = 0; i < polyCount; ++i )
					{
						PxHullPolygon polygon;
						mesh->getPolygonData( i, polygon );

						Uint32 polyTris = polygon.mNbVerts - 1;
						for ( Uint32 tri = 1; tri < polyTris; ++tri )
						{
							outIndices[ baseInd+0 ] = baseVertexIdx + pxIndices[ polygon.mIndexBase ];
							outIndices[ baseInd+1 ] = baseVertexIdx + pxIndices[ polygon.mIndexBase + tri + 1 ];
							outIndices[ baseInd+2 ] = baseVertexIdx + pxIndices[ polygon.mIndexBase + tri ];
							baseInd += 3;
						}
					}

					ASSERT( baseInd == outIndices.Size() );

					initialized = true;
				}
			}

			break;
		case PxGeometryType::eTRIANGLEMESH:
			{
				PxTriangleMeshGeometry triGeometry;
				if ( pxShape->getTriangleMeshGeometry( triGeometry ) && triGeometry.triangleMesh )
				{
					Uint32 baseVertexIdx = outVerts.Size();
					Uint32 baseIndiceIdx = outIndices.Size();

					const auto& transform = triGeometry.scale;
					const auto* mesh = triGeometry.triangleMesh;

					// copy vertices
					Uint32 verticesCount = mesh->getNbVertices();
					outVerts.Grow( verticesCount );
					auto* triMeshVertOutput = &outVerts[ baseVertexIdx ];

					const auto* verticesList = mesh->getVertices();
					for ( Uint32 i = 0; i < verticesCount; ++i )
					{
						triMeshVertOutput[ i ] = TO_VECTOR( transform.transform( verticesList[ i ] ));
					}

					// copy indices
					Uint32 triCount = mesh->getNbTriangles();

					outIndices.Grow( triCount*3 );
					auto* triMeshIndOutput = &outIndices[ baseIndiceIdx ];

					if ( mesh->getTriangleMeshFlags() & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES )
					{
						const Uint16* indices = static_cast< const Uint16* >( mesh->getTriangles() );
						for ( Uint32 i = 0; i < triCount*3; ++i )
						{
							triMeshIndOutput[ i ] = indices[ i ]+baseVertexIdx;
						}
					}
					else
					{
						Red::MemoryCopy( outIndices.TypedData(), mesh->getTriangles(), outIndices.DataSize() );
					}

					initialized = true;
				}
			}

			break;
		default:
		case PxGeometryType::eHEIGHTFIELD:
		case PxGeometryType::eSPHERE:
		case PxGeometryType::ePLANE:
		case PxGeometryType::eCAPSULE:
			RED_LOG_ERROR( CNAME( PathLib ), TXT("Obstacle system don't support destructable shape geometry type(%d)!\n"), pxShape->getGeometryType() );
			break;
		}
	}

	if ( initialized )
	{
		Box& shapeBBox = shapeData.m_shapeBBox;
		shapeBBox.Clear();
		Matrix localToWorld;
		destructionSystem->GetLocalToWorld( localToWorld );
		for ( Uint32 i = 0, n = outVerts.Size(); i != n; ++i )
		{
			outVerts[ i ] = localToWorld.TransformPoint( outVerts[ i ] );
			shapeBBox.AddPoint( outVerts[ i ] );
		}
		m_bbox.AddBox( shapeBBox );
		m_shapes.Resize( 1 );
		m_shapes[ 0 ] = new CObstacleIndicedVertsGeometryData( Move( shapeData ) );
	}
	

#endif
}

#ifndef NO_OBSTACLE_MESH_DATA
Bool CObstacleSpawnData::TryProcessNavObstacleData(CMesh* mesh, Matrix localToWorld)
{
	const Float ALLOWED_ROTATION_ANGLE = 1.5f;

	const CNavigationObstacle& navigationObstacle = mesh->GetNavigationObstacle();
	if ( !navigationObstacle.IsEmpty() )
	{
		EulerAngles angles = localToWorld.ToEulerAnglesFull();

		if ( Abs( angles.Pitch ) < ALLOWED_ROTATION_ANGLE && Abs( angles.Roll ) < ALLOWED_ROTATION_ANGLE )
		{
			const auto& shapes = navigationObstacle.GetShapes();
			m_shapes.Resize( shapes.Size() );

			for( Uint32 i = 0, n = shapes.Size(); i != n; ++i )
			{
				const auto& shapeData = shapes[ i ];
				const auto& verts = shapeData.m_verts;

				CObstacleConvexOccluderData* convexData = new CObstacleConvexOccluderData();
				convexData->m_occluder.Resize( verts.Size() );
				Box bbox( Box::RESET_STATE );
				for ( Uint32 j = 0, vertsCount = verts.Size(); j != vertsCount; ++j )
				{
					Vector v = localToWorld.TransformPoint( Vector( verts[ j ].X, verts[ j ].Y, 0.f ) );
					convexData->m_occluder[ j ].Set( v.X, v.Y );

					bbox.Min.X = Min( bbox.Min.X, v.X );
					bbox.Max.X = Max( bbox.Max.X, v.X );
					bbox.Min.Y = Min( bbox.Min.Y, v.Y );
					bbox.Max.Y = Max( bbox.Max.Y, v.Y );
				}

				bbox.Min.Z = localToWorld.TransformPoint( shapeData.m_bbox.Min ).Z;
				bbox.Max.Z = localToWorld.TransformPoint( shapeData.m_bbox.Max ).Z;

				convexData->m_shapeBBox = bbox;

				m_bbox.AddBox( bbox );

				m_shapes[ i ] = convexData;
			}
			return true;;
		}
	}
	return false;
}
#endif

#ifndef NO_RESOURCE_IMPORT
void CObstacleSpawnData::TryProcessCollisionCache(CMesh* mesh, Matrix localToWorld)
{
	const CCollisionMesh* collisionMesh = mesh->GetCollisionMesh();
	if ( collisionMesh )
	{
		const auto& shapeList = collisionMesh->GetShapes();

		if ( !shapeList.Empty() )
		{
			CObstacleIndicedVertsGeometryData* shapeData = new CObstacleIndicedVertsGeometryData();
			shapeData->m_shapeBBox.Clear();

			TDynArray< Vector > vertices;
			TDynArray< Uint32 > indices;

			// create indiced geometry from all shapes
			for ( Uint32 i = 0, n = shapeList.Size(); i != n; ++i )
			{
				vertices.ClearFast();
				indices.ClearFast();

				shapeList[ i ]->GetShape( localToWorld, vertices, indices );

				if ( indices.Empty() )
				{
					continue;
				}

				for ( Uint32 vert = 0, vertCount = vertices.Size(); vert != vertCount; ++vert )
				{
					shapeData->m_shapeBBox.AddPoint( vertices[ vert ] );
				}

				Uint32 baseVertIdx = shapeData->m_verts.Size();
				Uint32 baseIndices = shapeData->m_indices.Size();

				// copy indices (shifted)
				{
					shapeData->m_indices.Grow( indices.Size() );
					auto* indexPtr = &shapeData->m_indices[ baseIndices ];
					for( Uint32 i = 0, n = indices.Size(); i != n; ++i )
					{
						(*indexPtr++) = indices[ i ] + baseVertIdx;
					}
				}

				// copy vertexes (raw copy)
				{
					shapeData->m_verts.Grow( vertices.Size() );
					Red::System::MemoryCopy( &shapeData->m_verts[ baseVertIdx ], vertices.Data(), vertices.DataSize() );
				}
			}

			m_bbox.AddBox( shapeData->m_shapeBBox );
			m_shapes.Resize( 1 );
			m_shapes[ 0 ] = shapeData;
		}
	}
}
#endif

void CObstacleSpawnData::InitializeTree( const SFoliageInstance & foliageInstance, TDynArray< Sphere >& collisionShapes )
{
	m_mapping.m_entityGuid = CGUID::ZERO;
	m_mapping.m_componentHash = 0;
	m_collisionType = PLC_Static;

	Box overalBox( Box::RESET_STATE );

	Float scale = foliageInstance.GetScale();

	// Don't ask why. I have no clue. It works...
	Matrix baseTransform;
	Vector instanceQuat = foliageInstance.GetQuaterion();
	RedQuaternion q;
	q.SetMul( RedQuaternion( 0.f, 0.f, -instanceQuat.Z, instanceQuat.W ), RedQuaternion( RedVector4::EZ, -M_PI_HALF ) );
	
	baseTransform.BuildFromQuaternion( reinterpret_cast< Vector& >( q ) );
	baseTransform.SetTranslation( foliageInstance.GetPosition() );
	

	// WARNING: heavy ctrl+c ctrl+v from CPhysicsTileWrapper::AddFoliageBody ahead

	// prepare geometry
	for ( Uint32 i = 0, n = collisionShapes.Size(); i+1 < n; i += 2 )
	{
		Vector shapePosition1 = collisionShapes[ i ].GetCenter() * scale;
		Vector shapePosition2 = collisionShapes[ i + 1 ].GetCenter() * scale;
		Float radius = collisionShapes[ i ].GetRadius();

		Vector shapeWorldPos1 = baseTransform.TransformPoint( shapePosition1 );
		Vector shapeWorldPos2 = baseTransform.TransformPoint( shapePosition2 );

		if (
			( shapeWorldPos1.AsVector2() - shapeWorldPos2.AsVector2() ).SquareMag() < 0.1f*0.1f
			)
		{
			// threat a tree just as cylinder
			CObstacleCylinderData* cylinder = new CObstacleCylinderData( shapeWorldPos1.AsVector2(), Min( shapeWorldPos1.Z, shapeWorldPos2.Z ) - radius, Max( shapeWorldPos1.Z, shapeWorldPos2.Z ) + radius, radius );
			m_shapes.PushBack( cylinder );
		}
		else
		{
			CObstacleIndicedVertsGeometryData* shapeData = new CObstacleIndicedVertsGeometryData();

			Vector dir = shapePosition2 - shapePosition1;
			Float height = dir.Normalize3();

			auto& verts = shapeData->m_verts;
			auto& indices = shapeData->m_indices;

			Matrix localTransform;
			localTransform.BuildFromDirectionVector( dir );
			localTransform.SetTranslation( (shapePosition2 + shapePosition1) * 0.5f );

			Matrix shapeTransform( localTransform * baseTransform );

			Float halfHeight = height / 2.f;

			Box bbox( Box::RESET_STATE );
			// collect full geometry and create convex occluder
			CollisionShapeBuilder::BuildBox( Box( Vector( -radius, -halfHeight, -radius ), Vector( radius, halfHeight, radius ) ), verts, indices );
			for( auto it = verts.Begin(), end = verts.End(); it != end; ++it )
			{
				(*it) = shapeTransform.TransformPoint( *it );
				bbox.AddPoint( *it );
			}

			shapeData->m_shapeBBox = bbox;
			overalBox.AddBox( bbox );

			m_shapes.PushBack( shapeData );
		}


	}
	m_bbox = overalBox;
}

Bool CObstacleSpawnData::Initialize( CComponent* component, CPathLibWorld& pathlib, CLayerGroup* layerGroup )
{
	if ( !layerGroup )
	{
		CLayer* layer = component->GetEntity()->GetLayer();
		if ( layer )
		{
			CLayerInfo* layerInfo = layer->GetLayerInfo();
			if ( layerInfo )
			{
				layerGroup = layerInfo->GetLayerGroup();
			}
		}
		
	}
	m_mapping = SComponentMapping( component );
	m_layerMapping = layerGroup ? SLayerMapping( layerGroup ) : SLayerMapping( Uint64( 0 ) );
	m_isOnHiddeableLayer = true; //layerGroup ? layerGroup->CanBeHidden() : false;
	m_isLayerBasedGrouping = component->AsPathLibComponent()->IsLayerBasedGrouping();

	if ( component->IsA< CStaticMeshComponent >() )
	{
		Initialize( static_cast< CStaticMeshComponent* >( component ), pathlib );
		return true;
	}
	else if ( component->IsA< CDeniedAreaComponent >() )
	{
		Initialize( static_cast< CDeniedAreaComponent* >( component ), pathlib );
		return true;
	}
#ifdef USE_APEX
	else if ( component->IsA< CDestructionSystemComponent >() )
	{
		Initialize( static_cast< CDestructionSystemComponent* >( component ), pathlib );
		return true;
	}
#endif
	else if ( component->IsA< CDestructionComponent >() )
	{
		Initialize( static_cast< CDestructionComponent* >( component ), pathlib );
		return true;
	}

	return false;
}

CObstacleSpawnData::CObstacleSpawnData( CObstacleSpawnData&& data )
	: m_shapes( Move( data.m_shapes ) )
	, m_collisionType( data.m_collisionType )
	, m_bbox( data.m_bbox )
	, m_mapping( data.m_mapping )
	, m_layerMapping( data.m_layerMapping )
	, m_isOnHiddeableLayer( data.m_isOnHiddeableLayer )
	, m_isLayerBasedGrouping( data.m_isLayerBasedGrouping )
{

}
CObstacleSpawnData::~CObstacleSpawnData()
{
	for( auto it = m_shapes.Begin(), end = m_shapes.End(); it != end; ++it )
	{
		delete *it;
	}
	m_shapes.ClearFast();
}
//
//void CObstacleSpawnContext::StartOptimization()
//{
//	ASSERT( !m_dataOptimized, TXT("Multiple calls to obstacle optimization procedures") );
//	m_optimizedData = m_baseData;
//	m_dataOptimized = true;
//}


////////////////////////////////////////////////////////////////////////////
// CMetalinkConfigurationCommon
////////////////////////////////////////////////////////////////////////////

CMetalinkConfigurationCommon::~CMetalinkConfigurationCommon()
{
}

Bool CMetalinkConfigurationCommon::IsEmpty()
{
	if ( m_nodes.Empty() )
	{
		return true;
	}

	if ( m_connections.Empty() )
	{
		Bool isEmpty = true;
		// look for connectors - that would render that configuration non-empty
		for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
		{
			if ( (*it).m_processingFlags & NODEPROCESSING_POSSIBLE_CONNECTOR )
			{
				isEmpty = false;
				break;
			}
		}

		return isEmpty;
	}

	return false;
}

void CMetalinkConfigurationCommon::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	writer.SmartPut( m_nodes );
	writer.SmartPut( m_connections );
}

Bool CMetalinkConfigurationCommon::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.SmartGet( m_nodes ) )
	{
		return false;
	}
	if ( !reader.SmartGet( m_connections ) )
	{
		return false;
	}
	return true;
}
void CMetalinkConfigurationCommon::ComputeBBox()
{
	m_bbox.Clear();

	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		m_bbox.AddPoint( it->m_pos );
	}
}
void CMetalinkConfigurationCommon::Clear()
{
	m_nodes.ClearFast();
	m_connections.ClearFast();
	
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkConfiguration
////////////////////////////////////////////////////////////////////////////
CMetalinkConfiguration::~CMetalinkConfiguration()
{
	if ( m_internalObstacle )
	{
		delete m_internalObstacle;
	}
}
void CMetalinkConfiguration::Clear()
{
	Super::Clear();

	if ( m_internalObstacle )
	{
		delete m_internalObstacle;
		m_internalObstacle = NULL;
	}
}

void CMetalinkConfiguration::ComputeBBox()
{
	Super::ComputeBBox();

	if ( m_internalObstacle && !m_internalObstacle->m_shapes.Empty() )
	{
		m_bbox.AddBox( m_internalObstacle->m_bbox );
	}
}


////////////////////////////////////////////////////////////////////////////
// CMetalinkGraph
////////////////////////////////////////////////////////////////////////////
void CMetalinkGraph::CopyGraphFrom( const CMetalinkConfigurationCommon& data )
{
	Super::operator=( data );
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkComputedData
////////////////////////////////////////////////////////////////////////////
CMetalinkComputedData::~CMetalinkComputedData()
{
	if ( m_internalShape )
	{
		delete m_internalShape;
	}
}

void CMetalinkComputedData::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	Bool hasInternalObstacle = m_internalShape != NULL;
	writer.Put( hasInternalObstacle );
	if( hasInternalObstacle )
	{
		m_internalShape->WriteToBuffer( writer );
	}
}
Bool CMetalinkComputedData::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	Bool hasInternalObstacle = m_internalShape != NULL;
	if ( !reader.Get( hasInternalObstacle ) )
	{
		return false;
	}
	if ( hasInternalObstacle )
	{
		m_internalShape = CObstacleShape::NewFromBuffer( reader );
		if ( !m_internalShape )
		{
			return false;
		}
	}

	return true;
}

void CMetalinkComputedData::CopyGraphFrom( const CMetalinkConfigurationCommon& data )
{
	Super::operator=( data );
}

void CMetalinkComputedData::Clear()
{
	Super::Clear();

	if ( m_internalShape )
	{
		delete m_internalShape;
		m_internalShape = NULL;
	}
}

void CMetalinkComputedData::ComputeBBox()
{
	Super::ComputeBBox();

	if ( m_internalShape )
	{
		m_bbox.AddBox( Box( m_internalShape->GetBBoxMin(), m_internalShape->GetBBoxMax() ) );
	}
}


};			// namespace PathLib
