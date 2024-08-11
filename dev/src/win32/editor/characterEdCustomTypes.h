#pragma once
#include "../../common/game/character.h"
#include "gridColumnDesc.h"
#include "gridTypeDesc.h"
#include "gridCellEditors.h"
#include "gridCustomCellEditors.h"
#include "gridCellAttrCustomizer.h"


// Changes cell colour when property is inherited
class CCharacterCellAttrCustomizer : public IGridCellAttrCustomizer
{
	static const wxColour INHERITED_PROPERTY_COLOUR;

public:
	CCharacterCellAttrCustomizer() {}
	~CCharacterCellAttrCustomizer() {}

	virtual void CustomizeCellAttr( wxGridCellAttr* attr, const IRTTIType* gridType, const void *gridData, const IRTTIType* cellRttiType, const void *cellData, const CProperty* property ) const;
};


// Shows character name for parent property
class CGridCharacterNameCellDesc : public IGridTypeDesc
{
public:
	CGridCharacterNameCellDesc( CEdCharacterResourceContainer& resourceContainer ) : m_resourceContainer( &resourceContainer ) {}
	virtual const CName GetName() const { return CName( TXT("ptr:CCharacter") ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text) const;

private:
	CEdCharacterResourceContainer* m_resourceContainer;
};
