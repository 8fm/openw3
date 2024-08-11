/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CGrassCellMask
{
	DECLARE_RTTI_SIMPLE_CLASS( CGrassCellMask );

	String			m_srtFileName;	//!< What object instance cells are we masking out
	Int32			m_firstRow;		//!< Cell grid coords
	Int32			m_lastRow;		//!< Cell grid coords
	Int32			m_firstCol;		//!< Cell grid coords
	Int32			m_lastCol;		//!< Cell grid coords
	Float			m_cellSize;		//!< size of the cell in meters
	LongBitField	m_bitmap;		//!< The mask

public:
	CGrassCellMask() {}

	Uint32 GetNumRows() const { return m_lastRow - m_firstRow + 1; }
	Uint32 GetNumCols() const { return m_lastCol - m_firstCol + 1; }
	void Allocate()
	{
		m_bitmap.Resize( GetNumRows() * GetNumCols() );
	}
	void SetBit( Uint32 row, Uint32 col )
	{
		const Int32 occMapRow = row - m_firstRow;
		const Int32 occMapCol = col - m_firstCol;

		const Int32 bitIndex = ( occMapRow * GetNumCols() + occMapCol );
		m_bitmap.SetBit( bitIndex, true );
	}
	Bool IsBitSet( Uint32 row, Uint32 col ) const
	{
		static Bool overrider = false;
		if ( overrider ) return true;
		const Int32 occMapRow = row - m_firstRow;
		const Int32 occMapCol = col - m_firstCol;

		const Int32 bitIndex = ( occMapRow * GetNumCols() + occMapCol );
		return m_bitmap.IsBitSet( bitIndex );
	}
};

BEGIN_CLASS_RTTI( CGrassCellMask );
PROPERTY( m_srtFileName );
PROPERTY( m_firstRow );
PROPERTY( m_lastRow );
PROPERTY( m_firstCol );
PROPERTY( m_lastCol );
PROPERTY( m_cellSize );
PROPERTY( m_bitmap );
END_CLASS_RTTI();