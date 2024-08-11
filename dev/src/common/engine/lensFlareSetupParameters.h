/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "flareParameters.h"


struct SLensFlareElementSetupParameters
{
	SLensFlareElementSetupParameters ();
	~SLensFlareElementSetupParameters ();

	void AddRefAll();
	void ReleaseAll();

	IRenderResource*	m_materialResource;
	IRenderResource*	m_materialParamsResource;
	Bool				m_isConstRadius;
	Bool				m_isAligned;
	Float				m_centerFadeStart;
	Float				m_centerFadeRange;
	Uint32				m_colorGroupParamsIndex;
	Float				m_alpha;
	Float				m_size;
	Float				m_aspect;
	Float				m_shift;
	Float				m_pivot;
	Vector				m_colorLinear;
};

struct SLensFlareSetupParameters
{
	SLensFlareSetupParameters ();

	void AddRefAll();
	void ReleaseAll();

	Float	m_nearDistance;
	Float	m_nearInvRange;
	Float	m_farDistance;
	Float	m_farInvRange;
	TDynArray<SLensFlareElementSetupParameters>		m_elements;
};

struct SLensFlareGroupsSetupParameters
{
	void AddRefAll();
	void ReleaseAll();

	SLensFlareSetupParameters m_groups[LFG_MAX];
};