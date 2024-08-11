/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct SFoliageRenderParams
{
	Float						m_foliageDistShift;
	Float						m_grassDistScale;

	SFoliageRenderParams()
		: m_foliageDistShift( 1.f )
		, m_grassDistScale( 1.f )
	{
	}
};