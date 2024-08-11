/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "lensFlareSetupParameters.h"
#include "renderResource.h"
#include "environmentAreaParams.h"

SLensFlareElementSetupParameters::SLensFlareElementSetupParameters ()
	: m_materialResource( nullptr )
	, m_materialParamsResource( nullptr )
	, m_isConstRadius( false )
	, m_isAligned( false )
	, m_centerFadeStart( 0 )
	, m_centerFadeRange( 0 )
	, m_colorGroupParamsIndex( 0 )
	, m_alpha( 1.f )
	, m_size( 1.f )
	, m_aspect( 1.f )
	, m_shift( 1.f )
	, m_pivot( 0.f )
	, m_colorLinear( Vector::ONES )
{}

SLensFlareElementSetupParameters::~SLensFlareElementSetupParameters ()
{
	ReleaseAll();
}

void SLensFlareElementSetupParameters::AddRefAll()
{
	if ( m_materialResource )				m_materialResource->AddRef();
	if ( m_materialParamsResource )			m_materialParamsResource->AddRef();
}

void SLensFlareElementSetupParameters::ReleaseAll()
{
	SAFE_RELEASE( m_materialResource );
	SAFE_RELEASE( m_materialParamsResource );
}

SLensFlareSetupParameters::SLensFlareSetupParameters ()
	: m_nearDistance( 5.f )
	, m_nearInvRange( 1.f )
	, m_farDistance( 100.f )
	, m_farInvRange( 1.f )
{}

void SLensFlareSetupParameters::AddRefAll()
{
	for ( Uint32 i=0; i<m_elements.Size(); ++i )
	{
		m_elements[i].AddRefAll();
	}
}

void SLensFlareSetupParameters::ReleaseAll()
{
	for ( Uint32 i=0; i<m_elements.Size(); ++i )
	{
		m_elements[i].ReleaseAll();
	}
}

void SLensFlareGroupsSetupParameters::AddRefAll()
{
	for ( Uint32 i=0; i<LFG_MAX; ++i )
	{
		m_groups[i].AddRefAll();
	}
}

void SLensFlareGroupsSetupParameters::ReleaseAll()
{
	for ( Uint32 i=0; i<LFG_MAX; ++i )
	{
		m_groups[i].ReleaseAll();
	}
}
