/**
* Copyright 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CVariant;

/// Access to the array in the Variant type
class CVariantArray
{
protected:
	CVariant&		m_variant;		//!< Value

public:
	//! Get referenced variant
	RED_INLINE CVariant& GetVariant() { return m_variant; }

	//! Get referenced variant ( read only )
	RED_INLINE const CVariant& GetVariant() const { return m_variant; }

public:
	//! Construct by wrapping existing value
	CVariantArray( CVariant& variant );

	//! Is this really an array
	bool IsArray() const;

	//! Get size of the enclosed array
	Uint32 Size() const;

	//! Clear the array
	Bool Clear();

	//! Get n-th item
	Bool Get( Uint32 index, CVariant& result ) const;

	//! Set n-th item, no grow
	Bool Set( Uint32 index, const CVariant& val );

	//! Delete n-th item
	Bool Delete( Uint32 index );

	//! Delete element
	Bool Delete( const CVariant& val, Bool all );

	//! Push back item at the end of the array
	Bool PushBack( const CVariant& val );

	//! Find index of element
	Bool Find( const CVariant& val, Int32& index ) const;

	//! Does the element exist in the array ?
	Bool Contains( const CVariant& val ) const;

public:
	//! Get pointer to array data buffer
	void* GetData();

	//! Get pointer to array data buffer
	const void* GetData() const;

	//! Get pointer to data of given array's item
	void* GetItemData( Uint32 index );

	//! Get pointer to data of given array's item
	const void* GetItemData( Uint32 index ) const;

public:
	CVariantArray( const CVariantArray& other );
	CVariantArray& operator=( const CVariantArray& other );
};