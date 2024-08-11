#include "build.h"
#include "physicsDataProviders.h"
#include "renderer.h"
#include "component.h"
#include "..\core\scriptStackFrame.h"
#include "staticMeshComponent.h"
#include "..\physics\physicsDebugger.h"
#include "..\core\dataError.h"
#include "utils.h"
#include "renderVertices.h"
#include "physicsCharacterWrapper.h"
#include "renderFragment.h"

// HACK: This shouldn't be here, but to enable physics debug visualization they are brought back to editor
#ifndef NO_EDITOR
#include "apexClothWrapper.h"
#include "apexDestructionWrapper.h"
#include "apexDebugVisualizer.h"

#include "..\physics\physicsWrapper.h"
#include "..\physics\physicsChainedRagdollWrapper.h"
#include "..\physics\physicsJointedRagdollWrapper.h"
#endif

//HACKS TO SIMPLY REMOVAL 
#ifdef USE_APEX
class physx::apex::NxUserRenderResourceManager* CreateUserRenderResourceManager()
{
	return GRender->CreateApexRenderResourceManager();
}
#endif
//HACKS TO SIMPLY REMOVAL

void CPhysicsWrapperParentComponentProvider::operator= (const IPhysicsWrapperParentProvider& other)
{
	CComponent* component = nullptr;
	other.GetParent( component );
	m_parent = component;
}

CPhysicsWorld* CPhysicsWrapperParentComponentProvider::GetPhysicsWorld() const
{
	CPhysicsWorld* result = nullptr;
	if( CComponent* component = m_parent.Get() )
	{
		if( CWorld* world = component->GetWorld() )
		{
			world->GetPhysicsWorld( result );
		}
	}
	return result;
}


void CPhysicsWrapperParentComponentProvider::DataHalt( IScriptable* parent, const Char* category, const Char* reason )
{
	if( CObject* object = Cast< CObject >( parent ) )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( object ), category, reason );
	}
}

void CPhysicsWrapperParentResourceProvider::operator= (const IPhysicsWrapperParentProvider& other)
{
	CResource* component = nullptr;
	other.GetParent( component );
	m_parent = component;
}


void CPhysicsWrapperParentResourceProvider::DataHalt( IScriptable* parent, const Char* category, const Char* reason )
{
	if( CResource* resource = Cast< CResource >( parent ) )
	{
		DATA_HALT( DES_Major, resource, category, reason );
	}
}

static void funcSetPhysicalEventOnCollision( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComponent >, triggerObject, NULL );		
	GET_PARAMETER_OPT( THandle< IScriptable >, receiverObject, NULL );			
	GET_PARAMETER_OPT( CName, eventName, CName::NONE );		
	FINISH_PARAMETERS;

	if( eventName.Empty() )
	{
		eventName = CNAME( OnCollision );
	}
	CComponent* component = triggerObject.Get();
	if( component )
	{
		CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnCollision, receiverObject, eventName );
			RETURN_BOOL( true );
			return;
		}
	}

	RETURN_BOOL( false );
}

static void funcSetPhysicalEventOnTriggerFocusFound( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComponent >, triggerObject, NULL );		
	GET_PARAMETER_OPT( THandle< IScriptable >, receiverObject, NULL );		
	GET_PARAMETER_OPT( CName, eventName, CName::NONE );		
	FINISH_PARAMETERS;

	CComponent* component = triggerObject.Get();
	if( component )
	{
		CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, receiverObject, eventName );
			RETURN_BOOL( true );
			return;
		}
	}
	RETURN_BOOL( false );
}

static void funcSetPhysicalEventOnTriggerFocusLost( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComponent >, triggerObject, NULL );		
	GET_PARAMETER_OPT( THandle< IScriptable >, receiverObject, NULL );			
	GET_PARAMETER_OPT( CName, eventName, CName::NONE );		
	FINISH_PARAMETERS;

	CComponent* component = triggerObject.Get();
	if( component )
	{
		CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, receiverObject, eventName );
			RETURN_BOOL( true );
			return;
		}
	}
	RETURN_BOOL( false );
}

static void funcPhysxDebugger( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( String, host, String::EMPTY );		
	FINISH_PARAMETERS;

	if( host.Empty() )
	{
		RETURN_BOOL( false );
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if( !world )
	{
		RETURN_BOOL( false );
		return;
	}
	CPhysicsWorld* physicsWorld = nullptr;
	if( !world->GetPhysicsWorld( physicsWorld ) )
	{
		RETURN_BOOL( false );
		return;
	}

#ifndef PHYSICS_RELEASE
	const char* ansi = UNICODE_TO_ANSI( host.AsChar() );
	Int32 errorCode = GPhysicsDebugger->AttachToWorld( physicsWorld, ansi );
	if ( errorCode )
	{
		RED_LOG( CNAME( Physx ), TXT("Error code: %ld"), errorCode );
		GPhysicsDebugger->DetachFromWorld();
		RETURN_BOOL( false );
		return;
	}
	RETURN_BOOL( true );
#else
	RETURN_BOOL( false );
#endif
}

void CComponent::funcHasDynamicPhysic( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		Bool isDynamic = !wrapper->IsKinematic() && !wrapper->IsStatic();
		RETURN_BOOL( isDynamic );
		return;
	}
	RETURN_BOOL( false );
}

void CComponent::funcHasCollisionType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, collisionTypeName, CName::NONE );
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	GET_PARAMETER_OPT( Int32, shapeIndex, 0 );			
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask requestedCollisionType = GPhysicEngine->GetCollisionTypeBit( collisionTypeName );

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		CPhysicsEngine::CollisionMask type = wrapper->GetCollisionTypesBits( actorIndex, shapeIndex );
		if( requestedCollisionType & type )
		{
			RETURN_BOOL( true );
		}
		else
		{
			RETURN_BOOL( false );
		}
		return;
	}
	RETURN_BOOL( false );
}

void CComponent::funcGetPhysicalObjectLinearVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		Vector linear;
		Vector angular;
		if( wrapper->GetVelocity( linear, angular, actorIndex ) )
		{
			RETURN_STRUCT( Vector, linear );
			return;
		}
	}
	RETURN_STRUCT( Vector, Vector::ZEROS );
}

void CComponent::funcGetPhysicalObjectAngularVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		Vector linear;
		Vector angular;
		if( wrapper->GetVelocity( linear, angular, actorIndex ) )
		{
			RETURN_STRUCT( Vector, angular );
			return;
		}
	}

	RETURN_STRUCT( Vector, Vector::ZEROS );
}

void CComponent::funcSetPhysicalObjectLinearVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, velocity, Vector::ZERO_3D_POINT );		
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		if( wrapper->SetVelocityLinear( velocity, actorIndex ) )
		{
			RETURN_BOOL( true );
			return;
		}
	}

	RETURN_BOOL( false );
}

void CComponent::funcSetPhysicalObjectAngularVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, velocity, Vector::ZERO_3D_POINT );		
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		if( wrapper->SetVelocityAngular( velocity, actorIndex ) )
		{
			RETURN_BOOL( true );
			return;
		}
	}

	RETURN_BOOL( false );
}

void CComponent::funcGetPhysicalObjectMass( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );	
	FINISH_PARAMETERS;

	Float mass = -1;
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		mass = 	wrapper->GetMass( actorIndex );
	}

	RETURN_FLOAT( mass );
}

void CComponent::funcApplyTorqueToPhysicalObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, torque, Vector() );		
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );	
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		wrapper->ApplyTorque( torque, actorIndex );
	}
}

void CComponent::funcApplyForceAtPointToPhysicalObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, force, Vector() );		
	GET_PARAMETER( Vector, point, Vector() );	
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			

	FINISH_PARAMETERS;

	String name;
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		wrapper->ApplyForce( force, point, actorIndex );
	}
}

void CComponent::funcApplyForceToPhysicalObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, force, Vector() );		
	GET_PARAMETER( Vector, point, Vector() );	
	GET_PARAMETER_OPT( Float, radius, 1.0f );	
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			

	FINISH_PARAMETERS;

	String name;
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		wrapper->ApplyForce( force, point, actorIndex );
	}
}


void CComponent::funcApplyLocalImpulseToPhysicalObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, force, Vector() );		
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	String name;
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		wrapper->ApplyImpulse(force, wrapper->GetPosition(), actorIndex);
	}
}

void CComponent::funcApplyTorqueImpulseToPhysicalObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, torque, Vector() );		
	GET_PARAMETER_OPT( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	String name;
	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		wrapper->ApplyTorqueImpulse(torque, actorIndex);
	}
}

void CComponent::funcGetPhysicalObjectBoundingVolume( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Box, box, Box() );
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		box = wrapper->GetWorldBounds();
		RETURN_BOOL( true );
		return;
	}
	RETURN_BOOL( false );
}

void CStaticMeshComponent::funcGetPhysicalObjectBoundingVolume( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Box, box, Box() );
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if( wrapper )
	{
		if( m_physicsBodyIndex >= 0 )
		{
			box = wrapper->GetWorldBounds( m_physicsBodyIndex );
			RETURN_BOOL( true );
			return;
		}
	}
	RETURN_BOOL( false );
}

void ExportPhysicsNatives()
{
	NATIVE_GLOBAL_FUNCTION( "SetPhysicalEventOnCollision", funcSetPhysicalEventOnCollision );
	NATIVE_GLOBAL_FUNCTION( "SetPhysicalEventOnTriggerFocusFound", funcSetPhysicalEventOnTriggerFocusFound );
	NATIVE_GLOBAL_FUNCTION( "SetPhysicalEventOnTriggerFocusLost", funcSetPhysicalEventOnTriggerFocusLost );
	NATIVE_GLOBAL_FUNCTION( "PhysxDebugger", funcPhysxDebugger );
}


#ifdef USE_APEX
#include "NxApexSDK.h"
#include "NxApexScene.h"
#include "NxParamUtils.h"
#include "NxModuleDestructible.h"
#endif

#ifndef NO_EDITOR

void ShowDebugStuff( CRenderFrame* frame, class physx::PxRigidBody* body )
{
	Bool dampers = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorDampers );
	Bool velocities = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorVelocities );
	Bool masses = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorMasses );
	Bool destructionFractureRatio = frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexFractureRatio );
	Bool destructionThresoldLeft = frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexThresoldLeft );
	Bool iterations = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorIterations );
	Bool buoyancies = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorFloatingRatio );

#ifdef USE_APEX
	static physx::apex::NxModuleDestructible* destructibleModule = 0;
	if( !destructibleModule )
	{
		physx::apex::NxApexSDK* apexSdk = physx::apex::NxGetApexSDK();
		for( Uint32 i = 0; i != apexSdk->getNbModules(); ++i )
		{
			physx::apex::NxModule* module = apexSdk->getModules()[ i ];
			if( !strcmp( module->getName(), "Destructible" ) )
			{
				destructibleModule = static_cast< physx::apex::NxModuleDestructible* >( module );
			}
		}
	}
#endif

	physx::PxMat44 pose = body->getGlobalPose();
	pose = pose * body->getCMassLocalPose();
	Vector pos = TO_VECTOR( pose.column3 );

	String text;
	if( velocities )
	{
		text += String::Printf( TXT( "%f / %f\n" ), body->getLinearVelocity().magnitude(), body->getAngularVelocity().magnitude() );
	}
	if( masses )
	{
		text += String::Printf( TXT( "%f" ), body->getMass() );
	}

	Color color = Color::GREEN;
	if( physx::PxRigidDynamic* dynamic = body->isRigidDynamic() )
	{
		if( dampers )
		{
			text = String::Printf( TXT( "%f / %f\n" ), dynamic->getLinearDamping(), dynamic->getAngularDamping() );
		}
		if( iterations && dynamic->getNbConstraints() > 0 )
		{
			Uint32 minPositionIters, minVelocityIters;
			dynamic->getSolverIterationCounts( minPositionIters, minVelocityIters );
			text += String::Printf( TXT( "%i / %i\n" ), minPositionIters, minVelocityIters );
		}
		if( dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC ) color = Color::BLUE;

	}
	frame->AddDebugText( pos, text, false, Color::GREEN, Color::GREEN );

#ifdef USE_APEX
	if( destructibleModule )
	{
		if( destructionFractureRatio )
		{
			if( destructibleModule->owns( body ) )
			{
				CApexDestructionWrapper* wrapper = static_cast< CApexDestructionWrapper* >( body->userData );
				float ratio = wrapper->GetFractureRatio();
				text = String::Printf( TXT( "%f\n" ), ratio );
				CComponent* parent = nullptr;
				if( wrapper->GetParent( parent ) )
				{
					frame->AddDebugText( parent->GetLocalToWorld().GetTranslation(), text, false, Color::GREEN, Color::GREEN );
				}
			}
		}
		else if( destructionThresoldLeft )
		{
			if( destructibleModule->owns( body ) )
			{
				CApexDestructionWrapper* wrapper = static_cast< CApexDestructionWrapper* >( body->userData );
				if( body->getNbShapes() )
				{
					physx::PxShape* shape = 0;
					body->getShapes( &shape, 1, 0 );
					SActorShapeIndex& index = ( SActorShapeIndex& ) shape->userData;
					float ratio = wrapper->GetThresholdLeft( index.m_actorIndex );
					if( ratio >= 0.0f )
					{
						text = String::Printf( TXT( "%f\n" ), ratio );
						CComponent* parent = nullptr;
						if( wrapper->GetParent( parent ) )
						{
							frame->AddDebugText( parent->GetLocalToWorld().GetTranslation(), text, false, Color::GREEN, Color::GREEN );
						}
					}
				}
			}
		}
	}
#endif


}

#endif //! NO_EDITOR

#ifdef USE_APEX
void ProcessApexScene(physx::PxScene* scene, CRenderFrame* frame)
{
	physx::apex::NxApexScene* apexScene = (physx::apex::NxApexScene*)(scene->userData);

	// Global flags
	NxParameterized::Interface* apexSceneParams = apexScene->getDebugRenderParams();
	NxParameterized::setParamBool( *apexSceneParams, "Enable",							true );
	NxParameterized::setParamF32 ( *apexSceneParams, "Scale",							1.0f );
	NxParameterized::setParamBool( *apexSceneParams, "Bounds",							frame->GetFrameInfo().IsShowFlagOn( SHOW_Bboxes ) );
	NxParameterized::setParamF32 ( *apexSceneParams, "RenderTangents",					frame->GetFrameInfo().IsShowFlagOn( SHOW_TBN ) ? 0.1f : 0.0f );
	NxParameterized::setParamF32 ( *apexSceneParams, "RenderBitangents",				frame->GetFrameInfo().IsShowFlagOn( SHOW_TBN ) ? 0.1f : 0.0f );
	NxParameterized::setParamF32 ( *apexSceneParams, "RenderNormals",					frame->GetFrameInfo().IsShowFlagOn( SHOW_TBN ) ? 0.1f : 0.0f );

	// Clothing module
	NxParameterized::Interface* clothParams = apexScene->getModuleDebugRenderParams( "Clothing" );
	NxParameterized::setParamBool( *clothParams, "Actors",								true );
	//PhysicsMeshSolid and Wire
	NxParameterized::setParamF32 ( *clothParams, "PhysicsMeshWire",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothPhysicsMeshWire ) );
	NxParameterized::setParamF32 ( *clothParams, "PhysicsMeshSolid",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothPhysicsMeshSolid ) );
	//CollisionShapes
	NxParameterized::setParamBool( *clothParams, "CollisionShapesWire",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothCollisionWire ) );
	NxParameterized::setParamBool( *clothParams, "CollisionShapes",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothCollisionSolid ) );
	//Skeleton info
	NxParameterized::setParamBool( *clothParams, "Skeleton",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothSkeleton ) );
	NxParameterized::setParamF32 ( *clothParams, "BoneFrames",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothSkeleton ) ? 0.05f : 0.0f );
	//NxParameterized::setParamF32 ( *clothParams, "BoneNames",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothSkeleton ) ? 0.75f : 0.0f );
	NxParameterized::setParamBool( *clothParams, "Backstop",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothBackstop ) );
	NxParameterized::setParamF32 ( *clothParams, "BackstopPrecise",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothBackstopPrecise ) ? 0.05f : 0.0f );
	NxParameterized::setParamBool( *clothParams, "MaxDistance",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothMaxDistance ) );
	//MaxDistanceInwards
	NxParameterized::setParamF32 ( *clothParams, "Velocities",							frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothVelocity ) ? 1.0f : 0.0f );
	NxParameterized::setParamF32 ( *clothParams, "Wind",								frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothWind ) ? 0.05f : 0.0f );
	NxParameterized::setParamF32 ( *clothParams, "SkinnedPositions",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothSkinnedPosition ) ? 1.0f : 0.0f );
	//FiberRange
	NxParameterized::setParamBool( *clothParams, "TethersActive",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothActiveTethers ) );
	//TethersInactive
	NxParameterized::setParamBool( *clothParams, "LengthFibers",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothLength ) );
	NxParameterized::setParamBool( *clothParams, "CrossSectionFibers",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothCrossSection ) );
	NxParameterized::setParamBool( *clothParams, "BendingFibers",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothBending ) );
	NxParameterized::setParamBool( *clothParams, "ShearingFibers",						frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothShearing ) );
	NxParameterized::setParamBool( *clothParams, "ZerostretchFibers",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothZeroStretch ) );

	//SelfCollision
	//SelfCollisionAttenuation
	NxParameterized::setParamBool( *clothParams, "SelfCollisionWire",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothSelfCollision ) );

	NxParameterized::setParamBool( *clothParams, "VirtualCollision",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothVirtualCollision ) );
	NxParameterized::setParamBool( *clothParams, "ShowInLocalSpace",					frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexClothLocalSpace ) );

	// Destruction module
	NxParameterized::Interface* destParams = apexScene->getModuleDebugRenderParams( "Destructible" );
	NxParameterized::setParamBool( *destParams, "VISUALIZE_DESTRUCTIBLE_ACTOR",			true );
	NxParameterized::setParamF32 ( *destParams, "VISUALIZE_DESTRUCTIBLE_SUPPORT",		frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexDestructSupport ) ? 1.0f : 0.0f );
	// Turn off some things that are on by default...
	NxParameterized::setParamBool( *destParams, "VISUALIZE_DESTRUCTIBLE_ACTOR_NAME",	false );
	NxParameterized::setParamBool( *destParams, "VISUALIZE_DESTRUCTIBLE_ACTOR_POSE",	false );


#ifdef APEX_ENABLE_DEBUG_VISUALIZATION
	// Special renderer that just pushes geometry to the render frame.
	CApexDebugVisualizerRenderer renderer( frame );

	apexScene->lockRenderResources();
	// Pass null user data here. This indicates that we're doing debug visualization, and need to create
	// a special main-thread-friendly render resource (CApexDebugVisualizerResourceBase).
	apexScene->updateRenderResources( true, nullptr );
	apexScene->dispatchRenderResources( renderer );
	apexScene->unlockRenderResources();
#endif // APEX_ENABLE_DEBUG_VISUALIZATION
}

#endif // USE_APEX

void PhysicsGenerateEditorFragments( CRenderFrame* frame, CWorld* activeWorld )
{
#ifndef NO_EDITOR

	Box										debugVisualisationBox( Box::EMPTY );
	void*									debugLineBuffer( nullptr );
	Uint32									debugLineBufferSize( 0 );
	void*									debugTriangleBuffer( nullptr );
	Uint32									debugTriangleBufferSize( 0 );

#ifndef NO_EDITOR
	IRenderResource*						debugMesh;
#endif 
	
	TDynArray< Bool >						flags;

	struct Local
	{
		static Float SlopeOf( const Vector3& p1, const Vector3& p2 )
		{
			return RAD2DEG( MAsin( ( p1.Z - p2.Z ) / ( p1 - p2 ).Mag() ) ); 
		}

		static void ChangeColorOfLine( DebugVertex* verts, const Float maxSlope )
		{																										
			const Vector3& p1 = *( ( const Vector3* ) &verts[ 0 ].x );
			const Vector3& p2 = *( ( const Vector3* ) &verts[ 1 ].x );
			if ( MAbs( SlopeOf( p1, p2 ) ) > maxSlope )
			{
				verts[ 0 ].color = 0xaa0000cc;
				verts[ 1 ].color = 0xaa0000cc;
			}
			else
			{
				verts[ 0 ].color = 0xaa00cc00;
				verts[ 1 ].color = 0xaa00cc00;
			}
		}

		static void ChangeColorsOfLines( DebugVertex* lines, Uint32 numLines )
		{
			Float maxSlope = SCCTDefaults::DEFAULT_MAX_SLOPE;
			maxSlope = GGame->GetGameplayConfig().m_physicsTerrainDebugMaxSlope;
			ASSERT( maxSlope > 0.f && maxSlope < 90.f );

			for ( Uint32 i = 0; i < numLines; ++i )
			{
				const Uint32 k = i * 2;
				//if ( lines[ k ].color == 0xff000000 )
				{
					ChangeColorOfLine( &lines[ k ], maxSlope );
				}
			}
		}
	};

	if( !activeWorld )
	{
		return;
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysXVisualization ) )
	{
		const Float dist = Min( GGame->GetGameplayConfig().m_physicsTerrainDebugRange, frame->GetFrameInfo().m_camera.GetFarPlane() );
		const Float fov = frame->GetFrameInfo().m_camera.GetFOV() / 2.f * frame->GetFrameInfo().m_camera.GetAspect();
		const Vector position = frame->GetFrameInfo().m_camera.GetPosition();
		const Vector direction = frame->GetFrameInfo().m_camera.GetRotation().TransformVector( Vector::EY );
#ifndef NO_EDITOR
		debugVisualisationBox = Box( position, 1.f );
		debugVisualisationBox.AddPoint( position + ( direction * dist ) );
		EulerAngles r;
		r.Yaw = fov;
		debugVisualisationBox.AddPoint( position + ( r.TransformVector( direction ) * dist ) );
		r.Yaw = -fov;
		debugVisualisationBox.AddPoint( position + ( r.TransformVector( direction ) * dist ) );
		r.Yaw = 0.f;
		r.Pitch = fov;
		debugVisualisationBox.AddPoint( position + ( r.TransformVector( direction ) * dist ) );
		r.Pitch = -fov;
		debugVisualisationBox.AddPoint( position + ( r.TransformVector( direction ) * dist ) );
		debugVisualisationBox.AddPoint( position + Vector( 0.f, 0.f, dist ) );
		debugVisualisationBox.AddPoint( position + Vector( 0.f, 0.f, -dist ) );
#endif
#ifndef NO_EDITOR
		CPhysicsWorld* physicsWorld = nullptr;
		if( !activeWorld->GetPhysicsWorld( physicsWorld ) )
		{
			return;
		}

		physicsWorld->SetDebuggingVisualizationBox( debugVisualisationBox );
		
		debugTriangleBuffer	= physicsWorld->GetDebugTriangles( debugTriangleBufferSize );
		debugLineBuffer		= physicsWorld->GetDebugLines( debugLineBufferSize );
	
#endif

		// draw triangles
#ifndef NO_EDITOR
		const Uint32 numTris = debugTriangleBufferSize;
		if ( numTris && debugTriangleBuffer )
		{
			const void* tris = debugTriangleBuffer;
			Uint32 numVerts = numTris * 3;

			TDynArray< DebugVertex > vertices;
			vertices.ResizeFast( numVerts );
			Red::System::MemoryCopy( vertices.TypedData(), tris, sizeof( DebugVertex ) * numVerts );

			for ( Uint32 i=0; i< vertices.Size(); ++i )
			{
				Uint32 srcColor = vertices[ i ].color; 
				Uint8* srcDataByteColor = (Uint8*)( &srcColor ); 
				Uint8* dstDataByteColor = (Uint8*)( &vertices[ i ].color ); 

				dstDataByteColor[0] = srcDataByteColor[2]; 
				dstDataByteColor[1] = srcDataByteColor[1]; 
				dstDataByteColor[2] = srcDataByteColor[0]; 
				dstDataByteColor[3] = srcDataByteColor[3]; 

			}

			TDynArray< Uint32 > indices;
			indices.ResizeFast( numVerts );
			for ( Uint32 i = 0; i < numVerts; ++i )
			{
				indices[ i ] = i;
			}

			debugMesh = GRender->UploadDebugMesh( vertices, indices );
			if ( debugMesh )
			{
				new ( frame ) CRenderFragmentDebugMesh( frame, Matrix::IDENTITY, debugMesh );  
			} 
		}

		// draw lines
		const Uint32 numLines = debugLineBufferSize;
		if ( numLines && debugLineBuffer )
		{
			const void* lines = debugLineBuffer;
			// paint some colors ;)
			Local::ChangeColorsOfLines( ( DebugVertex* ) lines, numLines );

			// GpuApi has limited buffer size for primitives like such
			// I don't want to put a strange extern here, so I'll just duplicate the value - it's a debug functionality after all
			Int32 g_drawPrimitiveUPBufferSize = 3 * 1024 * 1024;
			Uint32 maxLinesToDrawInOneCall = g_drawPrimitiveUPBufferSize / sizeof( DebugVertex ) / 2;

			for ( Uint32 i = 0; i < numLines; i += maxLinesToDrawInOneCall )
			{
				Uint32 linesToDrawInThisCall = Min( numLines - i, maxLinesToDrawInOneCall );
				new ( frame ) CRenderFragmentDebugLineList( frame, Matrix::IDENTITY, ( reinterpret_cast< const DebugVertex* > ( lines ) ) + ( i * 2 ), linesToDrawInThisCall * 2, RSG_DebugUnlit );
			}
		}
#endif
	}
#ifndef NO_EDITOR
	else // not interested in PhysX visualization
	{
		debugVisualisationBox = Box::EMPTY;
	}
#endif

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysXMaterials ) )
	{
#ifndef NO_EDITOR
		CPhysicsWorldPhysXImpl* world;
		activeWorld->GetPhysicsWorld( world );
		if( !world ) return;

		const Vector position = frame->GetFrameInfo().m_camera.GetPosition();
		TDynArray< SPhysicsDebugLine > lines;
		TDynArray< Color > colors;
		world->GetDebugVisualizationForMaterials( position, lines );

		Uint32 linesCount = lines.Size();
		for( Uint32 i = 0; i != linesCount; ++i )
		{
			frame->AddDebugLine( lines[ i ].m_start, lines[i].m_end, lines[i].m_color );
		}
#endif
	}

#ifdef USE_APEX
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexDebugAll ) )
	{
		CPhysicsWorldPhysXImpl* world;
		activeWorld->GetPhysicsWorld( world );
		if( !world )		
		{
			return;
		}
		physx::PxScene* scene = world->GetPxScene();

		ProcessApexScene(scene, frame);

		CPhysicsWorldPhysXImpl* secondaryWorld;
		activeWorld->GetPhysicsWorldSecondary( secondaryWorld );
		if( !secondaryWorld )		
		{
			return;
		}
		scene = secondaryWorld->GetPxScene();

		ProcessApexScene(scene, frame);
	}
#endif // USE_APEX

	Bool dampers = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorDampers );
	Bool velocities = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorVelocities );
	Bool masses = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorMasses );
	Bool destructionFractureRatio = frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexFractureRatio );
	Bool destructionThresoldLeft = frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexThresoldLeft );
	Bool iterations = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorIterations );

	if ( dampers || velocities || masses || destructionFractureRatio || destructionThresoldLeft || iterations )
	{
		CPhysicsWorldPhysXImpl* world;
		activeWorld->GetPhysicsWorld( world );
		if( !world ) return;
		physx::PxScene* scene = world->GetPxScene();

		const Vector position = frame->GetFrameInfo().m_camera.GetPosition();
		Uint32 actorsCount = scene->getNbActors( physx::PxActorTypeSelectionFlag::eRIGID_DYNAMIC );
		for( Uint32 i = 0; i != actorsCount; ++i )
		{
			physx::PxActor* actor = 0;
			scene->getActors( physx::PxActorTypeSelectionFlag::eRIGID_DYNAMIC, &actor, 1, i );
			physx::PxRigidBody* body = actor->isRigidBody();
			if( !body ) continue;

			if( GGame->IsActive() && body->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC ) continue;

			physx::PxMat44 pose = body->getGlobalPose();
			pose = pose * body->getCMassLocalPose();
			Vector pos = TO_VECTOR( pose.column3 );

			Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
			if( position.DistanceSquaredTo( pos ) > dist ) continue;

			ShowDebugStuff( frame, body );

		}

		Uint32 articualtionsCount = scene->getNbArticulations();
		for( Uint32 i = 0; i != articualtionsCount; ++i )
		{
			physx::PxArticulation* articulation = 0;
			scene->getArticulations( &articulation, 1, i );

			Uint32 articualtionLinksCount = articulation->getNbLinks();
			TDynArray< physx::PxArticulationLink* > links;
			links.Resize( articualtionLinksCount );
			articulation->getLinks( links.TypedData() , articualtionLinksCount );
			for( Uint32 j = 0; j != articualtionLinksCount; ++j )
			{
				physx::PxArticulationLink* link = links[ j ];

				physx::PxMat44 pose = link->getGlobalPose();
				pose = pose * link->getCMassLocalPose();
				Vector pos = TO_VECTOR( pose.column3 );

				Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
				if( position.DistanceSquaredTo( pos ) > dist ) continue;

				ShowDebugStuff( frame, link );
			}

		}
	}

	Bool motionIntensity = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysMotionIntensity );
	if( motionIntensity )
	{
		CPhysicsWorldPhysXImpl* world;
		activeWorld->GetPhysicsWorld( world );
		if( !world ) return;
		{
			CPhysicsChainedRagdollWrapper* wrapper = world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetWrapperFirst();
			while( wrapper )
			{
				if( Float ratio = wrapper->GetMotionIntensity() > 0.0f )
				{
					CComponent* parent = nullptr;
					if( wrapper->GetParent( parent ) ) frame->AddDebugText( parent->GetLocalToWorld().GetTranslation(), String::Printf( TXT( "%f\n" ), ratio ), false, Color::GREEN, Color::GREEN );
				}
				wrapper =  world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetWrapperNext( wrapper );
			}
		}
		{
			CPhysicsJointedRagdollWrapper* wrapper =  world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->GetWrapperFirst();
			while( wrapper )
			{
				if( Float ratio = wrapper->GetMotionIntensity() > 0.0f )
				{
					CComponent* parent = nullptr;
					if( wrapper->GetParent( parent ) ) frame->AddDebugText( parent->GetLocalToWorld().GetTranslation(), String::Printf( TXT( "%f\n" ), ratio ), false, Color::GREEN, Color::GREEN );
				}
				wrapper =  world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->GetWrapperNext( wrapper );
			}
		}
	}

	if( motionIntensity )
	{
#ifdef USE_APEX
		CPhysicsWorldPhysXImpl* world;
		activeWorld->GetPhysicsWorld( world );
		if( !world ) return;

		CApexClothWrapper* wrapper = world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetWrapperFirst();
		while( wrapper )
		{
			if( Float ratio = wrapper->GetMotionIntensity() > 0.0f )
			{
				CComponent* parent = nullptr;
				if( wrapper->GetParent( parent ) ) frame->AddDebugText( parent->GetLocalToWorld().GetTranslation(), String::Printf( TXT( "%f\n" ), ratio ), false, Color::GREEN, Color::GREEN );
			}
			wrapper = world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetWrapperNext( wrapper );
		}
#endif //! USE_APEX
	}

#endif //! NO_EDITOR
}