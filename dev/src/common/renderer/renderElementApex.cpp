/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementApex.h"
#include "renderProxyApex.h"
#include "renderMaterial.h"

#ifdef USE_APEX

CRenderElement_Apex::CRenderElement_Apex( IRenderProxyDrawable* proxy, CRenderMaterial* material, CRenderMaterialParameters* parameters, const String& materialName, physx::apex::NxApexRenderable* renderable )
	: IRenderElement( RET_Apex, proxy, material, parameters )
	, m_materialName( materialName )
	, m_renderable( renderable )
	, m_originalMaterial( nullptr )
	, m_originalParameters( nullptr )
{
}

CRenderElement_Apex::~CRenderElement_Apex()
{
	ClearMaterialReplacement();
}

Vector CRenderElement_Apex::GetDissolveValues( Bool shadows ) const
{
	if ( m_proxy )
	{
		ASSERT( m_proxy->GetType() == RPT_Apex, TXT("CRenderElement_Apex without a CRenderProxy_Apex?") );
		if ( m_proxy->GetType() == RPT_Apex )
		{
			const CRenderProxy_Apex* proxy = static_cast< const CRenderProxy_Apex* >( m_proxy );

			// Apex desn't have proper LOD Selector ability due to how PhysX switches rendering proxies. So We are using non LodSelector dissolves
			const Vector& dissolveParams = shadows ? proxy->CalcShadowDissolveValuesNoLod() : proxy->IRenderProxyFadeable::CalcDissolveValues();
			return Vector( dissolveParams.X, dissolveParams.Y, 0.0f, 0.0f );
		}
	}
	return Vector::ZEROS;
}


Bool CRenderElement_Apex::IsDissolved( Bool shadows ) const
{
	if ( m_proxy )
	{
		ASSERT( m_proxy->GetType() == RPT_Apex, TXT("CRenderElement_Apex without a CRenderProxy_Apex?") );
		if ( m_proxy->GetType() == RPT_Apex )
		{
			const CRenderProxy_Apex* proxy = static_cast< const CRenderProxy_Apex* >( m_proxy );
			return shadows ? proxy->IsShadowDissolved() : proxy->IsDissolved();
		}
	}
	return false;
}

void CRenderElement_Apex::SetMaterialReplacement( CRenderMaterial* material, CRenderMaterialParameters* parameters )
{
	// Remove previous replacement, if any
	ClearMaterialReplacement();

	// Save original material and parameters
	m_originalMaterial = m_material;
	m_originalParameters = m_parameters;

	// Replace the material and parameters with the new ones and keep ref
	m_material = material;
	m_parameters = parameters;
	m_material->AddRef();
	m_parameters->AddRef();

	// Set sort group
	m_sortGroup = material->GetRenderSortGroup();
}

void CRenderElement_Apex::ClearMaterialReplacement()
{
	if ( HasMaterialReplacement() )
	{
		// Remove references from the replacement material and parameters
		SAFE_RELEASE(m_material);
		SAFE_RELEASE(m_parameters);

		// Restore original material and parameters
		m_material = m_originalMaterial;
		m_parameters = m_originalParameters;
		m_originalMaterial = nullptr;
		m_originalParameters = nullptr;

		// Restore sort group
		m_sortGroup = m_material->GetRenderSortGroup();
	}
}

#endif
