/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderVisibilityQuery.h"

class IRenderEntityGroupProxy;
class CRenderElementMap;

class CRenderEntityGroup : public IRenderEntityGroup
{
	friend IRenderEntityGroupProxy;

public:
	enum eShadowVisibilityGroup
	{
		SVG_Base,
		SVG_Detail,
		SVG_Fur,
		SVG_Fallback,

		SVG_MAX
	};

	enum eProxyCategory
	{
		PROXYCAT_Main,
		PROXYCAT_Fallback,

		PROXYCAT_MAX,
	};

public:
	//! Internal linked list
	CRenderEntityGroup*		m_next;

	//! Last repeated frame this group was visited
	SFrameTracker							m_frameTracker;
	SFrameTracker							m_repeatedFrameTracker;
					
private:
	TDynArray< IRenderEntityGroupProxy* >	m_proxies[PROXYCAT_MAX];					//< Proxies that are in this group
	CRenderDissolveValue					m_shadowFade;								//< Dissolve value
	Red::Threads::CAtomic< Int32 >			m_lastShadowFrame;							//< Last shadow frame
	Bool									m_useHiResShadows				: 1;		//< Do we have a high-resolution self shadowing enabled for this group ?
	Bool									m_shadowsEnabled				: 1;
	CRenderElementMap*						m_renderElementMap;							//< RenderMap the group is attached to
	Box										m_box;										//< Combined bounding box of proxies in this group

private:
	// Add proxy
	void AddProxy( IRenderEntityGroupProxy *proxy );

	// Remove proxy
	void RemoveProxy( IRenderEntityGroupProxy *proxy );

	// Has any proxy
	Bool HasAnyProxy() const;

	// Has given proxy
	Bool HasProxy( IRenderEntityGroupProxy *proxy ) const;

	// Update proxies shadow distance usage
	void UpdateProxiesUseShadowDistances();

	void RegisterToRenderElementMap();
	void UnregisterFromRenderElementMap();

public:
	CRenderEntityGroup();
	virtual ~CRenderEntityGroup();

	// Get whether to use hires shadows
	RED_FORCE_INLINE Bool IsUsingHiResShadows() const { return m_useHiResShadows && m_shadowsEnabled; }

	// Get whether shadows are enabled
	RED_FORCE_INLINE Bool IsShadowsEnabled() const { return m_shadowsEnabled; }

	// Get whether to use shadow distances
	RED_FORCE_INLINE Bool IsUsingShadowDistances() const { return !m_proxies[PROXYCAT_Fallback].Empty(); }

	// Set whether to use hires shadows
	RED_FORCE_INLINE void SetUseHiResShadows( Bool enable ) { m_useHiResShadows = enable; }

	// Set whether to enable shadows
	RED_FORCE_INLINE void SetUseShadows( Bool enable ) { m_shadowsEnabled = enable; }

	// Get proxy category
	static eProxyCategory GetProxyCategory( const IRenderEntityGroupProxy &proxy );

	// Update shadow fade
	void UpdateShadowFade( const CRenderCollector &collector, const Bool wasVisibleLastFrame );

	void UpdateOncePerFrame( const CRenderCollector &collector );

	void CollectElements( CRenderCollector &collector );

	// Get shadow fade alpha
	CRenderDissolveAlpha GetShadowFadeAlpha( const CRenderDissolveSynchronizer &sc, Bool isShadowFallback ) const;

	// Get proxies
	RED_FORCE_INLINE const TDynArray< IRenderEntityGroupProxy* >& GetProxies( eProxyCategory category ) const { return m_proxies[category]; }

	// Get proxies
	RED_FORCE_INLINE const TDynArray< IRenderEntityGroupProxy* >& GetMainProxies() const { return m_proxies[PROXYCAT_Main]; }

	RED_FORCE_INLINE const Box& GetBoundingBox() const { return m_box; }
	RED_FORCE_INLINE const Vector CalculatePosition() const { return m_box.CalcCenter(); }

	void SetRenderElementMap( CRenderElementMap* renderElementMap );
	const Box& CalculateBoundingBox();
};


//
// All derived classes should be included in CRenderCommand_BindEntityGroupToProxy::Execute() function.
// 
class IRenderEntityGroupProxy
{
private:
	CRenderEntityGroup*		m_entityGroup;		//!< Entity group in which this proxy is (can be NULL for non-characters)

public:
	IRenderEntityGroupProxy ( IRenderEntityGroup *entityGroup );
	virtual ~IRenderEntityGroupProxy ();

	//! Get the entity group this proxy is in
	RED_INLINE CRenderEntityGroup* GetEntityGroup() const { return m_entityGroup; }

	//! Bind/Unbind from entity group
	void SetEntityGroup( CRenderEntityGroup* group, Bool isAttachedToScene );

	//! Attach to scene
	void AttachToScene();

	//! Detach from scene
	void DetachFromScene();

	//! As mesh proxy
	virtual CRenderProxy_Mesh* AsMeshProxy() { return nullptr; }

	//! As mesh proxy
	virtual IRenderProxyBase* AsBaseProxy() { return nullptr; }

	//! Is character shadow mesh
	virtual Bool IsCharacterShadowFallback() const { return false; }

	//! Set use shadow distances
	virtual void SetUseShadowDistances( Bool enable ) = 0;
};
