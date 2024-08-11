/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _TYPE_NAME_H_
#define _TYPE_NAME_H_

#include "names.h"

//////////////////////////////////////////////////////////////////////////
// TTypeName templates for providing human-readable names for RTTI types

// no body, so we get compilation errors when required TypeName is not available
// if you get errors regarding this, try adding your class to proper xxxTypeRegistry.h file
template< class _Type >
struct TTypeName
{
	// By default try to get the type name from the type itself
	static const CName& GetTypeName()
	{
		return _Type::GetTypeName();
	}
};

// partial specializations for pointers...
template< class _Type >
struct TTypeName< _Type* >
{
	static const CName& GetTypeName()
	{
		static const CName name( InitName() );
		return name;
	}

private:
	static CName InitName()
	{
		Char typeName[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName<_Type>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("ptr:%ls"), innerTypeName );

		return CName( typeName );
	}
};

// static array type name generator
template< class _Type, Uint32 _Count >
struct TTypeName< _Type[_Count] >
{
	static const CName& GetTypeName()
	{
		static const CName name( InitName() );
		return name;
	}

private:
	static CName InitName()
	{
		Char typeName[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName<_Type>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("[%d]%ls"), _Count, innerTypeName );

		return CName( typeName );
	}
};

template< class _Type >
RED_INLINE const CName& GetTypeName()
{
	return TTypeName<_Type>::GetTypeName();
}

template< class _Type >
RED_INLINE const CName& GetTypeName( const _Type& )
{
	return TTypeName<_Type>::GetTypeName();
}

#endif
