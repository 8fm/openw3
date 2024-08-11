/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "physicsDebugger.h"
#include "physXEngine.h"
#include "physicsWorld.h"
#include "physicsIncludes.h"
#include "physicsSettings.h"

Bool GEnablePhysicsDebuggerInGameExe = false;

#ifdef USE_PHYSX
using namespace physx;
#endif

CPhysicsDebugger* GPhysicsDebugger;

Bool CPhysicsDebugger::Init()
{
	return true;
}
Bool CPhysicsDebugger::ShutDown()
{
	return true;
}

Bool CPhysicsDebugger::IsAttached()
{
#ifndef USE_PHYSX
	return false;
#else
	if( !GPhysXEngine )
		return false;

	PxPhysics* physics = GPhysXEngine->GetPxPhysics();
	if( !physics )
		return false;

	physx::debugger::comm::PvdConnectionManager* connectionManager = GPhysXEngine->GetPxPhysics()->getPvdConnectionManager();
	if( !connectionManager )
		return false;
	
	return connectionManager->isConnected();
#endif
}

Int32 CPhysicsDebugger::AttachToWorld( CPhysicsWorld* world, const char* host )
{
#ifdef USE_PHYSX
	m_world = world;
	if ( m_world )
	{
		if( GPhysXEngine->GetPxPhysics()->getPvdConnectionManager() )
		{
			if ( GPhysXEngine->GetPxPhysics()->getPvdConnectionManager()->isConnected() )
			{
				GPhysXEngine->GetPxPhysics()->getPvdConnectionManager()->disconnect();
			}

			PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerConnectionFlag::eDEBUG;
#ifdef PHYSICS_PROFILE
			connectionFlags |= PxVisualDebuggerConnectionFlag::ePROFILE;
			connectionFlags |= PxVisualDebuggerConnectionFlag::eMEMORY;
#endif
			PxVisualDebuggerExt::createConnection( GPhysXEngine->GetPxPhysics()->getPvdConnectionManager(), host, 5425, 500, connectionFlags );
			GPhysXEngine->GetPxPhysics()->getVisualDebugger()->setVisualDebuggerFlag( PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, SPhysicsSettings::m_pvdTransimtScenequeries );
			GPhysXEngine->GetPxPhysics()->getVisualDebugger()->setVisualDebuggerFlag( PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, SPhysicsSettings::m_pvdTransmitContacts );
			GPhysXEngine->GetPxPhysics()->getVisualDebugger()->setVisualDebuggerFlag( PxVisualDebuggerFlag::eTRANSMIT_CONSTRAINTS, SPhysicsSettings::m_pvdTransimtConstraints );
		}
	}
#endif
	return 0;
}

void CPhysicsDebugger::DetachFromWorld()
{
#ifdef USE_PHYSX
	if ( IsAttached() )
	{
		GPhysXEngine->GetPxPhysics()->getPvdConnectionManager()->disconnect();
	}
#endif

	m_world = NULL;
}

void CPhysicsDebugger::Tick()
{
#ifdef USE_PHYSX
	if ( IsAttached() && m_world )
	{
		ASSERT( m_world != NULL );

		physx::debugger::comm::PvdConnectionManager* connectionManager = GPhysXEngine->GetPxPhysics()->getPvdConnectionManager();

		Vector pos = m_world->GetWorldParentProvider()->GetCameraPosition();
		Vector forward = m_world->GetWorldParentProvider()->GetCameraForward();
		Vector up = m_world->GetWorldParentProvider()->GetCameraUp();
		Vector target = pos + forward * 10.0f;
		connectionManager->setCamera( "camera", TO_PX_VECTOR( pos ), TO_PX_VECTOR( up ), TO_PX_VECTOR( target ) );

	}
#endif
}

void CPhysicsDebugger::SetCamera(const Vector *position, const EulerAngles *rotation)
{
	m_cameraPosition = position;
	m_cameraRotation = rotation;
}

