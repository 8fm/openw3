/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorDataRawData.h"
#include "sectorDataObjects.h"
#include "sectorDataMerged.h"
#include "sectorDataResourceLoader.h"

#include "sectorDataGenericRuntime.h"
#include "sectorDataParticlesRuntime.h"
#include "sectorDataRigidMeshRuntime.h"

namespace SectorData
{
	// Check sizes of the data buffers
#define CHECK_PACKED_SIZE( expression ) static_assert( expression, #expression )
	CHECK_PACKED_SIZE( sizeof(PackedMatrix) == 48 );
	CHECK_PACKED_SIZE( sizeof(PackedBase) == 56 );
	CHECK_PACKED_SIZE( sizeof(PackedMesh) == 64 );
	CHECK_PACKED_SIZE( sizeof(PackedCollision) == 76 );	
	CHECK_PACKED_SIZE( sizeof(PackedDimmer) == 68 );
	CHECK_PACKED_SIZE( sizeof(PackedDecal) == 76 );
	CHECK_PACKED_SIZE( sizeof(PackedLight) == 108 );
	CHECK_PACKED_SIZE( sizeof(PackedSpotLight) == 136 );
	CHECK_PACKED_SIZE( sizeof(PackedRigidBody) == 96 );
#undef CHECK_PACKED_SIZE	
}

#ifndef RED_FINAL_BUILD
Red::Threads::AtomicOps::TAtomic32 GNumSectorDataObjects = 0;

Uint32 GetNumSectorDataObjects()
{
	return GNumSectorDataObjects;
}
#else
Uint32 GetNumSectorDataObjects()
{
	return 0;
}
#endif

ISectorDataObject::ISectorDataObject()
{
#ifndef RED_FINAL_BUILD
	Red::Threads::AtomicOps::ExchangeAdd32( &GNumSectorDataObjects, 1 );
#endif
}

ISectorDataObject::~ISectorDataObject()
{
#ifndef RED_FINAL_BUILD
	Red::Threads::AtomicOps::ExchangeAdd32( &GNumSectorDataObjects, -1 );
#endif
}

CSectorDataObjectWrapper::CSectorDataObjectWrapper( const class CSectorDataMerged* sourceData )
	: m_sourceData( sourceData )
{
}

CSectorDataObjectWrapper::~CSectorDataObjectWrapper()
{
}

#define HANDLE_OBJECT_TYPE( packedType, packedClass, runtimeClass )									\
		case packedType:																			\
		{																							\
			auto ret = new runtimeClass;															\
			ret->Setup( object, *object.GetData< packedClass >( m_sourceData->m_dataStream ) );		\
			return ret;																				\
		}

class ISectorDataObject* CSectorDataObjectWrapper::CreateObjectWrapper( const Uint32 globalMergedObjectID ) const
{
	if ( !globalMergedObjectID )
		return 0;

	// TODO: this is a manual RTTI system, technically we could connect our RTTI system here but ATM it's not necessary, maybe for CP
	const auto& object = m_sourceData->m_objects[ globalMergedObjectID ];
	switch ( object.m_type )
	{
		HANDLE_OBJECT_TYPE( SectorData::eObject_Mesh, SectorData::PackedMesh, CSectorDataObjectMesh );
		HANDLE_OBJECT_TYPE( SectorData::eObject_Collision, SectorData::PackedCollision, CSectorDataObjectCollision );
		HANDLE_OBJECT_TYPE( SectorData::eObject_Decal, SectorData::PackedDecal, CSectorDataObjectDecal );
		HANDLE_OBJECT_TYPE( SectorData::eObject_Dimmer, SectorData::PackedDimmer, CSectorDataObjectDimmer );
		HANDLE_OBJECT_TYPE( SectorData::eObject_PointLight, SectorData::PackedLight, CSectorDataObjectPointLight );
		HANDLE_OBJECT_TYPE( SectorData::eObject_SpotLight, SectorData::PackedSpotLight, CSectorDataObjectSpotLight );
		HANDLE_OBJECT_TYPE( SectorData::eObject_Particles, SectorData::PackedParticles, CSectorDataObjectParticles );
		HANDLE_OBJECT_TYPE( SectorData::eObject_RigidBody, SectorData::PackedRigidBody, CSectorDataObjectRigidBody );
	}

	RED_FATAL( "Unknown sector data object type %d", object.m_type );
	return nullptr;
}

CSectorDataResourceRef::CSectorDataResourceRef()
	: m_resources( nullptr )
{
}

CSectorDataResourceRef::~CSectorDataResourceRef()
{
	Release();
}

void CSectorDataResourceRef::Bind( class CSectorDataResourceLoader* loader, SectorData::PackedResourceRef ref )
{
	Release();

	if ( loader && ref.m_resourceIndex )
	{
		m_resources = loader;
		m_ref = ref.m_resourceIndex;
	}
}

void CSectorDataResourceRef::Release()
{
	if ( m_resources && m_ref.m_resourceIndex )
	{
		m_resources->Release( m_ref.m_resourceIndex );
	}

	m_resources = nullptr;
	m_ref = SectorData::PackedResourceRef();
}
