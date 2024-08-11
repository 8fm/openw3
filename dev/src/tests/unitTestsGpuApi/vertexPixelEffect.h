/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "effectInterface.h"

class CVertexPixelEffect : public IEffect
{
protected:
	Char shaderFilenameStub[128];

	GpuApi::ShaderRef m_vertexShader;
	GpuApi::ShaderRef m_pixelShader;

public:
	CVertexPixelEffect( const Char* filenameStub )
	{
		Red::System::SNPrintF( shaderFilenameStub, ARRAY_COUNT( shaderFilenameStub ), TXT( "%ls/%ls" ), SHADERS_PATH, filenameStub );
	};

	virtual ~CVertexPixelEffect()
	{
		GpuApi::SafeRelease(m_vertexShader);
		GpuApi::SafeRelease(m_pixelShader);
	};

	virtual void SetShaders() override
	{
		GpuApi::SetShader( m_vertexShader, GpuApi::VertexShader );
		GpuApi::SetShader( m_pixelShader, GpuApi::PixelShader );
	}

	virtual Bool Initialize() override
	{
		Char shaderFileVS[128];
		Char shaderFilePS[128];
		GpuApi::eShaderLanguage shaderLanguage = GpuApi::GetShaderLanguage();
		switch (shaderLanguage)
		{
		case GpuApi::SL_HLSL:
			Red::System::SNPrintF( shaderFileVS, ARRAY_COUNT( shaderFileVS ), TXT( "%ls.hlsl" ), shaderFilenameStub );
			Red::System::SNPrintF( shaderFilePS, ARRAY_COUNT( shaderFilePS ), TXT( "%ls.hlsl" ), shaderFilenameStub );
			break;
		case GpuApi::SL_GLSL:
			Red::System::SNPrintF( shaderFileVS, ARRAY_COUNT( shaderFileVS ), TXT( "%ls.vert" ), shaderFilenameStub );
			Red::System::SNPrintF( shaderFilePS, ARRAY_COUNT( shaderFilePS ), TXT( "%ls.frag" ), shaderFilenameStub );
			break;
		case GpuApi::SL_PSSL:
			Red::System::SNPrintF( shaderFileVS, ARRAY_COUNT( shaderFileVS ), TXT( "%ls_vs.sb" ), shaderFilenameStub );
			Red::System::SNPrintF( shaderFilePS, ARRAY_COUNT( shaderFilePS ), TXT( "%ls_ps.sb" ), shaderFilenameStub );
			break;
		default:
			break;
		}
		return InitializeVertexPixel( shaderFileVS, shaderFilePS );
	}

protected:
	Bool InitializeVertexPixel( const Char* shaderFileVS, const Char* shaderFilePS )
	{
		Bool ret = true;

		m_vertexShader = CreateShaderFromFile( shaderFileVS, GpuApi::VertexShader );

		if( m_vertexShader.isNull() )
		{
			ERR_GAT( TXT( "Failed to load VS from file!" ) );
			ret = false;
		}

		m_pixelShader = CreateShaderFromFile( shaderFilePS, GpuApi::PixelShader );

		if( m_pixelShader.isNull() )
		{
			ERR_GAT( TXT( "Failed to load PS from file!" ) );
			GpuApi::SafeRelease( m_vertexShader );
			ret = false;
		}

		return ret;
	}
};
