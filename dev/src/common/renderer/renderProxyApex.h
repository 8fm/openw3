/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderProxyDissolvable.h"
#include "renderDynamicDecalTarget.h"
#include "renderEntityGroup.h"

#ifdef USE_APEX

namespace physx
{
	namespace apex
	{
		class NxApexRenderable;
	}
}

class CApexWrapper;
class CRenderElement_Apex;

/// APEX destructible proxy
class CRenderProxy_Apex : public IRenderProxyDissolvable, public IDynamicDecalTarget, public IRenderEntityGroupProxy
{
public:
	enum EApexType : Int8
	{
		AT_Unknown,
		AT_Cloth,
		AT_Destructible,
	};

protected:
	TDynArray< CRenderElement_Apex*, MC_RenderData > m_renderElements;

	physx::apex::NxApexRenderable*	m_renderable;
	CApexWrapper*					m_wrapper;							//!< needed for the multithreaded release


	Float							m_wetness;							//!< Wetness factor for apex render proxy
	Uint32							m_originalElementCount;				//!< If above, this is the original number of elements

	CRenderDissolveValue			m_shadowDissolve;

	Float							m_shadowDistanceSquared;			//!< When not using an entity group, this is how far away the shadows should be drawn

	EApexType						m_apexType;

	Uint8							m_renderMask;						//!< Mask of where this proxy can be drawn

	Bool							m_hasMaterialReplacement	: 1;	//!< True if it has
	Bool							m_drawOriginalMaterials		: 1;	//!< True if we draw with the original materials too

	Bool							m_useShadowDistances		: 1;	//!< Use shadow distances from entity group
	Bool							m_isRenderableDirty			: 1;	//!< For cloth, do we need to call updateRenderResource?
	Bool							m_skipOcclusionTest			: 1;	//!< HACK! HACK! HACK! for blinking cloth during Ciri cutscene

#ifndef NO_EDITOR
	// Selection
	Vector							m_selectionColor;					//!< Selection color
#endif

public:
	CRenderProxy_Apex( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Apex();

	virtual const Uint8 GetRenderMask() const override { return m_renderMask; }

	virtual IDynamicDecalTarget* ToDynamicDecalTarget() override { return this; }

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	virtual void OnNotVisibleFromAutoHide( CRenderCollector& collector ) override;

	virtual void AttachToScene( CRenderSceneEx* scene ) override;
	virtual void DetachFromScene( CRenderSceneEx* scene ) override;

	virtual void CollectedTick( CRenderSceneEx* scene ) override;

	virtual void CollectElements( CRenderCollector& collector ) override;

	virtual void CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults ) override;

	//! Collect local shadow elements ( for point and spot lights )
	virtual void CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector );

	void SetApexRenderable( physx::apex::NxApexRenderable* renderable );

	RED_INLINE void SetWetness( Float wetness ) { m_wetness = wetness; }
	RED_INLINE Float GetWetness() const { return m_wetness; }
	RED_INLINE Bool SkipOcclusionTest() const { return m_skipOcclusionTest; }

	EApexType GetApexType() const { return m_apexType; }

	// Not virtual, but hide the base class's functions. When possible, should call these instead of the base.
	Bool IsDissolved() const;
	Bool IsShadowDissolved() const;


	physx::apex::NxApexRenderable* GetApexRenderable() const { return m_renderable; }

#ifndef NO_EDITOR
	//! Get selection color
	virtual void GetSelectionColor( Vector &selectionColor /* out */ ) override { selectionColor = m_selectionColor; }

	//! Set selection color
	virtual void SetSelectionColor( const Vector &selectionColor ) override { m_selectionColor = selectionColor; }
#endif

	//! Set material replacement
	virtual void SetReplacedMaterial( IRenderResource* material, IRenderResource* parameters, Bool drawOriginal ) override;

	//! Disable material replacement for proxy
	virtual void DisableMaterialReplacement() override;

	//! Update apex render resources if needed. Not needed for Cloth where !m_isRenderableDirty.
	void UpdateRenderResources();

	//////////////////////////////////////////////////////////////////////////
	// IDynamicDecalTarget

	virtual void CreateDynamicDecalChunks( CRenderDynamicDecal* decal, DynamicDecalChunkList& outChunks ) override;

	//////////////////////////////////////////////////////////////////////////
	// IRenderEntityGroupProxy

	virtual IRenderProxyBase* AsBaseProxy() override { return this; }

	//! Set use shadow distances
	virtual void SetUseShadowDistances( Bool enable ) override;

	//////////////////////////////////////////////////////////////////////////

protected:
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

	void UpdateShadowFadingValues( const CRenderCollector& collector, const Bool wasVisibleLastFrame );

};

#endif
