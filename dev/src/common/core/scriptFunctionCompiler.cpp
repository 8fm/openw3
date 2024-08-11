/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "scriptFunctionCompiler.h"

#include "scriptFunctionParser.h"
#include "scriptSyntaxNode.h"
#include "scriptCodeNode.h"
#include "scriptCodeNodeCompilationPool.h"
#include "scriptCodeGenerator.h"
#include "scriptCompiler.h"
#include "function.h"

CScriptFunctionCompiler::CScriptFunctionCompiler( CScriptCompiler* compiler, CScriptSystemStub& stubs, Bool strictMode )
:	m_compiler( compiler )
,	m_class( NULL )
,	m_function( NULL )
,	m_stubs( stubs )
,	m_strictMode( strictMode )
{
}

CScriptFunctionCompiler::~CScriptFunctionCompiler()
{
}

void CScriptFunctionCompiler::EmitError( const CScriptFileContext& context, const String& text )
{
	// Pass to the compiler
	m_compiler->ScriptError( context, text.AsChar() );
}

void CScriptFunctionCompiler::EmitWarning( const CScriptFileContext& context, const String& text )
{
	// Pass to the compiler
	m_compiler->ScriptWarn( context, text.AsChar() );
}

Bool CScriptFunctionCompiler::Compile( CScriptClassStub* classStub, CScriptFunctionStub* functionStub, CScriptCodeNodeCompilationPool& pool )
{
	// Set the bindings
	m_class = classStub ? classStub->m_createdClass : NULL;
	m_function = functionStub->m_createdFunction;

	// Do not compile code for imported functions
	CScriptCodeNode* codeNodes = NULL;
	if ( m_function->IsExported() )
	{
		CScriptCodeNode* postFunctionLabel = pool.Create( functionStub->m_context, OP_Target );

		// Function call
		CScriptCodeNode* funcNode = pool.Create( functionStub->m_context, OP_FinalFunc );
		funcNode->m_label = postFunctionLabel;
		funcNode->m_value.m_function = m_function;

		// Glue header
		codeNodes = pool.Create( functionStub->m_context, OP_Return );
		codeNodes = CScriptCodeNode::Glue( codeNodes, funcNode );

		// Generate parameter access
		const Uint32 numParams = static_cast< Uint32 >( m_function->GetNumParameters() );
		for ( Uint32 i=0; i<numParams; i++ )
		{
			// Get parameter
			CProperty* param = m_function->GetParameter(i);

			// Create wrapper
			CScriptCodeNode* paramNode = pool.Create( functionStub->m_context, OP_ParamVar );
			paramNode->m_value.m_property = param;

			// Merge
			codeNodes = CScriptCodeNode::Glue( codeNodes, paramNode );
		}

		// End of parameters
		CScriptCodeNode* tail = pool.Create( functionStub->m_context, OP_ParamEnd );
		codeNodes = CScriptCodeNode::Glue( codeNodes, tail );
		codeNodes = CScriptCodeNode::Glue( codeNodes, postFunctionLabel );
	}
	else
	{
		// For script functions generate full code
		CScriptFunctionParser parser( m_compiler );
		CScriptSyntaxNode* tree = parser.ParseFunctionCode( functionStub );

		// Generate int code
		if ( tree )
		{
			// Show the tree
			//RED_LOG( RED_LOG_CHANNEL( ScriptParser ), TXT( "File: %ls" ), tree->m_context.m_file.AsChar() );
			//tree->Print();

			Bool controlPathsOK = true;
			if( !m_function->IsEntry() && !m_function->IsEvent() && m_function->GetReturnValue() )
			{
				CScriptSyntaxNode::TControlPathPool pool;

				CScriptSyntaxNode::SControlPath* paths = tree->MapControlPaths( pool );

				if( paths )
				{
					controlPathsOK = paths->CheckControlPaths( this, m_function->m_returnProperty != NULL );
				}
				else if( !( functionStub->m_flags & FF_UndefinedBody ) )
				{
					EmitError( functionStub->m_context, TXT( "Function must return a value" ) );
					controlPathsOK = false;
				}
			}

			// Check types
			//LOG_CORE( TXT("Checking types...") );
			if ( !controlPathsOK || !tree->CheckNodeTypes( this ) )
			{
				tree->Release( true );
				return false;
			}

			// Show the changed tree
			//LOG_CORE( TXT("Syntax tree of '%ls':"), m_function->GetName().AsString().AsChar() );
			//tree->Print();

			// Generate code
			codeNodes = tree->GenerateCode( pool );

			// Release syntax tree
			tree->Release( true );
		}
	}

	// Generate binary code
	if ( codeNodes )
	{
		// Show code
		//LOG_CORE( TXT("Binary code of '%ls':"), m_function->GetName().AsChar() );
		//codeNodes->PrintTree();

		// Generate binary code
		const CScriptFileContext& context = functionStub->m_context;
		CScriptCodeGenerator generator( context.m_file, context.m_line );
		generator.GenerateCode( codeNodes );

		// Set compiled code
		m_function->InitailizerCode( generator );
	}

	// Compiled
	return true;
}


