/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_TYPE_TRAITS_H_
#define _RED_TYPE_TRAITS_H_

#include "types.h"

// type selector
template < Red::System::Bool cond, typename T1, typename T2 >
struct TSelector
{
	typedef T1 Type;
};

template < typename T1, typename T2 >
struct TSelector< false, T1, T2 >
{
	typedef T2 Type;
};

// checks if type is a pointer
template < typename T > struct TPtrType					{ enum { Value = false }; };
template < typename T > struct TPtrType< T* >			{ enum { Value = true }; };

// checks if type is a reference
template < typename T > struct TRefType					{ enum { Value = false }; };
template < typename T > struct TRefType< T& >			{ enum { Value = true }; };

// checks if types are the same
template < typename T, typename U > struct TSameType	{ enum { Value = false }; };
template < typename T > struct TSameType<T, T>			{ enum { Value = true }; };

template < typename T > struct TPlainType				{ enum { Value = false }; };

#define RED_DEFINE_TYPE_PLAIN(type)	\
template <>							\
struct TPlainType<type>				\
{									\
	enum							\
	{								\
		Value = true				\
	};								\
}

RED_DEFINE_TYPE_PLAIN( Red::System::Bool );
					   
RED_DEFINE_TYPE_PLAIN( Red::System::Int8 );
RED_DEFINE_TYPE_PLAIN( Red::System::Int16 );
RED_DEFINE_TYPE_PLAIN( Red::System::Int32 );
RED_DEFINE_TYPE_PLAIN( Red::System::Int64 );

RED_DEFINE_TYPE_PLAIN( Red::System::Uint8 );
RED_DEFINE_TYPE_PLAIN( Red::System::Uint16 );
RED_DEFINE_TYPE_PLAIN( Red::System::Uint32 );
RED_DEFINE_TYPE_PLAIN( Red::System::Uint64 );

RED_DEFINE_TYPE_PLAIN( Red::System::Float );
RED_DEFINE_TYPE_PLAIN( Red::System::Double );


template < typename T >
struct TCopyableType
{
	enum
	{
		Value = TPtrType<T>::Value || TPlainType<T>::Value || std::is_scalar< T >::value
	};
};


// helper template for overloading
template < Red::System::Bool b >
struct TBool2Type
{
	enum
	{
		Value = b
	};
};

typedef TBool2Type< true > TrueType;
typedef TBool2Type< false > FalseType;

//////////////////////////////////////////////////////////////////////////
// Allows for correct string literal types in template functions that can swap between Ansi and Uni

// The only valid types for these template functions are UniChar and AnsiChar
// Anything else will break
template< typename TUndefined >	RED_INLINE const TUndefined* SelectCharType( const Red::System::AnsiChar*, const Red::System::UniChar* )										{ Red::System::AnsiChar breakingLocal[ 0 ]; RED_UNUSED( breakingLocal[ 0 ] ); return nullptr; }
template <>						RED_INLINE const Red::System::UniChar* SelectCharType< Red::System::UniChar >( const Red::System::AnsiChar*, const Red::System::UniChar* u )	{ return u; }
template <>						RED_INLINE const Red::System::AnsiChar* SelectCharType< Red::System::AnsiChar >( const Red::System::AnsiChar* a, const Red::System::UniChar* )	{ return a; }

template< typename TUndefined >	RED_INLINE const TUndefined SelectCharType( const Red::System::AnsiChar, const Red::System::UniChar )											{ Red::System::AnsiChar breakingLocal[ 0 ]; RED_UNUSED( breakingLocal[ 0 ] ); return 0; }
template <>						RED_INLINE const Red::System::UniChar SelectCharType< Red::System::UniChar >( const Red::System::AnsiChar, const Red::System::UniChar u )		{ return u; }
template <>						RED_INLINE const Red::System::AnsiChar SelectCharType< Red::System::AnsiChar >( const Red::System::AnsiChar a, const Red::System::UniChar )		{ return a; }

// This macro is designed, like the TXT() macro to correctly output a string literal in AnsiChar* or UniChar* format
// but for templates where the type is determined at the compilation stage rather than the pre-processor stage
// Example usage:
// 
// template< typename TChar >
// TChar* TestFunc() { return RED_TEMPLATE_TXT( TChar, "Some Text" ); }
// One level of indirection exists in this macro to allow for the txt parameter to itself also be a macro
#define _RED_TEMPLATE_TXT( type, txt ) SelectCharType< type >( txt, L##txt )
#define RED_TEMPLATE_TXT( type, txt ) _RED_TEMPLATE_TXT( type, txt )

#endif // _RED_TYPE_TRAITS_H_
