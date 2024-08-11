#pragma once

#ifndef NO_EDITOR

#include "../core/hashmap.h"
#include "sectorData.h"

/// Helper class
class CSectorDataBuilder
{
public:
	CSectorDataBuilder( CSectorData* data );
	~CSectorDataBuilder();

	// Try to extract component, returns true if successful
	Bool ExtractComponent( const class CComponent* component );

	// Get used resources
	void GetUsedResources( TDynArray< String >& outResources ) const;

private:
	// Component extraction
	Bool ExtractMeshComponent( const class CMeshComponent* component );
	Bool ExtractDimmerComponent( const class CDimmerComponent* component );
	Bool ExtractDecalComponent( const class CDecalComponent* component );
	Bool ExtractPointLightComponent( const class CPointLightComponent* component );
	Bool ExtractSpotLightComponent( const class CSpotLightComponent* component );
	Bool ExtractRigidMeshComponent( const class CRigidMeshComponent* component );
	Bool ExtractParticleComponent( const class CParticleComponent* component );

	// Add objects to data
	void AddMesh( const SectorData::PackedMesh& mesh, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddCollision( const SectorData::PackedCollision& mesh, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddDecal( const SectorData::PackedDecal& decal, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddDimmer( const SectorData::PackedDimmer& dimmer, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddLight( const SectorData::PackedLight& light, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddSpotLight( const SectorData::PackedSpotLight& light, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddRigidBody( const SectorData::PackedRigidBody& body, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );
	void AddParticles( const SectorData::PackedParticles& particles, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags = 0 );

	// raw interface
	void AddDataObject( const SectorData::EObjectType type, const Uint8 internalObjectFlags, const Uint32 internalDataOffset, const Vector& streamingPos, const Float radius );
	Uint32 AddDataBlock( const void* data, const Uint32 dataSize ); // alignment: implicit 4

	// Map resource
	SectorData::PackedResourceRef MapResource( const String& depotPath, const Box& box );
	SectorData::PackedResourceRef MapResource( const class CResource* resource );

	// Mapping tables
	THashMap< Uint64, Uint32 >		m_resourceMap;
	TDynArray< String >				m_resources;

	// Target data
	CSectorData*					m_data;
};

#endif