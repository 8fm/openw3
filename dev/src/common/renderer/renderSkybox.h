/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/skyboxSetupParameters.h"

class CRenderFrameInfo;

class CRenderSkybox
{
private:
	SSkyboxSetupParameters m_setup;

public:
	enum
	{
		SUN_AND_MOON_RENDER_DIST		= 100,
		SUN_AND_MOON_RENDER_SCALE_BASE	= 10,
	};

public:
	CRenderSkybox ();
	~CRenderSkybox ();

	const SSkyboxSetupParameters& GetParameters() const { return m_setup; }
	void SetParameters( const SSkyboxSetupParameters &params );
	void Render( const CRenderFrameInfo &info, const RenderingContext &context, MeshDrawingStats &meshStats, Bool allowNonClouds, Bool allowClouds ) const;
	void Tick( Float timeDelta );
};
