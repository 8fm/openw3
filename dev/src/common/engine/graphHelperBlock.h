/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "graphBlock.h"

/// Base class for general helper blocks
class CGraphHelperBlock : public CGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CGraphHelperBlock, CGraphBlock );
};

BEGIN_ABSTRACT_CLASS_RTTI( CGraphHelperBlock );
	PARENT_CLASS( CGraphBlock );
END_CLASS_RTTI();
