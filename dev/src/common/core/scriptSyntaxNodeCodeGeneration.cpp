/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptCodeNode.h"
#include "scriptingSystem.h"
#include "scriptCodeNodeCompilationPool.h"


#define MAKE_CODE( x ) pool.Create( m_context, x )

#define MAKE_CAST( x )																\
	if ( m_type == CNAME( Syntax##x ) )												\
	{																				\
		node = MAKE_CODE( OP_##x );													\
		return CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );	\
	}

CScriptCodeNode* CScriptSyntaxNode::GenerateCode( CScriptCodeNodeCompilationPool& pool )
{
	CScriptCodeNode* node = NULL;

	// Empty opcode
	if ( m_type == CNAME( SyntaxNop ) )
	{
		node = MAKE_CODE( OP_Nop );
		return node;
	}

	// Breakpoint wrapper
	if ( m_type == CNAME( SyntaxBreakpoint ) )
	{
		if ( GScriptingSystem->IsFinalRelease() )
		{
			// Generate glue code
			if ( m_children[0] )
			{
				return m_children[0]->GenerateCode( pool );
			}
			else
			{
				return MAKE_CODE( OP_Nop );
			}
		}
		else
		{
			// Generate glue code
			node = MAKE_CODE( OP_Breakpoint );
			if ( m_children[0] )
			{
				CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
			}
			else
			{
				CScriptCodeNode* nop = MAKE_CODE( OP_Nop );
				CScriptCodeNode::Glue( node, nop );
			}
		}

		return node;
	}

	// Null constant
	if ( m_type == CNAME( SyntaxNullConst ) )
	{
		return MAKE_CODE( OP_Null );
	}

	// Code block
	if ( m_type == CNAME( SyntaxCode ) )
	{
		// Process list
		for ( Uint32 i=0; i<m_list.Size(); i++ )
		{
			// Empty node (can happen)
			if ( !m_list[i] )
			{
				continue;
			}

			// Skip NOPs
			if ( m_list[i]->m_type == CNAME( SyntaxNop ) )
			{
				continue;
			}

			// Get code
			CScriptCodeNode* elem = m_list[i]->GenerateCode( pool );
			node = CScriptCodeNode::Glue( node, elem );
		}

		// Return generated code
		return node;
	}

	// Integer constant
	if ( m_type == CNAME( SyntaxIntConst ) )
	{
		if ( m_value.m_integer == 0 )
		{
			return MAKE_CODE( OP_IntZero );
		}
		else if ( m_value.m_integer == 1 )
		{
			return MAKE_CODE( OP_IntOne );
		}
		else
		{
			node = MAKE_CODE( OP_IntConst );
			node->m_value.m_integer = m_value.m_integer;
			return node;
		}
	}

	// Float constant
	if ( m_type == CNAME( SyntaxFloatConst ) )
	{
		node = MAKE_CODE( OP_FloatConst );
		node->m_value.m_float = m_value.m_float;
		return node;
	}

	// Float constant
	if ( m_type == CNAME( SyntaxBoolConst ) )
	{
		if ( m_value.m_bool )
		{
			return MAKE_CODE( OP_BoolTrue );
		}
		else
		{
			return MAKE_CODE( OP_BoolFalse );
		}
	}

	// String constant
	if ( m_type == CNAME( SyntaxStringConst ) )
	{
		node = MAKE_CODE( OP_StringConst );
		node->m_value.m_string = m_value.m_string;
		return node;	
	}

	// Name constant
	if ( m_type == CNAME( SyntaxNameConst ) )
	{
		node = MAKE_CODE( OP_NameConst );
		node->m_value.m_string = m_value.m_string;
		return node;	
	}

	// Name constant
	if ( m_type == CNAME( SyntaxEnumConst ) )
	{
		if ( m_value.m_dword == 1 )
		{
			node = MAKE_CODE( OP_ByteConst );
		}
		else if ( m_value.m_dword == 2 )
		{
			node = MAKE_CODE( OP_ShortConst );
		}
		else if ( m_value.m_dword == 4 )
		{
			node = MAKE_CODE( OP_IntConst );
		}

		node->m_value.m_integer = m_value.m_integer;
		return node;	
	}

	// Assignment
	if ( m_type == CNAME( SyntaxAssign ) )
	{
		// Evaluate l-param
		CScriptCodeNode *left = m_children[0]->GenerateCode( pool );
		ASSERT( left );

		// Evaluate r-param
		CScriptCodeNode *right = m_children[1]->GenerateCode( pool );
		ASSERT( right );

		// We should have known type during the assign
		ASSERT( m_valueType.m_type );

		// Create node
		node = MAKE_CODE( OP_Assign );
		node->m_value.m_type = m_valueType.m_type;
		node->m_value.m_dword = m_valueType.m_type->GetSize();

		// Glue left and right assignment sides
		CScriptCodeNode::Glue( node, left );
		CScriptCodeNode::Glue( node, right );
		return node;
	}

	// Parent scope of a state
	if ( m_type == CNAME( SyntaxScopeParent ) || m_type == CNAME( SyntaxScopeVirtualParent ) )
	{
		// Parent access node
		node = MAKE_CODE( OP_Parent );

		// Compile the context
		if ( m_children[0] )
		{
			// Use given context
			CScriptCodeNode* context = m_children[0]->GenerateCode( pool );
			node = CScriptCodeNode::Glue( node, context );
		}
		else
		{
			// Use this object as context
			CScriptCodeNode* context = MAKE_CODE( OP_This );
			node = CScriptCodeNode::Glue( node, context );
		}

		return node;
	}

	// Call to a function or operator
	if ( m_type == CNAME( SyntaxFuncCall ) )
	{
		CScriptCodeNode* postFunctionCall = MAKE_CODE( OP_Target );

		// Determine if we should bind to function statically
		Bool staticBind = false;
		Bool generateScope = true;
		CFunction* functionToCall = m_value.m_function;
		if ( functionToCall->m_flags & FF_OperatorFunction || functionToCall->m_flags & FF_StaticFunction )
		{
			// This is a call to an operator or a global function or a static function,
			// this type of calls directly call specified function so we can use static binding.
			generateScope = false;
			staticBind = true;
		}
		else if ( functionToCall->m_flags & FF_EntryFunction )
		{
			// This is a state entry function, bind it statically
			staticBind = true;
		}
		else if ( m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeSuper ) )
		{
			// We are calling a function from base class, use static binding
			generateScope = false;
			staticBind = true;
		}
		else if ( m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeParent ) )
		{
			// We are calling a function from the state machine, use static binding, prevents recursion
			staticBind = true;
		}
		else if ( functionToCall->m_flags & FF_FinalFunction )
		{
			// This is a final function, bind it statically to save on a virtual call and a FindFuction call			
			staticBind = true;
		}

		// Resolve function call context
		CScriptCodeNode *context = NULL;
		if ( generateScope && m_children[0] )
		{
			// Call function in context
			context = MAKE_CODE( OP_Context );
			context->m_label = postFunctionCall;
			context->m_value.m_integer = 0; // Do not clear result value
	
			// Glue the context
			CScriptCodeNode::Glue( context, m_children[0]->GenerateCode( pool ) );
		}

		// Get function
		const Bool isEntry = 0 != ( functionToCall->m_flags & FF_EntryFunction );
		const Bool isVirtualParent = ( m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeVirtualParent ) );
    
		// Create function node  
		CScriptCodeNode *header = NULL;
		if ( staticBind )
		{
			if ( isEntry )
			{
				header = MAKE_CODE( OP_EntryFunc );
				header->m_label = postFunctionCall;
			}
			else
			{
				header = MAKE_CODE( OP_FinalFunc );
				header->m_label = postFunctionCall;
			}
		}
		else if ( isVirtualParent )
		{
			header = MAKE_CODE( OP_VirtualParentFunc );
			header->m_label = postFunctionCall;
		}
		else
		{
			header = MAKE_CODE( OP_VirtualFunc );
			header->m_label = postFunctionCall;
		}
    
	    // Setup function   
		header->m_value.m_function = functionToCall;
  
		// Create code for params
		const Uint32 numParameters = functionToCall->m_parameters.Size();
		for ( Uint32 i=0; i<numParameters; i++ )
		{
			CScriptCodeNode *param = NULL;

			// Generate code for function param
			if ( i < m_list.Size() && m_list[i] )
			{
				param = m_list[ i ]->GenerateCode( pool );
			}

			// No param code generated, emit NOP
			if ( !param )
			{
				param = MAKE_CODE( OP_Nop );
			}
      
			// Add skip jump for skippable parameters
			CProperty* prop = functionToCall->m_parameters[i];
			if ( prop->GetFlags() & PF_FuncSkipParam )
			{
				CScriptCodeNode *skipStart = MAKE_CODE( OP_Skip );
				CScriptCodeNode *skipEnd = MAKE_CODE( OP_Target );
				skipStart->m_label = skipEnd;     
				param = CScriptCodeNode::Glue( skipStart, param );
				param = CScriptCodeNode::Glue( param, skipEnd );
			}
     
			// Merge header with parameter
			header = CScriptCodeNode::Glue( header, param );
		}     
   
		// End marker
		CScriptCodeNode* tail = MAKE_CODE( OP_ParamEnd );
		header = CScriptCodeNode::Glue( header, tail );
    
		// Glue with scope
		node = CScriptCodeNode::Glue( context, header );
		node = CScriptCodeNode::Glue( node, postFunctionCall );
		return node;			
	}

	// Return from function
	if ( m_type == CNAME( SyntaxReturn ) )
	{
	   // Header
	   CScriptCodeNode* node = MAKE_CODE( OP_Return );

	   // Return expression
	   if ( m_children[0] )
	   {
		   CScriptCodeNode* val = m_children[0]->GenerateCode( pool );
		   return CScriptCodeNode::Glue( node, val );
	   }
	   else
	   {
		   CScriptCodeNode* val = MAKE_CODE( OP_Nop );
		   return CScriptCodeNode::Glue( node, val );
	   }
	}
	
	// Identifier
	if ( m_type == CNAME( SyntaxIdent ) )
	{
		// Get the accessed property
		CProperty* prop = m_value.m_property;
		ASSERT( prop );

		// Structure access
		CClass* parentClass = prop->GetParent();

		// Are we in the scope of a function
		CScriptSyntaxNode* scopeNode = NULL;
		if ( parentClass && !parentClass->IsScriptable() )
		{
			node = MAKE_CODE( OP_StructMember );
			scopeNode = m_children[0];
		}
		else if ( prop->GetFlags() & PF_FuncLocal )
		{
			node = MAKE_CODE( OP_LocalVar );
			scopeNode = NULL;
		}
		else if ( prop->GetFlags() & PF_FuncParam )
		{
			node = MAKE_CODE( OP_ParamVar );
			scopeNode = NULL;
		}
		else if ( m_children[0] && m_children[0]->m_type == CNAME( SyntaxScopeDefault ) )
		{
			node = MAKE_CODE( OP_DefaultVar );
			scopeNode = m_children[0]->m_children[0];
		}
		else if ( parentClass )
		{
			if ( prop->IsAutoBindable() )
			{
				node = MAKE_CODE( OP_ObjectBindableVar );
				scopeNode = m_children[0];
			}
			else
			{
				node = MAKE_CODE( OP_ObjectVar );
				scopeNode = m_children[0];
			}
		}
		else
		{
			HALT( "Invalid type of Ident" );
			return NULL;
		}

		// Bind to property type
		node->m_value.m_property = prop;
		node->m_value.m_type = prop->GetType();
		ASSERT( node->m_value.m_type );

		// Create the scope node
		if ( scopeNode )
		{
			ASSERT( parentClass );

			// Scope code
			CScriptCodeNode* scopeCode = scopeNode->GenerateCode( pool );
			if ( scopeCode )
			{
				// We are in context of object
				if (  parentClass->IsScriptable() )
				{
					// Create context node
					CScriptCodeNode* context = MAKE_CODE( OP_Context );
					context = CScriptCodeNode::Glue( context, scopeCode );

					// Skip label called when dealing with invalid code
					CScriptCodeNode* target = MAKE_CODE( OP_Target );
	
					// Glue the context BEFORE the actual var, that's the whole trick :)
					node = CScriptCodeNode::Glue( context, node );
					node = CScriptCodeNode::Glue( node, target );

					// Remember when to jump when context will be invalid
					node->m_label = target;

					// Remember size of the data we tried to get
					node->m_value.m_integer = prop->GetType()->GetSize();
				}
				else
				{
					// Glue the structure member access AFTER the context
					node = CScriptCodeNode::Glue( node, scopeCode );
				}
			}
		}

		// Return the variable access node
		return node;
	}   

	// Structure constructor
	if ( m_type == CNAME( SyntaxConstructor ) )
	{
		// Create header
		node = MAKE_CODE( OP_Constructor );

		// Dump params
		ASSERT( m_value.m_type );
		ASSERT( m_value.m_type->GetType() == RT_Class );
		node->m_value.m_structure = static_cast< CClass* >( m_value.m_type );

		// Save number of parameters
		const Uint32 numParameters = m_list.Size();
		node->m_value.m_integer = numParameters;

		// Process parameters list
		for ( Uint32 i=0; i<numParameters; i++ )
		{
			if ( m_list[i] )
			{
				// Evaluate param
				CScriptCodeNode* param = m_list[i]->GenerateCode( pool );
				CScriptCodeNode::Glue( node, param );
			}
			else
			{
				// Parameter not given, insert NOP
				CScriptCodeNode* param = MAKE_CODE( OP_Nop );
				CScriptCodeNode::Glue( node, param );
			}
		}
  
		// Return node
		return node;
	}

	// Casting to defined type
	if ( m_type == CNAME( SyntaxDynamicCast ) )
	{
		// Resolve type
		ASSERT( m_value.m_type );
		ASSERT( m_value.m_type->GetType() == RT_Class );

		// Create node
		node = MAKE_CODE( OP_DynamicCast );
		node->m_value.m_type = m_value.m_type;

		// Glue to the casted type
		CScriptCodeNode* castedExpression = m_children[0]->GenerateCode( pool );
		CScriptCodeNode::Glue( node, castedExpression );

		// Done
		return node;
	}

	// Explicit casting node
	if ( m_type == CNAME( SyntaxCast ) )
	{
		ASSERT( m_children[0] );
		return m_children[0]->GenerateCode( pool );
	}

	// Conditional expression
	if ( m_type == CNAME( SyntaxConditional ) )
	{
		// Conditional expression:
		//  if (!expression) jmp @skip
		//   expr0
		//   jmp @end
		//  @skip:
		//   expr1
		//  @end:
    
	    // Create nodes
		CScriptCodeNode* header = MAKE_CODE( OP_Conditional );
		CScriptCodeNode* expression = m_children[0]->GenerateCode( pool );
		CScriptCodeNode* optionA = m_children[1]->GenerateCode( pool );
		CScriptCodeNode* selectLabel = MAKE_CODE( OP_Target );
		CScriptCodeNode* optionB = m_children[2]->GenerateCode( pool );
		CScriptCodeNode* skipLabel = MAKE_CODE( OP_Target );
    
		// Assemble code
		CScriptCodeNode::Glue( node, header );
		CScriptCodeNode::Glue( node, expression );
		CScriptCodeNode::Glue( node, optionA );
		CScriptCodeNode::Glue( node, selectLabel );
		CScriptCodeNode::Glue( node, optionB );
		CScriptCodeNode::Glue( node, skipLabel );
    
		// Setup labels
		header->m_label = selectLabel;
		header->m_skipLabel = skipLabel;    
    
		// Return created code
		return node;
	}   

	// Look break
	if ( m_type == CNAME( SyntaxBreak ) )
	{
	    node = MAKE_CODE( OP_Jump );
		ASSERT( m_value.m_node->m_breakLabel );
		node->m_label = m_value.m_node->m_breakLabel;
		return node;   
	}

	// Loop continue
	if ( m_type == CNAME( SyntaxContinue ) )
	{
		node = MAKE_CODE( OP_Jump );
		ASSERT( m_value.m_node->m_continueLabel );
		node->m_label = m_value.m_node->m_continueLabel;
		return node;   
	}
   
	// Simple conditional statement
	if ( m_type == CNAME( SyntaxIfThen ) )
	{
	    CScriptCodeNode *cond, *block, *label;
    
		// Block    
		node = MAKE_CODE( OP_JumpIfFalse );
		cond = m_children[0]->GenerateCode( pool );
		block = m_children[1]->GenerateCode( pool );
		label = MAKE_CODE( OP_Target );
		node->m_label = label;
    
		// Assemble code
		CScriptCodeNode::Glue( node, cond );
		CScriptCodeNode::Glue( node, block );
		CScriptCodeNode::Glue( node, label );    
		return node;
	}   
   
	// Conditional statement
	if ( m_type == CNAME( SyntaxIfThenElse ) )
	{
		CScriptCodeNode *cond, *block, *label, *lastLabel, *jump, *elseBlock;
    
		node = MAKE_CODE( OP_JumpIfFalse );
		cond = m_children[0]->GenerateCode( pool ); // Condition
		block = m_children[1]->GenerateCode( pool ); // TRUE part    
		jump = MAKE_CODE( OP_Jump );
		label = MAKE_CODE( OP_Target );
		elseBlock = m_children[2]->GenerateCode( pool ); // FALSE part    
		lastLabel = MAKE_CODE( OP_Target );    
		node->m_label = label;
		jump->m_label = lastLabel;
    
		// Assemble code
		CScriptCodeNode::Glue( node, cond );
		CScriptCodeNode::Glue( node, block );
		CScriptCodeNode::Glue( node, jump );
		CScriptCodeNode::Glue( node, label );
		CScriptCodeNode::Glue( node, elseBlock );
		CScriptCodeNode::Glue( node, lastLabel );
		return node;
	}   

	// Generic typed equality test
	if ( m_type == CNAME( SyntaxTypedEqual ) ) 
	{
		node = MAKE_CODE( OP_TestEqual );
		node->m_value.m_type = m_value.m_type;
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		CScriptCodeNode::Glue( node, m_children[1]->GenerateCode( pool ) );
		return node;
	}

	// Generic typed inequality test
	if ( m_type == CNAME( SyntaxTypedNotEqual ) ) 
	{
		node = MAKE_CODE( OP_TestNotEqual );
		node->m_value.m_type = m_value.m_type;
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		CScriptCodeNode::Glue( node, m_children[1]->GenerateCode( pool ) );
		return node;
	}

	// For loop
	if ( m_type == CNAME( SyntaxFor ) ) 
	{
		CScriptCodeNode *initBlock, *condBlock, *incrBlock, *stmtBlock;
		CScriptCodeNode *loopLabel, *loopJump, *conditionalJump;    
    
		// InitBlock
		// @Loop:
		//  if (!CondBlock) jump @BreakLabel;
		//  StmtBlock
		//  @ContinueLabel:
		//  IncrBlock
		// jump @Loop
		// @BreakLabel:    
        
	    // Continue/Break labels
	    m_continueLabel = MAKE_CODE( OP_Target );
		m_breakLabel = MAKE_CODE( OP_Target );    
    
		// Generate code for loop parts
		initBlock = m_children[0] ? m_children[0]->GenerateCode( pool ) : NULL;
		condBlock = m_children[1] ? m_children[1]->GenerateCode( pool ) : NULL;
		incrBlock = m_children[2] ? m_children[2]->GenerateCode( pool ) : NULL;
		stmtBlock = m_children[3] ? m_children[3]->GenerateCode( pool ) : NULL;
    
		// Main loop parts
		loopLabel = MAKE_CODE( OP_Target );
		loopJump = MAKE_CODE( OP_Jump );
		loopJump->m_label = loopLabel;
    
		// Exit condition
		if ( condBlock )
		{
			conditionalJump = MAKE_CODE( OP_JumpIfFalse );
			conditionalJump->m_label = m_breakLabel;
		}
		else
		{
			conditionalJump = NULL;
		}
    
		// Merge blocks
		node = initBlock;
		CScriptCodeNode::Glue( node, loopLabel );
		CScriptCodeNode::Glue( node, conditionalJump );
		CScriptCodeNode::Glue( node, condBlock );
		CScriptCodeNode::Glue( node, stmtBlock );
		CScriptCodeNode::Glue( node, m_continueLabel );
		CScriptCodeNode::Glue( node, incrBlock );
		CScriptCodeNode::Glue( node, loopJump );
		CScriptCodeNode::Glue( node, m_breakLabel );
		return node;
	}       
   
	// While loop
	if ( m_type == CNAME( SyntaxWhile ) )
	{
	    CScriptCodeNode *condBlock, *stmtBlock;
		CScriptCodeNode *loopLabel, *loopJump, *conditionalJump;
    
		// @Loop:
		//  if (!CondBlock) jump @BreakLabel;
		//  StmtBlock
		//  @ContinueLabel:
		// jump @Loop
		// @BreakLabel:          
    
		// Continue/Break labels
		m_continueLabel = MAKE_CODE( OP_Target );
		m_breakLabel = MAKE_CODE( OP_Target );    
    
	    // Generate code for loop parts
		condBlock = m_children[1] ? m_children[1]->GenerateCode( pool ) : NULL;
		stmtBlock = m_children[3] ? m_children[3]->GenerateCode( pool ) : NULL;
    
		// Main loop parts    
		loopLabel = MAKE_CODE( OP_Target );
		loopJump = MAKE_CODE( OP_Jump );
		conditionalJump = MAKE_CODE( OP_JumpIfFalse );
		loopJump->m_label = loopLabel;
		conditionalJump->m_label = m_breakLabel;   
     
		// Glue everything    
		node = loopLabel;
		CScriptCodeNode::Glue( node, conditionalJump );
		CScriptCodeNode::Glue( node, condBlock );
		CScriptCodeNode::Glue( node, stmtBlock );
		CScriptCodeNode::Glue( node, m_continueLabel );
		CScriptCodeNode::Glue( node, loopJump );
		CScriptCodeNode::Glue( node, m_breakLabel );
		return node;
	}
       
	// Do while loop       
	if ( m_type == CNAME( SyntaxDoWhile ) )
	{
		CScriptCodeNode *condBlock, *stmtBlock;
		CScriptCodeNode *loopLabel, *loopJump, *conditionalJump;
    
		// @Loop:
		//  StmtBlock
		//  @ContinueLabel:
		//  if (!CondBlock) jump @BreakLabel;
		// jump @Loop
		// @BreakLabel:    
    
		// Continue/Break labels
		m_continueLabel = MAKE_CODE( OP_Target );
		m_breakLabel = MAKE_CODE( OP_Target );    

		// Generate code for loop parts
		condBlock = m_children[1] ? m_children[1]->GenerateCode( pool ) : NULL;
		stmtBlock = m_children[3] ? m_children[3]->GenerateCode( pool ) : NULL;

		// Main loop parts    
		loopLabel = MAKE_CODE( OP_Target );
		loopJump = MAKE_CODE( OP_Jump );
		conditionalJump = MAKE_CODE( OP_JumpIfFalse );
		loopJump->m_label = loopLabel;
		conditionalJump->m_label = m_breakLabel;
   
		// Glue everything    
		node = loopLabel;
		CScriptCodeNode::Glue( node, stmtBlock );
		CScriptCodeNode::Glue( node, m_continueLabel );
		CScriptCodeNode::Glue( node, conditionalJump );
		CScriptCodeNode::Glue( node, condBlock );
		CScriptCodeNode::Glue( node, loopJump );
		CScriptCodeNode::Glue( node, m_breakLabel );
		return node;
	}    

	// Switch statement
	if ( m_type == CNAME( SyntaxSwitch ) )
	{
		CScriptCodeNode	*switchExpr, *switchBlock;

		// Reset
		m_switchLabels.Clear();
		m_switchDefaultLabel = NULL;
		
		// Continue/Break labels
		m_breakLabel = MAKE_CODE( OP_Target );

		// Generate code for switch statement
		switchBlock = MAKE_CODE( OP_Switch );
		switchExpr = m_children[0]->GenerateCode( pool );

		// Generate related code, this will create labels 
		CScriptCodeNode* switchCode = m_children[1]->GenerateCode( pool );

		// No empty switch is allowed
		ASSERT( m_switchLabels.Size(), TXT( "No emtpy switch allowed" ) );

		// Remember type of evaluated data
		ASSERT( m_value.m_type );
		switchBlock->m_value.m_type = m_value.m_type;

		// Link labels from switch cases into linked list, make sure default is the last
		CScriptCodeNode** prevLabel = &switchBlock->m_label;
		switchBlock->m_label = NULL;
		for ( Uint32 i=0; i<m_switchLabels.Size(); i++ )
		{
			CScriptCodeNode* node = m_switchLabels[i];
			ASSERT( node->m_opcode == OP_SwitchLabel );
			ASSERT( !node->m_skipLabel );

			*prevLabel = node;
			prevLabel = &node->m_skipLabel;
		}

		// Link default case
		if ( m_switchDefaultLabel )
		{
			ASSERT( m_switchDefaultLabel->m_opcode == OP_SwitchDefault );
			*prevLabel = m_switchDefaultLabel;
		}
		else
		{
			// Link to end
			*prevLabel = m_breakLabel;
		}

		// Glue everything
		node = switchBlock;
		CScriptCodeNode::Glue( node, switchExpr );
		CScriptCodeNode::Glue( node, switchCode );
		CScriptCodeNode::Glue( node, m_breakLabel );
		return node;
	}

	// Case in switch statement
	if ( m_type == CNAME( SyntaxSwitchCase ) )
	{
		CScriptCodeNode *caseBlock, *caseToken, *caseExpr, *caseLabel;

		// Create jump target
		caseToken = MAKE_CODE( OP_SwitchLabel );
		caseExpr = m_children[0]->GenerateCode( pool );
		caseLabel = MAKE_CODE( OP_Target );
		caseBlock = m_children[1]->GenerateCode( pool );

		// Skip expression in fall through code
		caseToken->m_label = caseLabel;	
		caseToken->m_skipLabel = NULL; // Next switch case label

		// Register case label in parent switch
		m_value.m_node->m_switchLabels.PushBack( caseToken );

		// Glue everything   
		node = caseToken;
		CScriptCodeNode::Glue( node, caseExpr );
		CScriptCodeNode::Glue( node, caseLabel );
		CScriptCodeNode::Glue( node, caseBlock );
		return node;
	}

	// Default case of switch statement
	if ( m_type == CNAME( SyntaxDefaultCase ) )
	{
		CScriptCodeNode *defaultLabel, *defaultBlock;

		// Default code
		defaultLabel = MAKE_CODE( OP_SwitchDefault );
		defaultBlock = m_children[0]->GenerateCode( pool );

		// Register default case label in parent switch
		ASSERT( !m_value.m_node->m_switchDefaultLabel );
		m_value.m_node->m_switchDefaultLabel = defaultLabel;

		// Glue everything    
		return CScriptCodeNode::Glue( defaultLabel, defaultBlock );
	}

	// Object deletion
	if ( m_type == CNAME( SyntaxDelete ) )
	{
		node = MAKE_CODE( OP_Delete );
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	};

	// This
	if ( m_type == CNAME( SyntaxThisValue ) )
	{
		node = MAKE_CODE( OP_This );
		return node;
	}

	// Object creation
	if ( m_type == CNAME( SyntaxNew ) )
	{
		node = MAKE_CODE( OP_New );
		node->m_value.m_type = m_value.m_type;
		ASSERT( node->m_value.m_type );
		ASSERT( node->m_value.m_type->GetType() == RT_Class );
		if ( m_children[0] )
		{
			CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		}
		else
		{
			// No target given, compile to NULL
			CScriptCodeNode* nop = MAKE_CODE( OP_Nop ) ;
			CScriptCodeNode::Glue( node, nop );
		}
		return node;
	}

	// Array element indexing
	if ( m_type == CNAME( SyntaxArrayElement ) )
	{
		if ( m_value.m_type->GetType() == RT_Array )
		{
			// dynamic array version
			node = MAKE_CODE( OP_ArrayElement );
		}
		else if ( m_value.m_type->GetType() == RT_NativeArray )
		{
			// static array version
			node = MAKE_CODE( OP_StaticArrayElement );
		}
		else
		{
			HALT( "Unknown array type when compiling scripts" );
		}

		node->m_value.m_type = m_value.m_type;
		ASSERT( node->m_value.m_type );

		// Glue parameters
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		CScriptCodeNode::Glue( node, m_children[1]->GenerateCode( pool ) );
		return node;
	}

	// Clear array
	if ( m_type == CNAME( SyntaxArrayClear ) )
	{
		node = MAKE_CODE( OP_ArrayClear );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array );  

	    node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}
	   
	// Get array size
	if ( m_type == CNAME( SyntaxArraySize ) )
	{
		const ERTTITypeType dataType = m_children[0]->m_valueType.m_type->GetType();
		if ( dataType == RT_Array )
		{
			// dynamic array version
			node = MAKE_CODE( OP_ArraySize );
		}
		else if ( dataType == RT_NativeArray )
		{
			// static array version
			node = MAKE_CODE( OP_StaticArraySize );
		}
		else
		{
			HALT( "Unknown array type when compiling scripts" );
		}

		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}

	// Resize array
	if ( m_type == CNAME( SyntaxArrayResize ) )
	{
	    node = MAKE_CODE( OP_ArrayResize );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array );  

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}
   
	// Find first array element
	if ( m_type == CNAME( SyntaxArrayFindFirst ) )
	{
		const ERTTITypeType dataType = m_children[0]->m_valueType.m_type->GetType();

		if ( dataType == RT_Array )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_ArrayFindFirstFast );
			}
			else
			{
				node = MAKE_CODE( OP_ArrayFindFirst );
			}
		}
		else if ( dataType == RT_NativeArray )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_StaticArrayFindFirstFast );
			}
			else
			{
				node = MAKE_CODE( OP_StaticArrayFindFirst );
			}
		}
		else
		{
			HALT( "Unknown array type when compiling scripts" );
		}

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}

	// Find last array element
	if ( m_type == CNAME( SyntaxArrayFindLast ) )
	{
		const ERTTITypeType dataType = m_children[0]->m_valueType.m_type->GetType();

		if ( dataType == RT_Array )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_ArrayFindLastFast );
			}
			else
			{
				node = MAKE_CODE( OP_ArrayFindLast );
			}
		}
		else if ( dataType == RT_NativeArray )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_StaticArrayFindLastFast );
			}
			else
			{
				node = MAKE_CODE( OP_StaticArrayFindLast );
			}
		}
		else
		{
			HALT( "Unknown array type when compiling scripts" );
		}

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}
   
	// Check if array contains element
	if ( m_type == CNAME( SyntaxArrayContains ) )
	{
		const ERTTITypeType dataType = m_children[0]->m_valueType.m_type->GetType();

		if ( dataType == RT_Array )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_ArrayContainsFast );
			}
			else
			{
				node = MAKE_CODE( OP_ArrayContains );
			}
		}
		else if ( dataType == RT_NativeArray )
		{
			if ( m_list[0]->m_valueType.m_isAssignable )
			{
				node = MAKE_CODE( OP_StaticArrayContainsFast );
			}
			else
			{
				node = MAKE_CODE( OP_StaticArrayContains );
			}
		}
		else
		{
			HALT( "Unknown array type when compiling scripts");
		}

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}

	// Add element to array
	if ( m_type == CNAME( SyntaxArrayPushBack ) )
	{
	    node = MAKE_CODE( OP_ArrayPushBack );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}

	// Insert element to array
	if ( m_type == CNAME( SyntaxArrayInsert ) )
	{
		node = MAKE_CODE( OP_ArrayInsert );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[1]->GenerateCode( pool ) );
		return node;
	}
   
	// Remove element from array
	if ( m_type == CNAME( SyntaxArrayRemove ) )
	{
		if ( m_list[0]->m_valueType.m_isAssignable )
		{
			node = MAKE_CODE( OP_ArrayRemoveFast );
		}
		else
		{
			node = MAKE_CODE( OP_ArrayRemove );
		}

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}   
   
	// Add space to array
	if ( m_type == CNAME( SyntaxArrayGrow ) )
	{
	    node = MAKE_CODE( OP_ArrayGrow );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}      
   
	// Remove elements to array
	if ( m_type == CNAME( SyntaxArrayErase ) )
	{
		node = MAKE_CODE( OP_ArrayErase );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

	    node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
//		node = CScriptCodeNode::Glue( node, m_list[1]->GenerateCode( pool ) );
		return node;
	}    

	// Remove elements to array
	if ( m_type == CNAME( SyntaxArrayEraseFast ) )
	{
		node = MAKE_CODE( OP_ArrayEraseFast );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		node = CScriptCodeNode::Glue( node, m_list[0]->GenerateCode( pool ) );
		return node;
	}    
   
	// Get last array element
	if ( m_type == CNAME( SyntaxArrayLast ) ) 
	{
		const ERTTITypeType dataType = m_children[0]->m_valueType.m_type->GetType();

		if ( dataType == RT_Array )
		{
			node = MAKE_CODE( OP_ArrayLast );
		}
		else if ( dataType == RT_NativeArray )
		{
			node = MAKE_CODE( OP_StaticArrayLast );
		}
		else
		{
			HALT( "Unknown array type when compiling scripts" );
		}

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}
   
	// Remove last array element
	if ( m_type == CNAME( SyntaxArrayPopBack ) )
	{
		node = MAKE_CODE( OP_ArrayPopBack );

		// Get array type
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		ASSERT( node->m_value.m_type );  
		ASSERT( node->m_value.m_type->GetType() == RT_Array ); 

		node = CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}

	// Creating a function savepoint
	if ( m_type == CNAME( SyntaxSavePoint ) )
	{
		// Header
		node = MAKE_CODE( OP_SavePoint );
		CScriptCodeNode* postSavePoint = MAKE_CODE( OP_Target );
		node->m_value.m_string = m_value.m_string;	// Name of the save point
		node->m_label = postSavePoint;					// SavePoint end jump point

		// Variables
		for ( auto it = m_list.Begin(); it != m_list.End(); ++it )
		{
			CScriptCodeNode* dataChunk = MAKE_CODE( OP_SaveValue );
			dataChunk->m_value.m_string = (*it)->m_value.m_string;

			node = CScriptCodeNode::Glue( node, dataChunk );
		}

		// insert an end-of-datablock marker
		CScriptCodeNode* dataBlockEndMarker = MAKE_CODE( OP_SavePointEnd );
		node = CScriptCodeNode::Glue( node, dataBlockEndMarker );

		// add the jump label node
		node = CScriptCodeNode::Glue( node, postSavePoint );

		return node;
	}

	// Casting to simple types
	MAKE_CAST( BoolToByte );
	MAKE_CAST( BoolToInt );
	MAKE_CAST( BoolToFloat );
	MAKE_CAST( BoolToString );
	MAKE_CAST( ByteToBool );
	MAKE_CAST( ByteToInt );
	MAKE_CAST( ByteToFloat );
	MAKE_CAST( ByteToString );
	MAKE_CAST( IntToBool );
	MAKE_CAST( IntToByte );
	MAKE_CAST( IntToFloat );
	MAKE_CAST( IntToString );
	MAKE_CAST( FloatToBool );
	MAKE_CAST( FloatToByte );
	MAKE_CAST( FloatToInt );
	MAKE_CAST( FloatToString );
	MAKE_CAST( NameToBool );
	MAKE_CAST( NameToString );
	MAKE_CAST( StringToBool );
	MAKE_CAST( StringToByte );
	MAKE_CAST( StringToInt );
	MAKE_CAST( StringToFloat );
	MAKE_CAST( ObjectToBool );
	MAKE_CAST( ObjectToString );
	
	if ( m_type == CNAME( SyntaxEnumToString ) ) 
	{
		node = MAKE_CODE( OP_EnumToString );
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}
	else if ( m_type == CNAME( SyntaxEnumToInt ) ) 
	{
		node = MAKE_CODE( OP_EnumToInt );
		node->m_value.m_type = m_children[0]->m_valueType.m_type;
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}
	else if ( m_type == CNAME( SyntaxIntToEnum ) ) 
	{
		node = MAKE_CODE( OP_IntToEnum );
		node->m_value.m_type = m_valueType.m_type;
		CScriptCodeNode::Glue( node, m_children[0]->GenerateCode( pool ) );
		return node;
	}

	// Globals
	if ( m_type == CNAME( SyntaxGlobalGame ) )
	{
		node = MAKE_CODE( OP_GetGame );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalPlayer ) )
	{
		node = MAKE_CODE( OP_GetPlayer );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalCamera ) )
	{
		node = MAKE_CODE( OP_GetCamera );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalHud ) )
	{
		node = MAKE_CODE( OP_GetHud );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalSound ) )
	{
		node = MAKE_CODE( OP_GetSound );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalDebug ) )
	{
		node = MAKE_CODE( OP_GetDebug );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalTimer ) )
	{
		node = MAKE_CODE( OP_GetTimer );
		return node;
	}
	if ( m_type == CNAME( SyntaxGlobalInput ) )
	{
		node = MAKE_CODE( OP_GetInput );
		return node;
	}
	if( m_type == CNAME( SyntaxGlobalTelemetry ) )
	{
		node = MAKE_CODE( OP_GetTelemetry );
		return node;
	}

	// Unknown node
	HALT(  "Unknown syntax node: '%ls'", m_type.AsString().AsChar() );
	return NULL;
}
