/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/atomicSharedPtr.h"
#include "../core/atomicWeakPtr.h"
#include "../core/uniqueBuffer.h"

static const Uint32 c_maxGeometrySize = 48;

struct SCachedGeometry
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

public:

	Matrix					m_pose;
	CName					m_physicalSingleMaterial;
	TDynArray< CName >		m_physicalMultiMaterials;
	Float					m_densityScaler;
	Uint16					m_assetId;		// for Apex...
	char					m_geometryType;
private:
	Bool					m_contructed; // This is because of Apex. It does not construct geometry while compiling it.
public:

	SCachedGeometry();
	~SCachedGeometry();

	SCachedGeometry( SCachedGeometry && other );
	SCachedGeometry & operator=( SCachedGeometry && other );

	void Initialize( void * buffer );

	template< typename T >
	void SetGeometry( char geometryType, const T & geometry );
	
	const void * GetGeometry() const;
	void * GetGeometry();
	
	void* AllocateCompiledData( const Uint32 size );
	void* GetCompiledData() const;
	Uint32 GetCompiledDataSize() const;
	Box2 GetBoundingVolume() const;

	void Serialize( IFile& file );

private:
	
	SCachedGeometry( const SCachedGeometry& );
	SCachedGeometry& operator=( const SCachedGeometry& );

	char m_geometryBuffer[ c_maxGeometrySize ];
	void * m_compiledDataProxy;
	Uint32 m_compiledDataSize;
};

/// Compiled collision mesh, a single PhysX object with 
/// one collision shape compiled from all the objects.
class CCompiledCollision
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

public:
	CCompiledCollision() {}	
	~CCompiledCollision();

    //! Get the root collision shape
	typedef TDynArray< SCachedGeometry, MC_CompiledCollision, MemoryPool_Physics >	TCompiledCollisions;
    RED_INLINE const TCompiledCollisions& GetGeometries() const { return m_geometries; }
	RED_INLINE TCompiledCollisions& GetGeometries() { return m_geometries; }

	Box2 GetBoundingArea();

	//! Get occlusion attenuation value
	RED_INLINE Float GetOcclusionAttenuation() const { return m_occlusionAttenuation; }

	//! Get occlusion limit
	RED_INLINE Float GetOcclusionDiagonalLimit() const { return m_occlusionDiagonalLimit; }

	//! Get occlusion limit
	RED_INLINE Int32 GetSwimmingRotationAxis() const { return m_swimmingRotationAxis; }

    //! Serialize to/from cache file
    void SerializeToCache( IFile& file );

	SCachedGeometry& InsertGeometry();

#ifndef NO_EDITOR
	Float GetMassFromResource() const;
#endif

	void SetOcclusionAttenuation( Float occlusionAttenuation );
	void SetOcclusionDiagonalLimit( Float occlusionDiagonalLimit );
	void SetSwimmingRotationAxis( Int32 swimmingRotationAxis );

private:
	
	TCompiledCollisions						m_geometries;

	Float									m_occlusionAttenuation;
	Float									m_occlusionDiagonalLimit;
	Int32									m_swimmingRotationAxis;
};

typedef Red::TAtomicSharedPtr< CCompiledCollision > CompiledCollisionPtr;
typedef Red::TAtomicWeakPtr< CCompiledCollision > CompiledCollisionWeakPtr;

template< typename T >
void SCachedGeometry::SetGeometry( char geometryType, const T & geometry )
{
	Red::System::MemoryCopy( m_geometryBuffer, &geometry, sizeof( T ) );
	m_geometryType = geometryType;
	m_contructed = true;
}

