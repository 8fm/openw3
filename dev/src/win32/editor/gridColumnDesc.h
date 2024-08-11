/**
 * Copyright c 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class IRTTIType;

class IGridColumnDesc
{
public:
	virtual ~IGridColumnDesc() {}
	virtual wxGridCellEditor*	GetCellEditor() const = 0;
	virtual wxGridCellRenderer*	GetCellRenderer() const = 0;

	virtual Bool AllowAutoSize() const { return true; }

	// Extra info
	virtual Bool DoesRequireExtraInfo() { return false; }
	virtual Int32 GetInterestColumnNumber() { return -1; }
	virtual void SetExtraInfo( const IRTTIType *type, void *data ) {}
};
