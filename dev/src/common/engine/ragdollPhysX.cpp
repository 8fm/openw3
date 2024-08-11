/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "ragdollPhysX.h"
#include "../physics/physicsWorld.h"
#include "../physics/PhysXStreams.h"
#include "animatedComponent.h"
#include "../core/dataError.h"
#include "../core/objectIterator.h"
#include "../physics/physicsSettings.h"
#include "../physics/physXEngine.h"
#ifdef USE_PHYSX
using namespace physx;
#endif

IMPLEMENT_ENGINE_CLASS( CRagdoll )

CRagdoll* CRagdoll::Create( const FactoryInfo& data )
{
	CRagdoll* obj = data.CreateResource();
	obj->m_repxBuffer = data.m_repxBuffer;
	obj->ReloadBuffers();

	// done
	return obj;
}

CRagdoll::CRagdoll()
	: m_repxBuffer( PhysXDataBufferAllocator< MC_PhysxRagdollBuffer >::GetInstance() )
	, m_physicsBuffer( PhysXDataBufferAllocator< MC_PhysxRagdollBuffer >::GetInstance() )

{
}

#ifdef USE_PHYSX
struct InputDataFromDataBuffer : public physx::PxInputData
{
	DataBuffer * m_dataBuffer;
	Uint32 m_readOffset;

	InputDataFromDataBuffer( DataBuffer& buffer )
		: m_dataBuffer( &buffer )
		, m_readOffset( 0 )
	{
	}

	virtual physx::PxU32 read(void* dest, physx::PxU32 count)
	{
		Red::System::MemoryCopy( dest, static_cast<char*>(m_dataBuffer->GetData()) + m_readOffset, count );
		return count;
	}

	virtual physx::PxU32 getLength() const
	{
		return static_cast< physx::PxU32 >( m_dataBuffer->GetSize() );
	}

	virtual void  seek(physx::PxU32 offset)
	{
		m_readOffset = offset;
	}

	virtual physx::PxU32	tell() const
	{
		return m_readOffset;
	}
};
#endif

void CRagdoll::ReloadBuffers()
{
#ifdef USE_PHYSX
	InputDataFromDataBuffer inputData( m_repxBuffer );

	PxStringTable* stringTable = GPhysXEngine->GetStringTable();

	const PxCollection* materialsCollection = GPhysXEngine->GetMaterialsCollection();

#ifndef RED_FINAL_BUILD
	CPhysXLogger::ClearLastError();
#endif
	PxCollection* physicsBinaryCollection = PxSerialization::createCollectionFromXml( inputData, *GPhysXEngine->GetCooking(), *GPhysXEngine->GetSerializationRegistry(), NULL, stringTable );
#ifndef RED_FINAL_BUILD
	if( CPhysXLogger::GetLastErrorString().Size() && CPhysXLogger::IsLastErrorFromSameThread() )
	{
		DATA_HALT( DES_Major, this, TXT("Ragdoll Resource"), CPhysXLogger::GetLastErrorString().AsChar() );
		CPhysXLogger::ClearLastError();
	}
#endif

	PxMaterial* fleshMaterial = GPhysXEngine->GetMaterial( CNAME( flesh ) );

	for ( Uint16 i = 0; i < physicsBinaryCollection->getNbObjects(); i++ )
	{
		PxBase& base = physicsBinaryCollection->getObject( i );
		if( PxShape* shape = base.is< PxShape >() )
		{
			const char* shapeName = shape->getName();
			PxMaterial* material = 0;
			if( shapeName )
			{
				material = GPhysXEngine->GetMaterial( shapeName );
			}
			if( !material ) material = fleshMaterial;
			shape->setMaterials( &material, 1 );
		}
		else if( PxD6Joint* joint = base.is< PxD6Joint >() )
		{
			if( PxD6Joint* d6Joint = base.is< PxD6Joint >() )
			{
				if( !d6Joint->getLinearLimit().isValid() )
				{
					d6Joint->setMotion( PxD6Axis::eX, PxD6Motion::eFREE );
					d6Joint->setMotion( PxD6Axis::eY, PxD6Motion::eFREE );
					d6Joint->setMotion( PxD6Axis::eZ, PxD6Motion::eFREE );
				}
				else if( m_state.m_jointBounce >= 0.f )
				{
					PxJointLinearLimit limit = d6Joint->getLinearLimit();
					limit.restitution = Clamp< Float >( m_state.m_jointBounce, 0.f, 1.f );
					d6Joint->setLinearLimit(limit);
				}

				if( !d6Joint->getTwistLimit().isValid() )
				{
					d6Joint->setMotion( PxD6Axis::eTWIST, PxD6Motion::eFREE );
				}
				else if( m_state.m_jointBounce >= 0.f )
				{
					PxJointAngularLimitPair limit = d6Joint->getTwistLimit();
					limit.restitution = Clamp< Float >( m_state.m_jointBounce, 0.f, 1.f );
					d6Joint->setTwistLimit(limit);
				}

				if( !d6Joint->getSwingLimit().isValid() )
				{
					d6Joint->setMotion( PxD6Axis::eSWING1, PxD6Motion::eFREE );
					d6Joint->setMotion( PxD6Axis::eSWING2, PxD6Motion::eFREE );
				}
				else if( m_state.m_jointBounce >= 0.f )
				{
					PxJointLimitCone limit = d6Joint->getSwingLimit();
					limit.restitution = Clamp< Float >( m_state.m_jointBounce, 0.f, 1.f );
					d6Joint->setSwingLimit(limit);
				}
			}
		}
	}

	for ( Int32 i = 0; i < ( Int32 ) physicsBinaryCollection->getNbObjects(); i++ )
	{
		PxMaterial* material = physicsBinaryCollection->getObject( i ).is<PxMaterial>();
		if( !material ) continue;

		i--;
		physicsBinaryCollection->remove( *material );
		material->release();
	}

	MemoryWriteBuffer writeBuffer;
	PxSerialization::serializeCollectionToBinary( writeBuffer, *physicsBinaryCollection, *GPhysXEngine->GetSerializationRegistry(), materialsCollection, true );
	m_physicsBuffer = DataBuffer( PhysXDataBufferAllocator< MC_PhysxRagdollBuffer >::GetInstance(), writeBuffer.currentSize, writeBuffer.data );

	m_state.m_states.Clear();
	m_state.m_states.Reserve( physicsBinaryCollection->getNbObjects() );

	for( Uint32 i = 0; i != physicsBinaryCollection->getNbObjects(); ++i )
	{
		PxBase& base = physicsBinaryCollection->getObject( i );
		PxRigidBody* actor = base.is< PxRigidBody >();
		if( actor )
		{
			const char* ansiName = actor->getName();
			if( !ansiName )
			{
				m_state.m_states.PushBack( SRagdollResourcePartState( CNAME( Unknown ), false ) );
			}
			else
			{
				CName name( ANSI_TO_UNICODE( ansiName ) );
				Bool isKinematic = actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
				m_state.m_states.PushBack( SRagdollResourcePartState( name, isKinematic ) );
			}
		}
		else
		{
			m_state.m_states.PushBack( SRagdollResourcePartState( CNAME( Unknown ), false ) );
		}
	}


	for ( PxU32 i = 0; i < physicsBinaryCollection->getNbObjects(); i++ )
	{
		PxBase& base = physicsBinaryCollection->getObject( i );
		RemoveSerializable( &base );
	}

	physicsBinaryCollection->release();
#endif
}

void CRagdoll::OnSerialize( IFile& file )
{	
	TBaseClass::OnSerialize( file );
	m_repxBuffer.Serialize( file );
	if( file.IsReader() )
	{
		ReloadBuffers();
#ifdef RED_FINAL_BUILD
		m_repxBuffer.Clear();
#endif	
	}
}

#if !defined(NO_EDITOR)
void CRagdoll::OnResourceSavedInEditor()
{
	CResource::OnResourceSavedInEditor();

	for ( ObjectIterator< CAnimatedComponent > it; it; ++it )
	{
		CAnimatedComponent* animatedComponent = *it;
		if( animatedComponent->GetRagdoll() == this )
		{
			animatedComponent->DestroyRagdoll();
		}
	}

}
#endif // NO_EDITOR_RESOURCE_SAVE
