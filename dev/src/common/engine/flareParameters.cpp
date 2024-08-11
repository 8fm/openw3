/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "flareParameters.h"
#include "environmentAreaParams.h"

IMPLEMENT_ENGINE_CLASS( SFlareParameters );
IMPLEMENT_ENGINE_CLASS( SLensFlareElementParameters );
IMPLEMENT_ENGINE_CLASS( SLensFlareParameters );


SFlareParameters::SFlareParameters ()		
{
	Reset();
}

SFlareParameters::SFlareParameters( const SFlareParameters& s )
{
	m_category			= s.m_category;
	m_lensFlareGroup	= s.m_lensFlareGroup;
	m_occlusionExtent	= s.m_occlusionExtent;
	m_flareRadius		= s.m_flareRadius;
	m_fadeInMaxSpeed	= s.m_fadeInMaxSpeed;
	m_fadeOutMaxSpeed	= s.m_fadeOutMaxSpeed;
	m_fadeInAccel		= s.m_fadeInAccel;
	m_fadeOutAccel		= s.m_fadeOutAccel;
	m_colorGroup		= s.m_colorGroup;
	m_visibilityFullDist	= s.m_visibilityFullDist;
	m_visibilityFadeRange	= s.m_visibilityFadeRange;
}

void SFlareParameters::Reset()
{
	m_category			= FLARECAT_Default;
	m_lensFlareGroup	= LFG_Default;
	m_occlusionExtent	= 1.f;
	m_flareRadius		= 1.f;
	m_fadeInMaxSpeed	= 1.f / 0.375f;
	m_fadeOutMaxSpeed	= 1.f / 0.115f;
	m_fadeInAccel		= 10.f;
	m_fadeOutAccel		= 12.5f;
	m_colorGroup		= EFCG_Default;
	m_visibilityFullDist	= 75.f;
	m_visibilityFadeRange	= 10.f;
}

SLensFlareElementParameters::SLensFlareElementParameters()
	: m_material( nullptr )
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
	, m_color( Color::WHITE )
{}

SLensFlareElementParameters::~SLensFlareElementParameters()
{}

SLensFlareParameters::SLensFlareParameters ()
	: m_nearDistance( 5.f )
	, m_nearRange( 1.f )
	, m_farDistance( 100.f )
	, m_farRange( 10.f )
{}

SLensFlareParameters::~SLensFlareParameters()
{}
