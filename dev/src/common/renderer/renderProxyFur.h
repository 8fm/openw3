#pragma once

#include "renderProxyDrawable.h"
#include "renderEntityGroup.h"


class CRenderShader;
class CRenderSkinningData;
#ifdef USE_NVIDIA_FUR
class CRenderFurMesh_Hairworks;
#endif

/// Fur proxy
class CRenderProxy_Fur : public IRenderProxyDrawable, public IRenderEntityGroupProxy
{
public:
	CRenderProxy_Fur( const RenderProxyInitInfo& info );
	virtual ~CRenderProxy_Fur();

	virtual void	Prefetch( CRenderFramePrefetch* prefetch ) const override;

	void 			Render( const CRenderCollector &collector, const class RenderingContext& context );

	void			UpdateFurSkinning();

	// Tidy up render state after we have called the external lib.
	static void		RestoreState( const CRenderFrameInfo &info );

	//! Set use shadow distances
	virtual void	SetUseShadowDistances( Bool enable );

public:
	//! Collect elements for rendering
	virtual void	CollectElements( CRenderCollector& collector ) override;

	virtual void	CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults ) override;

	//! Set skinning data
	void SetSkinningData( IRenderObject* skinningData );

	//! Attach to scene
	virtual void	AttachToScene( CRenderSceneEx* scene );

	//! Detach from scene
	virtual void	DetachFromScene( CRenderSceneEx* scene );

	virtual IRenderProxyBase* AsBaseProxy() { return this; }

public:
	//! Get skinning data for proxy
	RED_INLINE CRenderSkinningData*
					GetSkinningData() const { return m_skinningData; }

	void			UpdateFurParams( const Vector& wind, Float wetness );

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
	void			EditorSetFurParams( struct GFSDK_HairInstanceDescriptor* newParams, Uint32 index );
#endif //USE_NVIDIA_FUR
#endif //NO_EDITOR

#ifdef USE_NVIDIA_FUR
	RED_INLINE CRenderTexture* const* GetTextures() const { return m_furTextures; }
#else
	RED_INLINE CRenderTexture* const* GetTextures() const { return nullptr; }
#endif
	Uint32			GetNumTextures() const;

private:
#ifdef USE_NVIDIA_FUR
	CRenderFurMesh_Hairworks*					m_asset;

	struct GFSDK_HairInstanceDescriptor*	m_defaultParams;						//!< default sim params
	struct GFSDK_HairInstanceDescriptor*	m_wetnessMaterialParams;				//!< target material blend (if need more we support up to 4 materials with default one)
	Uint16									m_instanceID;
	CRenderTexture**						m_furTextures;
#endif
	CRenderSkinningData*					m_skinningData;							//!< Mesh skinning data
	GpuApi::BufferRef						m_psConstantBuffer;
	Bool									m_isTwoSided;
	Float									m_wetnessMaterialWeight;
	Bool									m_useShadowDistances;

protected:
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

	void CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame );
};
