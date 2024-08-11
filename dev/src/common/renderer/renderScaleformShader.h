/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformIncludes.h"

#include "renderScaleformTemp.h"
#include "Render/Render_Shader.h"

class CRenderScaleformHAL;
class CRenderScaleformTexture;

struct CRenderScaleformVertexShader
{
	const ScaleformGpuApi::VertexShaderDesc*             pDesc;
	GpuApi::ShaderRef					m_prog;
	int                                 UniformOffset[ScaleformGpuApi::Uniform::SU_Count];

	CRenderScaleformVertexShader() : pDesc(0) {}
	~CRenderScaleformVertexShader() { Shutdown(); }

	bool Init(const ScaleformGpuApi::VertexShaderDesc* pd);
	void Shutdown();
};

struct CRenderScaleformFragShader
{
	const ScaleformGpuApi::FragShaderDesc*       pDesc;
	GpuApi::ShaderRef			m_prog;
	SF::UPInt                   Offset;
	int                         UniformOffset[ScaleformGpuApi::Uniform::SU_Count];

	CRenderScaleformFragShader() { Offset = 0; }
	~CRenderScaleformFragShader() { Shutdown(); };

	bool Init(const ScaleformGpuApi::FragShaderDesc* pd);
	void Shutdown();
};

struct CRenderScaleformShaderPair
{
	const CRenderScaleformVertexShader*     pVS;
	const ScaleformGpuApi::VertexShaderDesc* pVDesc;
	const CRenderScaleformFragShader*       pFS;
	const ScaleformGpuApi::FragShaderDesc*   pFDesc;
	const SF::Render::VertexFormat*     pVFormat;

	CRenderScaleformShaderPair() : pVS(0), pVDesc(0), pFS(0), pFDesc(0), pVFormat(0) {}

	const CRenderScaleformShaderPair* operator->() const { return this; }
	operator bool() const { return pVS && pFS && pVS->m_prog && pFS->m_prog && pVFormat; }
};

class CRenderScaleformSysVertexFormat : public SF::Render::SystemVertexFormat
{
public:
	GpuApi::eBufferChunkType	m_chunkType;

public:
	CRenderScaleformSysVertexFormat(const SF::Render::VertexFormat* vf, const ScaleformGpuApi::VertexShaderDesc* vdesc );
};

class CRenderScaleformShaderInterface : public SF::Render::ShaderInterfaceBase<ScaleformGpuApi::Uniform,CRenderScaleformShaderPair>
{
	friend class CRenderScaleformShaderManager;

	CRenderScaleformHAL*					pHal;
	CRenderScaleformShaderPair            CurShaders;     // Currently used set of VS/FS.
	
	inline  CRenderScaleformHAL* GetHAL() const { return pHal; }

public:
	typedef const CRenderScaleformShaderPair CRenderScaleformShader;

	// FIXME:/CLEANUP
	// For the base class: typedef typename ShaderInterface::Shader        Shader;
	typedef CRenderScaleformShader Shader;

	CRenderScaleformShaderInterface(SF::Render::HAL* phal): pHal((CRenderScaleformHAL*)phal) { }

	void                BeginScene();

	const CRenderScaleformShader&       GetCurrentShaders() const { return CurShaders; }
	bool                SetStaticShader(ScaleformGpuApi::ShaderDesc::ShaderType shader, const SF::Render::VertexFormat* pvf);

	void                SetTexture(CRenderScaleformShader, unsigned stage, SF::Render::Texture* ptexture, SF::Render::ImageFillMode fm, unsigned index = 0);

	void                Finish(unsigned meshCount);
};

class CRenderScaleformShaderManager : public SF::Render::StaticShaderManager<ScaleformGpuApi::ShaderDesc, ScaleformGpuApi::VertexShaderDesc, ScaleformGpuApi::Uniform, CRenderScaleformShaderInterface, CRenderScaleformTexture>
{
	friend class CRenderScaleformShaderInterface;
public:
	typedef SF::Render::StaticShaderManager<ScaleformGpuApi::ShaderDesc, ScaleformGpuApi::VertexShaderDesc, ScaleformGpuApi::Uniform, CRenderScaleformShaderInterface, CRenderScaleformTexture> Base;
	typedef ScaleformGpuApi::Uniform UniformType;

	CRenderScaleformShaderManager(SF::Render::ProfileViews* prof);

	// *** StaticShaderManager
	void    MapVertexFormat(SF::Render::PrimitiveFillType fill, const SF::Render::VertexFormat* sourceFormat,
		const SF::Render::VertexFormat** single, const SF::Render::VertexFormat** batch, const SF::Render::VertexFormat** instanced);

	// D3D1x Specific.
	bool    HasInstancingSupport() const;

	bool    Initialize(CRenderScaleformHAL* phal);
	void    Reset();

	void    BeginScene();
	void    EndScene();

	ScaleformGpuApi::ShaderDesc::ShaderVersion GetShaderVersion() const { return ShaderModel; }
	static unsigned GetDrawableImageFlags() { return 0; }

private:
	CRenderScaleformFragShader                      StaticFShaders[ScaleformGpuApi::FragShaderDesc::FSI_Count];
	CRenderScaleformVertexShader                    StaticVShaders[ScaleformGpuApi::VertexShaderDesc::VSI_Count];

	ScaleformGpuApi::ShaderDesc::ShaderVersion       ShaderModel;    // Holds which ShaderModel version should be used.
};

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////