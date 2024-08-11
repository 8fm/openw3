#include "build.h"
#include "characterControllerManager.h"
#include "physicsCharacterVirtualController.h"
#include "physicsCharacterWrapper.h"
#include "baseEngine.h"


#ifdef USE_PHYSX
using namespace physx;
#endif

//////////////////////////////////////////////////////////////////////////

CCharacterControllersManager::CCharacterControllersManager( void* scene )
	: m_controllerManager( nullptr )
{
#ifdef USE_PHYSX
	PxControllerManager* manager = PxCreateControllerManager( *( ( PxScene* ) scene ) );
	m_controllerManager = manager;
	manager->setPreciseSweeps( true );
	manager->setDebugRenderingFlags( PxControllerDebugRenderFlag::eALL );
#endif
}

//////////////////////////////////////////////////////////////////////////

CCharacterControllersManager::~CCharacterControllersManager()
{
#ifdef USE_PHYSX
	if( m_controllerManager )
	{
		PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;
		RED_WARNING( manager->getNbControllers() == 0, "No. of not released controllers: %u", manager->getNbControllers() );
		manager->release();	// object is auto-released internally - by design
		m_controllerManager = nullptr;
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

void* CCharacterControllersManager::CreateController( void* desc )
{
#ifdef USE_PHYSX
	PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;

	ASSERT(manager, TXT("No Physx controller manager initialized! Call CCharacterControllersManager::Init() before usage!") );

	return manager->createController( *( ( PxCapsuleControllerDesc * )desc ) );
#else
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool CCharacterControllersManager::UnregisterController( CPhysicsCharacterWrapper *owner )
{
#ifdef USE_PHYSX
	// TODO
	// Physx manager has no function to remove controller!!

	// Clean up unpushable targets
	PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;

	const PxU32 noCc = manager->getNbControllers();
	for( PxU32 itA=0; itA<noCc; ++itA )
	{
		CPhysicsCharacterWrapper* wrp = GetController( itA );
		if ( wrp->m_unpushableTarget == owner )
			wrp->m_unpushableTarget = nullptr;
	}

#endif
	return false;
}

//////////////////////////////////////////////////////////////////////////

CPhysicsCharacterWrapper* CCharacterControllersManager::GetController( Uint32 index ) const
{
#ifdef USE_PHYSX
	PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;

	ASSERT( manager, TXT("No Physx controller manager initialized! Call CCharacterControllersManager::Init() before usage!") );
	ASSERT( index < manager->getNbControllers(), TXT("Wrong index number!") );

	//PC_SCOPE_PHYSICS( CCM_ResolveControllersSeparation_GetController );

	PxController* cc = manager->getController( (PxU32)index);

	if ( cc )
		return static_cast<CPhysicsCharacterWrapper*>( cc->getUserData() );
#endif
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

TPair< Vector, Vector > CCharacterControllersManager::ResolveControllerSeparation( 
	CPhysicsCharacterWrapper* controllerA, CPhysicsCharacterWrapper* controllerB, 
	const Vector& positionA, const Vector& positionB,
	const Vector& movementA, const Vector& movementB
	)
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS( CCM_ResolveControllerSeparation );

	// init
	Vector totalSeparationA( Vector::ZEROS );
	Vector totalSeparationB( Vector::ZEROS );
	Vector posA = positionA + movementA;
	Float radiusA	= controllerA->GetCurrentVirtualRadius();
	Float heightA	= controllerA->GetCurrentHeight();
	Bool enabledA	= true;

	const Uint32 noVccA = controllerA->m_virtualControllers.Size();
	Uint32 itVA = 0;

	// For all controllerA virtual controllers + in first step test against controllerA
	do
	{
		// if cc is enabled
		if ( enabledA )
		{
			// Test controller A against controller B
			{
				ComputeSeparation( 
						totalSeparationA
					,	totalSeparationB
					,	posA + totalSeparationA
					,	positionB + movementB + totalSeparationB
					,	radiusA
					,	controllerB->GetCurrentVirtualRadius()
					,	heightA
					,	controllerB->GetCurrentHeight()
					,	controllerA
					,	controllerB
					,	movementA
					,	movementB
					);
			}

			// Test controllerA against all virtual controllers of controllerB
			{
				PC_SCOPE_PHYSICS( CCM_ResolveControllerSeparation_Virtuals );

				const Uint32 noVccB = controllerB->m_virtualControllers.Size();
				for ( Uint32 itVB = 0; itVB < noVccB; ++itVB )
				{
					CVirtualCharacterController& virtualCC = controllerB->m_virtualControllers[ itVB ];
					if ( !virtualCC.IsEnabled() || !virtualCC.IsCollidable() )
						continue;
					
					ComputeSeparation( 
							totalSeparationA
						,	totalSeparationB		// this virtual controller is child of controllerB so update its separation
						,	posA + totalSeparationA
						,	virtualCC.GetGlobalPosition() + movementB + totalSeparationB
						,	radiusA
						,	virtualCC.GetCurrentRadius()
						,	heightA
						,	virtualCC.GetCurrentHeight()
						,	controllerA
						,	controllerB
						,	movementA
						,	movementB
						);
					
				}
			}
		}

		// Get parameters from next virtual controller of controller A
		// Priority remains the same as in wrapperA
		if ( itVA < noVccA )
		{
			CVirtualCharacterController& vcontroller = controllerA->m_virtualControllers[ itVA ];
			posA	= vcontroller.GetGlobalPosition() + movementA;
			radiusA	= vcontroller.GetCurrentRadius();
			heightA	= vcontroller.m_height;
			enabledA = vcontroller.IsEnabled() && vcontroller.IsCollidable();
		}
	}
	while ( itVA++ < noVccA );

	// return total separation from collision for both controllers
	return MakePair( totalSeparationA, totalSeparationB );
#else
	return MakePair( Vector::ZERO_3D_POINT, Vector::ZERO_3D_POINT );
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool CCharacterControllersManager::ComputeSeparation( 
	Vector& outSepA, Vector& outSepB, const Vector& posA, const Vector& posB, 
	const Float radiusA, const Float radiusB, const Float heightA, const Float heightB, 
	CPhysicsCharacterWrapper* wrapA, CPhysicsCharacterWrapper* wrapB, 
	const Vector& movementA, const Vector& movementB
	)
{
#ifdef USE_PHYSX
    // compare space in Z axis
	if ( posB.Z < posA.Z + heightA && posB.Z + heightB > posA.Z )
	{
		PC_SCOPE_PHYSICS( CCM_ComputeSeparation );

        const Float radiusSum = radiusA + radiusB;
		const Float radiusSumSqr = radiusSum*radiusSum;

        Vector distance = posA - posB;
        distance.Z = 0.0f;
        Float length = distance.SquareMag2();

        // Square distance compare
        if ( radiusSumSqr > length )
        {
			PC_SCOPE_PHYSICS( CCM_ComputeSeparation_Intersection );

			// update stats
			++m_lastFrameCollisionsCount;

			// Compute overlap amount
            Vector AtBseparation;

            // If capsules are inside each other
            if ( length < 0.001f )
            {
                // Set random separation
                AtBseparation = Vector( GEngine->GetRandomNumberGenerator().Get<Float>( 0, 1.0f), GEngine->GetRandomNumberGenerator().Get<Float>( 0, 1.0f), 0.0f ).Normalized2();
                AtBseparation *= radiusSum;

                length = 0.0f;
                distance = Vector::ZEROS;
            }
            else
            {
                // Set normal separation
                length = MSqrt( length );
                const Float radiusSumLes = radiusSum - length;
                AtBseparation = distance * ( radiusSumLes / length );
            }

			// collision point and normal
			Vector normal = distance;
			Vector point( 0, 0, posA.Z );
            
			// check priorities
			const InteractionPriorityType prioA = wrapB->m_unpushableTarget == wrapA ? InteractionPriorityTypeMax : wrapA->m_interactionPriority;
			const InteractionPriorityType prioB = wrapA->m_unpushableTarget == wrapB ? InteractionPriorityTypeMax : wrapB->m_interactionPriority;

            // Set separation vector according to characters priorities
            if ( (InteractionPriorityTypeCompare)prioA > (InteractionPriorityTypeCompare)prioB )
			{
                outSepB -= AtBseparation;

				// compute collision point
				point = posB + distance*(length-radiusA);
			}
            else if ( (InteractionPriorityTypeCompare)prioA < (InteractionPriorityTypeCompare)prioB )
			{
                outSepA += AtBseparation;

                // compute collision point
				point = posB + distance*radiusB;
			}
            else   // Same priority
            {
                // If characters are in combat mode they don't push themselves
                if( prioA == prioB && prioA == InteractionPriorityTypeMax && length > 0.001f )
                {
					PC_SCOPE_PHYSICS( CCM_ComputeSeparation_Combat );

                    const Vector distNormal = distance.Div3( length );
                    const Vector A = Vector::Project( movementA, distNormal );
                    const Vector B = Vector::Project( movementB, -distNormal );
                    const Float Al = A.Mag2();
                    const Float Bl = B.Mag2();
                    const Float ABl = Al + Bl;
                    const Float AtBsepL = AtBseparation.Mag2();

                    if( ABl > 0 )
                    {
                        outSepA += AtBseparation * Al/(ABl);
                        outSepB -= AtBseparation * Bl/(ABl);

                        point = posB + AtBseparation * Al/(ABl);
                    }
                }
                else
                {
                    AtBseparation *= 0.5f;
                    outSepA += AtBseparation;
                    outSepB -= AtBseparation;

                    // compute collision point
                    point = posB + distance * 0.5f;
                }
            }

			{
				PC_SCOPE_PHYSICS( CCM_ComputeSeparation_StoreCollisions );

				CComponent* componentA;
				CComponent* componentB;

				// add collision data
				normal.Div3( length );	// normalize
				if ( wrapB->GetParent( componentA ) && componentA )
					wrapA->AddCollisionCharacter( componentA->GetEntity(), point, normal );
				if ( wrapA->GetParent( componentB ) && componentB )
					wrapB->AddCollisionCharacter( componentB->GetEntity(), point, -normal );
			}

            return true;
        }
    }
#endif
    return false;
}

//////////////////////////////////////////////////////////////////////////


void* CCharacterControllersManager::CreateObstacleContext()
{
#ifdef USE_PHYSX
	if( m_controllerManager == nullptr )
	{
		ASSERT(m_controllerManager, TXT("No Physx controller manager initialized! Call CCharacterControllersManager::Init() before usage!") );
		return nullptr;
	}
	PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;
	return manager->createObstacleContext();
#else
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

const Uint32 CCharacterControllersManager::GetControllersCount() const
{
#ifdef USE_PHYSX
	if( m_controllerManager == nullptr )
	{
		ASSERT(m_controllerManager, TXT("No Physx controller manager initialized! Call CCharacterControllersManager::Init() before usage!") );
		return 0;
	}

	PxControllerManager* manager = ( PxControllerManager* ) m_controllerManager;
	return ( Uint32 )manager->getNbControllers();
#else
	return 0;
#endif
}



