#pragma once
#include "../../games/r6/traitData.h"
#include "gridColumnDesc.h"
#include "gridTypeDesc.h"
#include "gridCellEditors.h"


class CGridTraitNameColumnDesc : public IGridColumnDesc
{
public:

	CGridTraitNameColumnDesc( TDynArray<STraitTableEntry>& traits ) : m_traits( traits ) {}

	virtual wxGridCellRenderer*	GetCellRenderer() const;
	virtual wxGridCellEditor*	GetCellEditor() const;

private:
	TDynArray< STraitTableEntry >& m_traits;
};

class CGridAbilityCellDesc : public IGridTypeDesc
{
public:

	virtual const CName GetName() const { return CName( TXT("*CTraitAbilityWrapper") ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text) const;
	virtual wxGridCellRenderer *GetCellRenderer() const { return new CGridCellObjectRenderer(); } 
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const { return new CGridCellObjectEditor(); } 

};


class CGridRequirementCellDesc : public IGridTypeDesc
{
public:

	virtual const CName GetName() const { return CName( TXT("*CTraitRequirementWrapper") ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text) const;
	virtual wxGridCellRenderer *GetCellRenderer() const { return new CGridCellObjectRenderer(); } 
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const { return new CGridCellObjectEditor(); } 

};