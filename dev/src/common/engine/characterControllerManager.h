#ifndef _CCHARACTER_CONTROLLER_MANAGER_H_
#define _CCHARACTER_CONTROLLER_MANAGER_H_

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

class CCharacterControllersManager
{
	friend class CPhysicsWorldPhysXImpl;

public:
	CCharacterControllersManager( void* scene );
	~CCharacterControllersManager();

	// common
	void						Init(  );

	// controller
	void*						CreateController( void* desc );
	Bool						UnregisterController( class CPhysicsCharacterWrapper* owner );
	void*						CreateObstacleContext();

	// accessors
	const Uint32				GetControllersCount() const;
	class CPhysicsCharacterWrapper*	GetController( Uint32 index ) const;

	// collisions/intersections
	RED_INLINE const Uint32		GetCollisionsCountFromLastFrame() const { return m_lastFrameCollisionsCount; }
	void						AddCollisionsCountFromLastFrame( const Uint32 collisionCount ) { m_lastFrameCollisionsCount += collisionCount; }
	void						ResetCollisionsCountFromLastFrame() { m_lastFrameCollisionsCount = 0; }
	RED_INLINE const Uint32		GetIntersectionsCountFromLastFrame() const { return m_lastFrameIntersectionsCount; }
	void						AddIntersectionsCountFromLastFrame( const Uint32 interCount ) { m_lastFrameIntersectionsCount += interCount; }
	void						ResetIntersectionsCountFromLastFrame() { m_lastFrameIntersectionsCount = 0; }

	TPair< Vector, Vector >		ResolveControllerSeparation( 
		class CPhysicsCharacterWrapper* controllerA, class CPhysicsCharacterWrapper* controllerB, 
		const Vector& positionA, const Vector& positionB,
		const Vector& movementA, const Vector& movementB 
		);

protected:
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	Bool						ComputeSeparation( Vector& outSepA, Vector& outSepB, const Vector& posA,const Vector& posB, const Float radiusA, const Float radiusB, const Float heightA, const Float heightB, CPhysicsCharacterWrapper* wrapA, CPhysicsCharacterWrapper* wrapB, const Vector& movementA, const Vector& movementB );

private:
	void*						m_controllerManager;
	Uint32						m_lastFrameCollisionsCount;
	Uint32						m_lastFrameIntersectionsCount;
};



#endif