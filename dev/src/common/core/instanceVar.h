/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "types.h"
#include "names.h"
#include "typeName.h"

class IRTTIType;

/// Container for instance variable
class InstanceVar
{
	friend class InstanceDataLayout;
	friend class InstanceDataLayoutCompiler;

protected:
	Uint32			m_offset;		//!< Offset in the instance buffer
#ifdef RED_ASSERTS_ENABLED
	Uint32			m_index;		//!< Index in the instance buffer
#endif
	IRTTIType*		m_type;			//!< Type of the data ( used to serialize, destroy, etc )

protected:
	RED_INLINE void SetOffset( Uint32 offset ) { m_offset = offset; }
#ifdef RED_ASSERTS_ENABLED
	RED_INLINE void SetIndex( Uint32 index ) { m_index = index; }
#endif

public:
	//! Get offset in the instance buffer
	RED_INLINE Uint32 GetOffset() const { return m_offset; }

	//! Get type of data
	RED_INLINE IRTTIType* GetType() const { return m_type; }

	//! Is this a valid variable ?
	RED_INLINE Bool IsValid() const { return m_type != NULL; }

	//! Is this variable instanced in current data layout ?
	RED_INLINE Bool IsInstanced() const { return m_offset != 0xFFFFFFFF; }

public:
	//! Constructor, assign type to instance var
	InstanceVar( CName typeName ); 
};

/// Template version of instance data
 template< class T >
 class TInstanceVar : public InstanceVar
 {
 public:
 	//! Constructor
 	RED_INLINE TInstanceVar()
 		: InstanceVar( GetTypeName<T>() )
 	{};
 };

 // FIXME: Temporary workaround for x64.
 // Won't be a fix for deserializing with
 // same data on x64 and x86.
 typedef char* TGenericPtr;

/*template< class _Type >
static CName GetTypeMemName()
{
	const CName& name = GetTypeName< _Type >();

	if ( name.AsChar()[ 0 ] == '@' )
	{
		String str = name.AsString();
		str.TypedData()[ 0 ] = '$';
		return CName( str );
	}

	return name;
}

template< class T >
class TInstanceVar : public InstanceVar
{
public:
	//! Constructor
	RED_INLINE TInstanceVar()
		: InstanceVar( GetTypeMemName<T>() )
	{};
};

template< class T >
class TInstanceVar< TDynArray< T, MC_Temporary > > : public InstanceVar
{
public:
	//! Constructor
	RED_INLINE TInstanceVar()
		: InstanceVar( GetTypeMemName<T>() )
	{};
};*/
