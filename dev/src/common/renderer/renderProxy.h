/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "..\engine\renderProxy.h"
#include "..\engine\globalVisibilityId.h"
#include "renderFramePrefetch.h"
#include "renderFrameTracker.h"

class CRenderElementMap;
class CRenderSceneEx;
class CRenderCollector;
class CHitProxyID;


/// Proxy for rendering scene
class IRenderProxyBase : public IRenderProxy, public IRenderPrefetchable
{
	friend class CRenderSceneEx;

protected:
	// TODO: quantize !
	Box								m_boundingBox;				//!< Bounding box of the object
	// TODO: 3 vectors
	Matrix							m_localToWorld;				//!< Local to world transform of the object

	// TODO: remove, this is needed for debug mostly
	CRenderSceneEx*					m_scene;					//!< Scene this proxy was registered for

protected:
	Uint32							m_entryID;
	Float							m_autoHideDistance;			//!< Autohide for this proxy
	Float							m_autoHideDistanceSquared;  //!< Autohide squared for this proxy

	GlobalVisID						m_umbraProxyId;
	Red::Threads::CAtomic< Int32 >	m_registrationRefCount;

	SFrameTracker					m_frameTracker;

public:
	//! Get the scene this proxy was registered for
	RED_INLINE CRenderSceneEx* GetScene() const					{ return m_scene; }

	//! Get the autohide distance for proxy
	RED_INLINE Float GetAutoHideDistance() const				{ return m_autoHideDistance; }

	//! Get the autohide squared distance for proxy
	RED_INLINE Float GetAutoHideDistanceSquared() const { return m_autoHideDistanceSquared; }

	//! Get bounding box
	RED_INLINE const Box& GetBoundingBox() const				{ return m_boundingBox; }

	//! Get local to world matrix
	RED_INLINE const Matrix& GetLocalToWorld() const			{ return m_localToWorld; }

	RED_INLINE const GlobalVisID& GetUmbraProxyId() const		{ return m_umbraProxyId; }
	RED_FORCE_INLINE Uint32 GetEntryID() const					{ return m_entryID; }
	RED_FORCE_INLINE void SetEntryID( Uint32 entryID )			{ m_entryID = entryID; }

	RED_INLINE void OnRegistered()								{ m_registrationRefCount.Increment(); }
	RED_INLINE void OnUnregistered()							{ m_registrationRefCount.Decrement(); }
	RED_INLINE Int32 GetRegCount() const						{ return m_registrationRefCount.GetValue(); }

public:
	IRenderProxyBase( ERenderProxyType type, const RenderProxyInitInfo& initInfo );
	virtual ~IRenderProxyBase();

	//! Attach to scene
	virtual void AttachToScene( CRenderSceneEx* scene );

	//! Detach from scene
	virtual void DetachFromScene( CRenderSceneEx* scene );

	//! Relink proxy - update bounding box, local to world matrix, spatial caching etc
	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld );
	virtual void RelinkTransformOnly( const Matrix& localToWorld );
	virtual void RelinkBBoxOnly( const Box& boundingBox );

	//! Update hit proxy ID
	virtual void UpdateHitProxyID( const CHitProxyID& id );

	//! Update selection flag
	virtual void UpdateSelection( Bool isSelected );

	virtual Bool Register( CRenderElementMap* reMap );
	virtual Bool Unregister( CRenderElementMap* reMap, Bool deferredUnregister = false );

	RED_INLINE Bool ShouldBeManagedByRenderElementMap() const
	{
		return m_type == RPT_Mesh
			|| m_type == RPT_Apex
			|| m_type == RPT_SSDecal
			|| m_type == RPT_Particles
			|| m_type == RPT_Flare
			|| m_type == RPT_Fur
			|| m_type == RPT_Dimmer
			|| m_type == RPT_PointLight
			|| m_type == RPT_SpotLight
			|| m_type == RPT_Stripe
			|| m_type == RPT_Swarm;
	}

	//! Can this be cast to IRenderProxyDrawable?
	virtual Bool IsDrawable() const { return false; }

	//! Can this be cast to IRenderProxyLight?
	virtual Bool IsLight() const { return false; }

	//! Add any required resources to a frame prefetch. This can be called from a job thread, so must be safe to call at any time.
	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override {}

	//! Fast access interface for drawable proxies
	virtual class IRenderProxyDrawable* AsDrawable() { return nullptr; }
	virtual const class IRenderProxyDrawable* AsDrawable() const { return nullptr; }

	//! Fast access interface for drawable proxies
	virtual class IRenderProxyLight* AsLight() { return nullptr; }
	virtual const class IRenderProxyLight* AsLight() const { return nullptr; }

public:
	//! Called only on proxies that were collected (i.e. going to be drawn somewhere) and requested it.
	//! This allows a proxy to do work only if it's going to be drawn (whether to the screen, or into a shadow map, etc.)
	virtual void CollectedTick( CRenderSceneEx* scene ) {}

	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );
};