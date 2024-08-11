/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Base slot
class ISlot : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISlot, CObject );

public:
	// Calculate slot matrix
	virtual Matrix CalcSlotMatrix() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( ISlot );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

	