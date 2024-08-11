///////////////////////////////////////////////////////////////////////  
//  StateFunctions.inl
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
//
//  *** Release version 6.0 ***


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::Initialize

inline void CStateFunctionsOrbis::Init(void)
{
	//cellGcmSetWriteCommandLabel(gCellGcmCurrentContext, SPEEDTREE_LABEL_RENDER_2_TEXTURE, SPEEDTREE_FLAG_AVAILABLE);
	//cellGcmSetWriteCommandLabel(gCellGcmCurrentContext, SPEEDTREE_LABEL_IMMEDIATE_WAIT, SPEEDTREE_FLAG_AVAILABLE);
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::ApplyStates

inline void CStateFunctionsOrbis::ApplyStates(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::Destroy

inline void CStateFunctionsOrbis::Release(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::ClearScreen

inline void CStateFunctionsOrbis::ClearScreen(st_bool bClearColor, st_bool bClearDepth)
{
	/*cellGcmSetClearColor(gCellGcmCurrentContext, 0xFF000000);
	cellGcmSetClearDepthStencil(gCellGcmCurrentContext, 0xFFFFFF00);

	uint32_t uiBitField = ((bClearColor ? (CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A) : 0) |
							(bClearDepth ? (CELL_GCM_CLEAR_Z) : 0));
	cellGcmSetClearSurface(gCellGcmCurrentContext, uiBitField);*/ 
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetBlending

inline void CStateFunctionsOrbis::SetBlending(st_bool bFlag)
{
	/*if (bFlag)
	{
		cellGcmSetBlendFunc(gCellGcmCurrentContext, CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA, 
													CELL_GCM_ZERO, CELL_GCM_ZERO);
	}

	st_uint32 uiFlag = (bFlag ? CELL_GCM_TRUE : CELL_GCM_FALSE);
	cellGcmSetBlendEnable(gCellGcmCurrentContext, uiFlag);
	cellGcmSetBlendEnableMrt(gCellGcmCurrentContext, uiFlag, uiFlag, uiFlag);*/
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetColorMask

inline void CStateFunctionsOrbis::SetColorMask(st_bool bRed, st_bool bGreen, st_bool bBlue, st_bool bAlpha)
{
	/*cellGcmSetColorMask(gCellGcmCurrentContext, (bRed ? CELL_GCM_COLOR_MASK_R : 0) | 
												(bGreen ? CELL_GCM_COLOR_MASK_G : 0) | 
												(bBlue ? CELL_GCM_COLOR_MASK_B : 0) | 
												(bAlpha ? CELL_GCM_COLOR_MASK_A : 0));*/
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetDepthMask

inline void CStateFunctionsOrbis::SetDepthMask(st_bool bFlag)
{
	//cellGcmSetDepthMask(gCellGcmCurrentContext, bFlag ? CELL_GCM_TRUE : CELL_GCM_FALSE);
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetDepthTestFunc

inline void CStateFunctionsOrbis::SetDepthTestFunc(EDepthTestFunc eDepthTestFunc)
{
	/*static st_uint32 aCmpFuncs[ ] =
	{
		CELL_GCM_NEVER,
		CELL_GCM_LESS,
		CELL_GCM_EQUAL,
		CELL_GCM_LEQUAL,
		CELL_GCM_GREATER,
		CELL_GCM_NOTEQUAL,
		CELL_GCM_GEQUAL,
		CELL_GCM_ALWAYS
	};

	cellGcmSetDepthFunc(gCellGcmCurrentContext, aCmpFuncs[eDepthTestFunc]);*/
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetDepthTesting

inline void CStateFunctionsOrbis::SetDepthTesting(st_bool bFlag)
{
	//cellGcmSetDepthTestEnable(gCellGcmCurrentContext, bFlag ? CELL_GCM_TRUE : CELL_GCM_FALSE);
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetFaceCulling

inline void CStateFunctionsOrbis::SetFaceCulling(ECullType eCullType)
{
	/*if (eCullType == CULLTYPE_NONE)
		cellGcmSetCullFaceEnable(gCellGcmCurrentContext, CELL_GCM_FALSE);
	else 
	{
		cellGcmSetCullFaceEnable(gCellGcmCurrentContext, CELL_GCM_TRUE);
		if (eCullType == CULLTYPE_BACK)
			cellGcmSetCullFace(gCellGcmCurrentContext, CELL_GCM_BACK);
		else
			cellGcmSetCullFace(gCellGcmCurrentContext, CELL_GCM_FRONT);
	}*/
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetPointSize

inline void CStateFunctionsOrbis::SetPointSize(st_float32 fSize)
{
	//cellGcmSetPointSize(gCellGcmCurrentContext, fSize);
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetPolygonOffset

inline void CStateFunctionsOrbis::SetPolygonOffset(st_float32 fFactor, st_float32 fUnits)
{
	if (fFactor == 0.0f && fUnits == 0.0f)
		cellGcmSetPolygonOffsetFillEnable(gCellGcmCurrentContext, CELL_GCM_FALSE);
	else
	{
		cellGcmSetPolygonOffsetFillEnable(gCellGcmCurrentContext, CELL_GCM_TRUE);
		cellGcmSetPolygonOffset(gCellGcmCurrentContext, fFactor * 2.0f, fUnits);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetRenderStyle

inline void CStateFunctionsOrbis::SetRenderStyle(ERenderStyle eStyle)
{
	//cellGcmSetFrontPolygonMode(gCellGcmCurrentContext, (eStyle == RENDERSTYLE_WIREFRAME) ? CELL_GCM_POLYGON_MODE_LINE : CELL_GCM_POLYGON_MODE_FILL);
	//cellGcmSetBackPolygonMode(gCellGcmCurrentContext, (eStyle == RENDERSTYLE_WIREFRAME) ? CELL_GCM_POLYGON_MODE_LINE : CELL_GCM_POLYGON_MODE_FILL);
}

///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetMultisampling

inline void CStateFunctionsOrbis::SetMultisampling(st_bool bMultisample)
{
	/*m_bMultisample = bMultisample;
	cellGcmSetAntiAliasingControl(gCellGcmCurrentContext, 
									m_bMultisample ? CELL_GCM_TRUE : CELL_GCM_FALSE,
									m_bAlpha2Coverage ? CELL_GCM_TRUE : CELL_GCM_FALSE, 
									CELL_GCM_FALSE, 0xFFFF);*/
}


///////////////////////////////////////////////////////////////////////  
//  CStateFunctionsOrbis::SetAlphaToCoverage

inline void CStateFunctionsOrbis::SetAlphaToCoverage(st_bool bFlag)
{
	/*m_bAlpha2Coverage = bFlag;

	cellGcmSetAntiAliasingControl(gCellGcmCurrentContext, 
									m_bMultisample ? CELL_GCM_TRUE : CELL_GCM_FALSE,
									m_bAlpha2Coverage ? CELL_GCM_TRUE : CELL_GCM_FALSE, 
									CELL_GCM_FALSE, 0xFFFF);*/
}

