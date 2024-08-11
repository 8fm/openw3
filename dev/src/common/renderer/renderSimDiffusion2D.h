
/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define DIFFUSIONMAPRESOLUTION 512

class CRenderSimDiffusion2D
{
public:
	CRenderSimDiffusion2D();
	~CRenderSimDiffusion2D();
	void Initialize();
	GpuApi::TextureRef Calculate( GpuApi::TextureRef helperMask, const Float frameTime, const Vector& m_cameraForward );

private:
	Bool  m_flipped;
	Int32 m_resolution;

	DebugVertexUV m_points[6];
	GpuApi::TextureRef m_tex1;
	GpuApi::TextureRef m_tex2;
	Bool m_initialized;

	Vector m_cameraPreviousForward;				// used by focus mode
	Float m_timeAccum;							// used by focus mode
};