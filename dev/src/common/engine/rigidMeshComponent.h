/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "staticMeshComponent.h"
#include "animationEngine.h"

/// Simulated mesh component
class CRigidMeshComponent : public CStaticMeshComponent
{
	DECLARE_ENGINE_CLASS( CRigidMeshComponent, CStaticMeshComponent, 0 );

public:
	CRigidMeshComponent();

	//! Properties
	RED_INLINE const EMotionType GetMotionType() const { return m_motionType; }
	RED_INLINE const Float GetLinearDamping() const { return m_linearDamping; }
	RED_INLINE const Float GetAngularDamping() const { return m_angularDamping; }
	RED_INLINE const Float GetLinearVelocityClamp() const { return m_linearVelocityClamp; }
		
	//! Property was changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );

	// Attached to world
	virtual void OnAttached( CWorld* world );

	// Detached from world
	virtual void OnDetached( CWorld* world );

	virtual void OnStreamIn();

	// added, so rigid mesh component will be updated even if it is not visible
	virtual Bool UsesAutoUpdateTransform();

	// internal transform updated
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// contains dynamic geometry ?
	virtual Bool IsDynamicGeometryComponent() const { return true; }

	void UpdateKinematicState();

	virtual class CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const { return m_physicsBodyWrapper; }

	virtual void SetEnabled( Bool enabled );

#ifndef NO_EDITOR
	virtual void EditorOnTransformChangeStop() override;
	virtual void EditorRecreateCollision() override;
#endif

	//! Rigid body selection changed
	virtual void OnSelectionChanged();

protected:
	virtual Bool ShouldScheduleFindCompiledCollision() const override;
	virtual void OnCompiledCollisionFound( CompiledCollisionPtr collision ) override;

	void funcEnableBuoyancy( CScriptStackFrame& stack, void* result );

	EMotionType							m_motionType;
	Float								m_linearDamping;
	Float								m_angularDamping;
	Float								m_linearVelocityClamp;
	CPhysicsWrapperInterface*			m_physicsBodyWrapper;
};

BEGIN_CLASS_RTTI( CRigidMeshComponent );
	PARENT_CLASS( CStaticMeshComponent );
	PROPERTY_EDIT( m_motionType, TXT("Motion type") );
	PROPERTY_EDIT_RANGE( m_linearDamping, TXT("linear damping, negative value means that scene default setting will be used"), -1.000f, +FLT_MAX );
	PROPERTY_EDIT_RANGE( m_angularDamping, TXT("angular damping, negative value means that scene default setting will be used"), -1.000f, +FLT_MAX );
	PROPERTY_EDIT_RANGE( m_linearVelocityClamp, TXT("linear velocity clampling"), -1.0f, +FLT_MAX );
	NATIVE_FUNCTION( "EnableBuoyancy", funcEnableBuoyancy );
END_CLASS_RTTI();

