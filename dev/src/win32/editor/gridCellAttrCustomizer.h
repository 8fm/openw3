#pragma once

class IRTTIType;

//  Interface for cell attr customizer - this class can customize any cell in grid editor based on grid editor data.
class IGridCellAttrCustomizer
{

public:
	virtual void CustomizeCellAttr( wxGridCellAttr* attr, const IRTTIType* gridRttiType, const void *gridData, const IRTTIType* cellRttiType, const void *cellData, const CProperty* property ) const = 0;
};
