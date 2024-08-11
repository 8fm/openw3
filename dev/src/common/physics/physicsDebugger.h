/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CPhysicsWorld;

class CPhysicsDebugger
{
DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_PhysicsDebug );

protected:
	CPhysicsWorld*		m_world;

	const Vector*		m_cameraPosition;
	const EulerAngles*	m_cameraRotation;

	Uint32				m_port;

public:

	CPhysicsDebugger()
		: m_world( NULL)
		, m_cameraPosition( NULL )
		, m_cameraRotation( NULL )
		, m_port( 25001 )
	{}
		
	~CPhysicsDebugger() {}

	Bool Init();
	Bool ShutDown();
	Bool IsAttached();
		
	RED_INLINE Uint32 GetPortNumber() const { return m_port; }
	RED_INLINE void SetPortNumber( Uint32 port ) { m_port = port; }

	Int32 AttachToWorld( CPhysicsWorld* world, const char* host = "localhost" );
	void Tick();
	void DetachFromWorld();
	void SetCamera(const Vector *position, const EulerAngles *rotation);
};

// Physics Debugger instance
extern CPhysicsDebugger* GPhysicsDebugger;

