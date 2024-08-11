///////////////////////////////////////////////////////////////////////  
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Class CVisibleInstancesRI

typedef CVisibleInstancesRI<CStateBlock,
							CTexture, 
							CGeometryBuffer,
							CInstancingMgr,
							CShaderTechnique,
							CShaderConstant,
							CShaderConstantBuffer> CVisibleInstancesRender;


///////////////////////////////////////////////////////////////////////  
//  Class CForestRender

typedef CForestRI<CStateBlock,
				  CTexture, 
				  CGeometryBuffer,
				  CInstancingMgr,
				  CShaderTechnique,
				  CShaderConstant,
				  CShaderConstantBuffer> CForestRender;


///////////////////////////////////////////////////////////////////////  
//  Class CRenderState

typedef CRenderStateRI<CStateBlock,
					   CTexture, 
					   CShaderTechnique,
					   CShaderConstant,
					   CShaderConstantBuffer> CRenderState; 


///////////////////////////////////////////////////////////////////////  
//  Class CTreeRender

typedef CTreeRI<CStateBlock,
				CTexture, 
				CGeometryBuffer,
				CShaderTechnique,
				CShaderConstant,
				CShaderConstantBuffer> CTreeRender;


///////////////////////////////////////////////////////////////////////  
//  Class CTerrainRender

typedef CTerrainRI<CStateBlock,
				   CTexture, 
				   CGeometryBuffer, 
				   CShaderTechnique,
				   CShaderConstant,
				   CShaderConstantBuffer> CTerrainRender;


///////////////////////////////////////////////////////////////////////  
//  Class CSkyRender

typedef CSkyRI<CStateBlock,
			   CTexture,
			   CGeometryBuffer, 
			   CShaderTechnique, 
			   CShaderConstant,
			   CShaderConstantBuffer> CSkyRender;


///////////////////////////////////////////////////////////////////////  
//  Explicit static member template specialization declarations and exports

#if defined(SPEEDTREE_USE_SDK_AS_DLLS) || defined(SPEEDTREE_NEEDS_EXPLICIT_TEMPLATE_DECL)
	template<> ST_DLL_LINK CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache;
	template<> ST_DLL_LINK CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache;
	template<> ST_DLL_LINK CTexture::CTextureCache* CTexture::m_pCache;
	template<> ST_DLL_LINK CStateBlock::CStateBlockCache* CStateBlock::m_pCache;
	template<> ST_DLL_LINK CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS];
	template<> ST_DLL_LINK CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS];
	template<> ST_DLL_LINK st_int32 CRenderState::m_nFallbackTextureRefCount;
	template<> ST_DLL_LINK CShaderConstantBuffer CForestRender::m_cFrameConstantBuffer;
	template<> ST_DLL_LINK SFrameCBLayout CForestRender::m_sFrameConstantBufferLayout;
#endif
