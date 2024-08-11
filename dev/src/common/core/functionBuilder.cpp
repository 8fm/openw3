/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "functionBuilder.h"
#include "namesPool.h"
#include "namesRegistry.h"

RED_DEFINE_NAME( OperatorAdd );					// +
RED_DEFINE_NAME( OperatorSubtract );			// -
RED_DEFINE_NAME( OperatorMultiply );			// *
RED_DEFINE_NAME( OperatorDivide );				// /
RED_DEFINE_NAME( OperatorNeg );					// -
RED_DEFINE_NAME( OperatorAnd );					// &
RED_DEFINE_NAME( OperatorOr );					// |
RED_DEFINE_NAME( OperatorXor );					// ^
RED_DEFINE_NAME( OperatorBitNot );				// ~
RED_DEFINE_NAME( OperatorLogicAnd );			// &&
RED_DEFINE_NAME( OperatorLogicOr );				// ||
RED_DEFINE_NAME( OperatorLogicNot );			// !
RED_DEFINE_NAME( OperatorModulo );				// %
RED_DEFINE_NAME( OperatorEqual );				// ==
RED_DEFINE_NAME( OperatorNotEqual );			// !=
RED_DEFINE_NAME( OperatorGreater );				// >
RED_DEFINE_NAME( OperatorGreaterEqual );		// >=
RED_DEFINE_NAME( OperatorLess );				// <
RED_DEFINE_NAME( OperatorLessEqual );			// <=
RED_DEFINE_NAME( OperatorAssignAdd );			// +=
RED_DEFINE_NAME( OperatorAssignSubtract );		// -=
RED_DEFINE_NAME( OperatorAssignMultiply );		// *=
RED_DEFINE_NAME( OperatorAssignDivide );		// /=
RED_DEFINE_NAME( OperatorAssignAnd );			// &=
RED_DEFINE_NAME( OperatorAssignOr );			// |=

CScriptOperator::CScriptOperator( CName operation, Bool isBinary, TNativeGlobalFunc func )
	: CFunction( CNAME( __operator ), func )
	, m_operation( operation )
	, m_isBinary( isBinary )
{
	// Register in operator's list
	ASSERT( !GetOperators().Exist( this ) );
	GetOperators().PushBack( this );
}

CScriptOperator::~CScriptOperator()
{
	ASSERT( GetOperators().Exist( this ) );
	GetOperators().Remove( this );
}

TDynArray< CScriptOperator* >& CScriptOperator::GetOperators()
{
	static TDynArray< CScriptOperator* > operators;
	return operators;
}