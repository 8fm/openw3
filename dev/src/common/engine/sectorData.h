#pragma once

#include "sectorDataRawData.h"

/// Sector data represents extracted and packed stuff from the 3D world that is light weight enough
class CSectorData : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CSectorData );

public:
	CSectorData();

	// Do we have any data ?
	Bool HasData() const;

	// Calculate memory size of the data
	const Uint32 CalcMemorySize() const;

	// Get number of resources in the sector
	const Uint32 GetNumResources() const;

	// Get path hash to the resource
	const Uint64 GetResourcePathHash( const Uint32 resourceIndex ) const;

	// Get resource local bounding box (if aviable, for some resources it returns unit box)
	const Box GetResourceLocalBounds( const Uint32 resourceIndex ) const;

	// Get resource index for given object, returns -1 if not known or invalid
	const Int32 GetResourceIndexForObject( const Uint32 objectIndex ) const;

	// Get number of objects in the sector
	RED_INLINE const Uint32 GetNumObjects() const { return m_objects.Size(); }

	// Get packed object data
	RED_INLINE const SectorData::PackedObject& GetObject( const Uint32 index ) const { return m_objects[ index ]; }

protected:
	// Save/Load
	virtual void OnSerialize( IFile& file ) override;

#pragma pack(push,4)
	// Data stream
	typedef TDynArray< Uint8, MC_SectorData >					TPackedDataStream;

	// Resource reference
	struct PackedResource
	{
		Float		m_box[6];					//!< Bounding box (local space), not always valid (for textures this is bullshit)
		Uint64		m_pathHash;					//!< We only store hash to the path (the same as the depot hash)
	};
#pragma pack(pop)

	// Packed data, NOTE: all object types are not packed in one big data stream
	// The object ID is not the data offset in that stream
	// The object type tells us how to interpret stuff
	typedef TDynArray< PackedResource, MC_SectorData >				TPackedResources;
	typedef TDynArray< SectorData::PackedObject, MC_SectorData >	TPackedObjects;

	TPackedResources		m_resources;
	TPackedDataStream		m_dataStream;
	TPackedObjects			m_objects;

	friend class CSectorDataBuilder;
	friend class CSectorDataMerged;
	friend class CSectorDataRuntime;
	friend class CSectorDataObjectWrapper;

	// tempshit - we need better "runtime" representation than just a component
	friend class CRigidMeshComponentCooked;
	friend class CParticleComponentCooked;
	friend class CFileDumpCommandlet;
};

BEGIN_CLASS_RTTI( CSectorData );
	PARENT_CLASS( ISerializable );
END_CLASS_RTTI();
