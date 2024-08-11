/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialRootBlock );

Bool CMaterialRootBlock::IsInvariant() const
{
	// Root blocks are always variant ( they need to be evaluated in hardware )
	return false;
}

Bool CMaterialRootBlock::IsRootBlock() const
{
	// Obvious
	return true;
}

bool CMaterialRootBlock::IsMasked() const
{
	return HasInput( CNAME( Mask ) );
}

#endif