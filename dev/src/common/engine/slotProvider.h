/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Slot provider
class ISlotProvider
{
public:
	virtual ~ISlotProvider() {};

	// Create slot
	virtual ISlot*	CreateSlot( const String& slotName ) = 0;

	// Enumerate all available slots
	virtual void	EnumSlotNames( TDynArray< String >& slotNames ) const = 0;
};
