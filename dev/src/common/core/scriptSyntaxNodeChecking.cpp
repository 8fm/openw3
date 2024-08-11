/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptFunctionCompiler.h"
#include "gameConfiguration.h"
#include "class.h"
#include "function.h"
#include "object.h"
#include "enum.h"
#include "scriptingSystem.h"
#include "scriptFieldStubs.h"
#include "scriptCompiler.h"
#include "scriptable.h"
#include "scriptableStateMachine.h"

THashMap< CName, CScriptSyntaxNode::TNodeCheckFunc > CScriptSyntaxNode::m_nodeCheckers;
THashMap< CName, CScriptSyntaxNode::SGlobalKeyword > CScriptSyntaxNode::m_globalKeywords;
Bool CScriptSyntaxNode::m_nodeCheckersInitialised = false;

void CScriptSyntaxNode::InitialiseNodeCheckers()
{
	if( !m_nodeCheckersInitialised )
	{
		// Unused
		m_nodeCheckers[ CNAME( SyntaxCode ) ]				= &CScriptSyntaxNode::CheckNodeTypeDummy;
		m_nodeCheckers[ CNAME( SyntaxNop ) ]				= &CScriptSyntaxNode::CheckNodeTypeDummy;

		m_nodeCheckers[ CNAME( SyntaxBreakpoint ) ]			= &CScriptSyntaxNode::CheckNodeTypeBreakpoint;
		m_nodeCheckers[ CNAME( SyntaxThisValue ) ]			= &CScriptSyntaxNode::CheckNodeTypeThis;
		m_nodeCheckers[ CNAME( SyntaxNullConst ) ]			= &CScriptSyntaxNode::CheckNodeTypeNullConst;
		m_nodeCheckers[ CNAME( SyntaxIntConst ) ]			= &CScriptSyntaxNode::CheckNodeTypeIntConst;
		m_nodeCheckers[ CNAME( SyntaxFloatConst ) ]			= &CScriptSyntaxNode::CheckNodeTypeFloatConst;
		m_nodeCheckers[ CNAME( SyntaxBoolConst ) ]			= &CScriptSyntaxNode::CheckNodeTypeBoolConst;
		m_nodeCheckers[ CNAME( SyntaxStringConst ) ]		= &CScriptSyntaxNode::CheckNodeTypeStringConst;
		m_nodeCheckers[ CNAME( SyntaxNameConst ) ]			= &CScriptSyntaxNode::CheckNodeTypeNameConst;
		m_nodeCheckers[ CNAME( SyntaxAssign ) ]				= &CScriptSyntaxNode::CheckNodeTypeAssign;
		m_nodeCheckers[ CNAME( SyntaxScopeSuper ) ]			= &CScriptSyntaxNode::CheckNodeTypeSuper;
		m_nodeCheckers[ CNAME( SyntaxScopeDefault ) ]		= &CScriptSyntaxNode::CheckNodeTypeDefault;
		m_nodeCheckers[ CNAME( SyntaxScopeParent ) ]		= &CScriptSyntaxNode::CheckNodeTypeParent;
		m_nodeCheckers[ CNAME( SyntaxScopeVirtualParent ) ]	= &CScriptSyntaxNode::CheckNodeTypeParent;
		m_nodeCheckers[ CNAME( SyntaxIdent ) ]				= &CScriptSyntaxNode::CheckNodeTypeIdentifier;
		m_nodeCheckers[ CNAME( SyntaxIfThen ) ]				= &CScriptSyntaxNode::CheckNodeTypeIfThen;
		m_nodeCheckers[ CNAME( SyntaxIfThenElse ) ]			= &CScriptSyntaxNode::CheckNodeTypeIfThenElse;
		m_nodeCheckers[ CNAME( SyntaxConditional ) ]		= &CScriptSyntaxNode::CheckNodeTypeConditional;
		m_nodeCheckers[ CNAME( SyntaxWhile ) ]				= &CScriptSyntaxNode::CheckNodeTypeLoopWhile;
		m_nodeCheckers[ CNAME( SyntaxDoWhile ) ]			= &CScriptSyntaxNode::CheckNodeTypeLoopWhile;
		m_nodeCheckers[ CNAME( SyntaxFor ) ]				= &CScriptSyntaxNode::CheckNodeTypeLoopFor;
		m_nodeCheckers[ CNAME( SyntaxBreak ) ]				= &CScriptSyntaxNode::CheckNodeTypeBreak;
		m_nodeCheckers[ CNAME( SyntaxContinue ) ]			= &CScriptSyntaxNode::CheckNodeTypeContinue;
		m_nodeCheckers[ CNAME( SyntaxArrayElement ) ]		= &CScriptSyntaxNode::CheckNodeTypeArrayElement;
		m_nodeCheckers[ CNAME( SyntaxSwitch ) ]				= &CScriptSyntaxNode::CheckNodeTypeSwitch;
		m_nodeCheckers[ CNAME( SyntaxSwitchCase ) ]			= &CScriptSyntaxNode::CheckNodeTypeSwitchCase;
		m_nodeCheckers[ CNAME( SyntaxDefaultCase ) ]		= &CScriptSyntaxNode::CheckNodeTypeSwitchCaseDefault;
		m_nodeCheckers[ CNAME( SyntaxFuncCall ) ]			= &CScriptSyntaxNode::CheckNodeTypeCallFunction;
		m_nodeCheckers[ CNAME( SyntaxOperatorCall ) ]		= &CScriptSyntaxNode::CheckNodeTypeCallOperator;
		m_nodeCheckers[ CNAME( SyntaxReturn ) ]				= &CScriptSyntaxNode::CheckNodeTypeReturn;
		m_nodeCheckers[ CNAME( SyntaxNew ) ]				= &CScriptSyntaxNode::CheckNodeTypeNew;
		m_nodeCheckers[ CNAME( SyntaxDelete ) ]				= &CScriptSyntaxNode::CheckNodeTypeDelete;
		m_nodeCheckers[ CNAME( SyntaxCast ) ]				= &CScriptSyntaxNode::CheckNodeTypeCast;
		m_nodeCheckers[ CNAME( SyntaxEnter ) ]				= &CScriptSyntaxNode::CheckNodeTypeEnter;
		m_nodeCheckers[ CNAME( SyntaxSavePoint ) ]			= &CScriptSyntaxNode::CheckNodeTypeSavePoint;

		//TODO: Move to more generic location
		RegisterGlobalKeyword( CNAME( SyntaxGlobalGame ),		GGameConfig::GetInstance().GetGameClassName().AsChar(),			TXT( "theGame" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalPlayer ),		GGameConfig::GetInstance().GetPlayerClassName().AsChar(),			TXT( "thePlayer" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalTelemetry ),	GGameConfig::GetInstance().GetTelemetryClassName().AsChar(),		TXT( "theTelemetry" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalCamera ),		GGameConfig::GetInstance().GetCameraDirectorClassName().AsChar(),	TXT( "theCamera" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalSound ),		TXT( "CScriptSoundSystem" ),								TXT( "theSound" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalDebug ),		TXT( "CDebugAttributesManager" ),							TXT( "theDebug" ) );
		RegisterGlobalKeyword( CNAME( SyntaxGlobalTimer ),		TXT( "CTimerScriptKeyword" ),								TXT( "theTimer" ) ); 
		RegisterGlobalKeyword( CNAME( SyntaxGlobalInput ),		TXT( "CInputManager" ),										TXT( "theInput" ) );
		 
		m_nodeCheckersInitialised = true;
	}
}

void CScriptSyntaxNode::RegisterGlobalKeyword( const CName& nodeType, const String& className, const String& scriptKeyword )
{
	SGlobalKeyword keyword;

	keyword.nodeType = nodeType;
	keyword.className = className;
	keyword.scriptKeyword = scriptKeyword;

	ASSERT( !m_globalKeywords.KeyExist( nodeType ), TXT( "Cannot add global keyword '%ls' twice" ), nodeType.AsString().AsChar() );

	m_globalKeywords[ nodeType ] = keyword;

	m_nodeCheckers[ nodeType ] = &CScriptSyntaxNode::CheckNodeTypeGlobalKeyword;
}

void CScriptSyntaxNode::EmitError( CScriptFunctionCompiler* compiler, const Char* errorString, ... )
{
	// Format text
	va_list arglist;
	va_start( arglist, errorString );
	Char formatedText[ 4096 ];
	Red::System::VSNPrintF( formatedText, ARRAY_COUNT( formatedText ), errorString, arglist );

	// Emit to compiler
	compiler->EmitError( m_context, formatedText );
}

CScriptSyntaxNode::SControlPath::SControlPath()
:	m_node			( nullptr )
,	m_prev			( nullptr )
,	m_next			( nullptr )
,	m_branchIf		( nullptr )
,	m_branchElse	( nullptr )
{
}

CScriptSyntaxNode::SControlPath::~SControlPath()
{
}

CScriptSyntaxNode::SControlPath* CScriptSyntaxNode::SControlPath::GetEndNode()
{
	SControlPath* endNode = this;

	while( endNode->m_next )
	{
		endNode = endNode->m_next;
	}

	return endNode;
}

Bool CScriptSyntaxNode::SControlPath::CheckControlPaths( CScriptFunctionCompiler* compiler, Bool returnValueRequired )
{
	// We need to examine this in reverse order
	SControlPath* current = GetEndNode();

	Uint32 linesAfterReturn = 0;

	do 
	{
		if( current->m_node->m_type == CNAME( SyntaxReturn ) )
		{
			if( linesAfterReturn > 0 )
			{
				compiler->EmitWarning( current->m_node->m_context, TXT( "Return statement prevents the execution of successive lines of script" ) );
			}

			return true;
		}
		else
		{
			if( current->m_branchIf )
			{
				return
				(
					current->m_branchIf->CheckControlPaths( compiler, returnValueRequired ) &&
					(
						!current->m_branchElse ||
						current->m_branchElse->CheckControlPaths( compiler, returnValueRequired )
					)
				);
			}
		}

		++linesAfterReturn;
		current = current->m_prev;
	} while( current );

	if( returnValueRequired )
	{
		compiler->EmitError( m_node->m_context, TXT( "Not all control paths return a value" ) );
	}

	return !returnValueRequired;
}

//////////////////////////////////////////////////////////////////////////
// The following MapControlPath* functions will take a script function, like the following:
// 
// function testFunc() : int
// {
// 	var a : int;
// 	a = 1;
// 	if( a == 1 )
// 	{
// 		return 1;
// 	}
// 	else if( a == 2 )
// 	{
// 		return 2;
// 	}
// 	else
// 	{
// 		return 3;
// 	}
// }
// 
//////////////////////////////////////////////////////////////////////////
// ...of which the tree looks like this:
// 
// SyntaxCode
//   SyntaxBreakpoint
//     SyntaxAssign
//       SyntaxIdent   (str: "a" )
//       SyntaxIntConst   (int: 1 )
//   SyntaxBreakpoint
//     SyntaxIfThenElse
//       SyntaxOperatorCall   (op: Equal )
//         SyntaxIdent   (str: "a" )
//         SyntaxIntConst   (int: 1 )
//       SyntaxCode
//         SyntaxReturn
//           SyntaxBreakpoint
//             SyntaxIntConst   (int: 1 )
//         SyntaxNop
//       SyntaxBreakpoint
//         SyntaxIfThenElse
//           SyntaxOperatorCall   (op: Equal )
//             SyntaxIdent   (str: "a" )
//             SyntaxIntConst   (int: 2 )
//           SyntaxCode
//             SyntaxReturn
//               SyntaxBreakpoint
//                 SyntaxIntConst   (int: 2 )
//             SyntaxNop
//           SyntaxCode
//             SyntaxReturn
//               SyntaxBreakpoint
//                 SyntaxIntConst   (int: 3 )
//             SyntaxNop
//   SyntaxNop
//   
//////////////////////////////////////////////////////////////////////////
//   ...And reduce it down to a simple tree like so (it removes a lot of unnecessary information when looking for control paths)
//   
//   SyntaxAssign SyntaxIdent SyntaxIntConst SyntaxIfThenElse
//                                           (if)   - SyntaxReturn
//                                           (else) - SyntaxIfThenElse
//                                                    (if)   - SyntaxReturn
//                                                    (else) - SyntaxReturn

CScriptSyntaxNode::SControlPath* CScriptSyntaxNode::MapControlPaths( TControlPathPool& pool, SControlPath* previousNode )
{
	SControlPath* currentNode = NULL;

	if( m_type == CNAME( SyntaxBreakpoint ) || m_type == CNAME( SyntaxCode ) )
	{
		// These syntax nodes are inherently meaningless by themselves, but they have child nodes that we need to concern ourselves with
		currentNode = MapControlPathNodeChildren( pool );
	}

	// Do not create an entry in the path for no-ops
	else if( m_type != CNAME( SyntaxNop ) )
	{
		// This is a type of node we need to add to the path
		pool.Grow();
		currentNode = &pool.Back();
		currentNode->m_node = this;

		if( m_type == CNAME( SyntaxIfThenElse ) )
		{
			// The child node at position 0 is the comparison, so we don't need to worry about that
			currentNode->m_branchIf		= m_children[ 1 ]->MapControlPaths( pool, nullptr );
			currentNode->m_branchElse	= m_children[ 2 ]->MapControlPaths( pool, nullptr );
		}
 		else if( m_type == CNAME( SyntaxSwitch ) )
 		{
			MapControlPathNodeSwitch( pool, currentNode );
 		}

		// - We *want* return nodes themselves, but we don't want their child value nodes flattened into the path as well
		// - We want to ignore the children of an "IfThen" (No "Else") because they may contain a return statement that gets flattened into the path
		else if( m_type != CNAME( SyntaxReturn ) && m_type != CNAME( SyntaxIfThen ) )
		{
			//Flatten the tree into a straight linked list
			SControlPath* childPath = MapControlPathNodeChildren( pool );

			if( childPath )
			{
				ASSERT( !currentNode->m_next );
				ASSERT( !childPath->m_prev );

				currentNode->m_next = childPath;
				childPath->m_prev = currentNode;
			}
		}
	}

	// Link the node we just created with the previous node
	if( currentNode && previousNode )
	{
		previousNode = previousNode->GetEndNode();

		ASSERT( !currentNode->m_prev );
		ASSERT( !previousNode->m_next );

		currentNode->m_prev = previousNode;
		previousNode->m_next = currentNode;
	}

	return currentNode;
}

CScriptSyntaxNode::SControlPath* CScriptSyntaxNode::MapControlPathNodeChildren( TControlPathPool& pool )
{
	SControlPath* firstNode = nullptr;
	SControlPath* currentNode = nullptr;

	for ( Uint32 i = 0; i < ARRAY_COUNT( m_children ); ++i )
	{
		if ( m_children[ i ] )
		{
			currentNode = m_children[ i ]->MapControlPaths( pool, currentNode );

			if( !firstNode )
			{
				firstNode = currentNode;
			}
		}
	}

	for ( Uint32 i = 0; i < m_list.Size(); ++i )
	{
		if( m_list[ i ] )
		{
			SControlPath* nextNode = m_list[ i ]->MapControlPaths( pool, currentNode );

			if( nextNode )
			{
				currentNode = nextNode;

				if( !firstNode )
				{
					firstNode = currentNode;
				}
			}
		}
	}

	return firstNode;
}

void CScriptSyntaxNode::MapControlPathNodeSwitch( TControlPathPool& pool, SControlPath* currentPath )
{
	// In order to find the various positions of the switch cases
	// First flatten the path and then rearrange the linked list
	// according to the rules of the switch
	// This will mimic the rules of the IfThenElse,
	// where each successive "ElseIf" is actually a child "IfThenElse"
	SControlPath* switchPath = MapControlPathNodeChildren( pool );

	SControlPath* currentBranch = currentPath;
	SControlPath* previousDisconnection = nullptr;

	Bool stateConnectToSwitchPath = true;

	while( switchPath )
	{
		// Do we have a switch case
		if( switchPath->m_node->m_type == CNAME( SyntaxSwitchCase ) || switchPath->m_node->m_type == CNAME( SyntaxDefaultCase ) )
		{
			// If this is the first switch case we've come across (since the start of the switch statement or
			// since the last "break") - remove it from the path
			// Alternatively, the previous case is falling through to this one, so don't do anything
			if( stateConnectToSwitchPath )
			{
				if( switchPath->m_prev )
				{
					previousDisconnection = switchPath->m_prev;
					switchPath->m_prev->m_next = nullptr;
					switchPath->m_prev = nullptr;
				}

				stateConnectToSwitchPath = false;
			}

			// Attach this to the appropriate branch path
			if( !currentBranch->m_branchIf )
			{
				currentBranch->m_branchIf = switchPath;
			}
			else
			{
				currentBranch->m_branchElse = switchPath;
				currentBranch = currentBranch->m_branchElse;
			}

			switchPath = switchPath->m_next;
		}

		// Otherwise, do we have the break statement and hence, end of execution for this code path?
		else if( switchPath->m_node->m_type == CNAME( SyntaxBreak ) )
		{
			ASSERT( previousDisconnection );

			SControlPath* disconnectMe = switchPath;

			previousDisconnection = switchPath = switchPath->m_next;

			if( disconnectMe->m_next )
			{
				disconnectMe->m_next->m_prev = nullptr;
				disconnectMe->m_next = nullptr;
			}

			stateConnectToSwitchPath = true;
		}

		// Finally just move along the code path
		else
		{
			// This will chop out any code between a "break" and a new "case"
			if( stateConnectToSwitchPath )
			{
				if( previousDisconnection )
				{
					previousDisconnection->m_next = switchPath;
					switchPath->m_prev = previousDisconnection;
					previousDisconnection = nullptr;
				}
			}

			switchPath = switchPath->m_next;
		}
	}
}


Bool CScriptSyntaxNode::CheckNodeTypes( CScriptFunctionCompiler* compiler )
{
	Bool state = true;

	// Check types in children first
	for ( Uint32 i = 0; i < ARRAY_COUNT( m_children ); ++i )
	{
		CScriptSyntaxNode* subNode = m_children[ i ];
		if ( subNode )
		{
			subNode->m_parent = this;
			state &= subNode->CheckNodeTypes( compiler );
		}
	}

	// Check types in sub items then
	if ( m_type != CNAME( SyntaxSwitch ) )
	{
		for ( Uint32 i = 0; i < m_list.Size(); ++i )
		{
			CScriptSyntaxNode* subNode = m_list[ i ];
			if ( subNode )
			{
				state &= subNode->CheckNodeTypes( compiler );
			}
		}
	}

	// Error occurred in child nodes, do not bother checking mode
	if ( !state )
	{
		return false;
	}

	TNodeCheckFunc typeCheckFunc = NULL;
	m_nodeCheckers.Find( m_type, typeCheckFunc );
	ASSERT( typeCheckFunc != NULL, TXT( "No node type checker function found in map for node type '%ls'" ), m_type.AsString().AsChar() );

	return (this->*typeCheckFunc)( compiler );
}

Bool CScriptSyntaxNode::MatchNodeType( CScriptSyntaxNode*& nodePtr, CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& destType, Bool explicitCast, Bool functionParam ) const
{
	// Get casting cost
	const ScriptSyntaxNodeType &srcType = nodePtr->m_valueType;
	Int32 castCost = ScriptSyntaxNodeType::GetCastCost( srcType, destType );
	if ( castCost == -1 )
	{
		compiler->EmitError( m_context, String::Printf( TXT( "Unable to convert from '%ls' to '%ls'" ), srcType.ToString().AsChar(), destType.ToString().AsChar() ) );
		return false;
	}

	// Well if we need explicit cast and it's not allowed then exit
	if ( castCost == -2 && !explicitCast )
	{
		compiler->EmitError( m_context, String::Printf( TXT("No implicit cast exists for converting '%ls' to '%ls'"), srcType.ToString().AsChar(), destType.ToString().AsChar() ) );
		return false;
	}

	// Function param, check types VERY carefully
	if ( functionParam )
	{
		// We need assignable type and we don't have one
		if ( destType.m_isAssignable && !srcType.m_isAssignable )
		{
			compiler->EmitError( m_context, String::Printf( TXT( "Expected type '%ls' to be assignable" ), srcType.ToString().AsChar() ) );
			return false;
		}

		// If types are different and we need assignable type...
		if ( destType.m_isAssignable && !ScriptSyntaxNodeType::CheckCompatibleTypes( srcType, destType ) )
		{
			compiler->EmitError( m_context, String::Printf( TXT( "Imcompatibile types '%ls' and '%ls'" ), srcType.ToString().AsChar(), destType.ToString().AsChar() ) );
			return false;
		}
	}    

	// No real cast needed or don't do it now
	if ( castCost == 0 )
	{
		return true;
	}

	// Get the type of cast node to insert
	CName nodeType = ScriptSyntaxNodeType::GetCastNode( srcType, destType );
	if ( nodeType == CNAME( SyntaxNoCast ) )
	{
		compiler->EmitError( m_context, String::Printf( TXT( "Internal conversion error ('%ls' -> '%ls')" ), srcType.ToString().AsChar(), destType.ToString().AsChar() ) );
		return false;
	}

	// No conversion node needed
	if ( nodeType == CName::NONE )
	{
		return true;
	}

	// Make sure wished type is not assignable
	if ( functionParam && destType.m_isAssignable )
	{
		compiler->EmitError( m_context, String::Printf( TXT( "Unable to convert to L-value" ) ) );
		return false;
	}  

//	Generates too many warnings
// 	if( !explicitCast && castCost > 3 )
// 	{
// 		compiler->EmitWarning( m_context, String::Printf( TXT( "Implicit conversion from '%ls' to '%ls'" ), srcType.ToString().AsChar(), destType.ToString().AsChar() ) );
// 	}

	// Create node
	CScriptSyntaxNode *newNode = new CScriptSyntaxNode( E_Tree, nodePtr->m_context, nodeType, nodePtr );
	newNode->m_valueType = destType;
	newNode->m_valueType.m_isAssignable = false;

	// Dynamic cast
	if ( newNode->m_type == CNAME( SyntaxDynamicCast ) )
	{
		ASSERT( destType.GetPtrClass() );
		newNode->m_value.m_type = destType.GetPtrClass();
	}

	// Substitute node with the casting node
	nodePtr = newNode;  

	// All ok
	return true;
}

CProperty* CScriptSyntaxNode::FindPropertyInScope( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType, const String& name )
{
	const CName propertyName( name );

	// No scope
	if ( scopeType.IsVoid() && compiler )
	{
		// In case if no scope was given search inside currently compiled function first
		CFunction* compiledFunction = compiler->GetCompiledFunction();
		if ( compiledFunction )
		{
			CProperty* prop = compiledFunction->FindProperty( propertyName );
			if ( prop )
			{
				return prop;
			}
		}

		// Search in compiled class then
		CClass* compiledClass = compiler->GetCompiledClass();
		if ( compiledClass )
		{
			CProperty* prop = compiledClass->FindProperty( propertyName );
			if ( prop )
			{
				return prop;
			}
		}
	}

	// A scope is a pointed class
	CClass* pointedClass = scopeType.GetPtrClass();
	if ( pointedClass )
	{
		CProperty* prop = pointedClass->FindProperty( propertyName );
		if ( prop )
		{
			return prop;
		}
	}

	// A scope is a structure
	CClass* structClass = scopeType.GetStruct();
	if ( structClass )
	{
		CProperty* prop = structClass->FindProperty( propertyName );
		if ( prop )
		{
			return prop;
		}
	}

	// No property found
	return NULL;
}

const CFunction* CScriptSyntaxNode::FindEntryFunctionInClass( CClass* compiledClass, const String& name )
{
	// Get states
	const auto& stateClasses = compiledClass->GetStateClasses();

	CName entryFunctionName( name );

	// Search in states
	for ( Uint32 i = 0; i < stateClasses.Size(); ++i )
	{
		// Search only in local functions
		const auto& localFunctions = stateClasses[i]->GetLocalFunctions();

		for ( Uint32 j = 0; j < localFunctions.Size(); ++j )
		{
			const CFunction* func = localFunctions[j];
			if ( func->IsEntry() && func->GetName() == entryFunctionName )
			{
				return func;
			}
		}
	}

	// Not found
	return NULL;
}

const CFunction* CScriptSyntaxNode::FindFunctionInScope( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType, const String& name )
{
	const CName functionName( name );

	// No scope
	if ( scopeType.IsVoid() && compiler )
	{
		// Search in compiled class then
		CClass* compiledClass = compiler->GetCompiledClass();
		if ( compiledClass )
		{
			// Find in class
			const CFunction* prop = compiledClass->FindFunctionNonCached( functionName );
			if ( prop )
			{
				return prop;
			}

			// Class is a state machine, try finding entry function
			prop = FindEntryFunctionInClass( compiledClass, name );
			if ( prop )
			{
				return prop;
			}
		}

		// Search in global scope
		CFunction* prop = SRTTI::GetInstance().FindGlobalFunction( functionName );
		if ( prop )
		{
			return prop;
		}
	}

	// A scope is a pointed class
	CClass* pointedClass = scopeType.GetPtrClass();
	if ( pointedClass )
	{
		// Find in class
		const CFunction* prop = pointedClass->FindFunctionNonCached( functionName );
		if ( prop )
		{
			return prop;
		}

		// Class is a state machine, try finding entry function
		prop = FindEntryFunctionInClass( pointedClass, name );
		if ( prop )
		{
			return prop;
		}
	}

	// No property found
	return NULL;
}

CEnum* CScriptSyntaxNode::FindEnumValue( CScriptFunctionCompiler*, const ScriptSyntaxNodeType&, const String& name, Int32& value )
{
	// Search in all enumerators
	return SRTTI::GetInstance().FindEnumValue( name, value );
}

Bool CScriptSyntaxNode::HasSavepointsSupport( const CFunction* function )
{
	return function->IsQuest() && function->IsLatent();
}


Bool CScriptSyntaxNode::FindLatentFunctions( CScriptSyntaxNode* nodePtr )
{
	ASSERT( nodePtr );
	if( nodePtr->m_type == CNAME( SyntaxFuncCall ) )
	{
		CFunction* fun = nodePtr->m_value.m_function;
		if( fun && fun->IsLatent() )
		{
			return true;
		}
	}

	// Find tree children
	for( Uint32 i = 0; i<CScriptSyntaxNode::MAX_TREE_CHILDREN; i++ )
	{
		if( nodePtr->m_children[i] )
		{
			if( FindLatentFunctions( nodePtr->m_children[i] ) )
			{
				return true;
			}
		}
	}

	// Find list children
	for( Uint32 i = 0; i<nodePtr->m_list.Size(); i++ )
	{
		if( nodePtr->m_list[i] )
		{
			if( FindLatentFunctions( nodePtr->m_list[i] ) )
			{
				return true;
			}
		}
	}

	return false;
}

Bool CScriptSyntaxNode::CheckNodeTypeDummy( CScriptFunctionCompiler* )
{
	// Nothing to do
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeBreakpoint( CScriptFunctionCompiler* )
{
	// Copy type from sub expression
	if ( m_children[ 0 ] )
	{
		m_valueType = m_children[ 0 ]->m_valueType;
		m_context = m_children[ 0 ]->m_context;
	}

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeThis( CScriptFunctionCompiler* compiler )
{
	// Don't use in global functions
	if ( !compiler->GetCompiledClass() )
	{
		EmitError( compiler, TXT( "'this' has no sense outside class scope" ) );
		return false;
	}

	// Don't use inside static function
	if ( compiler->GetCompiledFunction()->IsStatic() )
	{
		EmitError( compiler, TXT( "'this' has no sense inside static function" ) );
		return false;
	}

	// Set type
	CClass* functionClass = compiler->GetCompiledFunction()->GetClass();
	m_valueType.InitPointer( functionClass );

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeNullConst( CScriptFunctionCompiler* )
{
	m_valueType.InitNULL();
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeIntConst( CScriptFunctionCompiler* )
{
	m_valueType.InitSimple( ::GetTypeName< Int32 >() );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeFloatConst( CScriptFunctionCompiler* )
{
	m_valueType.InitSimple( ::GetTypeName< Float >() );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeBoolConst( CScriptFunctionCompiler* )
{
	m_valueType.InitSimple( ::GetTypeName< Bool >() );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeStringConst( CScriptFunctionCompiler* )
{
	m_valueType.InitSimple( ::GetTypeName< String >() );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeNameConst( CScriptFunctionCompiler* )
{
	m_valueType.InitSimple( ::GetTypeName< CName >() );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeAssign( CScriptFunctionCompiler* compiler )
{
	// Left side type should be assignable
	if ( !m_children[0]->m_valueType.m_isFromProperty )
	{
		EmitError( compiler, TXT("L-value required") );
		return false;
	}

	// Left side type should be assignable
	if ( !m_children[0]->m_valueType.m_isAssignable )
	{
		EmitError( compiler, TXT("Left hand side is const") );
		return false;
	}

	// Get target type
	m_valueType = m_children[0]->m_valueType;
	m_valueType.m_isAssignable = false;

	// Cast right side
	if ( !MatchNodeType( m_children[1], compiler, m_valueType, false, false ) )
	{
		return false;
	}

	// Casted
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeSuper( CScriptFunctionCompiler* compiler )
{
	// Super modified must be the first one
	if ( m_children[0] )
	{
		EmitError( compiler, TXT("'super' modified must go first") );
		return false;
	}

	// We must be in a function
	if ( !compiler->GetCompiledFunction() )
	{
		EmitError( compiler, TXT("'super' is valid only inside function") );
		return false;
	}

	// Get outer scope (scope inside which is our function)
	CClass* compiledClass = compiler->GetCompiledClass();
	if ( !compiledClass )
	{
		EmitError( compiler, TXT("'super' is valid only inside function in class") );
		return false;
	}

	// No super function at super class
	CClass* baseClass = compiledClass->GetBaseClass();
	if ( !baseClass )
	{
		EmitError( compiler, TXT("No valid context for 'super' modifier") );
		return false;
	}

	// Set our type
	m_valueType.InitPointer( baseClass, false );

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeDefault( CScriptFunctionCompiler* compiler )
{
	// Accessing default scope from other object
	if ( m_children[ 0 ] )
	{
		// Target object should has class
		CClass* scopeClass = m_children[ 0 ]->m_valueType.GetStruct();
		if ( scopeClass )
		{
			scopeClass = m_children[ 0 ]->m_valueType.GetPtrClass();
		}

		// Get outer scope
		if ( !scopeClass )
		{
			EmitError( compiler, TXT( "Unable to deduce the 'default' scope from '%ls'" ), m_children[ 0 ]->m_valueType.ToString().AsChar() );
			return false;
		}

		// Use the class scope
		m_valueType.InitPointer( scopeClass, false );

		return true;
	}

	// Accessing default scope
	else
	{
		// Get outer scope
		CClass* compiledClass = compiler->GetCompiledClass();
		if ( !compiledClass )
		{
			EmitError( compiler, TXT( "'default' is valid only inside a class" ) );
			return false;
		}

		// Use the class scope
		m_valueType.InitPointer( compiledClass, false );

		return true;
	}
}

Bool CScriptSyntaxNode::CheckNodeTypeParent( CScriptFunctionCompiler* compiler )
{
	CClass* scopeClass = NULL;
	if ( m_children[0] )
	{
		// Context should be a pointer
		CClass* stateClass = m_children[0]->m_valueType.GetPtrClass();
		if ( !stateClass )
		{
			EmitError( compiler, TXT("Unable to deduce the 'parent' scope from '%ls'"), m_children[0]->m_valueType.ToString().AsChar() );
			return false;
		}

		// We are not state
		if ( !stateClass->IsState() )
		{
			EmitError( compiler, TXT("Class '%ls' is not a state and therefore 'parent' has no sense"), stateClass->GetName().AsString().AsChar() );
			return false;
		}

		// Get the state machine
		scopeClass = stateClass->GetStateMachineClass();
	}
	else
	{
		// We are not inside a class
		CClass* compiledClass = compiler->GetCompiledClass();
		if ( !compiler->GetCompiledClass() )
		{
			EmitError( compiler, TXT("'parent' has no sense outside class") );
			return false;
		}

		// We are not state
		if ( !compiledClass->IsState() )
		{
			EmitError( compiler, TXT("Class '%ls' is not a state and therefore 'parent' has no sense"), compiledClass->GetName().AsString().AsChar() );
			return false;
		}

		// Get the scope of the state machine
		scopeClass = compiledClass->GetStateMachineClass();
	}

	// This should be a state machine
	if ( !scopeClass )
	{
		EmitError( compiler, TXT("'parent used inside invalid statemachine.") );
		return false;
	}

	// Use the state machine as a type
	m_valueType.InitPointer( scopeClass, false );

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeIdentifierClass( CScriptFunctionCompiler* compiler )
{
	const Bool hasNoScope = ( NULL == m_children[0] );

	CClass* structure = SRTTI::GetInstance().FindClass( CName( m_value.m_string ) );
	if ( structure != NULL )
	{
		// Structure constructors are allowed only in empty scope
		if ( !hasNoScope )
		{
			// General error
			EmitError( compiler, TXT("Structure constructor cannot be used inside parent scope") );
			return false;
		}

		// Valid
		m_value.m_structure = structure;
		return true;
	}

	return false;
}

Bool CScriptSyntaxNode::CheckNodeTypeIdentifierFunction( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType )
{
	// Start with function
	const CFunction* func = FindFunctionInScope( compiler, scopeType, m_value.m_string );
	if ( !func )
	{
		// Nope, might be a class/struct constructor
		if( !CheckNodeTypeIdentifierClass( compiler ) )
		{
			EmitError( compiler, TXT( "Could not find function '%ls'" ), m_value.m_string.AsChar() );
			return false;
		}

		return true;
	}

	const Bool hasNoScope = ( NULL == m_children[0] );
	const Bool isCompiledStaticFunc = compiler->GetCompiledFunction()->IsStatic();
	const Bool isParentDefault = m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeDefault );
	const Bool isTargetStaticFunc = func->IsStatic();

	// In default scope we cannot access functions
	if ( isParentDefault )
	{
		EmitError( compiler, TXT("You cannot call function '%ls' from within default scope"), m_value.m_string.AsChar() );
		return false;
	}

	// Accessing non static function from static one
	if ( !isTargetStaticFunc && isCompiledStaticFunc && hasNoScope )
	{
		EmitError( compiler, TXT("You cannot call nonstatic function '%ls' from static scope"), m_value.m_string.AsChar() );
		return false;
	}

	// We are inside a class scope
	CClass* classScope = scopeType.GetStruct();
	if ( classScope && !isTargetStaticFunc )
	{
		EmitError( compiler, TXT("You cannot call nonstatic function '%ls' from class scope"), m_value.m_string.AsChar() );
		return false;
	}

	const CClass* funcClass = func->GetClass();
	const CClass* compiledClass = compiler->GetCompiledClass();
	if ( compiler->IsStrictMode() && funcClass )
	{
		const CClass* stateMachineClass = compiler->GetCompiledClass() ? compiler->GetCompiledClass()->GetStateMachineClass() : nullptr;
		ASSERT( !stateMachineClass || compiler->GetCompiledClass()->IsState() ); // Just a sanity check to make sure the class flags aren't lying

		// Grant states protected access to the state machine inheritance hierarchy
		const Bool stateMachineClassIsAFuncClass = stateMachineClass ? stateMachineClass->IsA( const_cast< CClass* >( funcClass ) ) : false;

		if ( func->IsEntry() && funcClass != compiledClass )
		{
			EmitError( compiler, TXT("Cannot call entry function '%ls' in class '%ls' here."), m_value.m_string.AsChar(), funcClass->GetName().AsString().AsChar() );
			return false;
		}
		else if ( func->IsPrivate() && funcClass != compiledClass )
		{
			EmitError( compiler, TXT("Cannot call private function '%ls' in class '%ls' here."), m_value.m_string.AsChar(), funcClass->GetName().AsString().AsChar() );
			return false;
		}
		else if ( func->IsProtected() && ! (  ( compiledClass && compiledClass->IsA( const_cast< CClass* >( funcClass ) ) ) || stateMachineClassIsAFuncClass ) )
		{
			EmitError( compiler, TXT("Cannot call protected function '%ls' in class '%ls' here."), m_value.m_string.AsChar(), funcClass->GetName().AsString().AsChar() );
			return false;
		}
	}

	if ( compiler->IsStrictMode() && func->IsExec() )
	{
		EmitError( compiler, TXT("Cannot call exec function '%ls' from scripts instead of the console."), func->GetName().AsString().AsChar() );
		return false;
	}

	// Well, we can compile this
	m_value.m_function = const_cast< CFunction* >( func );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeIdentifier( CScriptFunctionCompiler* compiler )
{
	ScriptSyntaxNodeType scopeType;
	const Bool hasNoScope = ( NULL == m_children[0] );
	const Bool isCompiledStaticFunc = compiler->GetCompiledFunction()->IsStatic();
	const Bool isParentSuper = m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeSuper );
	const Bool isParentDefault = m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeDefault );
	const Bool isParentParent = m_children[0] && ( m_children[0]->m_type == CNAME( SyntaxScopeParent ) || m_children[0]->m_type == CNAME( SyntaxScopeVirtualParent ) );

	const CClass* stateMachineClass = compiler->GetCompiledClass() ? compiler->GetCompiledClass()->GetStateMachineClass() : 0;
	ASSERT( !stateMachineClass || compiler->GetCompiledClass()->IsState() ); // Just a sanity check to make sure the class flags aren't lying

	// We have parent scope		
	if ( m_children[0] )
	{
		scopeType = m_children[0]->m_valueType;
	}

	// Array property, we don't need more than that
	if ( scopeType.IsArray() )
	{
		return true;
	}

	// Does the code think it's a function call?
	if ( m_parent && m_parent->m_type == CNAME( SyntaxFuncCall ) )
	{
		return CheckNodeTypeIdentifierFunction( compiler, scopeType );
	}
	else
	{
		if( CheckNodeTypeIdentifierClass( compiler ) )
		{
			return true;
		}
	}

	// Find property
	CProperty* prop = FindPropertyInScope( compiler, scopeType, m_value.m_string );

	// We were unable to find field, try to search for Enum value
	if ( !prop )
	{
		// Search for enum value
		if ( hasNoScope )
		{
			Int32 enumValue = 0;
			CEnum* enumObject = FindEnumValue( compiler, scopeType, m_value.m_string, enumValue );
			if ( enumObject )
			{
				// Use direct value
				m_type = CNAME( SyntaxEnumConst );
				m_value.m_dword = enumObject->GetSize();
				m_value.m_integer = enumValue;

				// Setup type
				m_valueType.InitSimple( enumObject );
				return true;
			}
		}

		// Error - ident was not found
		if ( hasNoScope )
		{
			// General error
			EmitError( compiler, TXT("I dont know any '%ls'"), m_value.m_string.AsChar() );
		}
		else
		{
			// Specialized error - no symbol found under given scope
			EmitError( compiler, TXT("'%ls' is not a member of '%ls'"), m_value.m_string.AsChar(), scopeType.ToString().AsChar() );
		}

		// Exit
		return false;
	}

	// This is a native property
	if ( prop->IsNative() && !prop->IsExported() )
	{
		EmitError( compiler, TXT("Property '%ls' exists but was not imported from C++ code."), prop->GetName().AsString().AsChar() );
		return false;
	}

	// In super scope only functions can be accessed
	if ( isParentSuper )
	{
		EmitError( compiler, TXT("Only functions can be accessed in 'super' scope") );
		return false;
	}

	// Instance property in static function ?
	if ( hasNoScope && isCompiledStaticFunc && !prop->IsInFunction() )
	{
		EmitError( compiler, TXT("Instance properties are not accessible in static functions") );
		return false;
	}

	const CClass* propClass = prop->GetParent();
	const CClass* compiledClass = compiler->GetCompiledClass();
	if ( compiler->IsStrictMode() && propClass )
	{
		// Grant states protected access to the state machine inheritance hierarchy
		const Bool stateMachineClassIsAPropClass = stateMachineClass ? stateMachineClass->IsA( const_cast< CClass* >( propClass ) ) : false;

		if ( prop->IsPrivate() && propClass != compiledClass )
		{
			EmitError( compiler, TXT("Cannot access private property '%ls' in class '%ls' here."), m_value.m_string.AsChar(), propClass->GetName().AsString().AsChar() );
			return false;
		}
		else if ( prop->IsProtected() && ! ( ( compiledClass && compiledClass->IsA( const_cast< CClass* >( propClass ) ) ) || stateMachineClassIsAPropClass ) )
		{
			EmitError( compiler, TXT("Cannot access protected property '%ls' in class '%ls' here."), m_value.m_string.AsChar(), propClass->GetName().AsString().AsChar() );
			return false;
		}
	}

	// Determine if type is assignable
	Bool isAssignable = !prop->IsReadOnly();
	Bool isFromVariable = true;
	if ( m_children[0] )
	{
		Bool isParentScopeAssignable = false;
		if ( m_children[0]->m_type == CNAME( SyntaxThisValue ) )
		{
			// Values under this are assignable
			isParentScopeAssignable = true;
			isFromVariable = true;
		}
		else if ( isParentDefault )
		{
			// Properties from default scope are not assignable
			isParentScopeAssignable = false;
			isFromVariable = true;
		}
		else if ( isParentParent )
		{
			// Properties from default scope are not assignable
			isParentScopeAssignable = true;
			isFromVariable = true;
		}
		else 
		{
			// Ask scope
			isParentScopeAssignable = m_children[0]->m_valueType.m_isAssignable;
			isFromVariable = m_children[0]->m_valueType.m_isFromProperty;
		}

		// Parent has a structure type and this is a structure member access. This will work only if parent is accessed via property.
		CClass* parentStructType = m_children[0]->m_valueType.GetStruct();
		if ( parentStructType )
		{
			// We can index only direct properties
			if ( !m_children[0]->m_valueType.m_isFromProperty )
			{
				EmitError( compiler, TXT("Cannot access struct property '%ls'. Parent scope does not compe from variable."), prop->GetName().AsString().AsChar() );
				return false;		
			}
		}

		// This value is assignable if the parent scope is
		isAssignable &= isParentScopeAssignable;
	}

	// Save data
	m_value.m_property = prop;

	// Save type
	m_valueType.InitSimple( prop->GetType(), isAssignable );
	m_valueType.m_isFromProperty = isFromVariable;

	// Done
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeIfThen( CScriptFunctionCompiler* compiler )
{
	// Conditional type
	ScriptSyntaxNodeType conditionalType;
	conditionalType.InitSimple( ::GetTypeName< Bool >() );

	// Convert expression node to boolean type
	if ( !MatchNodeType( m_children[0], compiler, conditionalType, true, false ) )
	{
		EmitError( compiler, TXT( "Conditional expression should be convertable to boolean type" ) );
		return false;
	}

	// Latency check
	if( m_children[0] && FindLatentFunctions( m_children[0] ) )
	{
		EmitError( compiler, TXT("Conditional expression must not be latent") );
		return false;
	}

	// Detect empty IF statements
	if ( !m_children[1] || m_children[1]->m_type == CNAME( SyntaxNop ) )
	{
		EmitError( compiler, TXT("Empty 'if' statements not allowed") );
		return false;
	}

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeIfThenElse( CScriptFunctionCompiler* compiler )
{
	if( CheckNodeTypeIfThen( compiler ) )
	{
		if ( !m_children[2] || m_children[2]->m_type == CNAME( SyntaxNop ) )
		{
			EmitError( compiler, TXT("Empty 'else' statements not allowed") );
			return false;
		}

		return true;
	}

	return false;
}

Bool CScriptSyntaxNode::CheckNodeTypeConditional( CScriptFunctionCompiler* compiler )
{
	// Conditional type
	ScriptSyntaxNodeType conditionalType;
	conditionalType.InitSimple( ::GetTypeName< Bool >() );

	// Convert expression node to boolean type
	if ( !MatchNodeType( m_children[0], compiler, conditionalType, true, false ) )
	{
		EmitError( compiler, TXT( "Conditional expression should be convertable to boolean type" ) );
		return false;
	}

	// Check types
	ScriptSyntaxNodeType typeA = m_children[1]->m_valueType;
	ScriptSyntaxNodeType typeB = m_children[2]->m_valueType;
	Int32 costAB = ScriptSyntaxNodeType::GetCastCost( typeA, typeB );
	Int32 costBA = ScriptSyntaxNodeType::GetCastCost( typeB, typeA );
	if ( costAB == -1 && costBA == -1 )
	{
		EmitError( compiler, TXT("Incompatible types '%ls' and '%ls'"), typeA.ToString().AsChar(), typeB.ToString().AsChar() );
		return false;
	}

	// Get better output type
	ScriptSyntaxNodeType targetType;
	if ( costAB != -1 && costAB < costBA )
	{
		// It's better to cast to second type than to first one
		targetType = typeB;
	}
	else
	{
		// It's better to cast to first type than to second one
		targetType = typeA;
	}

	// Convert both sides to that type
	if ( !MatchNodeType( m_children[1], compiler, targetType, true, false ))
	{
		return false;
	}    

	// Convert both sides to that type
	if ( !MatchNodeType( m_children[2], compiler, targetType, true, false ))
	{
		return false;
	}   

	// Set output type
	m_valueType = targetType;
	m_valueType.m_isAssignable = m_children[1]->m_valueType.m_isAssignable;
	m_valueType.m_isAssignable &= m_children[2]->m_valueType.m_isAssignable;

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeLoopWhile( CScriptFunctionCompiler* compiler )
{
	// Conditional type
	ScriptSyntaxNodeType conditionalType;
	conditionalType.InitSimple( ::GetTypeName< Bool >() );

	if ( m_children[1] )
	{
		// Convert expression node to boolean type
		if ( !MatchNodeType( m_children[1], compiler, conditionalType, true, false ) )
		{
			EmitError( compiler, TXT( "Conditional expression should be convertable to boolean type" ) );
			return false;
		}

		// Latency check
		if( FindLatentFunctions( m_children[1] ) )
		{
			EmitError( compiler, TXT( "Conditional expression must not be latent" ) );
			return false;
		}
	}

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeLoopFor( CScriptFunctionCompiler* compiler )
{
	if( CheckNodeTypeLoopWhile( compiler ) )
	{
		// Latency check
		if( m_children[0] && FindLatentFunctions( m_children[0] ) )
		{
			EmitError( compiler, TXT("Expression must not be latent") );
			return false;
		}

		// Latency check
		if( m_children[2] && FindLatentFunctions( m_children[2] ) )
		{
			EmitError( compiler, TXT("Expression must not be latent") );
			return false;
		}

		return true;
	}

	return false;
}

Bool CScriptSyntaxNode::CheckNodeTypeBreak( CScriptFunctionCompiler* compiler )
{
	if ( !m_value.m_node )
	{
		EmitError( compiler, TXT("break used outside loop") );
		return false;
	}

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeContinue( CScriptFunctionCompiler* compiler )
{
	if ( !m_value.m_node )
	{
		EmitError( compiler, TXT("continue used outside loop") );
		return false;
	}

	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeReturn( CScriptFunctionCompiler* compiler )
{
	// Not in a function
	if ( !compiler->GetCompiledFunction() )
	{
		EmitError( compiler, TXT("return used outside function") );
		return false;
	}

	// Do we have return param ?
	CFunction *function = compiler->GetCompiledFunction(); 			
	if ( function->m_returnProperty )
	{
		if( function->IsEntry() )
		{
			EmitError( compiler, TXT("Entry function '%ls' cannot return a value"), function->GetName().AsString().AsChar() );
			return false;
		}			

		if ( !m_children[0] )
		{
			EmitError( compiler, TXT("Function '%ls' should return a value"), function->GetName().AsString().AsChar() );
			return false;
		}

		if( FindLatentFunctions( m_children[0] ) )
		{
			EmitError( compiler, TXT("Function '%ls' - latent calls not allowed in return statement"), function->GetName().AsString().AsChar() );
			return false;
		}

		// Return type
		ScriptSyntaxNodeType retType;
		retType.InitSimple( function->m_returnProperty->GetType() );

		// Cast
		if ( !MatchNodeType( m_children[0], compiler, retType, false, false ))
		{
			return false;
		}
	}
	else
	{
		if ( m_children[0] && m_children[0]->m_valueType.IsVoid() == false )
		{
			EmitError( compiler, TXT("Function '%ls' can't return a value"), function->GetName().AsString().AsChar() );
			return false;
		}
	}

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeArrayElement( CScriptFunctionCompiler* compiler )
{
	// We must have context
	ASSERT( m_children[0] );
	ASSERT( m_children[1] );

	// Copy type
	const ScriptSyntaxNodeType& arrayType = m_children[ 0 ]->m_valueType;
	if ( !arrayType.IsArray() )
	{
		EmitError( compiler, TXT("Subscript requires array type") );
		return false;		
	}

	// We can index only direct properties
	if ( !arrayType.m_isFromProperty )
	{
		EmitError( compiler, TXT("Only arrays coming from variables can be indexed") );
		return false;		
	}

	// Array indexing should be int based
	ScriptSyntaxNodeType indexType;
	indexType.InitSimple( ::GetTypeName<Int32>(), false );

	// Convert indexing value to int
	if ( !MatchNodeType( m_children[1], compiler, indexType, false, false ) )
	{
		EmitError( compiler, TXT( "Array subscript should be an integer type" ) );
		return false;
	}

	// Setup element type
	CRTTIArrayType* arrayRawType = ( CRTTIArrayType* ) arrayType.m_type;
	m_value.m_type = arrayRawType;
	IRTTIType* innerType = arrayRawType->GetInnerType();
	m_valueType.InitSimple( innerType, arrayType.m_isAssignable );
	m_valueType.m_isFromProperty = true;

	// Valid
	return true;
}


Bool CScriptSyntaxNode::CheckNodeTypeSwitch( CScriptFunctionCompiler* compiler )
{
	ASSERT( m_children[0] );

	// No cases
	if ( !m_list.Size() )
	{
		EmitError( compiler, TXT("Empty switch") );
		return false;
	}

	// Latency check
	if( FindLatentFunctions( m_children[0] ) )
	{
		EmitError( compiler, TXT("Switch expression must not be latent") );
		return false;
	}

	// Convert case expressions to tested type
	Bool state = true;
	ScriptSyntaxNodeType targetType = m_children[0]->m_valueType;
	for ( Uint32 i=0; i<m_list.Size(); i++ )
	{
		// Get expression item
		String castError;
		CScriptSyntaxNode *node = m_list[i];
		if ( !MatchNodeType( node->m_children[0], compiler, targetType, false, false ) )
		{
			state = false;
		}

		// Latency check
		if( FindLatentFunctions( node->m_children[0] ) )
		{
			EmitError( compiler, TXT("Case expression must not be latent") );
			return false;
		}
	}

	// Remember expression type
	m_value.m_type = targetType.m_type;
	ASSERT( m_value.m_type );

	// Return casting state
	return state;
}

Bool CScriptSyntaxNode::CheckNodeTypeSwitchCase( CScriptFunctionCompiler* compiler )
{
	// Empty expression
	if ( !m_children[0] )
	{
		EmitError( compiler, TXT("Switch case without expression") );
		return false;
	}

	// Empty block
	if ( !m_children[1] )
	{
		EmitError( compiler, TXT("Switch case without code") );
		return false;
	}

	// Get switch context
	CScriptSyntaxNode *switchNode = m_value.m_node;
	if ( !switchNode )
	{
		EmitError( compiler, TXT("Illegal case") );
		return false;
	}

	// Add to list of cases
	switchNode->m_list.PushBack( this );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeSwitchCaseDefault( CScriptFunctionCompiler* compiler )
{
	// Empty block
	if ( !m_children[0] )
	{
		EmitError( compiler, TXT("Default switch case without code") );
		return false;
	}

	// Get switch context
	CScriptSyntaxNode *switchNode = m_value.m_node;
	if ( !switchNode )
	{
		EmitError( compiler, TXT("Illegal default") );
		return false;
	}

	// Only one default case is allowed
	if ( switchNode->m_defaultSwitch )
	{
		EmitError( compiler, TXT("More than one default") );
		return false;
	}

	// Set as default case    
	switchNode->m_defaultSwitch = this;
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeCallFunction( CScriptFunctionCompiler* compiler )
{
	CFunction* function = NULL;

	// Handle special built-in functions
	if ( m_children[0]->m_children[0] )
	{
		const ScriptSyntaxNodeType& type = m_children[0]->m_children[0]->m_valueType;
		if ( type.IsArray() )
		{
			// Copy the function signature
			m_value.m_string = m_children[0]->m_value.m_string;
			m_children[0] = m_children[0]->m_children[0];

			// Handle as dynamic array function
			return HandleDynamicArrayFunction( compiler );
		}
	}

	// We are pointing to struct, handle constructor call
	CClass* structure = m_children[0]->m_value.m_structure;
	if ( structure )
	{
		return HandleStructureConstructor( structure, compiler );
	}

	// Make sure what we try to call is a function :)
	function = m_children[0]->m_value.m_function;
	if ( !function )
	{
		EmitError( compiler, TXT("'%ls' is not a function"), m_children[0]->m_valueType.ToString().AsChar() );
		return false;
	}

	// Global Functions must always be defined if they're not imported from C++
	// Class methods have their own checks that take into account inheritance
	if( function->HasUndefinedBody() && !function->IsNative() && !function->GetClass() )
	{
		EmitError( compiler, TXT( "Cannot call function %" ) RED_PRIWs TXT( "() -> it has no body!" ), function->GetName().AsChar() );
		return false;
	}

	// Extract the function call context by removing the function ident node
	CScriptSyntaxNode* functionIdentNode = m_children[0];
	CScriptSyntaxNode* callContet = functionIdentNode->m_children[0];
	m_children[0] = callContet;
	delete functionIdentNode;

	return CheckNodeTypeCallCommon( compiler, function );
}

Bool CScriptSyntaxNode::CheckNodeTypeCallOperator( CScriptFunctionCompiler* compiler )
{
	const CFunction *function = NULL;

	// Get type from left side expression
	ScriptSyntaxNodeType LType = m_children[0]->m_valueType;

	// Get the type from right side expression ( not set for unary operators )
	ScriptSyntaxNodeType RType;
	if ( m_children[1] )
	{
		RType = m_children[1]->m_valueType;
	}

	// Find best function
	function = FindBestOperator( m_operator, LType, RType );

	// No operator found, error
	if ( !function )
	{
		// Special case for equality and inequality
		const Bool isObjectTypeL = LType.m_type && LType.m_type->GetType() == RT_Handle;
		const Bool isObjectTypeR = RType.m_type && RType.m_type->GetType() == RT_Handle;
		if ( RType.m_type && ( ( LType.m_type == RType.m_type ) || ( isObjectTypeL && isObjectTypeR ) ) )
		{
			// Equality test
			if ( m_operator == CNAME( OperatorEqual ) )
			{
				m_type = CNAME( SyntaxTypedEqual );
				m_value.m_type = LType.m_type;
				m_valueType.InitSimple( ::GetTypeName< Bool >(), false );
				return true;
			}

			// Inequality test
			if ( m_operator == CNAME( OperatorNotEqual ) )
			{
				m_type = CNAME( SyntaxTypedNotEqual );
				m_value.m_type = LType.m_type;
				m_valueType.InitSimple( ::GetTypeName< Bool >(), false );
				return true;
			}
		}

		EmitError( compiler, TXT("Unable to find suitable operator '%ls' for given types (%ls, %ls)"), m_operator.AsString().AsChar(), LType.ToString().AsChar(), RType.ToString().AsChar() );
		return false;
	}

	// Push left side operator expression as first function param
	m_list.PushBack( m_children[0] );

	// Second param (if given)
	if ( m_children[1] )
	{
		m_list.PushBack( m_children[1] );
	}

	// Reset child nodes so we can mimic ordinal function calls
	m_children[0] = NULL;
	m_children[1] = NULL;

	// Change to function call, from now on, operator call is indistinguishable 
	m_type = CNAME( SyntaxFuncCall );

	return CheckNodeTypeCallCommon( compiler, function );
}

Bool CScriptSyntaxNode::CheckNodeTypeCallCommon( CScriptFunctionCompiler* compiler, const CFunction* function )
{
	// This is a log function, do not emit logging code in final builds
	if( GScriptingSystem->IsFinalRelease()  && 
		(	function->GetName() == TXT("Log") || 
			function->GetName() == TXT("LogChannel")
		)
	)
	{
		m_type = CNAME( SyntaxNop );
		m_children[0] = NULL;
		m_children[1] = NULL;
		m_list.ClearPtr();
		return true;
	}

	// We can call latent functions only from state entry functions
	if ( function->IsLatent() )
	{
		CFunction* inFunction = compiler->GetCompiledFunction();
		if ( !inFunction || ( !inFunction->IsEntry() && !inFunction->IsLatent() ) )
		{
			EmitError( compiler, TXT("Latent function '%ls' can be called only from inside of state entry or latent function"), function->GetName().AsString().AsChar() );
			return false;
		}
	}

	// Functions supporting the SavePoints can't be called from any other function
	if ( HasSavepointsSupport( function ) )
	{
		CFunction* inFunction = compiler->GetCompiledFunction();
		if ( inFunction )
		{
			EmitError( compiler, TXT("Quest function '%ls' can only be called from as ScriptQuestBlock - it can't be called from any other function "), function->GetName().AsString().AsChar() );
			return false;
		}
	}

	// We cannot call entry functions directly
/*		if ( function->IsEntry() )
	{
		EmitError( compiler, TXT("State entry function '%ls' should be called via the 'enter' statement"), m_children[0]->m_valueType.ToString().AsChar() );
		return false;
	}
*/

    // Remember function object
	m_value.m_function = const_cast< CFunction* >( function );
     
	// To many params specified
	const Uint32 numParameters = static_cast< Uint32 >( function->GetNumParameters() );
	if ( m_list.Size() > numParameters )
	{
		EmitError( compiler, TXT("Function '%ls' does not take %i param(s)"), function->GetName().AsString().AsChar(), m_list.Size() );
		return false;
	}      
     
	// Check types
	for ( Uint32 i=0; i<numParameters; i++ )
	{
		CProperty *param = function->GetParameter(i);
		const Bool isAssignable = 0 != ( param->GetFlags() & PF_FuncOutParam );
		const Bool isOptional = 0 != ( param->GetFlags() & PF_FuncOptionaParam );

		// Setup type
		ScriptSyntaxNodeType paramType;
		paramType.InitSimple( param->GetType(), isAssignable );
   
		// To few params ?
		if ( i >= m_list.Size() )
		{
			if ( !isOptional )
			{
				EmitError( compiler, TXT("To few params in call to function '%ls'"), function->GetName().AsString().AsChar() );
				return false;
			}
			else
			{
				continue;
			}
		}      
     
		// Not specified ? 
		if ( !m_list[i] && !isOptional )
		{
			EmitError( compiler, TXT("Obligatory param %i (%ls) was omitted"), i, param->GetName().AsString().AsChar() );
			return false;
		}
      
		// Check only existing params
		if ( i < m_list.Size() && m_list[i] )
		{
			if ( !MatchNodeType( m_list[i], compiler, paramType, false, true ) )
			{
				EmitError( compiler, TXT( "With parameter '%ls'" ), param->GetName().AsString().AsChar() );
				return false;
			}
		}
	}

    // Set return type
	CProperty* retProperty = function->GetReturnValue();
	if ( retProperty )
	{
		// Set value
		const Bool isAssignable = false;
		m_valueType.InitSimple( retProperty->GetType(), isAssignable );
	}
	else if ( function->IsEntry() )
	{
		// Entry function returns bool
		const Bool isAssignable = false;
		m_valueType.InitSimple( ::GetTypeName<Bool>(), isAssignable );
	}

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeNew( CScriptFunctionCompiler* compiler )
{
	// Find class
	CName className( m_value.m_string );
	CClass *classObject = SRTTI::GetInstance().FindClass( className );
	if ( !classObject )
	{
		EmitError( compiler, TXT("Class '%ls' is unknown"), m_value.m_string.AsChar() );
		return false;
	}

	// It should be an object class
	if ( !classObject->IsScriptable() )
	{
		EmitError( compiler, TXT("Class '%ls' is not a scriptable class. Only scriptable classes can be created in scripts."), m_value.m_string.AsChar() );
		return false;
	}

	// It cannot be an abstract class
	if ( classObject->IsAbstract() )
	{
		EmitError( compiler, TXT( "Cannot create instance of class '%ls'" ), m_value.m_string.AsChar() );

		CScriptSystemStub& definitions = compiler->GetStubs();

		for( Uint32 iClass = 0; iClass < definitions.m_classes.Size(); ++iClass )
		{
			CScriptClassStub* classStub = definitions.m_classes[ iClass ];
			if( classObject->GetName() == CName( classStub->m_name ) )
			{
				compiler->GetScriptCompiler()->ScriptError( classStub->m_context, TXT( " - Class '%ls' is abstract" ), m_value.m_string.AsChar() );
				break;
			}
		}

		return false;
	}

	// Class was not exported
	if ( classObject->IsNative() && !classObject->IsExported() )
	{
		EmitError( compiler, TXT("Class '%ls' was not exported from C++ code. Cannot create instance of it."), m_value.m_string.AsChar() );
		return false;
	}

	// Legacy CObject support and "new in"
	if ( classObject->IsObject() )
	{
		// Empty "new"s are not supported
		if ( !m_children[0] )
		{
			EmitError( compiler, TXT("Objects created using 'new' should be created in the context of a parent object.") );
			return false;
		}

		// Make sure right hand side casts to an object
		{
			ScriptSyntaxNodeType castToType;
			castToType.InitPointer( ClassID< IScriptable >(), false );
			if ( !MatchNodeType( m_children[0], compiler, castToType, true, false ) )
			{
				return false;
			}
		}
	}

	// Seems ok
	m_value.m_type = classObject;
	m_valueType.InitPointer( classObject, false );
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeDelete( CScriptFunctionCompiler* compiler )
{
	// Match param
	if ( !m_children[0]->m_valueType.IsPointer() )
	{
		EmitError( compiler, TXT("Delete can by used only on objects") );
		return false;
	}

	// All ok
	return true;
}

// What is the purpose of this node type? It appears to be unused
Bool CScriptSyntaxNode::CheckNodeTypeEnter( CScriptFunctionCompiler* compiler )
{
	// Match param
	if ( m_children[0]->m_type != CNAME( SyntaxFuncCall )  )
	{
		EmitError( compiler, TXT("Delete can by used only on objects") );
		return false;
	}

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeCast( CScriptFunctionCompiler* compiler )
{
	// Resolve type
	IRTTIType* type = SRTTI::GetInstance().FindType( CName( m_value.m_string ) );
	if ( !type )
	{
		EmitError( compiler, TXT("Unknown type '%ls' used in casting"), m_value.m_string.AsChar() );
		return false;
	}

	// Cast
	ScriptSyntaxNodeType castToType;
	castToType.InitSimple( type, false );
	if ( !MatchNodeType( m_children[0], compiler, castToType, true, false ) )
	{
		return false;
	}

	// Keep the casted type
	m_valueType = castToType;

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeSavePoint( CScriptFunctionCompiler* compiler )
{
	// check that the save point name is setup
	if ( m_value.m_string.Empty() )
	{
		EmitError( compiler, TXT("SavePoint name is not set.") );
		return false;
	}

	// verify that the SavePoint is called from a function
	CFunction *function = compiler->GetCompiledFunction(); 
	if ( !function )
	{
		EmitError( compiler, TXT("SavePoints need to be placed inside functions") );
		return false;
	}

	// validate that the function supports the SavePoints mechanism
	if ( !HasSavepointsSupport( function ) )
	{
		EmitError( compiler, TXT("Function '%ls' doesn't support SavePoints"), function->GetName().AsString().AsChar() );
		return false;
	}

	// validate the list of parameters we intend to save
	for ( auto it = m_list.Begin(); it != m_list.End(); ++it )
	{
		CScriptSyntaxNode* node = *it;

		// one can't specify an empty parameter.
		//		example: savepoint('label', paramA, , paramB )
		//			3rd param is empty in this case, and it's unacceptable
		if ( !node || node->m_value.m_string.Empty() )
		{
			EmitError( compiler, TXT("SavePoint doesn't accept null parameters") );
			return false;
		}

		// we only support saving of parameters local to a given function
		// or to those specified in its param list
		CProperty *param = function->FindProperty( CName( node->m_value.m_string ) );
		if ( !param || !( param->IsFuncLocal() || param->IsInFunction() ) )
		{
			EmitError( compiler, TXT("Parameter '%ls' can't be saved by the SavePoint - it's not function local."), node->m_value.m_string.AsChar() );
			return false;
		}

		// check that the parameter has a type specified - otherwise it's a major engine issue
		IRTTIType* paramType = param->GetType();
		if ( !paramType )
		{
			EmitError( compiler, TXT("SavePoint parameter '%ls' type is not defined"), node->m_value.m_string.AsChar() );
			return false;
		}

		// check that the property type is supported. It can be either:
		//		- a simple type
		//		- an enum
		//		- an array of simple types
		Bool doesPropertyHaveValidType = true;
		IRTTIType* checkedType = paramType;
		while( checkedType && doesPropertyHaveValidType )
		{
			doesPropertyHaveValidType = ( paramType->GetType() == RT_Simple 
				|| paramType->GetType() == RT_Array
				|| paramType->GetType() == RT_Enum
				|| paramType->GetType() == RT_Fundamental );

			if ( paramType->GetType() == RT_Array )
			{
				CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( paramType );
				checkedType = arrayType->GetInnerType();
			}
			else
			{
				checkedType = NULL;
			}
		}
		if ( !doesPropertyHaveValidType )
		{
			EmitError( compiler, TXT("SavePoint can only save: simple data types, enums and arrays of simple types - '%ls' is not one of them"), node->m_value.m_string.AsChar() );
			return false;
		}
	}

	// All ok
	return true;
}

Bool CScriptSyntaxNode::CheckNodeTypeGlobalKeyword( CScriptFunctionCompiler* compiler )
{
	SGlobalKeyword& keyword = m_globalKeywords[ m_type ];

	// TODO: Don't keep className string directly in code
	CClass* scopeClass = compiler->GetStubs().FindClass( keyword.className );
	ASSERT( scopeClass, TXT( "Could not find class '%ls' while syntax node checking" ), keyword.className.AsChar() );

	if ( m_children[ 0 ] )
	{
		EmitError( compiler, TXT( "'%ls' cannot have child nodes" ), keyword.scriptKeyword.AsChar() );
		return false;
	}

	m_valueType.InitPointer( scopeClass, false );
	m_valueType.m_isFromProperty = true;

	return true;
}


