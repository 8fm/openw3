/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct SMeshRenderParams
{
	Float						m_meshRenderDist;
	Float						m_meshLODRenderDist;

	SMeshRenderParams()
		: m_meshRenderDist( 1.f )
		, m_meshLODRenderDist( 1.f )
	{
	}
};