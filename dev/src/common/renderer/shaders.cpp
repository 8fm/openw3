/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeRenderInterface.h"

#ifdef USE_SPEED_TREE

#include "Core/FileSystem.h"
#include "Utilities/Utility.h"

using namespace SpeedTree;

///////////////////////////////////////////////////////////////////////
//  Shader static member variables

CShaderConstantGPUAPI::SCBuffer CShaderConstantGPUAPI::m_apConstantBuffers[SHADER_CONSTANT_GROUP_COUNT] = { CShaderConstantGPUAPI::SCBuffer() };
st_bool CShaderConstantGPUAPI::m_abBufferUpdated[SHADER_CONSTANT_GROUP_COUNT] = { true };
st_int32 CShaderConstantGPUAPI::m_anMapRegisterToGroup[c_nNumMainShaderUniformSlots] = { 0 };
st_float32 CShaderConstantGPUAPI::m_afConstantsMirror[c_nNumMainShaderUniformSlots * 4] = { 0.0f };

st_bool CShaderTechniqueGPUAPI::Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const
{
	st_bool bSuccess = false;

#ifndef NDEBUG
	if (cVertexShader.IsValid( ) && cPixelShader.IsValid( ))
	{
#endif
		GetRenderer()->GetStateManager().SetShader( cVertexShader.m_shaderRef, RST_VertexShader );

		GpuApi::ShaderRef psRef;
		if ( !cPixelShader.m_forcedNull )
		{
			psRef = cPixelShader.m_shaderRef;
		}
		GetRenderer()->GetStateManager().SetShader( psRef, RST_PixelShader );

		bSuccess = CShaderConstantGPUAPI::CommitConstants();

#ifndef NDEBUG
	}
	else
		CCore::SetError("CShaderTechniqueGPUAPI::Bind, either vertex or pixel shader was not valid");
#endif

	return bSuccess;
}


void CShaderTechniqueGPUAPI::UnBind(void)
{
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_VertexShader );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
}

#endif
