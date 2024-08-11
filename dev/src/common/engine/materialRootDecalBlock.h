/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

enum ERenderingSortGroup : CEnum::TValueType;

/// Decal root block - a block that defines material property
class CMaterialRootDecalBlock : public CMaterialRootBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialRootDecalBlock, CMaterialRootBlock );

public:
	// Get rendering sort group.
	virtual ERenderingSortGroup GetSortGroup() const=0;

	// Is this material accumulatively refracted
	virtual bool IsAccumulativelyRefracted() const { return false; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialRootDecalBlock );
	PARENT_CLASS( CMaterialRootBlock );
END_CLASS_RTTI();

#endif