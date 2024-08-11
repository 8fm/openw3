#pragma once

class IGridTypeDesc
{

public:

	virtual const CName GetName() const = 0;
	virtual wxString ToString( void *data ) const = 0;
	virtual Bool FromString( void *data, const wxString &text ) const = 0;
	virtual wxGridCellRenderer *GetCellRenderer() const { return NULL; } // default renderer
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const { return NULL; } // default editor
    virtual Int32 GetHorizontalAlignment() const { return wxALIGN_LEFT; }
    virtual Int32 GetVerticalAlignment() const { return wxALIGN_CENTER; }

};
