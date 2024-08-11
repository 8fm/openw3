/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialDefaults.h"
#include "../engine/renderFragment.h"

RED_INLINE bool CanApplyMaterialSelectionColorToPass( ERenderingPass pass )
{
	return pass == RP_NoLighting || pass == RP_GBuffer || pass == RP_Emissive;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

void ApplyMaterialSelectionColor3v( CodeChunk &color, ERenderingPass pass )
{
	if ( CanApplyMaterialSelectionColorToPass( pass ) )
	{
		color = CodeChunkOperators::Lerp( color.xyz(), CodeChunkOperators::Float3 ( 0.2f, 1.0f, 0.4f ), 0.35f );
	}
}

void ApplyMaterialSelectionColor4v( CodeChunk &color, ERenderingPass pass )
{
	if ( CanApplyMaterialSelectionColorToPass( pass ) )
	{
		CodeChunk rgb = color.xyz();
		ApplyMaterialSelectionColor3v( rgb, pass );
		color = CodeChunkOperators::Float4( rgb, color.w() );
	}
}

#endif
