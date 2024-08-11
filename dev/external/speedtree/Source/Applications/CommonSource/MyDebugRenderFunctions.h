///////////////////////////////////////////////////////////////////////
//  MyDebugRenderFunctions.h
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
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
//  Preprocessor

#pragma once
#include "MySpeedTreeRenderer.h"
#include "MyTerrain.h"


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////
	//  Class CMyDebugRenderFunctions
	//
	//	Only supported under SPEEDTREE_WIN_OPENGL.

	class CMyDebugRenderFunctions
	{
	public:
	static	void	RenderAxes(const CView& cView);
	static	void	RenderFrustum(const CView& cView);
	static	void	RenderTreeBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderTreeCullSpheres(const CView& cView, const CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderVisibleCellBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderRoughCellBoundingBoxes(const CView& cView, CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderCellCullSpheres(const CView& cView, CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderGrassAsLines(const CView& cView, const CVisibleInstancesRender& cVisibleInstances);
	static	void	RenderTerrainNormals(const CView& cView, const CMyTerrain& cTerrain);
	static	void	RenderDistanceCues(const CView& cView, const CMyTerrain& cTerrain);
	static	void	RenderBillboardSpikes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances);

	private:
	static	void	DrawBox(const st_float32 afMin[3], const st_float32 afMax[3], st_bool bLines);
	static	void	DrawSphere(const Vec3& vCenter, st_float32 fRadius);
	static	void	BeginFixedPipelineRender(const CView& cView);
	static	void	EndFixedPipelineRender(void);
	};
	
} // end namespace SpeedTree


