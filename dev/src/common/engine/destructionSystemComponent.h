/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "ApexDestructionWrapper.h"
#include "drawableComponent.h"
#include "pathlib.h"
#include "pathlibComponent.h"
#include "destructionSystemComponentPresets.h"
#include "apexResource.h"

#ifdef USE_APEX

namespace physx
{
	namespace apex
	{
		class NxDestructibleAsset;
		class NxDestructibleActor;
	}
}

#endif

struct SDestructionParameters 
{
	THandle< CApexResource >					m_resource;
	TDynArray< THandle< CMaterialGraph > >		m_materials;

	CPhysicalCollision							m_physicalCollisionType;
	CPhysicalCollision							m_fracturedPhysicalCollisionType;
	Matrix										m_pose;

	EDispatcherSelection						m_dispacherSelection;
	Bool										m_dynamic;
	Vector										m_scale;
	Float										m_damageCap;
	Float										m_damageThreshold;
	Float										m_damageToRadius;
	Int32										m_debrisDepth;
	Float										m_debrisDestructionProbability;
	Float										m_debrisLifetimeMin;
	Float										m_debrisLifetimeMax;
	Float										m_debrisMaxSeparationMin;
	Float										m_debrisMaxSeparationMax;
	Float										m_fadeOutTime;
	Uint32										m_essentialDepth;
	Float										m_forceToDamage;
	Float										m_fractureImpulseScale;
	Int32										m_impactDamageDefaultDepth;
	Float										m_impactVelocityThreshold;
	Float										m_materialStrength;
	Float										m_maxChunkSpeed;
	Uint32										m_minimumFractureDepth;
	Bool										m_useStressSolver;
	Float										m_stressSolverTimeDelay;
	Float										m_stressSolverMassThreshold;
	Uint32										m_supportDepth;
	Bool										m_useAssetDefinedSupport;
	Bool										m_useWorldSupport;
	Float										m_sleepVelocityFrameDecayConstant;
	Bool										m_useHardSleeping;
	Bool										m_accumulateDamage;
	Bool										m_debrisTimeout;
	Bool										m_debrisMaxSeparation;
	Bool										m_crumbleSmallestChunks;
	Bool										m_usePreviewAsset;

	SDestructionParameters();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDestructionSystemComponent class - destructible component
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDestructionSystemComponent : public CDrawableComponent, public IPhysicalCollisionTriggerCallback, public PathLib::IObstacleComponent
{
	DECLARE_ENGINE_CLASS( CDestructionSystemComponent, CDrawableComponent, 0 )

protected:
	SDestructionParameters						m_parameters;
	EDestructionPreset							m_preset;
	
	CName										m_targetEntityCollisionScriptEventName;
	CName										m_parentEntityCollisionScriptEventName;

	EPathLibCollision							m_pathLibCollisionType;
	Bool										m_disableObstacleOnDestruction;
	Bool										m_wasDestroyed;										// Gotta think how and when it should be serialized
	TDynArray< IPerformableAction* >			m_eventOnDestruction;
	Uint32										m_lastAmontOfBaseFractures;

#ifdef USE_APEX
	CApexDestructionWrapper*					m_wrapper;
#endif

	Float										m_shadowDistanceOverride;


	void										UpdateObstacle();

	Bool										IsObstacleDisabled() const 							{ return m_wasDestroyed && m_disableObstacleOnDestruction; }

	virtual void								SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup ) override;
	virtual EPathLibCollision					GetPathLibCollisionGroup() const override;			// PathLib::IObstacleComponent interface
	virtual CComponent*							AsEngineComponent() override;						// PathLib::IComponent interface
	virtual PathLib::IComponent*				AsPathLibComponent() override;						// PathLib::IComponent interface

public:
	CDestructionSystemComponent();
	~CDestructionSystemComponent();

	Float										GetShadowDistance( Uint8& outRenderMask ) const;

	virtual Bool								IsDynamicGeometryComponent() const { return true; }

	virtual void								OnAttached( CWorld* world ) override;

	virtual void								OnDetached( CWorld* world ) override;

	virtual void								RefreshRenderProxies();

	virtual Bool								CanAttachToRenderScene() const;

#ifndef NO_EDITOR
	virtual void								EditorOnTransformChanged() override;
	virtual void								EditorOnTransformChangeStop() override;
	virtual void								EditorPreDeletion() override;

	virtual void								PostNavigationCook( CWorld* world ) override;
	virtual void								OnNavigationCook( CWorld* world, CNavigationCookingContext* cookerData ) override;
#endif

	virtual void								OnUpdateBounds() override;

#ifndef NO_EDITOR_FRAGMENTS
	virtual void								OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
#endif

	virtual void								OnTickPostPhysics( Float timeDelta ) override;
	
	void										ScheduleTick( Uint32 fractureAmount );

#ifdef USE_APEX

	RED_INLINE Bool								HasResource() const { return m_parameters.m_resource.Get() != NULL; }

	RED_INLINE const TDynArray< String >&		GetApexMaterialNames() const { return m_parameters.m_resource.Get()->GetApexMaterialNames(); }
	RED_INLINE const CMeshTypeResource::TMaterials&	GetMaterials() const { return m_parameters.m_resource.Get()->GetMaterials(); }
	virtual Float								GetAutoHideDistance() const { return m_parameters.m_resource.Get() ? m_parameters.m_resource.Get()->GetAutoHideDistance() : GetDefaultAutohideDistance(); }
	virtual Float								GetDefaultAutohideDistance() const { return 20.0f; }
	virtual Float								GetMaxAutohideDistance() const { return 300.0f; }

	// Set resource
	virtual void								SetResource( CResource* resource ) override;
	virtual void								GetResource( TDynArray< const CResource* >& resources ) const override;

	CApexResource*								GetApexResource() const { return m_parameters.m_resource.Get(); }
	class CPhysicsWrapperInterface*				GetPhysicsRigidBodyWrapper() const { return m_wrapper; }
	class CApexDestructionWrapper*				GetDestructionBodyWrapper() const { return m_wrapper; }
	const SDestructionParameters&				GetParameters() const { return m_parameters; }
#endif	

#ifndef NO_EDITOR
	virtual void								OnPropertyPostChange( IProperty* property ) override;

	virtual Uint32								GetMinimumStreamingDistance() const override;

	/// Set whether to use a preview asset, or the saved asset. Defaults to false (use saved asset)
	void										SetUsePreview( Bool usePreview );

	/// Reset the physics actor, by recreating it. This effectively "unbreaks" it, allowing for quick iteration on actor properties.
	void										Reset();

	/// Load preset values in editor
	void										LoadPresetParams( Uint32 ind );
#endif

	void										ApplyForce( const Vector& force, const Vector& point, Float deltaTime );

	void										SetEnabled( Bool enabled );

	EPathLibCollision							GetPathLibCollisionType() const						{ return m_pathLibCollisionType; }

	virtual void								OnSaveGameplayState( IGameSaver* saver ) override;
	virtual void								OnLoadGameplayState( IGameLoader* loader ) override;
	virtual	Bool								CheckShouldSave() const override { return true; }
	
	// Called after the component has been created from the streaming system
	virtual void OnStreamIn() override;

protected:
	void										InitWrapper();
	void										ReleaseWrapper();

	virtual void								onCollision( const SPhysicalCollisionInfo& info ) override;
	
	void										funcIsDestroyed( CScriptStackFrame& stack, void* result );
	void										funcIsObstacleDisabled( CScriptStackFrame& stack, void* result );
	void										funcGetFractureRatio( CScriptStackFrame& stack, void* result );
	void										funcApplyFracture( CScriptStackFrame& stack, void* result );
	void										funcApplyForce( CScriptStackFrame& stack, void* result );
	void										funcApplyDamageAtPoint( CScriptStackFrame& stack, void* result );
	void										funcApplyRadiusDamage( CScriptStackFrame& stack, void* result );
};

// rtti properties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CDestructionSystemComponent );
	PARENT_CLASS( CDrawableComponent );

	PROPERTY_EDIT_NAME( m_parameters.m_resource, TXT( "m_resource" ), TXT( "" ) )

	PROPERTY_EDIT_NAME( m_targetEntityCollisionScriptEventName, TXT( "targetEntityCollisionScriptName" ), TXT( "Script function called on colliding entity while contact " ) )
	PROPERTY_EDIT_NAME( m_parentEntityCollisionScriptEventName, TXT( "parentEntityCollisionScriptEventName" ), TXT( "Script function called on parent entity while contact " ) )
	
	PROPERTY_CUSTOM_EDIT_ARRAY( m_parameters.m_materials, TXT("materials"), TXT("") )
	PROPERTY_CUSTOM_EDIT_NAME( m_parameters.m_physicalCollisionType, TXT( "m_physicalCollisionType" ), TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_CUSTOM_EDIT_NAME( m_parameters.m_fracturedPhysicalCollisionType, TXT( "m_fracturedPhysicalCollisionType" ), TXT( "Defines what of types it is from physical collision point of view for fractured elements" ), TXT("PhysicalCollisionTypeSelector") );

	PROPERTY_EDIT_NAME( m_parameters.m_dispacherSelection, TXT( "dispacher selection" ), TXT( "" ) )

	PROPERTY_EDIT_NAME( m_parameters.m_dynamic, TXT( "dynamic" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_supportDepth, TXT( "supportDepth" ), TXT( " The chunk hierarchy depth at which to create a support graph. The chunk hierarchy depth at which to create a support graph. Higher depth levels give more detailed support, but will give a higher computational load. Chunks below the support depth will never be supported. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_useAssetDefinedSupport, TXT( "useAssetDefinedSupport" ), TXT( " If set, then chunks which are tagged as 'support' chunks If set, then chunks which are tagged as 'support' chunks (via NxDestructibleChunkDesc::isSupportChunk) will have environmental support in static destructibles. Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as 'support' chunks AND overlap the NxScene's static geometry in order to be environmentally supported. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisDepth, TXT( "debrisDepth" ), TXT( "The chunk hierarchy depth at which chunks are considered to be \"debris.\" Chunks at this depth or below will be considered for various debris settings, such as debrisLifetime. Negative values indicate that no chunk depth is considered debris. Default value is -1. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_essentialDepth, TXT( "essentialDepth" ), TXT( "The chunk hierarchy depth up to which chunks will not be eliminated due to LOD considerations. These chunks are considered to be essential either for gameplay or visually. The minimum value is 0, meaning the level 0 chunk is always considered essential. Default value is 0. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisTimeout, TXT( "debrisTimeout" ), TXT( " Whether or not chunks at or deeper than the 'debris' depth will timeout. Whether or not chunks at or deeper than the 'debris' depth (see NxDestructibleParameters::debrisDepth) will time out. The lifetime is a value between NxDestructibleParameters::debrisLifetimeMin and NxDestructibleParameters::debrisLifetimeMax, based upon the destructible module's LOD setting. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisLifetimeMin, TXT( "debrisLifetimeMin" ), TXT( "'Debris chunks' (see debrisDepth, above) will be destroyed after a time (in seconds) separated from non-debris chunks. The actual lifetime is interpolated between debrisLifetimeMin (see above) and debrisLifetimeMax, based upon the module's LOD setting. To disable lifetime, clear the NxDestructibleDepthParametersFlag::DEBRIS_TIMEOUT flag in the flags field. If debrisLifetimeMax < debrisLifetimeMin, the mean of the two is used for both. Default debrisLifetimeMin = 1.0, debrisLifetimeMax = 10.0f. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisLifetimeMax, TXT( "debrisLifetimeMax" ), TXT( "'Debris chunks' (see debrisDepth, above) will be destroyed after a time (in seconds) separated from non-debris chunks. The actual lifetime is interpolated between debrisLifetimeMin (see above) and debrisLifetimeMax, based upon the module's LOD setting. To disable lifetime, clear the NxDestructibleDepthParametersFlag::DEBRIS_TIMEOUT flag in the flags field. If debrisLifetimeMax < debrisLifetimeMin, the mean of the two is used for both. Default debrisLifetimeMin = 1.0, debrisLifetimeMax = 10.0f. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisMaxSeparation, TXT( "debrisMaxSeparation" ), TXT( " Whether or not chunks at or deeper than the 'debris' depth will be removed if separated too far. Whether or not chunks at or deeper than the 'debris' depth (see NxDestructibleParameters::debrisDepth) will be removed if they separate too far from their origins. The maxSeparation is a value between NxDestructibleParameters::debrisMaxSeparationMin and NxDestructibleParameters::debrisMaxSeparationMax, based upon the destructible module's LOD setting. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisMaxSeparationMin, TXT( "debrisMaxSeparationMin" ), TXT( "Debris chunks (see debrisDepth, above) will be destroyed after a time (in seconds) separated from non-debris chunks. The actual lifetime is interpolated between these two values, based upon the module's LOD setting. To disable lifetime, clear the NxDestructibleDepthParametersFlag::DEBRIS_TIMEOUT flag in the flags field. If debrisLifetimeMax < debrisLifetimeMin, the mean of the two is used for both. Default debrisLifetimeMin = 1.0, debrisLifetimeMax = 10.0f. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_debrisMaxSeparationMax, TXT( "debrisMaxSeparationMax" ), TXT( "Debris chunks (see debrisDepth, above) will be destroyed after a time (in seconds) separated from non-debris chunks. The actual lifetime is interpolated between these two values, based upon the module's LOD setting. To disable lifetime, clear the NxDestructibleDepthParametersFlag::DEBRIS_TIMEOUT flag in the flags field. If debrisLifetimeMax < debrisLifetimeMin, the mean of the two is used for both. Default debrisLifetimeMin = 1.0, debrisLifetimeMax = 10.0f. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_fadeOutTime, TXT( "fadeOutTime" ), TXT( "The time it takes for a chunk for fade out when destroyed from Timeout or MaxSeparation. Default = 1.0f" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_minimumFractureDepth, TXT( "minimumFractureDepth" ), TXT( "The chunks will not be broken free below this depth." ) )
	
	PROPERTY_CUSTOM_EDIT( m_preset, TXT("Destruction preset behavior"), TXT("EdEnumRefreshableEditor") );
	PROPERTY_EDIT_NAME( m_parameters.m_debrisDestructionProbability, TXT( "debrisDestructionProbability" ), TXT( "The probability that a debris chunk, when fractured, will simply be destroyed instead of becoming dynamic or breaking down further into child chunks. Valid range = [0.0,1.0]. Default value = 0.0.'" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_crumbleSmallestChunks, TXT( "crumbleSmallestChunks" ), TXT( "  If set, the smallest chunks may be further broken down. If set, the smallest chunks may be further broken down, either by fluid crumbles (if a crumble particle system is specified in the NxDestructibleActorDesc), or by simply removing the chunk if no crumble particle system is specified. Note: the 'smallest chunks' are normally defined to be the deepest level of the fracture hierarchy. However, they may be taken from higher levels of the hierarchy if NxModuleDestructible::setMaxChunkDepthOffset is called with a non-zero value. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_accumulateDamage, TXT( "accumulateDamage" ), TXT( " Determines if chunks accumulate damage. If set, chunks will 'remember' damage applied to them, so that many applications of a damage amount below damageThreshold will eventually fracture the chunk. If not set, a single application of damage must exceed damageThreshold in order to fracture the chunk. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_damageCap, TXT( "damageCap" ), TXT( "Limits the amount of damage applied to a chunk. This is useful for preventing the entire destructible from getting pulverized by a very large application of damage. This can easily happen when impact damage is used, and the damage amount is proportional to the impact force (see forceToDamage). " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_damageThreshold, TXT( "damageThreshold" ), TXT( "The damage amount which will cause a chunk to fracture (break free) from the destructible. This is obtained from the damage value passed into the NxDestructibleActor::applyDamage, or NxDestructibleActor::applyRadiusDamage, or via impact (see 'forceToDamage', below)." ) )
	PROPERTY_EDIT_NAME( m_parameters.m_damageToRadius, TXT( "damageToRadius" ), TXT( "Controls the distance into the destructible to propagate damage. The damage applied to the chunk is multiplied by damageToRadius, to get the propagation distance. All chunks within the radius will have damage applied to them. The damage applied to each chunk varies with distance to the damage application position. Full damage is taken at zero distance, and zero damage at the damage radius. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_forceToDamage, TXT( "forceToDamage" ), TXT( "If a chunk is at a depth which takes impact damage (see NxDestructibleDepthParameters), then when a chunk has a collision in the NxScene, it will take damage equal to forceToDamage multiplied by the impact force. The default value is zero, which effectively disables impact damage. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_fractureImpulseScale, TXT( "fractureImpulseScale" ), TXT( "Scale factor used to apply an impulse force along the normal of chunk when fractured. This is used in order to \"push\" the pieces out as they fracture." ) )
	PROPERTY_EDIT_NAME( m_parameters.m_impactDamageDefaultDepth, TXT( "impactDamageDefaultDepth" ), TXT( "The default depth to which chunks will take impact damage. This default may be overridden in the depth settings. Negative values imply no default impact damage. Default value = -1. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_impactVelocityThreshold, TXT( "impactVelocityThreshold" ), TXT( "Large impact force may be reported if rigid bodies are spawned inside one another. In this case the relative velocity of the two objects will be low. This variable allows the user to set a minimum velocity threshold for impacts to ensure that the objects are moving at a min velocity in order for the impact force to be considered. Default value is zero. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_materialStrength, TXT( "materialStrength" ), TXT( "When a chunk takes impact damage due to physical contact (see NxDestructibleDepthParameters), this parameter is the maximum impulse the contact can generate. Weak materials such as glass may have this set to a low value, so that heavier objects will pass through them during fracture. N.B.: Setting this parameter to 0 disables the impulse cap; that is, zero is interpreted as infinite. Default value = 0.0f." ) )
	PROPERTY_EDIT_NAME( m_parameters.m_maxChunkSpeed, TXT( "maxChunkSpeed" ), TXT( "If greater than 0, the chunks' speeds will not be allowed to exceed maxChunkSpeed. Use 0 to disable this feature (this is the default)." ) )
	PROPERTY_EDIT_NAME( m_parameters.m_useWorldSupport, TXT( "useWorldSupport" ), TXT( " If set, then chunks which overlap the NxScene's static geometry will have environmental support If set, then chunks which overlap the NxScene's static geometry will have environmental support in static destructibles. Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as 'support' chunks AND overlap the NxScene's static geometry in order to be environmentally supported. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_useHardSleeping, TXT( "useHardSleeping" ), TXT( "  If true, turn chunk islands kinematic when they sleep. These islands may be turned dynamic again if enough damage is applied. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_useStressSolver, TXT( "useStressSolver" ), TXT( " If true, the structure containing this actor will use the stress solver. Determines whether to invoke the use of the stress solver. The stress solver is a self-checking mechanism employed within the structure, with the purpose of detecting and breaking off overly-strained links to masses of chunks. Its behavior can be tweaked by customizing the parameters stressSolverTimeDelay and stressSolverMassThreshold. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_stressSolverTimeDelay, TXT( "stressSolverTimeDelay" ), TXT( " The structure containing this actor will use the minimum stressSolverTimeDelay of all actors in the structure. Determines the amount of time to run down until an identified overly-strained link breaks. From the time the stress solver qualifies a link as being overly-strained, this value will be used to count down to the actual breaking-off event being executed. This should always be some positive value. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_stressSolverMassThreshold, TXT( "stressSolverMassThreshold" ), TXT( " The structure containing this actor will use the minimum stressSolverMassThreshold of all actors in the structure. Determines the minimum threshold mass to meet before an identified overly-strained link breaks. This mass threshold is part of the condition that the stress solver uses to qualify whether a link is overly-strained. The accumulated sum of the chunk masses that the link is supporting will be used against this value. This should always be some positive value. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_sleepVelocityFrameDecayConstant, TXT( "sleepVelocityFrameDecayConstant" ), TXT( " Frame memory decay constant used for actor velocity smoothing. Valid range: [1.0,infinity). Frame memory decay constant used for actor velocity smoothing. Valid range: [1.0,infinity). Roughly speaking, the number of frames for which past velocities have significance. A value of 1.0 (or less) gives no smoothing. " ) )

	PROPERTY_INLINED( m_eventOnDestruction, TXT("Actions to perform on destruction") )
	PROPERTY_EDIT( m_pathLibCollisionType, TXT("Collision type for navigation system. PLC_Dynamic is collision type that allows navigation obstacle to be removed in runtime.") )
	PROPERTY_EDIT( m_disableObstacleOnDestruction, TXT("Should navigation obstacle be removed on destruction. Works only with PLC_Dynamic and PLC_Immediate collision types.") )

	PROPERTY_EDIT( m_shadowDistanceOverride, TXT("If < 0, get a default from the resource.") );

	NATIVE_FUNCTION( "IsDestroyed", funcIsDestroyed );
	NATIVE_FUNCTION( "IsObstacleDisabled", funcIsObstacleDisabled );
	NATIVE_FUNCTION( "GetFractureRatio", funcGetFractureRatio );
	NATIVE_FUNCTION( "ApplyFracture", funcApplyFracture );
	NATIVE_FUNCTION( "ApplyForce", funcApplyForce );
	NATIVE_FUNCTION( "ApplyDamageAtPoint", funcApplyDamageAtPoint );
	NATIVE_FUNCTION( "ApplyRadiusDamage", funcApplyRadiusDamage );
END_CLASS_RTTI();
