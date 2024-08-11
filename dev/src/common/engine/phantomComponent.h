/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../physics/physicalCallbacks.h"
#include "component.h"
#include "collisionCache.h"

struct SPhantomCollisionInfo
{
	CComponent*			m_collidedComponent;
	SActorShapeIndex	m_rigidBodyIndex;
	Vector				m_point;
	Vector				m_normal;

	SPhantomCollisionInfo() : m_collidedComponent(), m_rigidBodyIndex() {}
};

///////////////////////////////////////////////////////////////////////////////

enum EPhantomShape
{
	PS_Sphere,
	PS_Box,
	PS_EntityBounds
};

BEGIN_ENUM_RTTI( EPhantomShape );
	ENUM_OPTION( PS_Sphere );
	ENUM_OPTION( PS_Box );
	ENUM_OPTION( PS_EntityBounds );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

// A proxy used by the phantom component to obtain a shape the phantom should take
class ICollisionListener
{
public:
	virtual ~ICollisionListener() {}

	// Called when a phantom component detects a collision with an entity.
	virtual void OnCollision( CComponent* hitComponent, const TDynArray< SPhantomCollisionInfo >& contacts, Float timeDelta ) = 0;
};


///////////////////////////////////////////////////////////////////////////////

class CPhantomComponent : public CComponent, public IPhysicalCollisionTriggerCallback, public ICollisionCacheCallback
{
//	friend class CClothComponent;
	DECLARE_ENGINE_CLASS( CPhantomComponent, CComponent, 0 );

private:
	CName							m_collisionGroupName;
	TDynArray< CName >				m_triggeringCollisionGroupNames;

	EPhantomShape					m_shapeType;
	Vector							m_shapeDimensions;

	CPhysicsWrapperInterface*		m_wrapper;
	Bool							m_activated;

	CName							m_onTriggerEnteredScriptEvent;
	CName							m_onTriggerExitedScriptEvent;
	Bool							m_eventsCalledOnComponent;
	Bool							m_useInQueries;

	TDynArray< STriggeringInfo >	m_triggeringInfo;

	THandle< CMesh >				m_meshCollision;

	CompiledCollisionPtr			m_compiledCollision;
	CNode *							m_activateParentObject;

	//! Activate with data from attachment (if it exists) or entity
	void Activate();

public:
	CPhantomComponent();
	virtual ~CPhantomComponent();

	//! Component was attached to world
	virtual void OnAttached( CWorld* world );

	//! Component was detached from world
	virtual void OnDetached( CWorld* world );

	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );


	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//! Activates the component
	void Activate( CNode* parentObject );

	//! Deactivates the component
	void Deactivate();

	//! Notified when the appearance changes
	void OnAppearanceChanged( Bool added ) override;

	//! Notified after the streaming system created this component
	void OnStreamIn() override;

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	void DefineManualy( EPhantomShape type, const Vector& dimensions );
	void Fill( const THandle< CPhantomComponent >& other );

	virtual class CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const { return m_wrapper; }

	virtual void onTriggerEntered( const STriggeringInfo& info );
	virtual void onTriggerExited( const STriggeringInfo& info );
	const TDynArray< STriggeringInfo >&	GetTriggered() const { return m_triggeringInfo; }

	const THandle< CMesh >& GetMeshCollision() const { return m_meshCollision; }

	const TDynArray< CName >& GetTriggeringCollisionGroupNames() { return m_triggeringCollisionGroupNames; }
	EPhantomShape GetShapeType() { return m_shapeType; }
	const Vector& GetDimensions() { return m_shapeDimensions; }

private:
	///////////////////////////////////////////
	// Scripting interface
	void funcActivate( CScriptStackFrame& stack, void* result );
	void funcDeactivate( CScriptStackFrame& stack, void* result );
	void funcGetTriggeringCollisionGroupNames( CScriptStackFrame& stack, void* result );
	void funcGetNumObjectsInside( CScriptStackFrame& stack, void* result );
	///////////////////////////////////////////

	virtual void OnPropertyPostChange( IProperty* property );

	virtual void SetEnabled( Bool enabled );

	virtual class IPhysicalCollisionTriggerCallback* QueryPhysicalCollisionTriggerCallback() { return this; }

	virtual void OnCompiledCollisionFound( CompiledCollisionPtr collision );
	virtual void OnCompiledCollisionInvalid();

	void SetupPhysicsWrapper();

	CNode * GetParentNode();
};

BEGIN_CLASS_RTTI( CPhantomComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_collisionGroupName, TXT( "Phantom component's collision group (used in queries)" ) );
	PROPERTY_CUSTOM_EDIT( m_triggeringCollisionGroupNames, TXT( "Defines which collision groups will trigger" ), TXT("PhysicalCollisionGroupSelector") );
	PROPERTY_EDIT( m_shapeType, TXT( "Phantom shape" ) );
	PROPERTY_EDIT( m_shapeDimensions, TXT( "Shape size ( applicable for a sphere and a box )" ) );
	PROPERTY_EDIT( m_onTriggerEnteredScriptEvent, TXT( "script event called when on focus found" ) );
	PROPERTY_EDIT( m_onTriggerExitedScriptEvent, TXT( "script event called when on focus lost" ) );
	PROPERTY_EDIT( m_eventsCalledOnComponent, TXT( "the events will be triggered on the component, otherwise on the entity" ) );
	PROPERTY_EDIT( m_useInQueries, TXT( "Component will be used in queries, not as a trigger (all trigger-related properties are ignored" ) );
	PROPERTY_EDIT( m_meshCollision, TXT("Mesh") );
	NATIVE_FUNCTION( "Activate", funcActivate );
	NATIVE_FUNCTION( "Deactivate", funcDeactivate );
	NATIVE_FUNCTION( "GetTriggeringCollisionGroupNames", funcGetTriggeringCollisionGroupNames );
	NATIVE_FUNCTION( "GetNumObjectsInside", funcGetNumObjectsInside );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
