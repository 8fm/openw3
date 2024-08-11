#include "build.h"
#include "apexWrapperBase.h"
#include "component.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "apexMaterialMapping.h"

IMPLEMENT_RTTI_ENUM( EDispatcherSelection )

#if defined( USE_PHYSX ) && defined( USE_APEX )
#include "NxApexActor.h"

#ifndef NO_EDITOR
/// Small record we can use to track whether an actor is still in use. This is used as the actor's userData field.
struct SApexActorUserData
{
	CApexWrapper*					m_wrapper;	//<! Not strictly required, but used to perform checks to ensure actors are being cleaned appropriately
	Red::Threads::CAtomic< Int32 >	m_refCount;

	SApexActorUserData( CApexWrapper* wrapper ) : m_wrapper( wrapper ) {}
};
#endif

CPhysicsWorld* CApexWrapper::GetPhysicsWorld()
{
	return m_world;
}

#ifdef USE_APEX

using namespace physx;
using namespace physx::apex;

#include "NxApexSDK.h"
#include "NxResourceProvider.h"
#include "NxApexNameSpace.h"

void RegisterApexMaterial( const String& name, IRenderResource* material, IRenderResource* parameters )
{
	// Check if we have an existing Mapping object. If so, we can just modify it rather than creating a new one.
	SApexMaterialMapping* mapping = GetMappingForApexMaterial( name );
	if ( !mapping )
	{
		mapping = new SApexMaterialMapping( material, parameters );
		NxGetApexSDK()->getNamedResourceProvider()->setResource( APEX_MATERIALS_NAME_SPACE, UNICODE_TO_ANSI( name.AsChar() ), mapping );
	}
	else
	{
		mapping->Set( material, parameters );
	}
}

void UnregisterApexMaterial( const String& name )
{
	SApexMaterialMapping* mapping = GetMappingForApexMaterial( name );
	if ( mapping )
	{
		NxGetApexSDK()->getNamedResourceProvider()->setResource( APEX_MATERIALS_NAME_SPACE, UNICODE_TO_ANSI( name.AsChar() ), NULL );
		delete mapping;
	}
}

SApexMaterialMapping* GetMappingForApexMaterial( const String& name )
{
	return static_cast< SApexMaterialMapping* >( NxGetApexSDK()->getNamedResourceProvider()->getResource( APEX_MATERIALS_NAME_SPACE, UNICODE_TO_ANSI( name.AsChar() ) ) );
}

void UnregisterApexMaterials()
{
	NxApexSDK* apexSdk = NxGetApexSDK();
	if ( apexSdk )
	{
		// Release all registered materials.
		PxU32 numMaterialNames;
		const char** materialNames = apexSdk->getNamedResourceProvider()->findAllResourceNames( APEX_MATERIALS_NAME_SPACE, numMaterialNames );
		if ( materialNames )
		{
			for ( Uint32 i = 0; i < numMaterialNames; ++i )
			{
				UnregisterApexMaterial( ANSI_TO_UNICODE( materialNames[i] ) );
			}
		}
	}
}

#ifndef NO_EDITOR

void CApexWrapper::AddActorRef( physx::apex::NxApexActor* actor )
{
	// If we have no userdata, we assume that the actor's just been created and we make a new SApexActorUserData.
	if ( !actor->userData )
	{
		actor->userData = new SApexActorUserData( this );
	}

	SApexActorUserData* data = ( SApexActorUserData* )actor->userData;

	ASSERT( data->m_wrapper == this, TXT("Trying to addref an Apex Actor through a CApexWrapper that does not own it.") );

	if ( data != NULL && data->m_wrapper == this )
	{
		data->m_refCount.Increment();
	}
}

void CApexWrapper::ReleaseActorRef( physx::apex::NxApexActor* actor )
{
	SApexActorUserData* data = ( SApexActorUserData* )actor->userData;

	ASSERT( data != NULL, TXT("Apex Actor has NULL userData. Not created through CApexWrapper?") );
	ASSERT( data->m_wrapper == this, TXT("Trying to release an Apex Actor through a CApexWrapper that does not own it.") );

	if ( data != NULL && data->m_wrapper == this )
	{
		Int32 refs = data->m_refCount.Decrement();
		ASSERT( refs >= 0, TXT("Negative refcount on Apex Cloth Actor. Released too many times?") );

		// No more references to this actor, so we can destroy it.
		if ( refs == 0 )
		{
			delete data;
			actor->userData = NULL;
			actor->release();
		}
	}
}
#else
void CApexWrapper::AddActorRef( physx::apex::NxApexActor* actor )
{
}

void CApexWrapper::ReleaseActorRef( physx::apex::NxApexActor* actor )
{
	if( !actor ) return;
	actor->release();
}
#endif

#endif
#endif