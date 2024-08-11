/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../physics/physicsIncludes.h"

#include "compiledCollision.h"

#include "../physics/physXEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../physics/PhysXStreams.h"
#include "NxModule.h"
#include "NxApexSDKCachedData.h"
#include "NxApexAsset.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

void ConstructBoxGeometry( void * geometryBuffer, void * inputBuffer, void *  )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxBoxGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." );  

	PxVec3* vectors = static_cast< PxVec3*>( inputBuffer );
	new ( geometryBuffer ) PxBoxGeometry( *vectors );
#endif
}

Box2 GetBoxBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxBoxGeometry* geometry = static_cast< const PxBoxGeometry* >( geometryBuffer );
	return Box2( -geometry->halfExtents.x, -geometry->halfExtents.y, geometry->halfExtents.x, geometry->halfExtents.y );
#else
	return Box2::ZERO;
#endif
}

void ConstructCapsuleGeometry( void * geometryBuffer, void * inputBuffer, void *  )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxCapsuleGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." );  

	float* sizes = static_cast< float* >( inputBuffer );
	new ( geometryBuffer ) PxCapsuleGeometry( sizes[ 0 ], sizes[ 1 ] );
#endif
}

Box2 GetCapsuleBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxCapsuleGeometry* geometry = static_cast< const PxCapsuleGeometry* >( geometryBuffer );
	return Box2( -geometry->radius, -geometry->radius, geometry->radius, geometry->radius );
#else
	return Box2::ZERO;
#endif
}

void ConstructSphereGeometry( void * geometryBuffer, void * inputBuffer, void *  )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxSphereGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." );  

	float* radius = static_cast< float* >( inputBuffer );
	new ( geometryBuffer ) PxSphereGeometry( *radius );
#endif
}

Box2 GetSphereBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxSphereGeometry* geometry = static_cast< const PxSphereGeometry* >( geometryBuffer );
	return Box2( -geometry->radius, -geometry->radius, geometry->radius, geometry->radius );
#else
	return Box2::ZERO;
#endif
}

void ConstructConvexMeshGeometry( void * geometryBuffer, void * inputBuffer, void *  )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxConvexMeshGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." );  

	MemoryReadBuffer mrb( static_cast< PxU8* >( inputBuffer ) );
	PxConvexMesh* convexMesh = GPhysXEngine->GetPxPhysics()->createConvexMesh( mrb );
	new ( geometryBuffer ) PxConvexMeshGeometry( convexMesh, PxMeshScale() );
#endif
}

void DestroyConvexMeshGeometry( void * geometryBuffer )
{
#ifdef USE_PHYSX
	PxConvexMeshGeometry * geometry = static_cast< PxConvexMeshGeometry* >( geometryBuffer );  
	geometry->convexMesh->release();
	geometry->~PxConvexMeshGeometry();
#endif
}

Box2 GetConvexMeshBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxConvexMeshGeometry* geometry = static_cast< const PxConvexMeshGeometry* >( geometryBuffer );
	PxBounds3 bounds = geometry->convexMesh->getLocalBounds();
	return Box2( bounds.minimum.x, bounds.minimum.y, bounds.maximum.x, bounds.maximum.y );
#else
	return Box2::ZERO;
#endif
}

void ConstructTriangleMeshGeometry( void * geometryBuffer, void * inputBuffer, void *  )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxTriangleMeshGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." ); 

	MemoryReadBuffer mrb( static_cast< PxU8* >( inputBuffer ) );
	PxTriangleMesh* triangleMesh = GPhysXEngine->GetPxPhysics()->createTriangleMesh( mrb );
	new ( geometryBuffer ) PxTriangleMeshGeometry( triangleMesh, PxMeshScale(), PxMeshGeometryFlags( PxMeshGeometryFlag::eDOUBLE_SIDED ) );
#endif
}

void DestroyTriangleMeshGeometry( void * geometryBuffer )
{
#ifdef USE_PHYSX
	PxTriangleMeshGeometry * geometry = static_cast< PxTriangleMeshGeometry* >( geometryBuffer );  
	geometry->triangleMesh->release();
	geometry->~PxTriangleMeshGeometry();
#endif
}

Box2 GetTriangleMeshBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxTriangleMeshGeometry* geometry = static_cast< const PxTriangleMeshGeometry* >( geometryBuffer );
	PxBounds3 bounds = geometry->triangleMesh->getLocalBounds();
	return Box2( bounds.minimum.x, bounds.minimum.y, bounds.maximum.x, bounds.maximum.y );
#else
	return Box2::ZERO;
#endif
}

void ConstructHeighfieldGeometry( void * geometryBuffer, void * inputBuffer, void * owner )
{
#ifdef USE_PHYSX
	static_assert( sizeof( PxHeightFieldGeometry ) <= c_maxGeometrySize, "Geometry Buffer is too small." ); 

	SCachedGeometry * cachedGeometry = static_cast< SCachedGeometry * >( owner ); 
	Vector scale = cachedGeometry->m_pose.GetScale33();
	MemoryReadBuffer mrb( static_cast< PxU8* >( inputBuffer ) );
	PxHeightField* highfield = GPhysXEngine->GetPxPhysics()->createHeightField( mrb );
	new ( geometryBuffer ) PxHeightFieldGeometry( highfield, PxMeshGeometryFlags(), scale.X, scale.Y, scale.Z );
#endif
}

void DestroyHeightfieldGeometry( void * geometryBuffer )
{
#ifdef USE_PHYSX
	PxHeightFieldGeometry * geometry = static_cast< PxHeightFieldGeometry* >( geometryBuffer );  
	geometry->heightField->release();
	geometry->~PxHeightFieldGeometry();
#endif
}

Box2 GetHeightfieldBoundingVolume( const void * geometryBuffer )
{
#ifdef USE_PHYSX
	const PxHeightFieldGeometry* geometry = static_cast< const PxHeightFieldGeometry* >( geometryBuffer );
	return  Box2( -( geometry->rowScale * geometry->heightField->getNbRows() ) * 0.5f - 1, -( geometry->columnScale * geometry->heightField->getNbColumns() ) * 0.5f - 1, ( geometry->rowScale * geometry->heightField->getNbRows() ) * 0.5f + 1, ( geometry->columnScale * geometry->heightField->getNbColumns() ) * 0.5f + 1 );
#else
	return Box2::ZERO;
#endif
}

#ifdef USE_APEX
struct ApexConstructorWrapper
{
	ApexConstructorWrapper( physx::apex::NxApexAsset* asset_ )
		: asset( asset_ )
	{}
	
	physx::apex::NxApexAsset *asset;
};
#endif

void ConstructApexGeometry( void * geometryBuffer, void * inputBuffer, void * owner )
{
#ifdef USE_APEX
	SCachedGeometry * cachedGeometry = static_cast< SCachedGeometry * >( owner ); 
	NxApexSDK* apexSdk = NxGetApexSDK();
	physx::PxFileBuf* stream = apexSdk->createMemoryReadStream(inputBuffer, cachedGeometry->GetCompiledDataSize());

	NxParameterized::Serializer::SerializeType serType = NxGetApexSDK()->getSerializeType( *stream );
	NxParameterized::Serializer*  serializer = apexSdk->createSerializer(serType);
	NxParameterized::Serializer::DeserializedData data;
	NxParameterized::Serializer::ErrorType error = serializer->deserialize(*stream, data);
	serializer->release();
	
	// ctremblay Hack. just need a unique name. 
	static Red::Threads::CAtomic< Uint32 > counter;
	StringAnsi tempAssetName = 	StringAnsi::Printf( "apex%i", counter.Increment() );
	physx::apex::NxApexAsset* asset = apexSdk->createAsset( data[0], tempAssetName.AsChar() );
	if( !asset )
	{
		asset = apexSdk->createAsset( data[0], nullptr );
	}

	RED_WARNING_ONCE( asset, "Apex asset was not correctly constructed. DEBUG THIS." );

	if( asset )
	{
		NxApexSDKCachedData& cachedData = apexSdk->getCachedData();
		NxApexModuleCachedData* moduleCache = cachedData.getCacheForModule( asset->getObjTypeID() );
		if( moduleCache )
		{
			moduleCache->deserializeSingleAsset( *asset, *stream );
		}	
	}
	
	new ( geometryBuffer ) ApexConstructorWrapper( asset );

	stream->release();
#endif
}

Box2 GetApexBoundingVolume( const void * geometryBuffer )
{
	return Box2::ZERO;
}


void DestroyApexGeometry( void * geometryBuffer )
{
#ifdef USE_APEX
	NxApexSDK* apexSdk = NxGetApexSDK();
	ApexConstructorWrapper* apexResource = static_cast< ApexConstructorWrapper* >( geometryBuffer );
	if( apexResource->asset )
	{
		apexSdk->releaseAsset( *apexResource->asset );
	}
#endif
}

struct GeometryHandler
{
	void (*ctor)(void *, void*, void *);
	void (*dtor)(void *);
	Box2 (*boundingVolume)(const void*);
};

GeometryHandler c_geometryHandlerTable[] =
{
	{ &ConstructSphereGeometry, nullptr, &GetSphereBoundingVolume },
	{ nullptr, nullptr, nullptr }, // Plane not supported
	{ &ConstructCapsuleGeometry, nullptr, &GetCapsuleBoundingVolume },
	{ &ConstructBoxGeometry, nullptr, &GetBoxBoundingVolume },
	{ &ConstructConvexMeshGeometry, &DestroyConvexMeshGeometry, &GetConvexMeshBoundingVolume },
	{ &ConstructTriangleMeshGeometry, &DestroyTriangleMeshGeometry, &GetTriangleMeshBoundingVolume },
	{ &ConstructHeighfieldGeometry, &DestroyHeightfieldGeometry, &GetHeightfieldBoundingVolume },
	{ &ConstructApexGeometry, &DestroyApexGeometry, &GetApexBoundingVolume }
};

SCachedGeometry::SCachedGeometry()
	: m_pose( Matrix::IDENTITY )
	, m_densityScaler( 1.0f )
	, m_assetId( 0 ) 
	, m_geometryType( -1 )
	, m_contructed( false )
	, m_compiledDataProxy( nullptr )
	, m_compiledDataSize( 0 )
{
	Red::System::MemoryZero( m_geometryBuffer, sizeof( m_geometryBuffer ) );
}

SCachedGeometry::~SCachedGeometry()
{
	if( m_contructed )
	{
		auto dtor = c_geometryHandlerTable[ static_cast< Uint32 >( m_geometryType ) ].dtor;
		if( dtor )
		{
			dtor( m_geometryBuffer );
		}
	}

#ifndef NO_EDITOR
	// In editor, this class own the buffer. Else, the parent class does, Mostly to reduce temp alloc.
	RED_MEMORY_FREE( MemoryPool_Physics, MC_CompiledCollision, m_compiledDataProxy );
#endif

}

SCachedGeometry::SCachedGeometry( SCachedGeometry&& other )
	:	m_pose( other.m_pose ),
		m_physicalSingleMaterial( other.m_physicalSingleMaterial ),
		m_physicalMultiMaterials( other.m_physicalMultiMaterials ),
		m_densityScaler( other.m_densityScaler ),
		m_assetId( other.m_assetId ),
		m_geometryType( other.m_geometryType ),
		m_contructed( other.m_contructed ),
		m_compiledDataProxy( other.m_compiledDataProxy ),
		m_compiledDataSize( other.m_compiledDataSize )
{
	Red::System::MemoryCopy( m_geometryBuffer, other.m_geometryBuffer, sizeof( m_geometryBuffer ) );
	other.m_geometryType = -1;
	other.m_contructed = false;
	other.m_compiledDataProxy = nullptr;
}

SCachedGeometry & SCachedGeometry::operator=( SCachedGeometry&& other )
{
	if( this != &other )
	{
		Red::System::MemoryCopy( m_geometryBuffer, other.m_geometryBuffer, sizeof( m_geometryBuffer ) );
		m_geometryType = other.m_geometryType;
		m_pose = other.m_pose;
		m_physicalSingleMaterial = other.m_physicalSingleMaterial;
		m_physicalMultiMaterials = std::move( other.m_physicalMultiMaterials );
		m_densityScaler = other.m_densityScaler;
		m_assetId = other.m_assetId;
		m_contructed = other.m_contructed;
		m_compiledDataProxy = other.m_compiledDataProxy;
		m_compiledDataSize = other.m_compiledDataSize;

		other.m_geometryType = -1;
		other.m_contructed = false;
		other.m_compiledDataProxy = nullptr;
		other.m_compiledDataSize = 0;
	}

	return *this;
}

void* SCachedGeometry::AllocateCompiledData( const Uint32 size )
{
#ifndef NO_EDITOR

	RED_MEMORY_FREE( MemoryPool_Physics, MC_CompiledCollision, m_compiledDataProxy );

	m_compiledDataSize = size;
	m_compiledDataProxy = RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_CompiledCollision, ( Uint32 )m_compiledDataSize);
	return m_compiledDataProxy;
#else

	RED_FATAL_ASSERT( 0, "Cannot allocate geometry compiled data anywhere than in editor" );
	return nullptr;
#endif
}

void* SCachedGeometry::GetCompiledData() const
{
	return m_compiledDataProxy;
}

Uint32 SCachedGeometry::GetCompiledDataSize() const
{
	return m_compiledDataSize;
}

void SCachedGeometry::Serialize( IFile& file )
{
	file << m_physicalSingleMaterial;
	file << m_physicalMultiMaterials;
	file << m_pose;
	file << m_densityScaler;
	file << m_assetId;
	file << m_compiledDataSize;
	file << m_geometryType;
}

void SCachedGeometry::Initialize( void * buffer )
{
	if ( buffer && m_compiledDataSize != 0 )
	{
#ifdef USE_PHYSX

#ifndef NO_EDITOR
		// We need to keep buffer around for edition/saving etc..
		AllocateCompiledData( m_compiledDataSize );
		Red::System::MemoryCopy( m_compiledDataProxy, buffer, m_compiledDataSize );
#endif
		
		if( m_geometryType != PxGeometryType::eINVALID )
		{
			c_geometryHandlerTable[ static_cast< Uint32 >( m_geometryType ) ].ctor( m_geometryBuffer, buffer, this );
			m_contructed = true;
		}
		
#endif

	}
}

Box2 SCachedGeometry::GetBoundingVolume() const
{
	if( m_contructed )
	{
		return c_geometryHandlerTable[ static_cast< Uint32 >( m_geometryType ) ].boundingVolume( m_geometryBuffer );
	}

	return Box2::ZERO;
}

const void * SCachedGeometry::GetGeometry() const
{
#ifdef USE_APEX
	if( m_contructed && m_geometryType == PxGeometryType::eGEOMETRY_COUNT )
	{
		const ApexConstructorWrapper* wrapper = (const ApexConstructorWrapper*) m_geometryBuffer;
		return wrapper->asset;
	}
#endif
	return m_contructed ? m_geometryBuffer : nullptr;
}

void * SCachedGeometry::GetGeometry()
{
#ifdef USE_APEX
	if( m_contructed && m_geometryType == PxGeometryType::eGEOMETRY_COUNT )
	{
		ApexConstructorWrapper* wrapper = (ApexConstructorWrapper*) m_geometryBuffer;
		return wrapper->asset;
	}
#endif
	return m_contructed ? m_geometryBuffer : nullptr;
}

CCompiledCollision::~CCompiledCollision()
{}

Box2 CCompiledCollision::GetBoundingArea()
{
	Box2 result = Box2::ZERO;
	for ( Uint32 i = 0; i < m_geometries.Size(); ++i )
	{
		SCachedGeometry& geometry = m_geometries[ i ];
		
		result.AddBox( geometry.GetBoundingVolume() );
	}

	return result;
}

void CCompiledCollision::SerializeToCache( IFile& file )
{
#ifdef USE_PHYSX
	file << m_occlusionAttenuation;
	file << m_occlusionDiagonalLimit;
	file << m_swimmingRotationAxis;

	if ( file.IsReader() )
	{
		Uint32 arraySize;
		file << arraySize;

		m_geometries.Resize( arraySize );
		
		Uint32 biggestBufferNeeded = 0;
		for ( Uint32 i = 0; i < arraySize; ++i )
		{
			SCachedGeometry &geometry = m_geometries[ i ];
			geometry.Serialize( file );
			biggestBufferNeeded = Max( biggestBufferNeeded, geometry.GetCompiledDataSize() );
		}

		Red::UniqueBuffer buffer = Red::CreateUniqueBuffer( biggestBufferNeeded, 1, MC_CompiledCollision );

		for ( Uint32 i = 0; i < arraySize; ++i )
		{
			SCachedGeometry &geometry = m_geometries[ i ];
			file.Serialize( buffer.Get(), geometry.GetCompiledDataSize() );
			geometry.Initialize( buffer.Get() ); 
		}
		
	}
	else if (file.IsWriter())
	{
		Uint32 arraySize = m_geometries.Size();
		file << arraySize;

		for ( Uint32 i = 0; i < arraySize; ++i )
		{
			SCachedGeometry& cachedGeom = m_geometries[i];
			cachedGeom.Serialize( file );
		}

		for ( Uint32 i = 0; i < arraySize; ++i )
		{
			const SCachedGeometry& cachedGeom = m_geometries[i];
			file.Serialize( cachedGeom.GetCompiledData(), cachedGeom.GetCompiledDataSize() );
		}
	}
#endif

}

SCachedGeometry& CCompiledCollision::InsertGeometry()
{
	m_geometries.PushBack( SCachedGeometry() );
	return m_geometries.Back();
}

#ifndef NO_EDITOR

#ifdef USE_PHYSX
//#include "../../../external\PhysX3\Source\PhysXExtensions\src\ExtRigidBodyExt.cpp"
#endif

Float CCompiledCollision::GetMassFromResource() const
{
	Float result = 0.0f;
#ifdef USE_PHYSX
	/*
	Ext::InertiaTensorComputer inertiaComp(true);

	for ( Uint32 i = 0; i < m_geometries.Size(); ++i )
	{
		const SCachedGeometry& shape = m_geometries[ i ];

		PxGeometry* geometry = ( PxGeometry* ) shape.GetGeometry();
		if( !geometry ) continue;

		Ext::InertiaTensorComputer it(false);

		PxTransform temp = PxTransform::createIdentity();

		switch(geometry->getType())
		{
		case PxGeometryType::eSPHERE : 
			{
				float radius = static_cast< PxSphereGeometry* >( geometry )->radius;
				it.setSphere(radius, &temp);
			}
			break;

		case PxGeometryType::eBOX : 
			{
				PxVec3 halfExtents = static_cast< PxBoxGeometry* >( geometry )->halfExtents;
				it.setBox(halfExtents, &temp);
			}
			break;

		case PxGeometryType::eCAPSULE : 
			{
				PxReal radius = static_cast< PxCapsuleGeometry* >( geometry )->radius;
				PxReal halfHeight = static_cast< PxCapsuleGeometry* >( geometry )->halfHeight;
				it.setCapsule(0, radius, halfHeight, &temp);
			}
			break;

		case PxGeometryType::eCONVEXMESH : 
			{
				PxConvexMeshGeometry* g = static_cast< PxConvexMeshGeometry* >( geometry );
				PxConvexMesh& convMesh = *( g->convexMesh );

				PxReal convMass;
				PxMat33 convInertia;
				PxVec3 convCoM;
				convMesh.getMassInformation(convMass, reinterpret_cast<PxMat33&>(convInertia), convCoM);

				//scale the mass:
				convMass *= (g->scale.scale.x * g->scale.scale.y * g->scale.scale.z);
				convCoM = g->scale.rotation.rotateInv(g->scale.scale.multiply(g->scale.rotation.rotate(convCoM)));
				convInertia = Ext::MassProps::scaleInertia(convInertia, g->scale.rotation, g->scale.scale);

				it = Ext::InertiaTensorComputer(convInertia, convCoM, convMass);
			}
			break;

		}

		float density = shape.m_densityScaler;

		const SPhysicalMaterial* physicalMaterial = GPhysicEngine->GetMaterial( shape.m_physicalSingleMaterial );
		if( physicalMaterial )
		{
			density *= physicalMaterial->m_density;
		}
		it.scaleDensity( density );

		inertiaComp.add(it);

		result = inertiaComp.getMass();
	}*/

#endif
	return result;
}
#endif

void CCompiledCollision::SetOcclusionAttenuation( Float occlusionAttenuation ) 
{ 
	m_occlusionAttenuation = occlusionAttenuation; 
}

void CCompiledCollision::SetOcclusionDiagonalLimit( Float occlusionDiagonalLimit )
{
	m_occlusionDiagonalLimit = occlusionDiagonalLimit;
}

void CCompiledCollision::SetSwimmingRotationAxis( Int32 swimmingRotationAxis )
{
	m_swimmingRotationAxis = swimmingRotationAxis;
}
