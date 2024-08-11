/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "names.h"
#include "rttiType.h"
#include "function.h"
#include "property.h"

class CObject;

// This is to deal with the fact that Scripts currently only support integers in the format of Int32
template < typename T >
struct AutoConvertParams
{
	typedef T Type;
};

template <>
struct AutoConvertParams< Int16 >
{
	typedef Int32 Type;
};

template <>
struct AutoConvertParams< Uint32 >
{
	typedef Int32 Type;
};

template <>
struct AutoConvertParams< Uint16 >
{
	typedef Int32 Type;
};

#ifndef NO_SCRIPT_FUNCTION_CALL_VALIDATION

// #define IGNORE_CHECK_RESULT

	#ifdef IGNORE_CHECK_RESULT
		#define RETURN_ON_CHECK_FAILURE( check ) check
	#else
		#define RETURN_ON_CHECK_FAILURE( check ) if( !( check ) ) return false
	#endif

	/// These add a slight performance impact to each call
	#define CHECK_FUNCTION( context, function, numParameters )	RETURN_ON_CHECK_FAILURE( CheckFunction( context, function, numParameters ) )
	#define CHECK_PARAMETER( function, paramIndex, T )			RETURN_ON_CHECK_FAILURE( CheckFunctionParameter( function, paramIndex, ::GetTypeObject< typename AutoConvertParams< T >::Type >() ) )
	#define CHECK_RETURN( function, T )							RETURN_ON_CHECK_FAILURE( CheckFunctionReturnParameter( function, ::GetTypeObject< typename AutoConvertParams< T >::Type >() ) )

	// Compatability with the old define
	#define CHECK_SCRIPT_FUNCTION_CALLS
#else
	#define CHECK_FUNCTION( context, function, numParameters )
	#define CHECK_PARAMETER( function, paramIndex, T )
	#define CHECK_RETURN( function, T )
#endif

#define ADD_PARAMETER( stack, param, paramIndex, function, T )																		\
	CHECK_PARAMETER( function, paramIndex, T );																						\
	TStackParam< typename AutoConvertParams< T >::Type > stackParam##paramIndex( stack, static_cast< typename AutoConvertParams< T >::Type >( param ), function->GetParameter( paramIndex )->GetDataOffset() )

//////////////////////////////////////////////////////////////////////////////////////////

/// Call function with no parameters and no return value
extern Bool CallFunction( IScriptable* context, CName functionName );

/// Find function, uses cache
extern Bool FindFunction( IScriptable*& context, CName functionName, const CFunction*& function );

/// Check if passed function is valid global function
extern Bool CheckFunction( IScriptable* context, const CFunction* function, Uint32 numParameters );

/// Check return parameter
extern Bool CheckFunctionReturnParameter( const CFunction* function, const IRTTIType* returnType );

/// Check type of n-th function parameter
extern Bool CheckFunctionParameter( const CFunction* function, Uint32 parameterIndex, const IRTTIType* parameterType, Bool silentCheck = false );

//////////////////////////////////////////////////////////////////////////////////////////

/// Copy parameter macro
template< typename T >
class TStackParam
{
private:
	T* m_ptr;

public:
	TStackParam( void* stack, const T& data, const Uint32 offset )
	{
		m_ptr = (T*)( (Uint8*) stack + offset );
		new ( m_ptr ) T( data );
	}

	~TStackParam()
	{
		m_ptr->~T();
	}

	const T& Get() const
	{
		return *m_ptr;
	}
};

/// Call function with one parameter
template< typename T0 >
Bool CallFunction( IScriptable* context, CName functionName, const T0& data0 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		// Copy params
		typename AutoConvertParams< T0 >::Type param0 = data0;

		CHECK_FUNCTION( context, function, 1 );
		CHECK_PARAMETER( function, 0, T0 );

		// Call the function
		function->Call( context, &param0, NULL);
		return true;
	}

	// Not found
	return false;
}

/// Call function with two parameters
template< typename T0, typename T1 >
Bool CallFunction( IScriptable* context, CName functionName, const T0& data0, const T1& data1 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 2 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with three parameters
template< typename T0, typename T1, typename T2 >
Bool CallFunction( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 3 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with four parameters
template< typename T0, typename T1, typename T2, typename T3 >
Bool CallFunction( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 4 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with five parameters
template< typename T0, typename T1, typename T2, typename T3, typename T4 >
Bool CallFunction( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 5 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );

		// Call the function
		return function->Call( context, stack, NULL );		
	}

	// Not found
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////

/// Call function with one parameter by ref
template< typename T0 >
Bool CallFunctionRef( IScriptable* context, CName functionName, T0& data0 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 1 );

		CHECK_PARAMETER( function, 0, T0 );

		typename AutoConvertParams< T0 >::Type param0 = data0;

		// Call the function
		if( !function->Call( context, &param0, NULL ) ) return false;

		data0 = static_cast< T0 >( param0 );

		return true;
	}

	// Not found
	return false;
}

/// Call function with two parameters by ref
template< typename T0, typename T1 >
Bool CallFunctionRef( IScriptable* context, CName functionName, T0& data0, T1& data1 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 2 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		// Copy the resultant value back into the original variable
		data0 = static_cast< T0 > ( stackParam0.Get() );
		data1 = static_cast< T1 > ( stackParam1.Get() );

		return true;
	}

	// Not found
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////

/// Call function with no parameters and return value
template< typename Ret >
Bool CallFunctionRet( IScriptable* context, CName functionName, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 0 );
		CHECK_RETURN( function, Ret );

		// Call the function
		if( !function->Call( context, NULL, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with one parameter and return value
template< typename Ret, typename T0 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 1 );
		CHECK_RETURN( function, Ret );
		
		CHECK_PARAMETER( function, 0, T0 );

		// Copy params
		T0 param0 = data0;

		// Call the function
		if( !function->Call( context, &param0, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with two parameters and return value
template< typename Ret, typename T0, typename T1 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 2 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}


/// Call function with three parameters and return value
template< typename Ret, typename T0, typename T1, typename T2 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 3 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with four parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 4 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with five parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 5 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with six parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 6 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );
		ADD_PARAMETER( stack, data5, 5, function, T5 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with seven parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 7 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );
		ADD_PARAMETER( stack, data5, 5, function, T5 );
		ADD_PARAMETER( stack, data6, 6, function, T6 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with eight parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 8 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );
		ADD_PARAMETER( stack, data5, 5, function, T5 );
		ADD_PARAMETER( stack, data6, 6, function, T6 );
		ADD_PARAMETER( stack, data7, 7, function, T7 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with nine parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 9 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );
		ADD_PARAMETER( stack, data5, 5, function, T5 );
		ADD_PARAMETER( stack, data6, 6, function, T6 );
		ADD_PARAMETER( stack, data7, 7, function, T7 );
		ADD_PARAMETER( stack, data8, 8, function, T8 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

/// Call function with ten parameters and return value
template< typename Ret, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
Bool CallFunctionRet( IScriptable* context, CName functionName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8, const T9& data9, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 10 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );
		ADD_PARAMETER( stack, data3, 3, function, T3 );
		ADD_PARAMETER( stack, data4, 4, function, T4 );
		ADD_PARAMETER( stack, data5, 5, function, T5 );
		ADD_PARAMETER( stack, data6, 6, function, T6 );
		ADD_PARAMETER( stack, data7, 7, function, T7 );
		ADD_PARAMETER( stack, data8, 8, function, T8 );
		ADD_PARAMETER( stack, data9, 9, function, T9 );
		
		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		return true;
	}

	// Not found
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////

/// Call function with one parameter by ref, one by val
template< typename T0, typename T1 >
Bool CallFunctionRef1Val1( IScriptable* context, CName functionName, T0& data0, const T1& data1 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 2 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		// Copy the resultant value back into the original variable
		data0 = static_cast< T0 >( stackParam0.Get() );

		return true;
	}

	// Not found
	return false;
}

/// Call function with one parameter by ref, one by val, and a return
template< typename Ret, typename T0, typename T1 >
Bool CallFunctionRef1Val1Ret( IScriptable* context, CName functionName, T0& data0, const T1& data1, Ret& result )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 2 );
		CHECK_RETURN( function, Ret );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );

		// Call the function
		if( !function->Call( context, stack, &result ) ) return false;

		// Copy the resultant value back into the original variable
		data0 = static_cast< T0 >( stackParam0.Get() );

		return true;
	}

	// Not found
	return false;
}

/// Call function with two parameters by ref, one by val
template< typename T0, typename T1, typename T2 >
Bool CallFunctionRef2Val1( IScriptable* context, CName functionName, T0& data0, T1& data1, const T2& data2 )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 3 );

		void* stack = RED_ALLOCA( function->GetStackSize() );

		ADD_PARAMETER( stack, data0, 0, function, T0 );
		ADD_PARAMETER( stack, data1, 1, function, T1 );
		ADD_PARAMETER( stack, data2, 2, function, T2 );

		// Call the function
		if( !function->Call( context, stack, NULL ) ) return false;

		// Copy the resultant value back into the original variable
		data0 = static_cast< T0 >( stackParam0.Get() );
		data1 = static_cast< T1 >( stackParam1.Get() );

		return true;
	}

	// Not found
	return false;
}
