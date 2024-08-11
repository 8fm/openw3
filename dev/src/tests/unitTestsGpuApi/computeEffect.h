/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "effectInterface.h"

class CComputeEffect : public IEffect
{
protected:
	Char shaderFilenameStub[128];

	GpuApi::ShaderRef m_computeShader;

public:
	CComputeEffect( const Char* filenameStub )
	{
		Red::System::SNPrintF( shaderFilenameStub, ARRAY_COUNT( shaderFilenameStub ), TXT( "%ls/%ls" ), SHADERS_PATH, filenameStub );
	};

	virtual ~CComputeEffect()
	{
		GpuApi::SafeRelease(m_computeShader);
	};
	
	virtual void SetShaders() override
	{
		GpuApi::SetShader( m_computeShader, GpuApi::ComputeShader );
	};

	virtual void Dispatch(int x, int y, int z)
	{
		GpuApi::DispatchCompute( m_computeShader, x, y, z );
	};

	virtual Bool Initialize() override
	{
#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_DURANGO)

		Char shaderFilename[128];
		Red::System::SNPrintF( shaderFilename, ARRAY_COUNT( shaderFilename ), TXT( "%ls.hlsl" ), shaderFilenameStub );
		
		m_computeShader = CreateShaderFromFile( shaderFilename, GpuApi::ComputeShader );

		if( m_computeShader.isNull() )
		{
			ERR_GAT( TXT( "Failed to load CS from file!" ) );
			return false;
		}

#elif defined(RED_PLATFORM_ORBIS)

		Char shaderFileCS[128];
		Red::System::SNPrintF( shaderFileCS, ARRAY_COUNT( shaderFileCS ), TXT( "%ls_cs.sb" ), shaderFilenameStub );

		return InitializeCompute( shaderFileCS );

#endif
		return true;
	};

	Bool InitializeCompute( const Char* shaderFileCS )
	{
		Bool ret = true;

		m_computeShader = CreateShaderFromFile( shaderFileCS, GpuApi::ComputeShader );

		if( m_computeShader.isNull() )
		{
			ERR_GAT( TXT( "Failed to load VS from file!" ) );
			ret = false;
		}

		return ret;
	};
};
