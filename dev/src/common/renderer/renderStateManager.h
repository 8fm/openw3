/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once 
#include "renderHelpers.h"


#define MAX_CONST_REGISTER		224
#define MAX_SAMPLERS			16
#define MAX_RENDER_STATES		256
#define MAX_SAMPLER_STATES		16

struct SCameraConstants
{
	Matrix worldToScreen;
	Matrix worldToView;
	Vector cameraPosition;
	Vector cameraVectorRight;
	Vector cameraVectorForward;
	Vector cameraVectorUp;
	Vector viewportParams;
	Vector wetSurfaceEffect;
	Vector revProjCameraInfo;
	Vector cameraNearFar;
};

class CGameplayEffects;

/// State manager
class CRenderStateManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
protected:
	GpuApi::ShaderRef				m_cachedShader[ RST_Max ];
	GpuApi::BufferRef				m_cameraConstantBuffer;
	GpuApi::BufferRef				m_globalConstantBuffer;
	const CRenderCamera*			m_prevCamera;
	Vector							m_prevViewportDimensions;
	Vector							m_weatherParams;
	Bool							m_forceNullPS;
	TDynArray<GpuApi::ShaderRef>	m_shaderStack;
	
public:
	CRenderStateManager( );
	~CRenderStateManager();
	void Reset();

	// Set states

	RED_FORCE_INLINE void SetPixelConst( Uint32 index, const Vector& data )
	{
		SetPixelShaderConstRaw( index, &data.A[0], 1 );
	}

	RED_FORCE_INLINE void SetPixelConst( Uint32 index, const Matrix& data )
	{
		Float matrixData[ 16 ];
		data.GetColumnMajor( matrixData );
		SetPixelShaderConstRaw( index, matrixData, 4 );
	}

	RED_FORCE_INLINE void SetVertexConst( Uint32 index, const Vector& data )
	{
		SetVertexShaderConstRaw( index, &data.A[0], 1 );
	}

	RED_FORCE_INLINE void SetVertexConst( Uint32 index, const Matrix& data )
	{
		Float matrixData[ 16 ];
		data.GetColumnMajor( matrixData );
		SetVertexShaderConstRaw( index, matrixData, 4 );
	}

	RED_FORCE_INLINE void SetShader( const GpuApi::ShaderRef& shader, ERenderShaderType shaderType )
	{
		if ( shader == m_cachedShader[shaderType] )
			return;

		m_cachedShader[shaderType] = shader;
		GpuApi::SetShader( shader, Map(shaderType) );
	}

	RED_FORCE_INLINE void SetWeatherParams( const Vector& weatherParams ) 
	{
		m_weatherParams = weatherParams;
	}

	RED_FORCE_INLINE void SetVertexShaderConstRaw( Uint32 reg, const Float* data, Uint32 numRegs )
	{
		GpuApi::SetVertexShaderConstF( reg, data, numRegs );
	}

	RED_FORCE_INLINE void SetPixelShaderConstBatch( Uint32 reg, const Float* data, Uint32 numRegs )
	{
		//const Uint32 dataSize = numRegs * 4 * sizeof( Float );
		GpuApi::SetPixelShaderConstF( reg, data, numRegs );
	}

	RED_FORCE_INLINE void SetVertexShaderConstBatch( Uint32 reg, const Float* data, Uint32 numRegs )
	{
		//const Uint32 dataSize = numRegs * 4 * sizeof( Float );
		GpuApi::SetVertexShaderConstF( reg, data, numRegs );
	}

	RED_FORCE_INLINE void SetPixelShaderConstRaw( Uint32 reg, const Float* data, Uint32 numRegs )
	{
		GpuApi::SetPixelShaderConstF( reg, data, numRegs );
	}

	void ForceNullPS( Bool enable );
	
	// High level functions
	void SetGlobalShaderConstants( const CRenderFrameInfo &info, Uint32 fullRenderTargetWidth, Uint32 fullRenderTargetHeight, CGameplayEffects* gameplayFx );
	void BindGlobalConstants();
	void UnbindGlobalConstants();

	void SetCamera2D();
	void SetCamera( const CRenderCamera& camera );
	void SetLocalToWorld( const Matrix* matrix );
	void SetLocalToWorldF( const Float* floatArray );

	void PushShaderSetup();
	void PopShaderSetup();
};
