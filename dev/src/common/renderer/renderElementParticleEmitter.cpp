/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementParticleEmitter.h"
#include "renderParticleEmitter.h"
#include "renderProxyDrawable.h"
#include "renderMaterial.h"
#include "../engine/materialCompiler.h"
#include "../engine/particleDrawer.h"

CRenderElement_ParticleEmitter::CRenderElement_ParticleEmitter( IRenderProxyDrawable* proxy, const IMaterial* material, EEnvColorGroup envColorGroup, Uint32 emitterID, CRenderParticleEmitter* renderEmitter )
	: IRenderElement( RET_ParticlesEmitter, proxy, material )
	, m_distanceFromCameraSquared( 0.0f )
	, m_sortingKey( 0 )
	, m_emitter( renderEmitter )
	, m_envColorGroup( envColorGroup )
	, m_data( NULL )
{
	m_emitter->AddRef();
	m_vertexFactory = ParticleVertexDrawerToMaterialVertexFactory( m_emitter->GetVertexType() );
	CreateParticleData();
}

CRenderElement_ParticleEmitter::CRenderElement_ParticleEmitter( IRenderProxyDrawable* proxy, CRenderParticleEmitter* renderEmitter, CRenderMaterial* material, CRenderMaterialParameters* parameters, EEnvColorGroup envColorGroup )
	: IRenderElement( RET_ParticlesEmitter, proxy, material, parameters )
	, m_distanceFromCameraSquared( 0.0f )
	, m_sortingKey( 0 )
	, m_emitter( renderEmitter )
	, m_envColorGroup( envColorGroup )
	, m_data( NULL )
{
	m_emitter->AddRef();
	m_vertexFactory = ParticleVertexDrawerToMaterialVertexFactory( m_emitter->GetVertexType() );
	CreateParticleData();
}

CRenderElement_ParticleEmitter::~CRenderElement_ParticleEmitter()
{
	if ( m_data )
	{
		delete m_data;
		m_data = NULL;
	}
	if ( m_emitter )
	{
		m_emitter->Release();
		m_emitter = NULL;
	}
}

void CRenderElement_ParticleEmitter::UpdateDistanceFromPoint( const Vector & point , Uint8 internalOrder , Uint8 renderPriority )
{
	// Update distance to camera
	m_distanceFromCameraSquared = m_proxy->GetLocalToWorld().GetTranslation().DistanceSquaredTo( point );

	COMPILE_ASSERT( sizeof( Uint32) == sizeof(m_distanceFromCameraSquared) );
	Uint32 disntanceMask = reinterpret_cast<Uint32&>( m_distanceFromCameraSquared );

	m_sortingKey =	( static_cast<Uint64>( renderPriority ) << 48 ) | 
					( static_cast<Uint64>( disntanceMask ) << 8 ) | 
					( static_cast<Uint64>( internalOrder ) );
}

void CRenderElement_ParticleEmitter::CreateParticleData()
{
	ASSERT( m_emitter );

	// Destroy previous buffer
	if ( m_data )
	{
		delete m_data;
		m_data = NULL;
	}

	// Create buffer to contain specific particles type
	Uint32 maxParticles = m_emitter->GetMaxParticles();
	switch ( m_emitter->GetParticleType() )
	{
	case RPT_Simple:
		m_data = new CParticleData< SimpleParticle >( maxParticles );
		break;
	case RPT_Trail:
		m_data = new CParticleData< TrailParticle >( maxParticles );
		break;
	case RPT_FacingTrail:
		m_data = new CParticleData< FacingTrailParticle >( maxParticles );
		break;
	case RPT_Beam:
		m_data = new CParticleData< BeamParticle >( maxParticles );
		break;
	case RPT_MeshParticle:
		m_data = new CParticleData< MeshParticle >( maxParticles );
		break;
	default: ASSERT( 0 );
	}

	m_data->SetGenerators( m_emitter->GetNumInitializers(), m_emitter->GetSeeds() );
}

#ifndef NO_EDITOR
void CRenderElement_ParticleEmitter::ReplaceEmitter( CRenderParticleEmitter* renderParticleEmitter, CRenderMaterial* material, CRenderMaterialParameters* parameters, EEnvColorGroup envColorGroup )
{
	ASSERT( renderParticleEmitter );
	ASSERT( material );
	ASSERT( parameters );

	// Perform buffer compatibility check to see if it should be built from scratch
	Bool bufferOk = m_emitter && m_emitter->CheckBufferCompatibility( renderParticleEmitter );

	// Release any previous emitter object
	if ( m_emitter )
	{
		m_emitter->Release();
		m_emitter = NULL;
	}

	// Assign new emitter
	m_emitter = renderParticleEmitter;
	m_emitter->AddRef();

	// Release old material and assign new one (don't care if it is the same)
	m_material->Release();
	m_material = material;
	m_material->AddRef();
	// The same with material parameters
	m_parameters->Release();
	m_parameters = parameters;
	m_parameters->AddRef();

	// Update vertex factory type
	m_vertexFactory = ParticleVertexDrawerToMaterialVertexFactory( m_emitter->GetVertexType() );

	// Update env color group
	m_envColorGroup = envColorGroup;

	// Recreate buffer if compatibility check failed
	if ( !bufferOk )
	{
		CreateParticleData();
	}
}

#endif
