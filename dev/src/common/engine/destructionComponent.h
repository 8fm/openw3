/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "pathlibComponent.h"
#include "..\physics\physicalCallbacks.h"
#include "physicsDestructionWrapper.h"
#include "pathlib.h"
#include "meshTypeComponent.h"
#include "collisionCache.h"

#ifndef PHYSICS_DESTRUCTION_COMPONENT_H
#define PHYSICS_DESTRUCTION_COMPONENT_H


class CDestructionComponent : public CMeshTypeComponent, public IPhysicalCollisionTriggerCallback, public PathLib::IObstacleComponent, public ICollisionCacheCallback
{
	DECLARE_ENGINE_CLASS( CDestructionComponent, CMeshTypeComponent, 0 )

protected:
	SPhysicsDestructionParameters				m_parameters;

	CPhysicsDestructionWrapper*					m_wrapper;

	CName										m_targetEntityCollisionScriptEventName;
	CName										m_parentEntityCollisionScriptEventName;

	EPathLibCollision							m_pathLibCollisionType;
	Bool										m_disableObstacleOnDestruction;
	Bool										m_wasDestroyed;										

	CompiledCollisionPtr						m_compiledCollisionBase;
	CompiledCollisionPtr						m_compiledCollisionFractured;

	TDynArray< IPerformableAction* >			m_eventOnDestruction;
	StringAnsi									m_fractureSoundEvent;
	CName										m_fxName;	

	IRenderSkinningData*						m_skinningData;										// Render skinning data
	Uint16										m_skinningDataBoneCount;							// Number of bones in the skinning data
public:
												CDestructionComponent(void);
												~CDestructionComponent(void);

	virtual void								OnAttached( CWorld* world ) override;
	virtual void								OnStreamIn() override;
	virtual void								OnStreamOut() override;
	virtual void								OnDetached( CWorld* world ) override;
	virtual void								OnTickPostPhysics( Float timeDelta ) override;
	void										ScheduleTickPostPhysics();
	void										ScheduleFindCompiledCollision();

	virtual void								RefreshRenderProxies() override;
	virtual void								OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	Bool										UpdateSkinning();
	Bool										EnableDissolve( Bool enable );
	void										ComputeSkinningData( );

	Bool										ShouldScheduleFindCompiledCollision() const;
	Bool										IsFractured() const;
	void										InitWrapper();

	// IPhysicalCollisionTriggerCallback interface
	virtual void								onCollision(const SPhysicalCollisionInfo& info);

	// PathLib::IObstacleComponent interface
	virtual EPathLibCollision					GetPathLibCollisionGroup() const;
	EPathLibCollision							GetPathLibCollisionType();
	virtual void								SetPathLibCollisionGroupInternal(EPathLibCollision collisionGroup);
	
	// PathLib::IComponent interface
	virtual CComponent*							AsEngineComponent() override;						
	virtual PathLib::IComponent*				AsPathLibComponent() override;	

	// ICollisionCacheCallback
	virtual void								OnCompiledCollisionFound( CompiledCollisionPtr collision ) override;
	virtual void								OnCompiledCollisionInvalid() override;

	virtual void								OnUpdateBounds();

#ifdef NO_EDITOR
	const SPhysicsDestructionParameters&		GetParameters() const { return m_parameters; }
#else
	SPhysicsDestructionParameters&				GetParameters() { return m_parameters; }
#endif
	CPhysicsWrapperInterface*					GetPhysicsRigidBodyWrapper() const { return m_wrapper; }
	CPhysicsDestructionWrapper*					GetDestructionBodyWrapper() const { return m_wrapper; }

	virtual CMeshTypeResource*					GetMeshTypeResource() const override;
	virtual CMesh*								TryGetMesh() const;

	CompiledCollisionPtr						GetCompCollisionBase();
	CompiledCollisionPtr						GetCompCollisionFractured();

#ifndef NO_EDITOR
	virtual void								EditorOnTransformChanged() override;
	void										EditorRecreateCollision();
	virtual void								EditorOnTransformChangeStop() override;
	virtual void								EditorPreDeletion() override;

	virtual void								OnSelectionChanged() override;

	virtual void								PostNavigationCook( CWorld* world ) override;
	virtual void								OnNavigationCook( CWorld* world, CNavigationCookingContext* cookerData ) override;

	void										SetPreviewResource(CPhysicsDestructionResource* physRes, CWorld* world);
#endif

	virtual void								OnSaveGameplayState( IGameSaver* saver ) override;
	virtual void								OnLoadGameplayState( IGameLoader* loader ) override;
	virtual	Bool								CheckShouldSave() const override { return true; }

#ifndef NO_EDITOR_FRAGMENTS
	virtual void								OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
#endif

protected:

	Bool										IsObstacleDisabled() const 							{ return m_wasDestroyed && m_disableObstacleOnDestruction; }
	void										ReleaseWrapper();
	void										GetSkinningMatrices(void* dataToFill, const Float* vertexEpsilons);


	void funcIsDestroyed( CScriptStackFrame& stack, void* result );
	void funcIsObstacleDisabled( CScriptStackFrame& stack, void* result );
	void funcApplyFracture( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDestructionComponent );
	PARENT_CLASS( CMeshTypeComponent );
	PROPERTY_EDIT_NAME( m_parameters.m_baseResource, TXT( "m_baseResource" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_fracturedResource, TXT( "m_fracturedResource" ), TXT( "" ) )
	PROPERTY( m_parameters.m_pose);
	PROPERTY_CUSTOM_EDIT_NAME( m_parameters.m_physicalCollisionType, TXT( "m_physicalCollisionType" ), TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_CUSTOM_EDIT_NAME( m_parameters.m_fracturedPhysicalCollisionType, TXT( "m_fracturedPhysicalCollisionType" ), TXT( "Defines what of types it is from physical collision point of view for fractured elements" ), TXT("PhysicalCollisionTypeSelector") );
	

		PROPERTY_EDIT_NAME( m_parameters.m_dynamic, TXT( "dynamic" ), TXT( "" ) )
		PROPERTY_EDIT_NAME( m_parameters.m_kinematic, TXT( "kinematic" ), TXT( "If it's not dynamic, this flag decides if base will be static or kinematic" ) )

		PROPERTY_EDIT_NAME( m_parameters.m_debrisTimeout, TXT( "debrisTimeout" ), TXT( "Whether or not fractured chunks should be destroyed if their lifetime exceeds certain value." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_debrisTimeoutMin, TXT( "debrisTimeoutMin" ), TXT( "Fractured chunks will be destroyed if their lifetime exeeds this value and the m_debrisTimeout flag is set" ) )
		PROPERTY_EDIT_NAME( m_parameters.m_debrisTimeoutMax, TXT( "debrisTimeoutMax" ), TXT( "Fractured chunks will be destroyed if their lifetime exeeds this value and the m_debrisTimeout flag is set" ) )

		PROPERTY_EDIT_NAME( m_parameters.m_initialBaseVelocity, TXT( "initialBaseVelocity" ), TXT( "If it's different than Zeroes vector, base destruction actor will have this velocity applied." ) )

		PROPERTY_EDIT_NAME( m_parameters.m_hasInitialFractureVelocity, TXT( "hasInitialFractureVelocity" ), TXT( "If it's set to true and we defined initial velocity in resource editor, it will be used when fracture is requested." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_maxVelocity, TXT( "maxVelocity" ), TXT( "If it's > -1, we will use this param to limit chunks speed." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_maxAngFractureVelocity, TXT( "maxAngularFractureVelocity" ), TXT( "If it's > -1, we will randomize an angular velocity upon destruction for each chunk and it won't be bigger than this value." ) )


		PROPERTY_EDIT_NAME( m_parameters.m_debrisMaxSeparation, TXT( "debrisMaxSeparationDistance" ), TXT( "Fractured chunks will be destroyed if the distance from the origin exceeds this value." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_simulationDistance, TXT( "simulationDistance" ), TXT( "Simulation distance for the wrapper." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_fadeOutTime, TXT( "fadeOutTime" ), TXT( "The time it takes for a chunk to fade out when destroyed from Timeout or MaxSeparation. Default = 1.0f" ) )
		
		PROPERTY_EDIT_NAME( m_parameters.m_forceToDamage, TXT( "forceToDamage" ), TXT( "When colliding, destruction component will take damage equal to forceToDamage multiplied by the impact force. The default value is zero, which effectively disables impact damage. " ) )

		PROPERTY_EDIT_NAME( m_parameters.m_damageThreshold, TXT( "damageThreshold" ), TXT( "If the resulting damage from forceToDamage * impact force is less then this threshold, it will be discarded." ) )
		PROPERTY_EDIT_NAME( m_parameters.m_damageEndurance, TXT( "damageEndurance" ), TXT( "Once accumulated damage exceeds this value, the destructible will get fractured" ) )
		PROPERTY_EDIT_NAME( m_parameters.m_accumulateDamage, TXT( "accumulateDamage" ), TXT( "If this flag is set, the component will accumulate damage and once it exceeds the endurance of the destructible, it will get fractured." ) )

		PROPERTY_EDIT_NAME( m_parameters.m_useWorldSupport, TXT( "useWorldSupport" ), TXT( " If set, then chunks which overlap the NxScene's static geometry will have environmental support If set, then chunks which overlap the NxScene's static geometry will have environmental support in static destructibles. Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as 'support' chunks AND overlap the NxScene's static geometry in order to be environmentally supported. " ) )

		PROPERTY_CUSTOM_EDIT_NAME(m_fractureSoundEvent, TXT( "fractureSoundEvent" ),TXT( "Sound event on destruction" ), TXT( "AudioEventBrowser" ))
		PROPERTY_EDIT_NAME(m_fxName, TXT( "fxName" ), TXT( "Name of fx effect played on destruction" ) )

		PROPERTY_INLINED( m_eventOnDestruction, TXT("Actions to perform on destruction") )
		PROPERTY_EDIT( m_pathLibCollisionType, TXT("Collision type for navigation system. PLC_Dynamic is collision type that allows navigation obstacle to be removed in runtime.") )
		PROPERTY_EDIT( m_disableObstacleOnDestruction, TXT("Should navigation obstacle be removed on destruction. Works only with PLC_Dynamic and PLC_Immediate collision types.") )


		NATIVE_FUNCTION( "IsDestroyed", funcIsDestroyed );
		NATIVE_FUNCTION( "IsObstacleDisabled", funcIsObstacleDisabled );
		NATIVE_FUNCTION( "ApplyFracture", funcApplyFracture );
END_CLASS_RTTI();
#endif