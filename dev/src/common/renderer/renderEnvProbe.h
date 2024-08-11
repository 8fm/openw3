/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderEnvProbeDynamicData.h"
#include "renderResourceIterator.h"
#include "renderHelpers.h"
#include "../engine/envProbeParams.h"


/// EnvProbe
class CRenderEnvProbe : public IDynamicRenderResource, public TRenderResourceList<CRenderEnvProbe>
{
	DECLARE_RENDER_RESOURCE_ITERATOR;
	
protected:
	EnvProbeDataSourcePtr				m_facesDataSource;
	SRenderEnvProbeDynamicData			m_dynamicData;
	SEnvProbeParams						m_probeParams;	

public:
	CRenderEnvProbe( const SEnvProbeParams &probeParams, const EnvProbeDataSourcePtr &facesDataSource );
	virtual ~CRenderEnvProbe();

	/// Describe resource
	virtual CName GetCategory() const;

	/// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	/// Get displayable name
	virtual String GetDisplayableName() const { return TXT("EnvProbe"); }

	/// Device lost/reset
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();

public:
	Bool IsGlobalProbe() const;
	SRenderEnvProbeDynamicData& RefDynamicData();
	const SRenderEnvProbeDynamicData& GetDynamicData() const;
	Vector GetProbeOrigin() const { return GetProbeParams().m_probeOrigin; }
	Vector GetProbeGenOrigin() const { return GetProbeParams().m_probeGenOrigin; }
	const SEnvProbeParams& GetProbeParams() const { return m_probeParams; }
	void SetProbeParams( const SEnvProbeParams &params );
	const EnvProbeDataSourcePtr& GetFacesDataSource() const { return m_facesDataSource; }

public:
	//! Create vertex type declaration
	static CRenderEnvProbe* Create( const SEnvProbeParams &params, const EnvProbeDataSourcePtr &facesDataSource );
};
