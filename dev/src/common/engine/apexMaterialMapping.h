/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderResource.h"


struct SApexMaterialMapping
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Physics, MC_ApexMaterialMapping );

	IRenderResource* m_material;
	IRenderResource* m_materialParameters;

	SApexMaterialMapping( IRenderResource* material, IRenderResource* params )
		: m_material( material )
		, m_materialParameters( params )
	{
		if ( m_material ) m_material->AddRef();
		if ( m_materialParameters ) m_materialParameters->AddRef();
	}

	~SApexMaterialMapping()
	{
		SAFE_RELEASE( m_material );
		SAFE_RELEASE( m_materialParameters );
	}

	void Set( IRenderResource* material, IRenderResource* params )
	{
		// Only need to set if the objects change. If we're setting the same material, we don't really need to release/addref again.
		if ( m_material != material )
		{
			if ( material ) material->AddRef();
			SAFE_RELEASE( m_material );
			m_material = material;
		}
		if ( m_materialParameters != params )
		{
			if ( params ) params->AddRef();
			SAFE_RELEASE( m_materialParameters );
			m_materialParameters = params;
		}
	}
};
