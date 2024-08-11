/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skyboxSetupParameters.h"
#include "renderResource.h"

SSkyboxSetupParameters::SSkyboxSetupParameters ()
	: m_sunMeshResource( nullptr )
	, m_sunMaterialResource( nullptr )
	, m_sunMaterialParamsResource( nullptr )
	, m_moonMeshResource( nullptr )
	, m_moonMaterialResource( nullptr )
	, m_moonMaterialParamsResource( nullptr )
	, m_skyboxMeshResource( nullptr )
	, m_skyboxMaterialResource( nullptr )
	, m_skyboxMaterialParamsResource( nullptr )
	, m_cloudsMeshResource( nullptr )
	, m_cloudsMaterialResource( nullptr )
	, m_cloudsMaterialParamsResource( nullptr )
{}

SSkyboxSetupParameters::~SSkyboxSetupParameters ()
{
	ReleaseAll();
}

void SSkyboxSetupParameters::AddRefAll()
{
	if ( m_sunMeshResource )				m_sunMeshResource->AddRef();
	if ( m_sunMaterialResource )			m_sunMaterialResource->AddRef();
	if ( m_sunMaterialParamsResource )		m_sunMaterialParamsResource->AddRef();
	if ( m_moonMeshResource )				m_moonMeshResource->AddRef();
	if ( m_moonMaterialResource )			m_moonMaterialResource->AddRef();
	if ( m_moonMaterialParamsResource )		m_moonMaterialParamsResource->AddRef();
	if ( m_skyboxMeshResource )				m_skyboxMeshResource->AddRef();
	if ( m_skyboxMaterialResource )			m_skyboxMaterialResource->AddRef();
	if ( m_skyboxMaterialParamsResource )	m_skyboxMaterialParamsResource->AddRef();
	if ( m_cloudsMeshResource )				m_cloudsMeshResource->AddRef();
	if ( m_cloudsMaterialResource )			m_cloudsMaterialResource->AddRef();
	if ( m_cloudsMaterialParamsResource )	m_cloudsMaterialParamsResource->AddRef();
}

void SSkyboxSetupParameters::ReleaseAll()
{
	SAFE_RELEASE( m_sunMeshResource );
	SAFE_RELEASE( m_sunMaterialResource );
	SAFE_RELEASE( m_sunMaterialParamsResource );
	SAFE_RELEASE( m_moonMeshResource );
	SAFE_RELEASE( m_moonMaterialResource );
	SAFE_RELEASE( m_moonMaterialParamsResource );
	SAFE_RELEASE( m_skyboxMeshResource );
	SAFE_RELEASE( m_skyboxMaterialResource );
	SAFE_RELEASE( m_skyboxMaterialParamsResource );
	SAFE_RELEASE( m_cloudsMeshResource );
	SAFE_RELEASE( m_cloudsMaterialResource );
	SAFE_RELEASE( m_cloudsMaterialParamsResource );
}