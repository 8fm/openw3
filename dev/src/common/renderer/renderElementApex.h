/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderElement.h"

#ifdef USE_APEX

namespace physx
{
	namespace apex
	{
		class NxApexRenderable;
	}
}

class CRenderElement_Apex : public IRenderElement
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderElementApex )
protected:
	String							m_materialName;
	physx::apex::NxApexRenderable*	m_renderable;
	CRenderMaterial*				m_originalMaterial;
	CRenderMaterialParameters*		m_originalParameters;

public:
	CRenderElement_Apex( IRenderProxyDrawable* proxy, CRenderMaterial* material, CRenderMaterialParameters* parameters, const String& materialName, physx::apex::NxApexRenderable* renderable );
	~CRenderElement_Apex();

	RED_INLINE const String& GetMaterialName() const { return m_materialName; }
	RED_INLINE class physx::apex::NxApexRenderable* GetApexRenderable() const { return m_renderable; }

	RED_INLINE void SetRenderable( physx::apex::NxApexRenderable* renderable ) { m_renderable = renderable; }

	Vector GetDissolveValues( Bool shadows ) const;
	Bool IsDissolved( Bool shadows ) const;

	RED_INLINE Bool HasMaterialReplacement() const { return m_originalMaterial != nullptr; }
	void SetMaterialReplacement( CRenderMaterial* material, CRenderMaterialParameters* parameters );
	void ClearMaterialReplacement();

	RED_INLINE CRenderMaterial* GetRealMaterial() const { return HasMaterialReplacement() ? m_originalMaterial : GetMaterial(); }
	RED_INLINE CRenderMaterialParameters* GetRealParameters() const { return HasMaterialReplacement() ? m_originalParameters : GetMaterialParams(); }
};

#endif
