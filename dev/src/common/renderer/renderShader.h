/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderHelpers.h"

class CMaterialCompilerDefines;

/// Shader
class CRenderShader : public IStaticRenderResource
{
	DECLARE_RENDER_OBJECT_MEMORYPOOL( MemoryPool_SmallObjects, MC_RenderResource );

protected:
	Uint64				m_hash;
	ERenderShaderType	m_type;			//!< Shader type
	GpuApi::ShaderRef	m_shader;		//!< Shader

public:
	CRenderShader( ERenderShaderType type, GpuApi::ShaderRef shader, Uint64 hash );
	~CRenderShader();

	// Describe resource
	virtual CName GetCategory() const;

	// Bind shader
	void Bind();

	// Unbind shader (required by some Xbox operations)
	void UnBind();

	GpuApi::ShaderRef& GetShader(){ return m_shader; }

public:
	// Compile shader from HLSL code
	static CRenderShader* Create( ERenderShaderType type, const AnsiChar* code, const CMaterialCompilerDefines& defines, const String& fileName, Uint64& hash, Bool isStatic = false );

	// Compile shader from HLSL code loaded from list
	static CRenderShader* Create( ERenderShaderType type, const String& fileName, const CMaterialCompilerDefines& defines );

	// Compile shader from precompiled data
	static CRenderShader* Create( ERenderShaderType type, const DataBuffer& shaderData, const Uint64 hash, const String& fileName );

	// Create a Geometry Shader that will be usable for stream-out. Simultaneous stream-out and rasterization is not currently supported.
	// 'outputDesc' describes the shader's output layout. Up to 4 output slots are supported (slot type is ignored).
	//
	// Unlike creating vertex layouts, PS_Invalid is allowed in the layout desc. Will cause the SO to skip that section of the buffer.
	// Using both PS_Invalid and PT_Invalid will mark the end of the desc elements.
	//
	// adjustedDesc can optionally return a modified version of the desc, which will be suitable for use as vertex layout (which further adjustments
	// needed if PS_Invalid was used to skip portions). This is needed since SO only seems to write full 4-byte register values. It is important to
	// note that if adjustments were made, the size of a vertex may be different and so the SO buffer(s) should be sized accordingly.
	static CRenderShader* CreateGSwithSO( const AnsiChar* code, const CMaterialCompilerDefines& defines, const String& fileName, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr, Bool isStatic = false );
	static CRenderShader* CreateGSwithSO( const String& fileName, const CMaterialCompilerDefines& defines, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );
	static CRenderShader* CreateGSwithSO( const DataBuffer& shaderData, const Uint64 hash, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );

	static Bool CreateFromCache( const String& fileName, const AnsiChar* code, const TDynArray< GpuApi::ShaderDefine >& defines, ERenderShaderType type, const AnsiChar* entryPoint, Bool isStatic, Bool hasStreamOut, CRenderShader*& outShader, Uint64& filenameHash, Uint64& contentHash, const GpuApi::VertexLayoutDesc* outputDesc = nullptr, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );
	static Bool CreateStaticFromCache( const String& fileName, const TDynArray< GpuApi::ShaderDefine >& defines, ERenderShaderType type, Bool hasStreamOut, CRenderShader*& outShader, const GpuApi::VertexLayoutDesc* outputDesc =nullptr, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );

private:
	static void MapDefines( ERenderShaderType type, const CMaterialCompilerDefines& defines, TDynArray< GpuApi::ShaderDefine >& outDefines );
};

class CRenderShaderMap
{
public:
	static Red::Threads::CMutex	s_mutex;

protected:
	THashMap< Uint64, CRenderShader* >	m_map;

public:
	CRenderShaderMap();
	~CRenderShaderMap();

	Bool Exists( Uint64 key );								//!< Locking is required on user side
	Bool Insert( Uint64 key, CRenderShader* shader );		//!< Locking is required on user side
	Bool Erase( Uint64 key );								//!< Locking is required on user side
	Bool Find( Uint64 hash, CRenderShader*& outShader );	//!< Locking is required on user side
};

extern CRenderShaderMap* GRenderShaderMap;