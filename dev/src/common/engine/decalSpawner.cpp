/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "decalSpawner.h"
#include "evaluatorFloat.h"


IMPLEMENT_ENGINE_CLASS( CDecalSpawner );


CDecalSpawner::CDecalSpawner()
	: m_farZ( 1.0f )
	, m_nearZ( 0.0f )
	, m_depthFadePower( 0.0f )
	, m_normalFadeBias( 0.0f )
	, m_normalFadeScale( 1.0f )
	, m_doubleSided( false )
	, m_projectionMode( RDDP_Ortho )
	, m_decalFadeTime( 0.1f )
	, m_decalFadeInTime( 0.1f )
	, m_projectOnlyOnStatic( false )
	, m_startScale( 1.0f )
	, m_scaleTime( 1.0f )
	, m_useVerticalProjection( false )
	, m_spawnPriority( EDynamicDecalSpawnPriority::RDDS_Normal )
	, m_autoHideDistance( -1.f )									// -1 means value will be taken from SRenderSettings
	, m_chance( 1.0f )
	, m_spawnFrequency( 0.0f )
{
	m_size = new CEvaluatorFloatConst( this, 1.0f );
	m_decalLifetime = new CEvaluatorFloatConst( this, 1.0f );
}
