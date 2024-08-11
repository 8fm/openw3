/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataMerged.h"

#include "../core/streamingGrid.h"
#include "../core/streamingGridHelpers.h"
#include "../core/configVar.h"

namespace Config
{
	TConfigVar<Float> cvStreamingGenericOffset( "Streaming/Sector", "GenericOffset", 45.0f, 0 );						//!< Streaming offset distance in meters (stream in Xm before autohide distance kicks in to allow dissolve to work)
	TConfigVar<Float> cvStreamingMeshOffset( "Streaming/Sector", "MeshOffset", 10.0f, 0 );								//!< Streaming offset distance in meters (stream in Xm before autohide distance kicks in to allow dissolve to work)
	TConfigVar<Float> cvStreamingCollisionPrefetchRadius( "Streaming/Sector", "CollisionPrefetchRadius", 50.0f, 0 );	//!< Prefetch all collisions within this radius
}

IMPLEMENT_ENGINE_CLASS( CSectorDataMerged );

TDynArray< Uint8, MC_SectorData > GSectorDataPreallocatedBuffer;
TDynArray< SectorData::PackedObject, MC_SectorData > GSectorDataPreallocatedObjectsBuffer;


CSectorDataMerged::CSectorDataMerged( const Float worldSize, const Bool hasPrefetchData )
	: m_worldSize( worldSize )
	, m_hasPrefetchData( hasPrefetchData )
{
	// NOTE: all of the experimental value are taken from Novigrad

	// initialize streaming grid
	const Uint32 gridLevels = 8; // experimental (measured) value that works best for Novigrad
	const Uint32 gridBuckets = SectorData::MAX_OBJECTS / 5;  // actual value for Novigrad (+EP1) worst case, 75% grid cell occupancy
	m_grid = new CStreamingGrid( gridLevels, gridBuckets );

	// create quantizer
	m_quantizer = new CStreamingPositionQuantizer( m_worldSize );

	// resource mapping
	m_resources.Reserve( 4096 );
	m_resourceMap.Reserve( 4096 );

	// preallocate arrays, value again based on W3 Novigrad worst case
	m_dataStream = std::move( GSectorDataPreallocatedBuffer );
	m_objects = std::move( GSectorDataPreallocatedObjectsBuffer );

	// reset the buffer
	m_dataStream.ClearFast();
	m_objects.ClearFast();

	// Object zero
	m_objects.PushBack( SectorData::PackedObject() );
	Red::MemoryZero( &m_objects[0], sizeof(SectorData::PackedObject) );
}

CSectorDataMerged::~CSectorDataMerged()
{
	if ( m_grid )
	{
		delete m_grid;
		m_grid = nullptr;
	}

	if ( m_quantizer )
	{
		delete m_quantizer;
		m_quantizer = nullptr;
	}

	GSectorDataPreallocatedBuffer = std::move( m_dataStream );
	GSectorDataPreallocatedObjectsBuffer = std::move( m_objects );
}

const Float CSectorDataMerged::CalcRadius( const SectorData::PackedObject& object ) const
{
	// special case for collision
	if ( object.m_type == SectorData::eObject_Collision )
	{
		// use absolute offset
		const Float radius = Config::cvStreamingCollisionPrefetchRadius.Get();

		const auto* collisionData = object.GetData< SectorData::PackedCollision >( m_dataStream );
		const auto box = GetResourceLocalBounds( collisionData->m_mesh.m_resourceIndex );
		return box.CalcExtents().Mag3() + radius;
	}

	// special case for proxy meshes if and only if we have prefetch data
	// this lowers the memory needed for the level
	if ( object.m_type == SectorData::eObject_Mesh && m_hasPrefetchData )
	{
		const Float objectRadius = (Float) object.m_radius;
		return objectRadius + Config::cvStreamingMeshOffset.Get();
	}

	// generic radius
	const Float objectRadius = (Float) object.m_radius;
	return objectRadius + Config::cvStreamingGenericOffset.Get();
}

Uint32 CSectorDataMerged::AppendSectorData( const CSectorData* data )
{
	PC_SCOPE( AppendSectorData );

	// ensure there's space in the internal buffers
	if ( m_dataStream.Size() + data->m_dataStream.Size() > m_dataStream.Capacity() )
	{
		ERR_ENGINE( TXT("!!! NOT ENOUGH SPACE IN STREAMING SYSTEM TO ACCOMODATE NEW DATA !!!") );
		return 0;
	}

	// allocate sector ID
	const Uint32 sectorID = m_sectorFreeIndices.Alloc();
	if ( 0 == sectorID )
		return 0;
	
	// map resources to global table
	TStaticArray< Uint32, 4096 > mappedResources;
	{
		mappedResources.Resize( data->m_resources.Size() );
		mappedResources[0] = 0; // NULL always map to NULL

		for ( Uint32 id=1; id<data->m_resources.Size(); ++id )
		{
			const CSectorData::PackedResource& res = data->m_resources[id];

			// lookup in map
			Uint32 currentId = 0;
			if ( m_resourceMap.Find( res.m_pathHash, currentId ) )
			{
				// already mapped
				mappedResources[id] = currentId;
			}
			else
			{
				// allocate new resource
				mappedResources[id] = m_resources.Size();
				m_resources.PushBack( res );
				m_resourceMap.Insert( res.m_pathHash, mappedResources[id] );
			}
		}
	}

	// merge data streams, remember where it was added so we can reindex the objects
	// use the remapped resources
	// TODO: make it less manual :P
	// TODO: in case of layer unload/reload we will need a defrag here evenually. Not for W3 though.
	const Uint32 baseDataOffset = m_dataStream.Size();
	{
		// copy data
		m_dataStream.Grow( data->m_dataStream.Size() );
		Red::MemoryCopy( m_dataStream.TypedData() + baseDataOffset, data->m_dataStream.TypedData(), data->m_dataStream.DataSize() );

		// adjust resource indices
		for ( /* will be modified*/ SectorData::PackedObject srcObject : data->m_objects ) 
		{
			// modify object offset
			srcObject.m_offset = baseDataOffset + srcObject.m_offset;
			srcObject.m_sectorID = sectorID;

			// modify resource indices, done like that because it's the fastest way to do it
			switch ( srcObject.m_type )
			{
				case SectorData::eObject_RigidBody:
				case SectorData::eObject_Mesh:
				{
					auto* data = srcObject.GetData< SectorData::PackedMesh >( m_dataStream );
					data->m_mesh.Remap( mappedResources );
					break;
				}

				case SectorData::eObject_Collision:
				{
					auto* data = srcObject.GetData< SectorData::PackedCollision >( m_dataStream );
					data->m_mesh.Remap( mappedResources );
					break;
				}

				case SectorData::eObject_Decal:
				{
					auto* data = srcObject.GetData< SectorData::PackedDecal >( m_dataStream );
					data->m_diffuseTexture.Remap( mappedResources );
					break;
				}

				case SectorData::eObject_Particles:
				{
					auto* data = srcObject.GetData< SectorData::PackedParticles >( m_dataStream );
					data->m_particleSystem.Remap( mappedResources );
					break;
				}

				case SectorData::eObject_SpotLight:
				{
					auto* data = srcObject.GetData< SectorData::PackedSpotLight >( m_dataStream );
					data->m_projectionTexture.Remap( mappedResources );
					break;
				}
			}

			// add to new object list - either at the end of at some previous index
			const Uint32 objectId = m_objectFreeIndices.Alloc();
			RED_FATAL_ASSERT( objectId <= m_objects.Size(), "New object index out of range" );
			if ( objectId == m_objects.Size() )
			{
				m_objects.PushBack( srcObject );
			}
			else
			{
				m_objects[ objectId ] = srcObject;
			}

			// register in grid
			const Float r = CalcRadius( srcObject );
			const auto q = m_quantizer->QuantizePositionAndRadius( Vector( srcObject.m_pos.X, srcObject.m_pos.Y, srcObject.m_pos.Z ), r );
			m_grid->Register( q.x, q.y, q.z, q.w, objectId );
		}
	}

	// return sector ID
	return sectorID;
}

void CSectorDataMerged::RemoveSectorData( const Uint32 id )
{
	PC_SCOPE( RemoveSectorData );

	if ( !id )
		return;

	// remove objects from that sector, NOTE: SLOW, does not happen during normal game
	for ( Uint32 id=0; id<m_objects.Size(); ++id )
	{
		auto& object = m_objects[id];

		if ( object.m_sectorID == id )
		{
			// unregister from grid
			const Float r = CalcRadius( object );
			const auto q = m_quantizer->QuantizePositionAndRadius( Vector( object.m_pos.X, object.m_pos.Y, object.m_pos.Z ), r );
			m_grid->Unregister( q.x, q.y, q.z, q.w, id );

			// release index
			m_objectFreeIndices.Release( id );
			object.m_sectorID = 0;
		}
	}
}

void CSectorDataMerged::CollectForPoint( const Vector& point, class CStreamingGridCollector& outCollector ) const
{
	PC_SCOPE( CollectForPoint );

	const auto q = m_quantizer->QuantizePosition( point );
	m_grid->CollectForPoint( q.x, q.y, q.z, outCollector );
}

void CSectorDataMerged::CollectForAreas( const Box* areas, const Uint32 numAreas, class CStreamingGridCollector& outCollector ) const
{
	PC_SCOPE( CollectForAreas );

	for ( Uint32 i=0; i<numAreas; ++i )
	{
		// quantize position
		const Box area = areas[i];
		const auto qMin = m_quantizer->QuantizePosition( area.Min );
		const auto qMax = m_quantizer->QuantizePosition( area.Max );

		// collect entries for given position
		m_grid->CollectForArea( qMin.x, qMin.y, qMax.x, qMax.y, outCollector );
	}
}

void CSectorDataMerged::CollectResourcesFromLocation( const Vector& point, const Float radius, TDynArray< SectorData::PackedResourceRef >& outResourceIndices ) const
{
	PC_SCOPE( CollectResourcesFromLocation );

	// prepare output table
	const Uint32 maxResources = m_resources.Size();
	outResourceIndices.Reserve(maxResources);

	// prepare bitmask
	TBitSet64< 65536 > usedResources; // resource index is 16-bit anyway
	usedResources.ClearAll();

	// check objects
	for ( const SectorData::PackedObject& obj : m_objects )
	{
		// in range ?
		const Float dist = ((Vector)obj.m_pos).DistanceSquaredTo( point );
		if ( dist > (radius*radius) )
			continue;

		// modify resource indices, done like that because it's the fastest way to do it
		switch ( obj.m_type )
		{
			case SectorData::eObject_RigidBody:
			case SectorData::eObject_Mesh:
			{
				const auto* data = obj.GetData< SectorData::PackedMesh >( m_dataStream );
				usedResources.Set( data->m_mesh.m_resourceIndex );
				break;
			}

			case SectorData::eObject_Collision:
			{
				const auto* data = obj.GetData< SectorData::PackedCollision >( m_dataStream );
				usedResources.Set( data->m_mesh.m_resourceIndex );
				break;
			}

			case SectorData::eObject_Decal:
			{
				const auto* data = obj.GetData< SectorData::PackedDecal >( m_dataStream );
				usedResources.Set( data->m_diffuseTexture.m_resourceIndex );
				break;
			}

			case SectorData::eObject_Particles:
			{
				const auto* data = obj.GetData< SectorData::PackedParticles >( m_dataStream );
				usedResources.Set( data->m_particleSystem.m_resourceIndex );
				break;
			}

			case SectorData::eObject_SpotLight:
			{
				const auto* data = obj.GetData< SectorData::PackedSpotLight >( m_dataStream );
				usedResources.Set( data->m_projectionTexture.m_resourceIndex );
				break;
			}
		}
	}

	// extract valid resource indices
	for ( Uint32 i=1; i<maxResources; ++i )
	{
		if ( usedResources.Get(i) )
		{
			SectorData::PackedResourceRef ret;
			ret.m_resourceIndex = (Uint16) i ;
			outResourceIndices.PushBack( ret );
		}
	}
}
