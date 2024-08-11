/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptFunctionCompiler.h"
#include "math.h"
#include "function.h"

/// Syntax node types
RED_DEFINE_NAME( SyntaxNop );
RED_DEFINE_NAME( SyntaxCode );
RED_DEFINE_NAME( SyntaxListItem );
RED_DEFINE_NAME( SyntaxIntConst );
RED_DEFINE_NAME( SyntaxBreakpoint );
RED_DEFINE_NAME( SyntaxFloatConst );
RED_DEFINE_NAME( SyntaxBoolConst );
RED_DEFINE_NAME( SyntaxStringConst );
RED_DEFINE_NAME( SyntaxNameConst );
RED_DEFINE_NAME( SyntaxNullConst );
RED_DEFINE_NAME( SyntaxEnumConst );
RED_DEFINE_NAME( SyntaxOperatorCall );
RED_DEFINE_NAME( SyntaxAssign );
RED_DEFINE_NAME( SyntaxConditional );
RED_DEFINE_NAME( SyntaxArrayElement );
RED_DEFINE_NAME( SyntaxNew );
RED_DEFINE_NAME( SyntaxDelete );
RED_DEFINE_NAME( SyntaxFuncCall );
RED_DEFINE_NAME( SyntaxScopeDefault );
RED_DEFINE_NAME( SyntaxScopeSuper );
RED_DEFINE_NAME( SyntaxScopeParent );
RED_DEFINE_NAME( SyntaxScopeVirtualParent );
RED_DEFINE_NAME( SyntaxIfThen );
RED_DEFINE_NAME( SyntaxIfThenElse );
RED_DEFINE_NAME( SyntaxSwitch );
RED_DEFINE_NAME( SyntaxSwitchCase );
RED_DEFINE_NAME( SyntaxDefaultCase );
RED_DEFINE_NAME( SyntaxConstructor );
RED_DEFINE_NAME( SyntaxFor );
RED_DEFINE_NAME( SyntaxWhile );
RED_DEFINE_NAME( SyntaxDoWhile );
RED_DEFINE_NAME( SyntaxReturn );
RED_DEFINE_NAME( SyntaxEnter );
RED_DEFINE_NAME( SyntaxContinue );
RED_DEFINE_NAME( SyntaxBreak );
RED_DEFINE_NAME( SyntaxThisValue );
RED_DEFINE_NAME( SyntaxIdent );
RED_DEFINE_NAME( SyntaxCast );
RED_DEFINE_NAME( SyntaxNoCast );
RED_DEFINE_NAME( SyntaxTypedEqual );
RED_DEFINE_NAME( SyntaxTypedNotEqual );
RED_DEFINE_NAME( SyntaxBoolToByte )
RED_DEFINE_NAME( SyntaxBoolToInt )
RED_DEFINE_NAME( SyntaxBoolToFloat )
RED_DEFINE_NAME( SyntaxBoolToString )
RED_DEFINE_NAME( SyntaxByteToBool )
RED_DEFINE_NAME( SyntaxByteToInt )
RED_DEFINE_NAME( SyntaxByteToFloat )
RED_DEFINE_NAME( SyntaxByteToString )
RED_DEFINE_NAME( SyntaxIntToBool )
RED_DEFINE_NAME( SyntaxIntToByte )
RED_DEFINE_NAME( SyntaxIntToFloat )
RED_DEFINE_NAME( SyntaxIntToString )
RED_DEFINE_NAME( SyntaxIntToEnum )
RED_DEFINE_NAME( SyntaxFloatToBool )
RED_DEFINE_NAME( SyntaxFloatToByte )
RED_DEFINE_NAME( SyntaxFloatToInt )
RED_DEFINE_NAME( SyntaxFloatToString )
RED_DEFINE_NAME( SyntaxNameToBool )
RED_DEFINE_NAME( SyntaxNameToString )
RED_DEFINE_NAME( SyntaxStringToBool )
RED_DEFINE_NAME( SyntaxStringToByte )
RED_DEFINE_NAME( SyntaxStringToInt )
RED_DEFINE_NAME( SyntaxStringToFloat )
RED_DEFINE_NAME( SyntaxObjectToBool )
RED_DEFINE_NAME( SyntaxObjectToString )
RED_DEFINE_NAME( SyntaxEnumToString )
RED_DEFINE_NAME( SyntaxEnumToInt )
RED_DEFINE_NAME( SyntaxDynamicCast )
RED_DEFINE_NAME( SyntaxArrayClear );
RED_DEFINE_NAME( SyntaxArrayGrow );
RED_DEFINE_NAME( SyntaxArrayPushBack );
RED_DEFINE_NAME( SyntaxArrayPopBack );
RED_DEFINE_NAME( SyntaxArrayInsert );
RED_DEFINE_NAME( SyntaxArrayErase );
RED_DEFINE_NAME( SyntaxArrayEraseFast );
RED_DEFINE_NAME( SyntaxArrayRemove );
RED_DEFINE_NAME( SyntaxArrayFindFirst );
RED_DEFINE_NAME( SyntaxArrayFindLast );
RED_DEFINE_NAME( SyntaxArrayContains );
RED_DEFINE_NAME( SyntaxArraySize );
RED_DEFINE_NAME( SyntaxArrayResize );
RED_DEFINE_NAME( SyntaxArrayLast );
RED_DEFINE_NAME( SyntaxSavePoint );
RED_DEFINE_NAME( SyntaxGlobalGame );
RED_DEFINE_NAME( SyntaxGlobalPlayer );
RED_DEFINE_NAME( SyntaxGlobalCamera );
RED_DEFINE_NAME( SyntaxGlobalHud );
RED_DEFINE_NAME( SyntaxGlobalSound );
RED_DEFINE_NAME( SyntaxGlobalDebug );
RED_DEFINE_NAME( SyntaxGlobalTimer );
RED_DEFINE_NAME( SyntaxGlobalInput );
RED_DEFINE_NAME( SyntaxGlobalTelemetry );

CScriptSyntaxNode::CScriptSyntaxNode( EScriptTreeNode, const CScriptFileContext& context, CName type, CScriptSyntaxNode* a, CScriptSyntaxNode* b, CScriptSyntaxNode* c, CScriptSyntaxNode* d, CScriptSyntaxNode* e )
	: m_context( context )
	, m_type( type )
	, m_parent( NULL )
	, m_defaultSwitch( NULL )
	, m_breakLabel( NULL )
	, m_continueLabel( NULL )
{
	m_children[ 0 ] = a;
	m_children[ 1 ] = b;
	m_children[ 2 ] = c;
	m_children[ 3 ] = d;
	m_children[ 4 ] = e;
}

CScriptSyntaxNode::CScriptSyntaxNode( EScriptListNode, const CScriptFileContext &context, CName type, CScriptSyntaxNode* listRoot )
	: m_context( context )
	, m_type( type )
	, m_parent( NULL )
	, m_defaultSwitch( NULL )
	, m_breakLabel( NULL )
	, m_continueLabel( NULL )
{
	// No tree children
	Red::System::MemoryZero( &m_children, sizeof( m_children ) );

	// Collect nodes from tree and put in into a list
	while ( listRoot )
	{
		CScriptSyntaxNode* next = NULL;

		// Add element to list and go down the tree
		if ( listRoot->m_type == CNAME( SyntaxListItem ) )
		{
			next = listRoot->m_children[ 0 ];
			m_list.PushBack( listRoot->m_children[ 1 ] );
			delete listRoot;
		}
		else
		{
			m_list.PushBack( listRoot );
		}   

		// Go to next item
		listRoot = next;   
	} 
}

CScriptSyntaxNode::CScriptSyntaxNode( EScriptOpNode, const CScriptFileContext& context, CName operatorType, CScriptSyntaxNode* left, CScriptSyntaxNode* right )
	: m_context( context )
	, m_type( CNAME( SyntaxOperatorCall ) )
	, m_operator( operatorType )
	, m_parent( NULL )
	, m_defaultSwitch( NULL )
	, m_breakLabel( NULL )
	, m_continueLabel( NULL )
{
	m_children[ 0 ] = left;
	m_children[ 1 ] = right;
	m_children[ 2 ] = NULL;
	m_children[ 3 ] = NULL;
	m_children[ 4 ] = NULL;
}

#ifdef RED_LOGGING_ENABLED
void CScriptSyntaxNode::Print( Uint32 printLevel, const Char* ChildOrList )
{

	String trail, extra;

	// Add trailing space
	for ( Uint32 i = 0; i < printLevel; ++i )
	{
		trail += TXT(" ");
	}

	// String
	if ( !m_value.m_string.Empty() )
	{
		extra += TXT("  (str: \"");
		extra += m_value.m_string;
		extra += TXT("\" )");
	}

	// Operator type
	if ( m_type == CNAME( SyntaxOperatorCall ) )
	{
		extra += TXT("  (op: ");
		extra += m_operator.AsString().AsChar() + Red::System::StringLengthCompileTime( TXT( "Operator" ) );
		extra += TXT(" )");
	}
	else if( m_type == CNAME( SyntaxIntConst ) )
	{
		extra += TXT("  (int: ");
		extra += ToString( m_value.m_integer );
		extra += TXT(" )");
	}
	else if( m_type == CNAME( SyntaxFloatConst ) )
	{
		extra += TXT("  (int: ");
		extra += ToString( m_value.m_float );
		extra += TXT(" )"); 
	}
	else if( m_type == CNAME( SyntaxBoolConst ) )
	{
		extra += TXT("  (bool: ");
		extra += ( m_value.m_bool )? TXT( "true" ) : TXT( "false" );
		extra += TXT(" )");
	}

	// Add type info
	if ( !m_valueType.IsVoid() )
	{
		extra += TXT("  (type: ");
		extra += m_valueType.ToString();
		extra += TXT(" )");
	}

	// Add property
	if ( m_value.m_property )
	{
		extra += TXT("  (prop: ");
		extra += m_value.m_property->GetName().AsString().AsChar();
		extra += TXT(" )");
	}

	// Add function
	if ( m_value.m_function )
	{
		extra += TXT("  (func: ");
		extra += m_value.m_function->GetName().AsString().AsChar();
		extra += TXT(" )");
	}

	// Show node name
	RED_LOG( ScriptParser, TXT( "0x%X(% 4i) %ls:  %ls%ls %ls" ), this, m_context.m_line, ChildOrList, trail.AsChar(), m_type.AsString().AsChar(), extra.AsChar() );

	// Show children
	for ( Uint32 i = 0; i < ARRAY_COUNT( m_children ); ++i )
	{
		if ( m_children[ i ] )
		{
			m_children[ i ]->Print( printLevel + 2, TXT( "C" ) );
		}
	}     

	// Show list
	if ( m_type == CNAME( SyntaxSwitch ) )
	{
		// Normal cases
		for ( Uint32 i = 0; i < m_list.Size(); ++i )
		{
			RED_LOG( ScriptParser, TXT("0x%X(% 4i) %ls:  %lscase %ls (0x%X)"), this, m_list[i]->m_context.m_line, TXT( "L" ), trail.AsChar(), m_list[i]->m_value.m_string.AsChar(), m_list[i] );
		}

		// Default case
		if ( m_defaultSwitch )
		{
			RED_LOG( ScriptParser, TXT("0x%(% 4i) X:  %lsdefault (0x%X)"), this, m_defaultSwitch->m_context.m_line, trail.AsChar(), m_defaultSwitch );
		}
	}
	else
	{
		for ( Uint32 i = 0; i < m_list.Size(); ++i )
		{
			if ( m_list[ i ] )
			{
				m_list[ i ]->Print( printLevel + 2, TXT( "L" ) );
			}
		}
	}
}
#endif // RED_LOGGING_ENABLED

void CScriptSyntaxNode::Release( Bool recursive )
{
	// Free children
	if ( recursive )
	{
		// Children
		for ( Uint32 i = 0; i < ARRAY_COUNT( m_children ); ++i )
		{
			if ( m_children[ i ] )
			{
				m_children[ i ]->Release( recursive );
			}
		}

		// List
		if ( m_type != CNAME( SyntaxSwitch ) )
		{
			for ( Uint32 i = 0; i < m_list.Size(); ++i )
			{
				if ( m_list[ i ] )
				{
					m_list[ i ]->Release( recursive );
				}
			}
		}
	}

	// Delete this node
	delete this;
}
