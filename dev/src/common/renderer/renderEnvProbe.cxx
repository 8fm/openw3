/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCube.h"
#include "renderEnvProbe.h"
#include "renderEnvProbeManager.h"
#include "renderHelpers.h"
#include "../core/fileLatentLoadingToken.h"
#include "../engine/envProbeComponent.h"

IMPLEMENT_RENDER_RESOURCE_ITERATOR( CRenderEnvProbe );

CRenderEnvProbe::CRenderEnvProbe( const SEnvProbeParams &probeParams, const EnvProbeDataSourcePtr &facesDataSource )
	: m_probeParams ( probeParams )
	, m_facesDataSource ( facesDataSource )
{}

CRenderEnvProbe::~CRenderEnvProbe()
{	
}

void CRenderEnvProbe::SetProbeParams( const SEnvProbeParams &params ) 
{
	m_probeParams = params;
}

SRenderEnvProbeDynamicData& CRenderEnvProbe::RefDynamicData()
{
	return m_dynamicData;
}

const SRenderEnvProbeDynamicData& CRenderEnvProbe::GetDynamicData() const
{
	return m_dynamicData;
}

CName CRenderEnvProbe::GetCategory() const
{
	return CNAME( RenderCubeTexture );
}

Uint32 CRenderEnvProbe::GetUsedVideoMemory() const
{
	return 0;
}

Bool CRenderEnvProbe::IsGlobalProbe() const
{
	return GetProbeParams().IsGlobalProbe();
}

CRenderEnvProbe* CRenderEnvProbe::Create( const SEnvProbeParams &params, const EnvProbeDataSourcePtr &facesDataSource )
{
	TScopedRenderResourceCreationObject< CRenderEnvProbe > probe ( 0 );

	if ( !facesDataSource || !facesDataSource->IsLoadable() )
	{
		return NULL;
	}

	probe.InitResource( new CRenderEnvProbe ( params, facesDataSource ) );

	return probe.RetrieveSuccessfullyCreated();
}

void CRenderEnvProbe::OnDeviceLost()
{
	// empty - everything should be handled by the envProbeManager
}

void CRenderEnvProbe::OnDeviceReset()
{
	// empty - everything should be handled by the envProbeManager
}

IRenderResource* CRenderInterface::UploadEnvProbe( const CEnvProbeComponent* envProbeComponent )
{
	ASSERT( !IsDeviceLost() && "Unable to create new render resources when device is lost" );

	if ( IsDeviceLost() )
	{
		return NULL;
	}
	
	return CRenderEnvProbe::Create( envProbeComponent->BuildProbeParams(), envProbeComponent->GetDataSource() );
}
