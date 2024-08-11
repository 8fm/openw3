/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once
//*******************************************************************************
//* LongBitField:																*
//* Allows for bits to be set and stored across multiple values, the current	*
//* version works with Uint32, and has a maximum bit count of					*
//* UINT_MAX(4294967295 (0xffffffff)).											*
//*																				*
//* NOTE: THIS SHOULD USE UINT64 AT A LATER DATE, ESPCIALLY ON x64 MACHINES.	*
//* WE SHOULD ALSO CONSIDER REMOVING THE DEPENDENCY ON TDynArray				*
//*******************************************************************************

#include "classBuilder.h"
#include "dynarray.h"

class LongBitField
{
public:
	DECLARE_RTTI_STRUCT( LongBitField );
	typedef Uint32 TBaseType; 
	TDynArray< TBaseType > m_mem;
	//*******************************************************************************
	//* Constructors																*
	//*******************************************************************************
	LongBitField(); 

	explicit LongBitField( Uint32 numBits );

	LongBitField( const LongBitField& other );
	LongBitField( LongBitField&& other );

	~LongBitField(); 
	
	//*******************************************************************************
	//* Operators																	*
	//*******************************************************************************
	RED_FORCE_INLINE LongBitField& operator=( const LongBitField& other );
	RED_FORCE_INLINE LongBitField& operator=( LongBitField&& other ); 
	RED_FORCE_INLINE Bool operator!=( const LongBitField& other );
	RED_FORCE_INLINE Bool operator==( const LongBitField& other );
	RED_FORCE_INLINE LongBitField& operator|=( const LongBitField& other );
	RED_FORCE_INLINE LongBitField& operator&=( const LongBitField& other );
	RED_FORCE_INLINE LongBitField& operator^=( const LongBitField& other );

	RED_FORCE_INLINE LongBitField operator~();
	RED_FORCE_INLINE LongBitField operator&( const LongBitField& other );
	RED_FORCE_INLINE LongBitField operator|( const LongBitField& other );
	RED_FORCE_INLINE LongBitField operator^( const LongBitField& other );

	//*******************************************************************************
	//* Functions																	*
	//*******************************************************************************
	RED_FORCE_INLINE void Clear();
	RED_FORCE_INLINE void SetAllBits();
	RED_FORCE_INLINE void Resize( Uint32 numBits );
	RED_FORCE_INLINE void ResizeFast( Uint32 numBits );
	RED_FORCE_INLINE Uint32 Size() const;
	RED_FORCE_INLINE Bool IsBitSet( Uint32 index ) const;
	RED_FORCE_INLINE void SetBit( Uint32 index, Bool set );
	RED_FORCE_INLINE void ClearBits( const LongBitField& other );
	void Swap( LongBitField & value );

};

BEGIN_CLASS_RTTI( LongBitField );
	PROPERTY( m_mem );
END_CLASS_RTTI();

// Include inline implementation
#include "longBitField.inl"