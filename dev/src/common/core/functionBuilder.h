/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "function.h"

/// Operator types
RED_DECLARE_NAME( OperatorAdd );				// +
RED_DECLARE_NAME( OperatorSubtract );			// -
RED_DECLARE_NAME( OperatorMultiply );			// *
RED_DECLARE_NAME( OperatorDivide );				// /
RED_DECLARE_NAME( OperatorNeg );				// -
RED_DECLARE_NAME( OperatorAnd );				// &
RED_DECLARE_NAME( OperatorOr );					// |
RED_DECLARE_NAME( OperatorXor );				// ^
RED_DECLARE_NAME( OperatorBitNot );				// ~
RED_DECLARE_NAME( OperatorLogicAnd );			// &&
RED_DECLARE_NAME( OperatorLogicOr );			// ||
RED_DECLARE_NAME( OperatorLogicNot );			// !
RED_DECLARE_NAME( OperatorModulo );				// %
RED_DECLARE_NAME( OperatorEqual );				// ==
RED_DECLARE_NAME( OperatorNotEqual );			// !=
RED_DECLARE_NAME( OperatorGreater );			// >
RED_DECLARE_NAME( OperatorGreaterEqual );		// >=
RED_DECLARE_NAME( OperatorLess );				// <
RED_DECLARE_NAME( OperatorLessEqual );			// <=
RED_DECLARE_NAME( OperatorAssignAdd );			// +=
RED_DECLARE_NAME( OperatorAssignSubtract );		// -=
RED_DECLARE_NAME( OperatorAssignMultiply );		// *=
RED_DECLARE_NAME( OperatorAssignDivide );		// /=
RED_DECLARE_NAME( OperatorAssignAnd );			// &=
RED_DECLARE_NAME( OperatorAssignOr );			// |=

/// Operator definition
class CScriptOperator : public CFunction
{
protected:
	CName	m_operation;		//!< Operation name supported by operator
	Bool	m_isBinary;			//!< Is this a binary operator

public:
	//! Get the operation supported by operator
	RED_INLINE CName GetOperation() const { return m_operation; }

	//! Is this a binary operator
	RED_INLINE Bool IsBinary() const { return m_isBinary; }

public:
	CScriptOperator( CName operation, Bool isBinary, TNativeGlobalFunc func );
	~CScriptOperator();

public:
	//! Get operator list
	static TDynArray< CScriptOperator* >& GetOperators();
};

/// Binary operator
template< typename RetType, typename TypeA, typename TypeB >
class CScriptBinaryOperator : public CScriptOperator
{
public:
	RED_INLINE CScriptBinaryOperator( CName operation, TNativeGlobalFunc func, Uint32 flagsA=0, Uint32 flagsB=0 )
		: CScriptOperator( operation, true, func )
	{
		DefineNativeReturnType( ::GetTypeName< RetType >() );
		DefineNativeParameter( ::GetTypeName< TypeA >(), flagsA );
		DefineNativeParameter( ::GetTypeName< TypeB >(), flagsB );
	}
};

/// Unary operator
template< typename RetType, typename Type >
class CScriptUnaryOperator : public CScriptOperator
{
public:
	RED_INLINE CScriptUnaryOperator( CName operation, TNativeGlobalFunc func, Uint32 flags=0 )
		: CScriptOperator( operation, false, func )
	{
		DefineNativeReturnType( ::GetTypeName< RetType >() );
		DefineNativeParameter( ::GetTypeName< Type >(), flags );
	}
};

#define NATIVE_FUNCTION( _name, _function )																										\
{																																				\
	CFunction* func = new CFunction( m_registeredClass, CName( TXT(_name) ), (TNativeFunc)&tCurrClassType::_function );		\
	m_registeredClass->AddFunction( func );																										\
}

#define NATIVE_GLOBAL_FUNCTION( _name, _function )																	\
{																													\
	CFunction* func = new CFunction( CName( TXT(_name) ), (TNativeGlobalFunc)&_function );		\
	SRTTI::GetInstance().RegisterGlobalFunction( func );															\
}

#define NATIVE_BINARY_OPERATOR( _op, _function, _ret, _typeA, _typeB )														\
{																															\
	new CScriptBinaryOperator< _ret, _typeA, _typeB >( CNAME( Operator##_op ), (TNativeGlobalFunc)&_function, 0, 0 );		\
}

#define NATIVE_BINARY_ASSIGNMNET_OPERATOR( _op, _function, _ret, _typeA, _typeB )													\
{																																	\
	new CScriptBinaryOperator< _ret, _typeA, _typeB >( CNAME( Operator##_op ), (TNativeGlobalFunc)&_function, PF_FuncOutParam, 0 );	\
}

#define NATIVE_BINARY_BOOL_OPERATOR( _op, _function, _ret, _typeA, _typeB )																\
{																																		\
	new CScriptBinaryOperator< _ret, _typeA, _typeB >( CNAME( Operator##_op ), (TNativeGlobalFunc)&_function, 0, PF_FuncSkipParam );	\
}

#define NATIVE_UNARY_OPERATOR( _op, _function, _ret, _type )												\
{																											\
	new CScriptUnaryOperator< _ret, _type >( CNAME( Operator##_op ), (TNativeGlobalFunc)&_function, 0 );	\
}
