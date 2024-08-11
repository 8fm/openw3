/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once						

#include "component.h"
#include "multiCurve.h"

//////////////////////////////////////////////////////////////////////////

enum EFixBonesHierarchyType
{
	FBHTAddMissingBones,
	FBHTRemoveDisconnectedBones,
};

BEGIN_ENUM_RTTI( EFixBonesHierarchyType );
	ENUM_OPTION( FBHTAddMissingBones );
	ENUM_OPTION( FBHTRemoveDisconnectedBones );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

class CParticleSystem;
class CEntity;

struct SDropPhysicsCurves
{
	DECLARE_RTTI_STRUCT( SDropPhysicsCurves );

	SMultiCurve	m_trajectory;
	SMultiCurve	m_rotation;

	void Init();
};

BEGIN_CLASS_RTTI( SDropPhysicsCurves )
	PROPERTY_CUSTOM_EDIT( m_trajectory, TXT( "Trajectory curve" ), TXT( "MultiCurveEditor3D" ) );
	PROPERTY_CUSTOM_EDIT( m_rotation, TXT( "Rotation curve" ), TXT( "MultiCurveEditor2D" ) );
END_CLASS_RTTI();

class CDropPhysicsSetup : public CObject
{
	DECLARE_ENGINE_CLASS( CDropPhysicsSetup, CObject, 0 );

	CName								m_name;
	TSoftHandle< CParticleSystem >		m_particles;
	TDynArray< SDropPhysicsCurves >		m_curves;

	const CName& GetName() const;
	SDropPhysicsCurves* SelectRandomCurves();
	void AttachCurves( CNode* parent );
	void DeleteCurveEditors( CWorld* world );

	void OnPropertyPostChange( IProperty* property ) override;

private:

	void InitCurves();
};

BEGIN_CLASS_RTTI( CDropPhysicsSetup )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_name, TXT( "Name" ) );
	PROPERTY_EDIT( m_particles, TXT( "Spawned particles" ) );
	PROPERTY_EDIT( m_curves, TXT( "Curves" ) )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDropPhysicsComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CDropPhysicsComponent, CComponent, 0 );

public:

	struct SDropPhysicsInfo
	{
		enum EDropPhysicsInfoFlags : Uint32
		{
			DPIF_DespawnAlongWithBase		= FLAG( 0 ),	// entity should be despawned along with base entity/component
			DPIF_NormalizeCurves			= FLAG( 1 ),	// normalize curve point at time 0, so that its value starts at 0 as well
			DPIF_DoNotUpdate				= FLAG( 2 ),	// entity should be "owned" by the component but not updated
		};

		enum EDropPhysicsInitialPositionType
		{
			DPIPT_Entity,
			DPIPT_CenterOfMass,
		};

		Vector					m_direction;
		CName					m_curveName;
		Uint32					m_flags;
		mutable SMultiCurve*	m_trajectoryCurve;
		mutable SMultiCurve*	m_rotationCurve;
		mutable Vector			m_initialPosition;
		mutable EulerAngles		m_initialRotation;
		mutable EDropPhysicsInitialPositionType	m_initialPositionType;
		Vector					m_centerOfMassOffsetLS;
		Vector					m_initialCenterOfMassOffsetWS;

		SDropPhysicsInfo();
	};

	struct SMappedBones
	{
		THashSet< String >	m_bones;

		SMappedBones()
		{}

		void Clear()
		{
			m_bones.Clear();
		}
	};

	enum EDisableRagdollType
	{
		DRT_Both,
		DRT_Spawned,
		DRT_Base,
	};

	struct SDisableRagdollInfo
	{
		EDisableRagdollType		m_type;
		CEntity*				m_spawnedEntity;
		CEntity*				m_baseEntity;
		SMappedBones*			m_spawnedEntityBones;
		THashSet< String >		m_baseEntityBonesToSkip;
		EFixBonesHierarchyType	m_fixBaseBonesHierarchyType;
		EFixBonesHierarchyType	m_fixSpawnedBonesHierarchyType;

		SDisableRagdollInfo();
	};

private:

	struct SDroppedEntityPhysicsWrapper
	{
	private:

		enum EDroppedEntityPhysicsWrapperFlags
		{
			DEPWF_Kinematic						= FLAG( 0 ),
			DEPWF_SwitchedToKinematic			= FLAG( 1 ),
			DEPWF_ForcesApplied					= FLAG( 2 ),
			DEPWF_WaitingForComponents			= FLAG( 3 ),
			DEPWF_WaitingForRagdollWrapper		= FLAG( 4 ),
			DEPWF_WaitingForRigidBodyWrapper	= FLAG( 5 ),
			DEPWF_WaitingForWrapper				= DEPWF_WaitingForComponents | DEPWF_WaitingForRagdollWrapper | DEPWF_WaitingForRigidBodyWrapper,
			DEPWF_HasRagdoll					= FLAG( 6 ),
			DEPWF_OffsetsCached					= FLAG( 7 ),
		};

	public:

		SDroppedEntityPhysicsWrapper( CEntity* entity );

		void SetEntity( CEntity* entity );
		void Update();
		void SwitchToKinematic( Bool kinematic );
		void ApplyForces( const Vector& linearVelocity, const Vector& angularVelocity );
		void InitCenterOfMassOffsets( const SDropPhysicsInfo& info );
		Vector GetCenterOfMassOffsetLS( SDropPhysicsInfo& info );
		Vector GetInitialCenterOfMassOffsetWS( SDropPhysicsInfo& info );
		Uint32 GetValidActorIndex();

		RED_FORCE_INLINE CPhysicsWrapperInterface* Get() const
		{
			return m_physicsInterface;
		}

		RED_FORCE_INLINE Bool IsKinematic() const
		{
			return ( m_flags & DEPWF_Kinematic );
		}

	private:

		THandle< CEntity >			m_entity;
		CPhysicsWrapperInterface*	m_physicsInterface;
		Uint32						m_flags;
		Vector						m_linearVelocity;
		Vector						m_angularVelocity;
		Int32						m_validActorIndex;

		void Init();
		Bool ShouldSwitchToKinematic() const;
		void SwitchToKinematicInternal();
		Bool ShouldApplyForces() const;
		void ApplyForcesInternal();
	};

	struct SDroppedEntity
	{
		THandle< CEntity >				m_entity;
		SDroppedEntityPhysicsWrapper	m_physicsWrapper;	
		Float							m_timer;
		Matrix							m_curveSpace;		
		SDropPhysicsInfo				m_dropInfo;		
		Vector							m_positionAt0;	// used for position normalization
		Vector							m_rotationAt0;	// used for rotation normalization

		SDroppedEntity();
		SDroppedEntity( CEntity* entity, const SDropPhysicsInfo& dropInfo );
		SDroppedEntity( CRigidMeshComponent* rigidMesh, const SDropPhysicsInfo& dropInfo );

		void Init();
		Bool Update( const Vector& baseEntityWorldPos, Float timeDelta );
		void GetCoordinatesAtTime( Float time, Vector& outEntityPosition, EulerAngles& outEntityRotation, Vector* outCenterOfMassPosition = nullptr, EulerAngles* outRotationDelta = nullptr );
		void OnBaseComponentDetached();
		Vector GetCurvePosition( Float time );
		Vector GetCurveRotation( Float time );
		Float GetCurveDuration();

		void DrawDebug( CRenderFrame* frame, EShowFlags flag );

		static const Float DESTROY_RANGE_SQUARE;
	};

	struct SRagdollTask
	{
		SRagdollTask();
		SRagdollTask( CAnimatedComponent* animatedComponent );
		virtual ~SRagdollTask(){}
		Bool Update();

	protected:

		CAnimatedComponent*				m_animatedComponent;
		class CPhysicsRagdollWrapper*	m_physicsWrapper;

		virtual Bool UpdateRagdoll() = 0;
	};

	struct SDisableRagdollParts : public SRagdollTask
	{
		SDisableRagdollParts();
		SDisableRagdollParts( CAnimatedComponent* animatedComponent, const THashSet< String > & allExceptBones );
		virtual Bool UpdateRagdoll() override;

	protected:

		TDynArray< Uint32 >		m_boneIndices;
	};

public:

	CDropPhysicsComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnTick( Float timeDelta ) override;
	virtual void OnPropertyPostChange( IProperty* property ) override;
	virtual bool UsesAutoUpdateTransform() override { return false; }

	Bool DropMeshByName( const String& name, const SDropPhysicsInfo& dropInfo );
	Bool DropMeshByTag( const CName& tag, const SDropPhysicsInfo& dropInfo );
	Bool DropExternalEntity( CEntity* entity, const SDropPhysicsInfo& dropInfo );

	void AddDropPhysicsSetup( CDropPhysicsSetup* dropPhysicsSetup );
	void RemoveDropPhysicsSetup( const CName& dropPhysicsSetupName );


	// Disable spawned entity's ragdoll parts that are not contained in mesh' bones list
	// Additionally disables base character's ragdoll parts that are contained in spawned entity mesh' bones list.
	// If baseEntity is nullptr, the current component's parent entity will be used.
	// Optionally one may pass already collected mapped bones list.
	Bool DisableRagdollParts( const SDisableRagdollInfo& info );

	// Collect all bones mapped in meshes attached to CAnimatedComponent along with
	// all bones that are not mapped but important for skeleton hierarchy (connecting
	// two mapped bones).
	static void CollectMappedBones( CAnimatedComponent* animatedComponent, SMappedBones& mappedBones, EFixBonesHierarchyType fixType );

	// If mapped bones contains disjoint parts (for example 'torso' and 'hand' are mapped, but there's no 'arm' connecting them)
	// we need to fix hierarchy either by adding missing bones (fixType == FBHTAddMissingBones) or by removing disjoint parts which
	// are not connected to the root (fixType == FBHTRemoveDisjointParts).
	static void FixMappedBonesHierarchy( CAnimatedComponent* animatedComponent, SMappedBones& mappedBones, THashSet< Int32 > * usedBonesIndicies = nullptr, EFixBonesHierarchyType fixType = FBHTAddMissingBones );

	// Calculate center of mass
	// If entity has ragdoll, value will be computed based on ragdoll actors (optionally filtered with specified actors/bones names),
	// otherwise actorIndex will be used to obtain center of mass of a single actor.
	static Vector GetCenterOfMassPosition( const CEntity* entity, const CPhysicsWrapperInterface* physicsInterface, Uint32 actorIndex = 0, THashSet< String > * usedBones = nullptr );

	// Calculate "center of mass offset", which is a vector from the entity position to the computed center of mass
	// Optionally one may pass additional rotation which can be used to compute rotation in global space (instead of the "current" one)
	static Bool GetCenterOfMassOffsets( const CEntity* entity, const CPhysicsWrapperInterface* physicsInterface, Vector& centerOfMassOffsetLS, Vector& centerOfMassOffsetWS,
										Uint32 actorIndex = 0, THashSet< String > * usedBones = nullptr, EulerAngles* relativeRotation = nullptr );

	// debug
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	//! CComponent
	virtual void OnInitialized() override;

	//! CObject
	virtual void OnFinalize() override;
	virtual void OnDetachFromEntityTemplate() override;

private:

	void UpdateDroppedEntities( Float timeDelta );
	void UpdateRagdollTasks();

	Bool DetachAndDropMesh( CMeshComponent* meshComponent, const SDropPhysicsInfo& dropInfo );

	CDropPhysicsSetup* GetDropSetup( const CName& setupName ) const;
	Bool UpdateCurves( const SDropPhysicsInfo& dropInfo ) const;
	Bool CreateParticles( const SDropPhysicsInfo& dropInfo ) const;

	typedef TDynArray< THandle< CDropPhysicsSetup > > TDropSetups;

	TDynArray< SDroppedEntity >		m_droppedEntities;
	TDropSetups						m_dropSetups;
	TDynArray< SRagdollTask* >		m_ragdollTasks;

public:

	void funcDropMeshByName( CScriptStackFrame& stack, void* result );
	void funcDropMeshByTag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDropPhysicsComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_INLINED( m_dropSetups, TXT( "Setups" ) );
	NATIVE_FUNCTION( "DropMeshByName", funcDropMeshByName );
	NATIVE_FUNCTION( "DropMeshByTag", funcDropMeshByTag );
END_CLASS_RTTI();
