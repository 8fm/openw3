/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#ifndef PHYSICS_BODY_WRAPPER_H
#define PHYSICS_BODY_WRAPPER_H

#include "../physics/physicsEngine.h"
#include "../physics/physicalCallbacks.h"
#include "../physics/physicalCollision.h"
#include "../physics/physicsWrapperPool.h"
#include "../physics/compiledCollision.h"

enum EPhysicsRigidBodyWrapperFlags
{
	PRBW_CollisionDisabled					= FLAG( 0 ),
	PRBW_DetailedConntactInfo				= FLAG( 1 ),
	PRBW_DisableGravity						= FLAG( 2 ),
	PRBW_PoseIsDirty						= FLAG( 3 ),
	PRBW_TrackKinematic						= FLAG( 4 ),
	PRBW_DisableBuoyancy					= FLAG( 5 ),
	PRBW_StateIsDirty						= FLAG( 6 ),
	PRBW_UpdateEntityPose					= FLAG( 7 ),
	PRBW_FlagsAreDirty						= FLAG( 8 ),

	PRBW_Reserved_1							= FLAG( 9 ),
	PRBW_Reserved_2							= FLAG( 10 ),
	PRBW_Reserved_3							= FLAG( 11 ),
	PRBW_Reserved_4							= FLAG( 12 ),
	PRBW_Reserved_5							= FLAG( 13 ),
	PRBW_Reserved_6							= FLAG( 14 ),
	PRBW_Reserved_7							= FLAG( 15 ),
	PRBW_Reserved_8							= FLAG( 16 ),
	PRBW_Reserved_9							= FLAG( 17 ),
	PRBW_Reserved_10						= FLAG( 18 ),
	PRBW_Reserved_11						= FLAG( 19 ),
	PRBW_Reserved_12						= FLAG( 20 ),	
	PRBW_Reserved_13						= FLAG( 21 ),
	PRBW_Reserved_14						= FLAG( 22 ),
	PRBW_Reserved_15						= FLAG( 23 ),
	PRBW_Reserved_16						= FLAG( 24 ),

	EPRW_SHAPE_FLAGS = PRBW_CollisionDisabled | PRBW_DetailedConntactInfo | PRBW_DisableGravity
};

enum ESimulationMode
{
	SM_DYNAMIC,
    SM_KINEMATIC,
	SM_STATIC
};

struct SWrapperContext
{
	Float m_x;
	Float m_y;
	Float m_resultDistanceSquared;
	Float m_desiredDistanceSquared;
	union 
	{
		struct  
		{
			Int32 m_visibilityQueryId;
		};
		struct  
		{
			Int8 m_visibilityQueryChunk[ 3 ];
			union 
			{
				struct 
				{
					Bool m_data0: 1;
					Bool m_data1: 1;
					Bool m_data2: 1;
					Bool m_data3: 1;
					Bool m_data4: 1;
					Bool m_data5: 1;
					Bool m_data6: 1;
					Bool m_requestProcessingFlag: 1;
				};
				struct  
				{
					Int8 m_visibilityQueryResult;
				};
			};
		};
	};
};

class IPhysicsWrapperParentProvider
{
	virtual IScriptable* GetParentObject() const = 0;
	virtual class CPhysicsWorld* GetPhysicsWorld() const = 0;

public:
	virtual void operator= (const IPhysicsWrapperParentProvider& other) = 0;

	template< typename T >
	Bool GetParent( T*& fill ) const { fill = Cast< T >( GetParentObject() ); return fill != 0; }

	template< typename T >
	Bool GetPhysicsWorld( T*& physicsWorld ) { physicsWorld = static_cast< T* >( GetPhysicsWorld() ); return physicsWorld != nullptr; }

	virtual Bool HasParent() const = 0;
	virtual const Matrix& GetLocalToWorld() const = 0;
	virtual const EngineTransform& GetTransform() const = 0;
	virtual Bool isRoot() = 0;

	virtual Uint32 GetVisibilityQuerty() = 0;
	virtual const String GetFriendlyName() = 0;

	virtual Bool SetRawPlacementNoScale( const Matrix& pose ) = 0;
	virtual	Bool ForceUpdateTransformWithGlobalPose( const Matrix& pose ) = 0;

	virtual void DataHalt( IScriptable* parent, const Char* category, const Char* reason ) = 0;
};

class CPhysicsWrapperInterface
{
    friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >;

protected:
	char m_parentHook[ 16 ];

	Int16										m_poolIndex;
    Red::Threads::CAtomic< Uint32 >				m_flags;
    ESimulationMode                             m_simulationType;
	CPhysicsEngine::CollisionMask				m_collisionType;
	CPhysicsEngine::CollisionMask				m_collisionGroup;

	Red::Threads::CAtomic< Int32 >				m_ref;
	struct SCallbackData
	{
		union
		{
			struct //part for script callback
			{
				CName m_scriptReciversOnEventName;
				THandle< IScriptable > m_scriptReciverObject;
			};
			struct //part for code callback
			{
				IPhysicalCollisionTriggerCallback* m_codeReceiverObject;
				THandle< IScriptable > m_parentObject;
			};
		};
		SCallbackData()
		{
			Red::System::MemorySet( &m_scriptReciverObject, 0, sizeof( m_scriptReciverObject ) );
			Red::System::MemorySet( &m_codeReceiverObject, 0, sizeof( m_codeReceiverObject ) );
		}

		SCallbackData( const SCallbackData& other )
		{
			Red::System::MemorySet( &m_scriptReciverObject, 0, sizeof( m_scriptReciverObject ) );
			Red::System::MemorySet( &m_codeReceiverObject, 0, sizeof( m_codeReceiverObject ) );

			m_scriptReciversOnEventName = other.m_scriptReciversOnEventName;
			m_scriptReciverObject = other.m_scriptReciverObject;
		}

		virtual ~SCallbackData()
		{

		}

		Bool isEmpty() { return m_codeReceiverObject == NULL && m_scriptReciverObject.Get() == NULL; }
	};
	TDynArray< SCallbackData >					m_callbacks;
	CPhysicsWorld*								m_world;

#ifndef NO_EDITOR
	StringAnsi									m_debugName;
#endif

protected:
	CPhysicsWrapperInterface( CPhysicalCollision collisionTypeAndGroup = CPhysicalCollision( 0, 0 ) );

	virtual ~CPhysicsWrapperInterface();

	virtual Bool MakeReadyToDestroy( TDynArray< void* >* toRemove ) = 0;

	virtual void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray<void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd ) = 0;
	virtual void PostSimulation( Uint8 visibilityQueryResult ) {}
	virtual void PostSimulationUpdateTransform( const Matrix& transform, void* actor ) {}

	void UpdateTriggerShapes();
	void UpdateMass( Uint32 actorIndex = 0, float densityScaler = 1.0f );
	void UpdateFlags();

	Bool ApplyBuoyancyForce( Uint32 actorIndex, float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep, float floatingRatio, Uint32 visibilityResult );
	void ApplyBuoyancyTorque( Uint32 actorIndex , const RedQuaternion & rotation, Uint32 visibilityResult );

	Int16 CreateScaledShape( Uint32 actorIndex, void* geometry, const CName* materialsNames, Uint16 materialsCount, const Matrix& localPose, const Vector& scale );

	virtual Bool SetOcclusionParameters( Uint32 actorIndex = 0, Float diagonalLimit = 1, Float attenuation = -1 ) { return false; }

	Bool RebuildCollision( Uint32 actorIndex, CompiledCollisionPtr collisionShape, const Vector& scale, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup );

public:
	virtual void Release( Uint32 actorIndex = 0 ) {}

	Int16 GetPoolIndex() const { return m_poolIndex; }

	RED_INLINE Bool IsStatic() const { return m_simulationType == SM_STATIC; }
	RED_INLINE Bool IsKinematic() const { return m_simulationType == SM_KINEMATIC; }
	virtual Bool IsReady() const { return false; }
	Bool IsToDestroy() const { return m_ref.GetValue() <= 0; }

	virtual IPhysicsWrapperParentProvider* GetParentProvider( Uint32 actorIndex = 0 ) const { return ( IPhysicsWrapperParentProvider* ) m_parentHook; }
	template< typename T >
	Bool GetParent( T*& fill, Uint32 actorIndex = 0 ) { return GetParentProvider( actorIndex )->GetParent( fill ); }

	template< typename T >
	Bool GetPhysicsWorld( T*& physicsWorld ) { physicsWorld = static_cast< T* >( m_world ); return physicsWorld != nullptr; }

	virtual Bool UpdatesEntity() { return false; }

	Bool GetFlag( EPhysicsRigidBodyWrapperFlags flag ) const;
	virtual void SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision );

	virtual Box GetWorldBounds( Int32 actorIndex = -1 );

	virtual Float GetMotionIntensity() { return -1.0f; }

	virtual Box CalcLocalBounds( Uint32 actorIndex );
	virtual Float GetDiagonal( Uint32 actorIndex );

	enum EPhysicalScriptCallbackType
	{
		EPSCT_OnTriggerFocusFound,
		EPSCT_OnTriggerFocusLost,
		EPSCT_OnCollision,
		EPSCT_COUNT
	};

	enum EPhysicalCodeCallbackType
	{
		EPCCT_OnCollision = EPSCT_COUNT,
		EPCCT_OnTriggerFocusFound,
		EPCCT_OnTriggerFocusLost,
		EPCCT_COUNT
	};

	void SetScriptCallback( EPhysicalScriptCallbackType type, const THandle< IScriptable >& object = NULL, CName onEventName = CNAME( Empty ) );
	void SetCodeCallback( EPhysicalCodeCallbackType type, IPhysicalCollisionTriggerCallback* callback, const THandle< IScriptable >& object );

	virtual void SwitchToKinematic( Bool decision );
	virtual void SwitchToStatic( );

	virtual Vector GetPosition( Uint32 actorIndex = 0 ) const;
	virtual Vector GetShapePosition( Uint32 actorIndex, Uint32 shapeIndex ) const;
    
	virtual Vector GetCenterOfMassPosition( Uint32 actorIndex = 0 ) const;
	virtual Float GetMass( Uint32 actorIndex = 0 ) const;
    virtual Vector GetLocalInertiaTensor( Uint32 actorIndex = 0 ) const;

	virtual Bool ApplyImpulse( const Vector& impulse, const Vector& point, Uint32 actorIndex = 0 );
	virtual Bool ApplyForce( const Vector& force, Uint32 actorIndex = 0 );
	virtual Bool ApplyForce( const Vector& force, const Vector& point, Uint32 actorIndex = 0 );
	virtual Bool ApplyTorque( const Vector& torque, Uint32 actorIndex = 0 );
	virtual Bool ApplyAcceleration( const Vector& force, Uint32 actorIndex = 0 );
	virtual Bool ApplyTorqueImpulse( const Vector& torque, Int32 actorIndex = 0 );

	virtual Int32 GetIndex( const char* actorName ) { return -1; }
	SPhysicalMaterial* GetMaterial( Int32 actorIndex = 0, Int32 shapeIndex = 0 );

	Bool SetVelocityLinear( const Vector& linear, Uint32 actorIndex = 0 );
	Bool SetVelocityAngular( const Vector& angular, Uint32 actorIndex = 0 );
	Bool SetMaxVelocityAngular( Float maxVelocity, Uint32 actorIndex = 0 );
	Bool GetVelocity( Vector& linear, Vector& angular, Uint32 actorIndex = 0 );
	Bool GetLinearVelocityAtPos( const Vector& pos, Vector & out, Uint32 actorIndex = 0 );

    Float GetDampingLinear( Uint32 actorIndex = 0 ) const;
    Float GetDampingAngular( Uint32 actorIndex = 0 ) const;

	Bool SetDampingLinear( float linear, Uint32 actorIndex = 0 );
	Bool SetDampingAngular( float angular, Uint32 actorIndex = 0 );

	virtual void SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 ) {}
	virtual Matrix GetPose( Uint32 actorIndex = 0 ) const { return Matrix::IDENTITY; }

	virtual Uint32 GetActorsCount() const { return 0; }
	virtual void* GetActor( Uint32 actorIndex = 0 ) const { return 0; }
	void* GetShape( Uint32 shapeIndex, Uint32 actorIndex = 0 ) const;
	Uint32 GetShapesCount( Uint32 actorIndex = 0 );

	CPhysicsEngine::CollisionMask GetCollisionTypesBits( Uint32 actorIndex, Uint32 shapeIndex ) const;
	CPhysicsEngine::CollisionMask GetCollisionTypesBits() const { return m_collisionType; }

	virtual Bool GetOcclusionParameters( Uint32 actorIndex = 0, Float* diagonalLimit = 0, Float* attenuation = 0 ) { return false; }

	virtual void OnContactModify( void* pair );

//removal
	virtual void SetMassPosition( const Vector& localPosition, Uint32 actorIndex = 0 );
	virtual void SetMass( Float mass, Uint32 actorIndex = 0 );
//removal

};


#endif
