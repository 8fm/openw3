/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShaderPair.h"
#include "renderShader.h"

CRenderShaderPair::CRenderShaderPair( const String& fileName, const CMaterialCompilerDefines& defines )
	: m_fileName( fileName )
	, m_pixelShader( NULL )
	, m_vertexShader( NULL )
	, m_defines( defines )
{
	// Reload shader
	Reload();
}

CRenderShaderPair::~CRenderShaderPair()
{
	SAFE_RELEASE( m_pixelShader );
	SAFE_RELEASE( m_vertexShader );
}

CName CRenderShaderPair::GetCategory() const
{
	return CNAME( RenderShaderPair );
}

Uint32 CRenderShaderPair::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderShaderPair::Bind()
{
	// Bind pixel shader
	if ( m_pixelShader ) 
	{
		m_pixelShader->Bind();
	}

	// Bind vertex shader
	if ( m_vertexShader )
	{
		m_vertexShader->Bind();
	}
}

void CRenderShaderPair::BindVS()
{
	// Bind vertex shader
	if ( m_vertexShader )
	{
		m_vertexShader->Bind();
	}

	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), ERenderShaderType::RST_PixelShader );
}

void CRenderShaderPair::Reload()
{
	SAFE_RELEASE( m_pixelShader );
	SAFE_RELEASE( m_vertexShader );

	// Setup material compile definitions
	CMaterialCompilerDefines defines( m_defines );

	// Load new shader
	m_pixelShader = CRenderShader::Create( RST_PixelShader, m_fileName, defines );
	m_vertexShader = CRenderShader::Create( RST_VertexShader, m_fileName, defines );
}

CRenderShaderPair* CRenderShaderPair::Create( const String& fileName, const CMaterialCompilerDefines& defines )
{
	return new CRenderShaderPair( fileName, defines );
}

CRenderShaderPair* CRenderShaderPair::Create( const String& fileName )
{
	CMaterialCompilerDefines defines;
	return new CRenderShaderPair( fileName, defines );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CRenderShaderCompute::CRenderShaderCompute( const String& fileName, const CMaterialCompilerDefines& defines )
	: m_fileName( fileName )
	, m_shader( NULL )
	, m_defines( defines )
{
	// Reload shader
	Reload();
}

CRenderShaderCompute::~CRenderShaderCompute()
{
	SAFE_RELEASE( m_shader );
}

CName CRenderShaderCompute::GetCategory() const
{
	return CNAME( RenderShaderCompute );
}

Uint32 CRenderShaderCompute::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderShaderCompute::Dispatch( Uint32 x, Uint32 y, Uint32 z )
{
	// Bind compute shader
	if ( m_shader ) 
	{
		GetRenderer()->GetStateManager().SetShader( m_shader->GetShader(), RST_ComputeShader );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_VertexShader );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
		GpuApi::DispatchCompute( m_shader->GetShader(), x, y, z );
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_ComputeShader );
	}
}

void CRenderShaderCompute::Reload()
{
	SAFE_RELEASE( m_shader );

	// Setup material compile definitions
	CMaterialCompilerDefines defines( m_defines );
	
	// Load new shader
	m_shader = CRenderShader::Create( RST_ComputeShader, m_fileName, defines );
}

CRenderShaderCompute* CRenderShaderCompute::Create( const String& fileName, const CMaterialCompilerDefines& defines )
{
	return new CRenderShaderCompute( fileName, defines );
}

CRenderShaderCompute* CRenderShaderCompute::Create( const String& fileName )
{
	CMaterialCompilerDefines defines;
	return new CRenderShaderCompute( fileName, defines );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CRenderShaderQuadruple::CRenderShaderQuadruple( const String& fileName, const CMaterialCompilerDefines& defines )
	: CRenderShaderPair( fileName, defines )
	, m_hullShader( NULL )
	, m_domainShader( NULL )
{
	// Reload shader
	Reload();
}

CRenderShaderQuadruple::~CRenderShaderQuadruple()
{
	SAFE_RELEASE( m_hullShader );
	SAFE_RELEASE( m_domainShader );
}

CName CRenderShaderQuadruple::GetCategory() const
{
	return CNAME( RenderShaderQuadruple );
}

Uint32 CRenderShaderQuadruple::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderShaderQuadruple::Bind()
{
	CRenderShaderPair::Bind();

	// Bind pixel shader
	if ( m_hullShader ) 
	{
		m_hullShader->Bind();
	}

	// Bind vertex shader
	if ( m_domainShader )
	{
		m_domainShader->Bind();
	}
}

void CRenderShaderQuadruple::Reload()
{
	CRenderShaderPair::Reload();

	SAFE_RELEASE( m_hullShader );
	SAFE_RELEASE( m_domainShader );

	// Setup material compile definitions
	CMaterialCompilerDefines defines( m_defines );

	// Load new shader
	m_hullShader = CRenderShader::Create( RST_HullShader, m_fileName, defines );
	m_domainShader = CRenderShader::Create( RST_DomainShader, m_fileName, defines );
}

CRenderShaderQuadruple* CRenderShaderQuadruple::Create( const String& fileName, const CMaterialCompilerDefines& defines )
{
	return new CRenderShaderQuadruple( fileName, defines );
}

CRenderShaderQuadruple* CRenderShaderQuadruple::Create( const String& fileName )
{
	CMaterialCompilerDefines defines;
	return new CRenderShaderQuadruple( fileName, defines );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CRenderShaderTriple::CRenderShaderTriple( const String& fileName, const CMaterialCompilerDefines& defines )
	: CRenderShaderPair( fileName, defines )
	, m_geometryShader( NULL )
{
	// Reload shader
	Reload();
}

CRenderShaderTriple::~CRenderShaderTriple()
{
	SAFE_RELEASE( m_geometryShader );
}

CName CRenderShaderTriple::GetCategory() const
{
	return CNAME( RenderShaderTriple );
}

Uint32 CRenderShaderTriple::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderShaderTriple::Bind()
{
	CRenderShaderPair::Bind();

	// Bind pixel shader
	if ( m_geometryShader ) 
	{
		m_geometryShader->Bind();
	}
}

void CRenderShaderTriple::Reload()
{
	CRenderShaderPair::Reload();

	SAFE_RELEASE( m_geometryShader );

	// Setup material compile definitions
	CMaterialCompilerDefines defines( m_defines );

	// Load new shader
	m_geometryShader = CRenderShader::Create( RST_GeometryShader, m_fileName, defines );
}

CRenderShaderTriple* CRenderShaderTriple::Create( const String& fileName, const CMaterialCompilerDefines& defines )
{
	return new CRenderShaderTriple( fileName, defines );
}

CRenderShaderTriple* CRenderShaderTriple::Create( const String& fileName )
{
	CMaterialCompilerDefines defines;
	return new CRenderShaderTriple( fileName, defines );
}



CRenderShaderStreamOut::CRenderShaderStreamOut( const String& fileName, const CMaterialCompilerDefines& defines, GpuApi::eBufferChunkType bctOutput )
	: m_fileName( fileName )
	, m_vertexShader( NULL )
	, m_geometryShader( NULL )
	, m_defines( defines )
	, m_bctOutput( bctOutput )
{
	Reload();
}

CRenderShaderStreamOut::~CRenderShaderStreamOut()
{
	SAFE_RELEASE( m_vertexShader );
	SAFE_RELEASE( m_geometryShader );
}

CName CRenderShaderStreamOut::GetCategory() const
{
	return CNAME( RenderShaderStreamOut );
}

Uint32 CRenderShaderStreamOut::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderShaderStreamOut::Bind()
{
	if ( m_vertexShader )
	{
		m_vertexShader->Bind();
	}

	if ( m_geometryShader ) 
	{
		m_geometryShader->Bind();
	}
}

void CRenderShaderStreamOut::Reload()
{
	SAFE_RELEASE( m_vertexShader );
	SAFE_RELEASE( m_geometryShader );

	// Setup material compile definitions
	CMaterialCompilerDefines defines( m_defines );

	// Load new shader
	m_vertexShader = CRenderShader::Create( RST_VertexShader, m_fileName, defines );
	m_geometryShader = CRenderShader::CreateGSwithSO( m_fileName, defines, GpuApi::GetPackingForFormat( m_bctOutput ) );
}

CRenderShaderStreamOut* CRenderShaderStreamOut::Create( const String& fileName, const CMaterialCompilerDefines& defines, GpuApi::eBufferChunkType bctOutput )
{
	return new CRenderShaderStreamOut( fileName, defines, bctOutput );
}

CRenderShaderStreamOut* CRenderShaderStreamOut::Create( const String& fileName, GpuApi::eBufferChunkType bctOutput )
{
	CMaterialCompilerDefines defines;
	return new CRenderShaderStreamOut( fileName, defines, bctOutput );
}
