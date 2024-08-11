/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibComponent.h"
#include "meshComponent.h"
#include "../physics/physicalCollision.h"
#include "../core/events.h"
#include "../physics/compiledCollision.h"
#include "collisionCache.h"

/////////////////////////////////////////////////////////

class CPhysicsWrapperInterface;
struct SPhysicsRigidBodyWrapperInit;
class IDynamicObstacle;

/////////////////////////////////////////////////////////

/// Type of path engine collision for static meshes
enum EPathEngineCollision
{
	PEC_Disabled,			//!< No collision at all
	PEC_Burned,				//!< Path engine collision is burned into obstacle set ( good for small convex objects )
	PEC_Static,				//!< Mesh collision is cut from navigation mesh
	PEC_StaticWalkable,		//!< Mesh collision is cut from navigation mesh but we can walk on this mesh also
	PEC_Dynamic,			//!< Path engine collision is spawned dynamically
	PEC_Walkable,			//!< Mesh used in generation of navmesh but don't create obstacles
};

BEGIN_ENUM_RTTI( EPathEngineCollision );
	ENUM_OPTION( PEC_Disabled );
	ENUM_OPTION( PEC_Burned );
	ENUM_OPTION( PEC_Static );
	ENUM_OPTION( PEC_StaticWalkable );
	ENUM_OPTION( PEC_Dynamic );
	ENUM_OPTION( PEC_Walkable );
END_ENUM_RTTI();

/// Component rendering mesh with collision
class CStaticMeshComponent : public CMeshComponent, public ICollisionCacheCallback, public PathLib::IObstacleComponent
{
	DECLARE_ENGINE_CLASS( CStaticMeshComponent, CMeshComponent, 0 )

protected:
	CPhysicalCollision					m_physicalCollisionType;
	EPathLibCollision					m_pathLibCollisionType;				//!< Type of path lib collision
	Bool								m_fadeOnCameraCollision;			//!< Fade on camera collision
	CompiledCollisionPtr				m_compiledCollision;
	Int32								m_physicsBodyIndex;

protected:

	void				SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup ) override;
	EPathLibCollision	GetPathLibCollisionGroup() const override;			// PathLib::IObstacleComponent interface
	CComponent*			AsEngineComponent() override;						// PathLib::IComponent interface
	PathLib::IComponent* AsPathLibComponent() override;						// PathLib::IComponent interface

public:
	//! Get path engine collision type
    RED_INLINE CPhysicalCollision GetPhysicalCollision() const { return m_physicalCollisionType; }
	RED_INLINE EPathLibCollision GetPathLibCollisionType() const { return m_pathLibCollisionType; }

	//! Set path engine collision type
	RED_INLINE void SetPathLibCollisionType( EPathLibCollision collisionType ) { m_pathLibCollisionType = collisionType; }

	//! Get the physics wrapper
	virtual CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const;

	//! Fade on camera collision
	RED_INLINE Bool IsFadingOnCameraCollision() const { return m_fadeOnCameraCollision; }

	//! PathLib interface to hash static mesh state (to check if it has changed)
	Uint32 CalculatePathLibObstacleHash();

	//! PathLib component interface determining streaming type
	virtual Bool IsLayerBasedGrouping() const override;
	virtual Bool IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const override;

public:
	// Default constructor
	CStaticMeshComponent();
	~CStaticMeshComponent();

#ifndef NO_RESOURCE_COOKING
	// Object is about to be cooked
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif
	
	// Property has been changed in the editor by the user
	virtual void OnPropertyPostChange( IProperty* property );

	// Attached to world
	virtual void OnAttached( CWorld* world );

	// Detached from world
	virtual void OnDetached( CWorld* world );

	// Handle missing m_pathEngineCollisionType property
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	// Should we auto update the transform?
	virtual Bool UsesAutoUpdateTransform() override;

#ifndef NO_EDITOR
	// Editor only stuff
	virtual void EditorOnTransformChangeStart() override;
	// Editor only stuff
	virtual void EditorOnTransformChanged() override;
	// Editor only stuff
	virtual void EditorOnTransformChangeStop() override;

	// navigation cooking
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context ) override;

	// recereate collision 
	virtual void EditorRecreateCollision();
#endif

	virtual Bool ShouldGenerateEditorFragments( CRenderFrame* frame ) const override;

	// Generate debug rendering fragments for editor
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
	
public:
	// Get the attachment order this component should be attached
	virtual EAttachmentGroup GetAttachGroup() const { return ATTACH_GROUP_P1; }

protected:

	void ScheduleFindCompiledCollision();
	void CancelFindCompiledCollision();
	virtual Bool ShouldScheduleFindCompiledCollision() const;
	virtual void OnCompiledCollisionFound( CompiledCollisionPtr collision ) override;
	virtual void OnCompiledCollisionInvalid() override;

protected:

	//! Get collision shape with simulation mass properties
	CompiledCollisionPtr GetCompiledCollision( Bool loadSynchronously = false ) const;

	virtual void funcGetPhysicalObjectBoundingVolume( CScriptStackFrame& stack, void* result );

public:

	CPhysicalCollision GetCollisionWithClimbFlags( ) const;
	Bool IsEntityMarkedWithTag( CName tag ) const;

	RED_INLINE Int32 GetPhysicsBodyIndex() const { return m_physicsBodyIndex; }

private:

	void GenerateDebugMesh();

	IRenderResource* m_debugClimbableMesh;
};

BEGIN_CLASS_RTTI( CStaticMeshComponent );
	PARENT_CLASS( CMeshComponent );
	PROPERTY_EDIT( m_pathLibCollisionType, TXT("Collision type for navigation system") );
	PROPERTY_EDIT( m_fadeOnCameraCollision, TXT("Fade on camera collision") );
	PROPERTY_CUSTOM_EDIT( m_physicalCollisionType, TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );
	NATIVE_FUNCTION( "GetPhysicalObjectBoundingVolume", funcGetPhysicalObjectBoundingVolume );
END_CLASS_RTTI();
