/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptFunctionCompiler.h"
#include "functionBuilder.h"
#include "property.h"
#include "class.h"

static Bool IsBinaryOperator( CName operation )
{
	// Filter unary operators
	if ( operation == CNAME( OperatorNeg ) ) return false;
	if ( operation == CNAME( OperatorLogicNot ) ) return false;
	if ( operation == CNAME( OperatorBitNot ) ) return false;

	// Binary operator
	return true;
}

const CFunction* CScriptSyntaxNode::FindBestOperator( CName operation, const ScriptSyntaxNodeType& LType, const ScriptSyntaxNodeType& RType )
{
	// Best operator so far
	Int32 bestCastCost = 0;
	const CScriptOperator* bestFunction = NULL;

	// Test all defined script operators
	const TDynArray< CScriptOperator* >& operators = CScriptOperator::GetOperators();
	for ( Uint32 i = 0; i < operators.Size(); ++i )
	{
		// Check only matching operators
		const CScriptOperator* operatorFunction = operators[i];
		if ( operatorFunction->GetOperation() != operation )
		{
			continue;
		}

		// Make sure operator is defined in the proper way
		const Bool isBinary = IsBinaryOperator( operation );
		if ( isBinary )
		{
			if ( operatorFunction->GetNumParameters() < 2 )
			{
				continue;
			}
		}
		else 
		{
			if ( operatorFunction->GetNumParameters() < 1 )
			{
				continue;
			}
		}

		// Determine the property for casting
		ScriptSyntaxNodeType LPropType;
		const Bool isLValueAssignable = 0 != ( operatorFunction->GetParameter(0)->GetFlags() & PF_FuncOutParam );
		LPropType.InitSimple( operatorFunction->GetParameter(0)->GetType(), isLValueAssignable );

		// Get casting cost for the left operand to the operator's expected left type
		Int32 leftCastCost = ScriptSyntaxNodeType::GetCastCost( LType, LPropType );
		if ( leftCastCost < 0 )
		{
			// We cannot cast to operator input type, skip the operator
			continue;
		}

		Int32 castCost = leftCastCost;

		// Check the right param
		if ( isBinary )
		{
			// Determine the property for casting
			ScriptSyntaxNodeType RPropType;
			const Bool isRValueAssignable = false;
			RPropType.InitSimple( operatorFunction->GetParameter(1)->GetType(), isRValueAssignable );

			// Get casting cost for the right operand to the operator's expected right type
			Int32 rightCastCost = ScriptSyntaxNodeType::GetCastCost( RType, RPropType ); 
			if ( rightCastCost < 0 )
			{
				// We cannot cast to operator input type, skip the operator
				continue;
			}

			// Accumulate final cost
			castCost += rightCastCost;

			//////////////////////////////////////////////////////////////////////////
			// NOTE: It's possible that we're losing the types of the operands being compared
			// as they're probably 
			//////////////////////////////////////////////////////////////////////////

			//If there is a noticeable cost involved in casting to the types used by this operator
			Bool isCastCostly = leftCastCost == 10 && rightCastCost == 10;

			// Check to see if we can directly compare the two operands
			Bool cannotCastImplicitly =
				( ScriptSyntaxNodeType::GetCastCost( LType, RType ) < 0 ) &&
				( ScriptSyntaxNodeType::GetCastCost( RType, LType ) < 0 );

			if( isCastCostly && cannotCastImplicitly )
			{
				// We cannot compare the two operands directly
				continue;
			}
		}

		// Remember best operator so far
		if ( !bestFunction || castCost < bestCastCost )
		{
			bestFunction = operatorFunction;
			bestCastCost = castCost;
		}
	}

	// Return best operator found
	return bestFunction;
}

Bool CScriptSyntaxNode::HandleDynamicArrayFunction( CScriptFunctionCompiler* compiler )
{
	// Get function name
	String functionName = m_value.m_string;
  
	// Some helper macros  
	#define cparams(x) if ( m_list.Size() != x ) { EmitError( compiler, TXT("Function '%ls' does not take %i params"), functionName.AsChar(), m_list.Size() ); return false; }
	#define cparam(x,type) { if ( !MatchNodeType( m_list[x], compiler, type, false, false ) ) { return false; } }
	#define cret(type) { m_valueType = type; m_valueType.m_isAssignable = false; }
	#define cnode(type) { m_type = CNAME( Syntax##type ); }

	// Get array property
	CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( m_children[0]->m_valueType.m_type );
	if ( !arrayType || arrayType->GetType() != RT_Array )
	{
		EmitError( compiler, TXT("Array functions used not on array property") );
		return false;
	}

	// We can index only direct properties
	if ( !m_children[0]->m_valueType.m_isFromProperty )
	{
		EmitError( compiler, TXT("Only arrays coming from variables can be modified") );
		return false;		
	}
  
	// Initialize commonly used types
	ScriptSyntaxNodeType elementType, boolType, intType;
	elementType.InitSimple( arrayType->GetInnerType() );
	intType.InitSimple( ::GetTypeName< Int32 >() );
	boolType.InitSimple( ::GetTypeName< Bool >() );
  
	// Process functions
	if ( functionName == TXT("Clear") )
	{
		cparams( 0 );
		cnode( ArrayClear );
	}
	else if ( functionName == TXT("Grow") )
	{
		cparams( 1 );
		cparam( 0, intType );
		cret( intType );
		cnode( ArrayGrow );
	}
	else if ( functionName == TXT("PushBack") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( intType );
		cnode( ArrayPushBack );
	}
	else if ( functionName == TXT("Insert") )
	{
		cparams( 2 );
		cparam( 0, intType );
		cparam( 1, elementType );
		cnode( ArrayInsert );
	}
	else if ( functionName == TXT("Erase") )
	{
		cparams( 1 );
		cparam( 0, intType );
		cnode( ArrayErase );
	}
	else if ( functionName == TXT("EraseFast") )
	{
		cparams( 1 );
		cparam( 0, intType );
		cnode( ArrayEraseFast );
	}
	else if ( functionName == TXT("Remove") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret ( boolType );
		cnode( ArrayRemove );
	}
	else if ( functionName == TXT("FindFirst") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( intType );
		cnode( ArrayFindFirst );
	}
	else if ( functionName == TXT("FindLast") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( intType );
		cnode( ArrayFindLast );
	}
	else if ( functionName == TXT("Contains") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( boolType );
		cnode( ArrayContains );
	}
	else if ( functionName == TXT("Size") )
	{
		cparams( 0 );
		cret( intType );
		cnode( ArraySize );
	}
	else if ( functionName == TXT("Resize") )
	{
		cparams( 1 );
		cparam( 0, intType );
		cret( intType );
		cnode( ArrayResize );
	}
	else if ( functionName == TXT("Last") )
	{
		cparams( 0 );
		cret( elementType );
		cnode( ArrayLast );
	} 
	else if ( functionName == TXT("PopBack") )
	{
		cparams( 0 );
		cret( elementType );
		cnode( ArrayPopBack );
	}
	else
	{
		EmitError( compiler, TXT("Unknown array function '%ls'"), functionName.AsChar() );
		return false;
	}
  
	// Undefine helpers
	#undef cparams
	#undef cparam
	#undef cret
	#undef cnode
  
	// All ok
	return true;
}

Bool CScriptSyntaxNode::HandleStaticArrayFunction( CScriptFunctionCompiler* compiler )
{
	// Get function name
	String functionName = m_value.m_string;
  
	// Some helper macros  
	#define cparams(x) if ( m_list.Size() != x ) { EmitError( compiler, TXT("Function '%ls' does not take %i params"), functionName.AsChar(), m_list.Size() ); return false; }
	#define cparam(x,type) { if ( !MatchNodeType( m_list[x], compiler, type, false, false ) ) { return false; } }
	#define cret(type) { m_valueType = type; m_valueType.m_isAssignable = false; }
	#define cnode(type) { m_type = CNAME( Syntax##type ); }

	// Get array property
	CRTTINativeArrayType* arrayType = static_cast< CRTTINativeArrayType* >( m_children[0]->m_valueType.m_type );
	if ( !arrayType || arrayType->GetType() != RT_NativeArray )
	{
		EmitError( compiler, TXT("Array functions used not on array property") );
		return false;
	}

	// We can index only direct properties
	if ( !m_children[0]->m_valueType.m_isFromProperty )
	{
		EmitError( compiler, TXT("Only arrays coming from variables can be modified") );
		return false;		
	}
  
	// Initialize commonly used types
	ScriptSyntaxNodeType elementType, boolType, intType;
	elementType.InitSimple( arrayType->GetInnerType() );
	intType.InitSimple( ::GetTypeName< Int32 >() );
	boolType.InitSimple( ::GetTypeName< Bool >() );
  
	// Process functions
	if ( functionName == TXT("FindFirst") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( intType );
		cnode( ArrayFindFirst );
	}
	else if ( functionName == TXT("FindLast") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( intType );
		cnode( ArrayFindLast );
	}
	else if ( functionName == TXT("Contains") )
	{
		cparams( 1 );
		cparam( 0, elementType );
		cret( boolType );
		cnode( ArrayContains );
	}
	else if ( functionName == TXT("Size") )
	{
		cparams( 0 );
		cret( intType );
		cnode( ArraySize );
	}
	else if ( functionName == TXT("Last") )
	{
		cparams( 0 );
		cret( elementType );
		cnode( ArrayLast );
	} 
	else
	{
		EmitError( compiler, TXT("Unknown static array function '%ls'"), functionName.AsChar() );
		return false;
	}
  
	// Undefine helpers
	#undef cparams
	#undef cparam
	#undef cret
	#undef cnode
  
	// All ok
	return true;
}

Bool CScriptSyntaxNode::HandleStructureConstructor( CClass* structure, CScriptFunctionCompiler* compiler )
{
	ASSERT( m_children[0] );

	// Get structure properties
	TDynArray< CProperty* > paramList;
	structure->GetProperties( paramList );

	// No context can be specified for structure construction
	if ( m_children[0]->m_children[0] )
	{
		EmitError( compiler, TXT("Struct construction has no meaning in given context") );
		return false;
	}

	// Delete ident children
	m_children[0]->Release( true );
	m_children[0] = NULL;

	// To many params ?
	if ( m_list.Size() > paramList.Size() )
	{
		EmitError( compiler, TXT( "To many initializers for '%ls'" ), structure->GetName().AsString().AsChar() );
		return false;
	}

	// Check params and create nodes
	Bool typeMatchState = true;
	for ( Uint32 i = 0; i < m_list.Size(); ++i )
	{
		// Allow empty param
		if ( !m_list[i] )
		{
			continue;
		}

		// Create property type
		ScriptSyntaxNodeType propType;
		propType.InitSimple( paramList[i]->GetType() );

		// Check node type
		if ( !MatchNodeType( m_list[i], compiler, propType, false, false ) )
		{
			EmitError( compiler, TXT( "Could not cast member variable '%ls'" ), paramList[i]->GetName().AsString().AsChar() );
			typeMatchState = false;
		}
	}

	// Exit if input types were not matched
	if ( !typeMatchState )
	{
		return false;
	}

	// Setup node type
	m_type = CNAME( SyntaxConstructor );
	m_value.m_type = structure;
	m_valueType.InitSimple( structure, false );
	return true;
}
