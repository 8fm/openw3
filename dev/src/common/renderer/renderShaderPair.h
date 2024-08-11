/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/materialCompilerDefines.h"
#include "renderStaticResource.h"

class CRenderShader;

/// Shader pair - pair of shaders ( pixel and vertex )
class CRenderShaderPair : public IStaticRenderResource
{
protected:	
	String						m_fileName;			//!< Source file name
	CRenderShader*				m_pixelShader;		//!< Pixel shader
	CRenderShader*				m_vertexShader;		//!< Vertex shader
	CMaterialCompilerDefines	m_defines;			//!< Compiler defines

public:
	CRenderShaderPair( const String& fileName, const CMaterialCompilerDefines& defines );
	~CRenderShaderPair();

	// Returns true if the pair is valid
	RED_INLINE Bool IsValid() const { return m_pixelShader != NULL && m_vertexShader != NULL; }

	// Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	// Bind shader
	virtual void Bind();

	// Bind VS
	void BindVS();

	// Reload shaders
	virtual void Reload();

public:
	// Load and compile shaders with provided defines list
	static CRenderShaderPair* Create( const String& fileName, const CMaterialCompilerDefines& defines );

	// Load and compile shaders
	static CRenderShaderPair* Create( const String& fileName );
};

/// Shader pair - pair of shaders ( pixel and vertex )
class CRenderShaderCompute : public IStaticRenderResource
{
protected:	
	String						m_fileName;			//!< Source file name
	CRenderShader*				m_shader;			//!< CS
	CMaterialCompilerDefines	m_defines;			//!< Compiler defines

public:
	CRenderShaderCompute( const String& fileName, const CMaterialCompilerDefines& defines );
	~CRenderShaderCompute();

	// Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	// Bind shader
	virtual void Dispatch( Uint32 x, Uint32 y, Uint32 z );

	// Reload shaders
	virtual void Reload();

	CRenderShader* GetShader() const {return m_shader;}

public:
	// Load and compile shaders with provided defines list
	static CRenderShaderCompute* Create( const String& fileName, const CMaterialCompilerDefines& defines );

	// Load and compile shaders
	static CRenderShaderCompute* Create( const String& fileName );
};

/// Shader pair - pair of shaders ( pixel and vertex )
class CRenderShaderQuadruple : public CRenderShaderPair
{
protected:	
	CRenderShader*				m_hullShader;
	CRenderShader*				m_domainShader;

public:
	CRenderShaderQuadruple( const String& fileName, const CMaterialCompilerDefines& defines );
	~CRenderShaderQuadruple();

	// Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	// Bind shader
	virtual void Bind();

	// Reload shaders
	virtual void Reload();

public:
	// Load and compile shaders with provided defines list
	static CRenderShaderQuadruple* Create( const String& fileName, const CMaterialCompilerDefines& defines );

	// Load and compile shaders
	static CRenderShaderQuadruple* Create( const String& fileName );
};

/// vertex and pixel shader pair extended by a geometry shader
class CRenderShaderTriple : public CRenderShaderPair
{
protected:	
	CRenderShader*				m_geometryShader;

public:
	CRenderShaderTriple( const String& fileName, const CMaterialCompilerDefines& defines );
	~CRenderShaderTriple();

	// Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	// Bind shader
	virtual void Bind();

	// Reload shaders
	virtual void Reload();

public:
	// Load and compile shaders with provided defines list
	static CRenderShaderTriple* Create( const String& fileName, const CMaterialCompilerDefines& defines );

	// Load and compile shaders
	static CRenderShaderTriple* Create( const String& fileName );
};


/// Vertex/Geometry pair for stream-out.
class CRenderShaderStreamOut : public IStaticRenderResource
{
protected:	
	String						m_fileName;			//!< Source file name
	CRenderShader*				m_vertexShader;
	CRenderShader*				m_geometryShader;
	CMaterialCompilerDefines	m_defines;			//!< Compiler defines
	GpuApi::eBufferChunkType	m_bctOutput;

public:
	CRenderShaderStreamOut( const String& fileName, const CMaterialCompilerDefines& defines, GpuApi::eBufferChunkType bctOutput );
	~CRenderShaderStreamOut();

	// Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	// Bind shader
	virtual void Bind();

	// Reload shaders
	virtual void Reload();

	GpuApi::eBufferChunkType GetOutputChunkType() const { return m_bctOutput; }

public:
	// Load and compile shaders with provided defines list
	static CRenderShaderStreamOut* Create( const String& fileName, const CMaterialCompilerDefines& defines, GpuApi::eBufferChunkType bctOutput );

	// Load and compile shaders
	static CRenderShaderStreamOut* Create( const String& fileName, GpuApi::eBufferChunkType bctOutput );
};
