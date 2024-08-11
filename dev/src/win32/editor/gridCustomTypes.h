#pragma once

#include "gridTypeDesc.h"
#include "../../common/game/communityData.h"

class CGridTagListDesc : public IGridTypeDesc
{
public:

    virtual const CName GetName() const { return CNAME( TagList ); }
    virtual wxString ToString( void *data ) const;
    virtual Bool FromString( void *data, const wxString &text ) const;
    virtual wxGridCellRenderer *GetCellRenderer() const;
    virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const;
    virtual Int32 GetHorizontalAlignment() const { return wxALIGN_CENTER; }
    virtual Int32 GetVerticalAlignment() const { return wxALIGN_CENTER; }
};

class CGridGameTimeDesc : public IGridTypeDesc
{
public:
	
	virtual const CName GetName() const { return CNAME( GameTime ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text ) const;

};

//////////////////////////////////////////////////////////////////////////

class CGridSpawnTypeDesc : public IGridTypeDesc
{
public:

	virtual const CName GetName() const { return CNAME( CSSpawnType ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text ) const;
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const;

};

//////////////////////////////////////////////////////////////////////////

class CGridStoryPhaseTypeDesc : public IGridTypeDesc
{
private:
	typedef THashMap< Int32, CGatheredResource* > CSVResources;
	CSVResources m_csvs;

public:
	void AddSource( Int32 col, CGatheredResource& res );

	virtual const CName GetName() const { return CNAME( CSStoryPhaseNames ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text ) const;
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const;

};

//////////////////////////////////////////////////////////////////////////

class CArrayEditorTypeDescBase : public IGridTypeDesc
{
private:
	const C2dArray*	m_res;
	Int32			m_col;
	Bool			m_isSorted;

public:
	CArrayEditorTypeDescBase( Int32 col, const C2dArray* res, Bool isSorted = false );

	virtual const CName GetName() const { return CNAME( CName ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text ) const;
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const;
};

//////////////////////////////////////////////////////////////////////////

class CArrayEditorTypeDesc : public CArrayEditorTypeDescBase
{
public:
	CArrayEditorTypeDesc( Int32 col, CGatheredResource& res, Bool isSorted = false );
};

//////////////////////////////////////////////////////////////////////////

class CGridLayerNameDesc : public IGridTypeDesc
{
public:

	virtual const CName GetName() const { return CNAME( CSLayerName ); }
	virtual wxString ToString( void *data ) const;
	virtual Bool FromString( void *data, const wxString &text ) const;
    virtual wxGridCellRenderer *GetCellRenderer() const;
	virtual wxGridCellEditor *GetCellEditor( Int32 row, Int32 col ) const;

};

//////////////////////////////////////////////////////////////////////////

class CGridVectorTypeDesc : public IGridTypeDesc
{
public:
    virtual const CName GetName() const { return CNAME( Vector ); }
    virtual wxString ToString( void *data ) const;
    virtual Bool FromString( void *data, const wxString &text) const;
};