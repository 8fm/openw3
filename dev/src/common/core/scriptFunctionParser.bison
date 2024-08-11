%{
// Some yacc (bison) defines
#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYINITDEPTH 400
#define YYMAXDEPTH 15000

/// Disable warnings caused by Bison (disable these when editing the grammar to make sure you don't cause any yourself)
RED_DISABLE_WARNING_MSC( 4244 )	// conversion from 'int' to 'short', possible loss of data
RED_DISABLE_WARNING_MSC( 4065 )	// switch statement contains 'default' but no 'case' labels
RED_DISABLE_WARNING_MSC( 4267 )	// conversion from 'size_t' to 'int', possible loss of data
RED_DISABLE_WARNING_MSC( 4127 )	// conditional expression is constant
RED_DISABLE_WARNING_MSC( 4702 )	// Unreachable code

/// The parsing token
#define YYSTYPE YYSTYPE_Function
#include "scriptFunctionParser_bison.cxx.h"

/// Parser context
struct FunctionParserContext
{
	CScriptTokenStream*					m_tokens;
	CScriptFunctionParser*				m_parser;
	TDynArray< CScriptSyntaxNode* >		m_loopStack;
	
	FunctionParserContext()
		: m_tokens( NULL )
		, m_parser( NULL )
	{};
	
	void BeginLoop( CScriptSyntaxNode *loop )
	{
		ASSERT( loop );
		m_loopStack.PushBack( loop );
	}
	
	CScriptSyntaxNode *EndLoop( CScriptSyntaxNode* init, CScriptSyntaxNode* stmt, CScriptSyntaxNode* incr, CScriptSyntaxNode* code )
	{
		CScriptSyntaxNode* node = m_loopStack[ m_loopStack.Size() - 1 ];
		node->m_children[ 0 ] = init;
		node->m_children[ 1 ] = stmt;
		node->m_children[ 2 ] = incr;
		node->m_children[ 3 ] = code;
		m_loopStack.PopBack();
		return node;
	}

	CScriptSyntaxNode *GetTopContextNode()
	{
		if ( !m_loopStack.Size() )
		{
			return NULL;
		}
		else
		{
			return m_loopStack[ m_loopStack.Size() - 1 ];
		}
	}

	CScriptSyntaxNode* GetLoopNode()
	{
		const Int32 numNodes = m_loopStack.Size();
		for ( Int32 i = numNodes - 1; i >= 0; --i )
		{
			CScriptSyntaxNode* node = m_loopStack[ i ];
			if ( node->m_type != CNAME( SyntaxSwitch ) )
			{
				return node;
			}
		}
		return NULL;
	}

	CScriptSyntaxNode* GetSwitchNode()
	{
		const Int32 numNodes = m_loopStack.Size();
		for ( Int32 i = numNodes - 1; i >= 0; --i )
		{
			CScriptSyntaxNode* node = m_loopStack[ i ];
			if ( node->m_type == CNAME( SyntaxSwitch ) )
			{
				return node;
			}
		}
		return NULL;
	}
};

// Temp spawn info
FunctionParserContext GFunctionParser;

// Reset
void GInitFunctionParser( CScriptTokenStream* tokens, CScriptFunctionParser* parser )
{
	// Bind data
	GFunctionParser.m_tokens = tokens;
	GFunctionParser.m_parser = parser;
}

// Error function
int yyfunc_error( const char* msg )
{
	// Emit error
	const CScriptFileContext& context = yylval.m_context;
	String errorString = String::Printf( TXT( "%" ) RED_PRIWas TXT( ", near '%" ) RED_PRIWs TXT( "'" ), msg, yylval.m_token.AsChar() );
	GFunctionParser.m_parser->EmitError( context, errorString );
	
	// Continue parsing
	// TODO: thing about error resolve conditions
	return 0;
}

// Token reader
int yyfunc_lex()
{
	if( GFunctionParser.m_tokens->IsEndOfStream() )
	{
		return 0;
	}
	
	CScriptToken& token = GFunctionParser.m_tokens->GetReadToken();
	GFunctionParser.m_tokens->IncrementReadPosition();

	yylval.m_context = token.m_context;
	yylval.m_token = token.m_text;

	return token.m_token;
}

/// Macros
#define cnop()							new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( SyntaxNop ) )
#define ctree0( type )					new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ) )
#define ctree1( type, a )				new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ), a )
#define ctree2( type, a, b)				new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ), a, b )
#define ctree3( type, a, b, c)			new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ), a, b, c )
#define ctree4( type, a, b, c, d )		new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ), a, b, c, d )
#define ctree5( type, a, b, c, d, e )	new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( Syntax##type ), a, b, c, d, a )
#define celem( x, data )				new CScriptSyntaxNode( E_Tree, yyfunc_lval.m_context, CNAME( SyntaxListItem ), x, data )
#define clist( x, type )				new CScriptSyntaxNode( E_List, yyfunc_lval.m_context, CNAME( Syntax##type ), x )
#define cop( op, x, y )					new CScriptSyntaxNode( E_Operator, yyfunc_lval.m_context, CNAME( Operator##op ), x, y )
#define cop2( op, x, y )				new CScriptSyntaxNode( E_Operator, yyfunc_lval.m_context, op, x, y )

%}

/* ------------------------------------------------------------------
   Token definitions
   ------------------------------------------------------------------ */

/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFileParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFileParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFileParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFileParser.bison */

/* Expect 0 shift/reduce conflicts */
/* The only expected shift/reduce conflicts is in the If-Then-Else statement */
%expect 1

/* Data tokens */
%token TOKEN_IDENT
%token TOKEN_INTEGER
%token TOKEN_FLOAT
%token TOKEN_NAME
%token TOKEN_STRING
%token TOKEN_BOOL_TRUE
%token TOKEN_BOOL_FALSE
%token TOKEN_NULL

/* Keywords */
%token TOKEN_IMPORT
%token TOKEN_ABSTRACT 
%token TOKEN_CONST 
%token TOKEN_EXTENDS
%token TOKEN_CLASS
%token TOKEN_STATE
%token TOKEN_STATEMACHINE
%token TOKEN_ENUM
%token TOKEN_STRUCT
%token TOKEN_FUNCTION
%token TOKEN_DEF
%token TOKEN_EDITABLE
%token TOKEN_FINAL
%token TOKEN_OUT
%token TOKEN_OPTIONAL
%token TOKEN_LOCAL
%token TOKEN_INLINED
%token TOKEN_ARRAY
%token TOKEN_DEFAULTS
%token TOKEN_DEFAULT
%token TOKEN_HINT
%token TOKEN_ENTRY
%token TOKEN_ENTER
%token TOKEN_LATENT
%token TOKEN_VAR
%token TOKEN_AUTOBIND
%token TOKEN_ANY
%token TOKEN_SINGLE
%token TOKEN_EXEC
%token TOKEN_TIMER
%token TOKEN_CLEANUP
%token TOKEN_SCENE
%token TOKEN_SAVED
%token TOKEN_QUEST
%token TOKEN_REWARD

/* Type names */
%token TOKEN_TYPE_BYTE 
%token TOKEN_TYPE_BOOL 
%token TOKEN_TYPE_INT 
%token TOKEN_TYPE_FLOAT 
%token TOKEN_TYPE_STRING 
%token TOKEN_TYPE_NAME
%token TOKEN_TYPE_VOID

%token TOKEN_CLASS_TYPE
%token TOKEN_ENUM_TYPE

/* Function parsing tokens */
%token TOKEN_OP_IADD
%token TOKEN_OP_ISUB
%token TOKEN_OP_IMUL
%token TOKEN_OP_IDIV
%token TOKEN_OP_IAND
%token TOKEN_OP_IOR
%token TOKEN_OP_LOGIC_OR
%token TOKEN_OP_LOGIC_AND
%token TOKEN_OP_EQUAL
%token TOKEN_OP_NOTEQUAL
%token TOKEN_OP_GREQ
%token TOKEN_OP_LEEQ
%token TOKEN_NEW 
%token TOKEN_DELETE
%token TOKEN_IN 
%token TOKEN_IF
%token TOKEN_ELSE
%token TOKEN_SWITCH
%token TOKEN_CASE
%token TOKEN_FOR
%token TOKEN_WHILE
%token TOKEN_DO
%token TOKEN_RETURN
%token TOKEN_BREAK
%token TOKEN_CONTINUE
%token TOKEN_THIS
%token TOKEN_SUPER
%token TOKEN_PARENT
%token TOKEN_VIRTUAL_PARENT
%token TOKEN_PRIVATE
%token TOKEN_PROTECTED
%token TOKEN_PUBLIC
%token TOKEN_EVENT
%token TOKEN_SAVEPOINT
%token TOKEN_GET_GAME
%token TOKEN_GET_PLAYER
%token TOKEN_GET_CAMERA
%token TOKEN_GET_SOUND
%token TOKEN_GET_DEBUG
%token TOKEN_GET_TIMER
%token TOKEN_GET_INPUT
%token TOKEN_GET_TELEMETRY

%%

////////////////////////////////
// FUNCTION CODE
////////////////////////////////

function_code
	: statement_list			{ $<m_node>$ = clist( $<m_node>1, Code ); GFunctionParser.m_parser->SetRootSyntaxNode( $<m_node>$ ); }
	;

////////////////////////////////
// STATEMENTS
////////////////////////////////


statement_list
	: statement statement_list	{ $<m_node>$ = celem( $<m_node>2, $<m_node>1 ); }
	| /* empty */               { $<m_node>$ = cnop(); }
	;

statement
	: compound_statement		{ $<m_node>$ = $<m_node>1; }
	| empty_statement			{ $<m_node>$ = $<m_node>1; }
	| expression_statement		{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| jump_statement			{ $<m_node>$ = $<m_node>1; }
	| selection_statement		{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| loop_statement			{ $<m_node>$ = $<m_node>1; }
	| delete_statement			{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| switch_statement			{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| savepoint_statement		{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| labeled_statement			{ $<m_node>$ = $<m_node>1; }
	| enter_statement			{ $<m_node>$ = ctree1( Breakpoint, $<m_node>1 ); }
	| error ';'					{ $<m_node>$ = cnop(); }
	;

compound_statement
	: '{' statement_list '}'	{ $<m_node>$ = clist( $<m_node>2, Code ); }
	;
 
empty_statement
	: /* empty */ ';'           { $<m_node>$ = cnop(); }
	;

expression_statement
	: expression ';'			{ $<m_node>$ = $<m_node>1; }
	;

selection_statement
	: TOKEN_IF '(' expression ')' statement                       { $<m_node>$ = ctree2( IfThen, $<m_node>3, $<m_node>5 );  $<m_node>$->m_context = $<m_context>1; }
	| TOKEN_IF '(' expression ')' statement TOKEN_ELSE statement  { $<m_node>$ = ctree3( IfThenElse, $<m_node>3, $<m_node>5, $<m_node>7); $<m_node>$->m_context = $<m_context>1; }
	;

switch_statement
	: switch_header statement				{ $<m_node>$ = GFunctionParser.EndLoop( $<m_node>1->m_children[0], $<m_node>2, NULL, NULL ); }
	;

switch_header
	: TOKEN_SWITCH '(' expression ')'		{ $<m_node>$ = ctree1( Switch, $<m_node>3 ); $<m_node>$->m_context = $<m_node>3->m_context; GFunctionParser.BeginLoop( $<m_node>$ );  }
	;

labeled_statement
	: TOKEN_CASE expression ':' statement	{ $<m_node>$ = ctree2( SwitchCase, $<m_node>2, $<m_node>4); $<m_node>$->m_value.m_node = GFunctionParser.GetSwitchNode(); }
	| TOKEN_DEFAULT ':' statement			{ $<m_node>$ = ctree1( DefaultCase, $<m_node>3 ); $<m_node>$->m_value.m_node = GFunctionParser.GetSwitchNode(); }
	;

loop_statement
	: TOKEN_FOR '(' optional_expression_list ';' optional_conditional_expression ';' optional_expression_list for_begin statement
		{ $<m_node>$ = GFunctionParser.EndLoop( ctree1( Breakpoint, $<m_node>3 ), $<m_node>5, ctree1( Breakpoint, $<m_node>7 ), $<m_node>9 );
		$<m_node>$->m_context = $<m_context>1; }

	| TOKEN_WHILE '(' assignment_expresion while_begin statement
		{ $<m_node>$ = GFunctionParser.EndLoop( NULL, ctree1( Breakpoint, $<m_node>3 ), NULL, $<m_node>5 );
		$<m_node>$->m_context = $<m_context>1; }

	| dowhile_begin statement TOKEN_WHILE '(' assignment_expresion ')'
		{ $<m_node>$ = GFunctionParser.EndLoop( NULL, ctree1( Breakpoint, $<m_node>5 ), NULL, $<m_node>2 );
		$<m_node>$->m_context = $<m_context>1; }
	;

while_begin
	: ')'	{ GFunctionParser.BeginLoop( ctree0( While ) ); }
	;

dowhile_begin
	: TOKEN_DO	{ GFunctionParser.BeginLoop( ctree0( DoWhile ) ); }
	;

for_begin
	: ')'	{ GFunctionParser.BeginLoop( ctree0( For ) ); }
	;

delete_statement
	: TOKEN_DELETE expression ';'	{ $<m_node>$ = ctree1( Delete, $<m_node>2 ); }
	;

enter_statement
	: TOKEN_ENTER expression ';'	{ $<m_node>$ = ctree1( Enter, $<m_node>2 ); }
	;
 
jump_statement
	: TOKEN_RETURN optional_expression ';'  { $<m_node>$ = ctree1( Return, ctree1( Breakpoint, $<m_node>2 ) ); $<m_node>$->m_context = $<m_context>1; $<m_node>$->m_value.m_node = GFunctionParser.GetLoopNode(); }
	| TOKEN_BREAK ';'                       { $<m_node>$ = ctree0( Break ); $<m_node>$->m_value.m_node = GFunctionParser.GetTopContextNode(); $<m_node>$ = ctree1( Breakpoint, $<m_node>$ ); }
	| TOKEN_CONTINUE ';'                    { $<m_node>$ = ctree0( Continue ); $<m_node>$->m_value.m_node = GFunctionParser.GetLoopNode(); $<m_node>$ = ctree1( Breakpoint, $<m_node>$ ); }
	;

savepoint_statement
	: TOKEN_SAVEPOINT '(' name ',' savepoint_param_list ')'
		{
			$<m_node>$ = clist( $<m_node>5, SavePoint );
			$<m_node>$->m_value.m_string = $<m_string>3;
			$<m_node>$->m_context = $<m_context>1;
		}
	| TOKEN_SAVEPOINT '(' name ')'
		{
			$<m_node>$ = ctree0( SavePoint );
			$<m_node>$->m_value.m_string = $<m_string>3;
			$<m_node>$->m_context = $<m_context>1;
		}
	;

////////////////////////////////
// EPRESSIONS
////////////////////////////////

optional_expression
	: /* empty */				{ $<m_node>$ = NULL; }
	| expression				{ $<m_node>$ = $<m_node>1; }
	;

optional_expression_list
	: /* empty */				{ $<m_node>$ = NULL; }
	| expression_list			{ $<m_node>$ = clist( $<m_node>1, Code ); }
	;

optional_conditional_expression
	: /* empty */				{ $<m_node>$ = NULL; }
	| conditional_expression	{ $<m_node>$ = $<m_node>1; }
	;

expression_list
	: expression                         { $<m_node>$ = $<m_node>1; }
	| expression ',' expression_list     { $<m_node>$ = celem( $<m_node>3, $<m_node>1 ); }
	; 

expression
	: assignment_expresion		{ $<m_node>$ = $<m_node>1; }
	;

assignment_expresion 
	: conditional_expression											{ $<m_node>$ = $<m_node>1; }
	| conditional_expression '=' assignment_expresion					{ $<m_node>$ = ctree2( Assign, $<m_node>1, $<m_node>3 ); }
	| conditional_expression assignment_operator assignment_expresion	{ $<m_node>$ = cop2( $<m_name>2, $<m_node>1, $<m_node>3 ); }
	;

assignment_operator  
	: TOKEN_OP_IADD            { $<m_name>$ = CNAME( OperatorAssignAdd ); }
	| TOKEN_OP_ISUB            { $<m_name>$ = CNAME( OperatorAssignSubtract ); }
	| TOKEN_OP_IMUL            { $<m_name>$ = CNAME( OperatorAssignMultiply ); }
	| TOKEN_OP_IDIV            { $<m_name>$ = CNAME( OperatorAssignDivide ); }
	| TOKEN_OP_IAND            { $<m_name>$ = CNAME( OperatorAssignAnd ); }
	| TOKEN_OP_IOR             { $<m_name>$ = CNAME( OperatorAssignOr ); }
	;

conditional_expression
	: logical_or_expression													{ $<m_node>$ = $<m_node>1; }
	| logical_or_expression '?' expression ':' conditional_expression		{ $<m_node>$ = ctree3( Conditional, $<m_node>1, $<m_node>3, $<m_node>5 ); }
	;

logical_or_expression
	: logical_and_expression												{ $<m_node>$ = $<m_node>1; }
	| logical_or_expression TOKEN_OP_LOGIC_OR logical_and_expression		{ $<m_node>$ = cop( LogicOr, $<m_node>1, $<m_node>3 );  }
	;

logical_and_expression
	: inclusive_or_expression												{ $<m_node>$ = $<m_node>1; }
	| logical_and_expression TOKEN_OP_LOGIC_AND inclusive_or_expression		{ $<m_node>$ = cop( LogicAnd, $<m_node>1, $<m_node>3 ); }
	;

inclusive_or_expression
	: exclusive_or_expression												{ $<m_node>$ = $<m_node>1; }
	| inclusive_or_expression '|' exclusive_or_expression					{ $<m_node>$ = cop( Or, $<m_node>1, $<m_node>3 ); }
	;

exclusive_or_expression
	: and_expression														{ $<m_node>$ = $<m_node>1; }
	| exclusive_or_expression '^' and_expression							{ $<m_node>$ = cop( Xor, $<m_node>1, $<m_node>3 ); }
	;

and_expression
	: equality_expression													{ $<m_node>$ = $<m_node>1; }
	| and_expression '&' equality_expression								{ $<m_node>$ = cop( And, $<m_node>1, $<m_node>3); }
	;

equality_expression
	: relational_expression													{ $<m_node>$ = $<m_node>1; }
	| equality_expression equality_operator relational_expression			{ $<m_node>$ = cop2( $<m_name>2, $<m_node>1, $<m_node>3); }
	;

equality_operator
	: TOKEN_OP_EQUAL		{ $<m_name>$ = CNAME( OperatorEqual ); }
	| TOKEN_OP_NOTEQUAL		{ $<m_name>$ = CNAME( OperatorNotEqual ); }
	;

relational_expression
	: addtive_expression												{ $<m_node>$ = $<m_node>1; }
	| relational_expression relational_operator addtive_expression		{ $<m_node>$ = cop2( $<m_name>2, $<m_node>1, $<m_node>3 ); }
	;

relational_operator
	: '<'				{ $<m_name>$ = CNAME( OperatorLess ); } 
	| '>'				{ $<m_name>$ = CNAME( OperatorGreater ); }
	| TOKEN_OP_GREQ		{ $<m_name>$ = CNAME( OperatorGreaterEqual ); }
	| TOKEN_OP_LEEQ		{ $<m_name>$ = CNAME( OperatorLessEqual ); }
	;

addtive_expression
	: multiplicative_expression                             { $<m_node>$ = $<m_node>1; }
	| addtive_expression '+' multiplicative_expression		{ $<m_node>$ = cop( Add, $<m_node>1, $<m_node>3 ); }
	| addtive_expression '-' multiplicative_expression		{ $<m_node>$ = cop( Subtract, $<m_node>1, $<m_node>3 ); }
	;

multiplicative_expression
	: cast_expression											{ $<m_node>$ = $<m_node>1; }
	| multiplicative_expression '*' cast_expression				{ $<m_node>$ = cop( Multiply, $<m_node>1, $<m_node>3 ); }
	| multiplicative_expression '/' cast_expression				{ $<m_node>$ = cop( Divide, $<m_node>1, $<m_node>3 ); }
	| multiplicative_expression '%' cast_expression	{ $<m_node>$ = cop( Modulo, $<m_node>1, $<m_node>3 ); }
	;

cast_expression
	: unary_expression                               { $<m_node>$ = $<m_node>1; } 
	| '(' basic_type ')' cast_expression             { $<m_node>$ = ctree1( Cast, $<m_node>4 ); $<m_node>$->m_value.m_string = $<m_string>2; }
	;

unary_expression
	: postfix_expression						{ $<m_node>$ = $<m_node>1; } 
	| '-' postfix_expression					{ $<m_node>$ = cop( Neg, $<m_node>2, NULL ); }
	| '!' postfix_expression					{ $<m_node>$ = cop( LogicNot, $<m_node>2, NULL ); } 
	| '~' postfix_expression					{ $<m_node>$ = cop( BitNot, $<m_node>2, NULL ); } 
	| allocation_expression						{ $<m_node>$ = $<m_node>1; }
	;

allocation_expression
	: TOKEN_NEW ident TOKEN_IN postfix_expression		{ $<m_node>$ = ctree1( New, $<m_node>4 ); $<m_node>$->m_value.m_string = $<m_string>2;  }
	| TOKEN_NEW ident									{ $<m_node>$ = ctree1( New, NULL ); $<m_node>$->m_value.m_string = $<m_string>2;  }
	;

postfix_expression
	: primary_expression											{ $<m_node>$ = $<m_node>1; } 
	| postfix_expression '[' expression ']'							{ $<m_node>$ = ctree2( ArrayElement, $<m_node>1, $<m_node>3 ); }
	| postfix_expression '(' func_param_list ')'					{ $<m_node>$ = clist( $<m_node>3, FuncCall ); $<m_node>$->m_children[0] = $<m_node>1; }
	| postfix_expression '.' base_ident								{ $<m_node>$ = $<m_node>3; $<m_node>$->m_children[0] = $<m_node>1; }
	;

func_param_list
	: func_expression_list    { $<m_node>$ = $<m_node>1; }
	;

func_expression_list
	: func_expression                              { $<m_node>$ = $<m_node>1; }
	| func_expression ',' func_expression_list     { $<m_node>$ = celem( $<m_node>3, $<m_node>1); }
	;

func_expression
	: expression                 { $<m_node>$ = $<m_node>1; }
	| /* empty */                { $<m_node>$ = NULL; }
	;

savepoint_param_list
	: savepoint_param							{ $<m_node>$ = $<m_node>1; }
	| savepoint_param ',' savepoint_param_list	{ $<m_node>$ = celem( $<m_node>3, $<m_node>1); }
	;

savepoint_param
	: ident						{ $<m_node>$ = ctree0( Ident ); $<m_node>$->m_value.m_string = $<m_string>1; }
	| /* empty */				{ $<m_node>$ = NULL; }
	;

primary_expression
	: base_ident                 { $<m_node>$ = $<m_node>1; }
	| base_const                 { $<m_node>$ = $<m_node>1; }
	| TOKEN_THIS                 { $<m_node>$ = ctree0( ThisValue ); }
	| '(' expression ')'         { $<m_node>$ = $<m_node>2; }
	;

base_ident 
	: ident					{ $<m_node>$ = ctree0( Ident ); $<m_node>$->m_value.m_string = $<m_string>1; }
	| global_variable       { $<m_node>$ = $<m_node>1; }
	| TOKEN_DEFAULT			{ $<m_node>$ = ctree0( ScopeDefault );  }
	| TOKEN_SUPER			{ $<m_node>$ = ctree0( ScopeSuper ); }
	| TOKEN_PARENT			{ $<m_node>$ = ctree0( ScopeParent ); }
	| TOKEN_VIRTUAL_PARENT	{ $<m_node>$ = ctree0( ScopeVirtualParent ); }
	;

global_variable
	: TOKEN_GET_GAME        { $<m_node>$ = ctree0( GlobalGame ); }
	| TOKEN_GET_PLAYER      { $<m_node>$ = ctree0( GlobalPlayer ); }
	| TOKEN_GET_CAMERA      { $<m_node>$ = ctree0( GlobalCamera ); }
	| TOKEN_GET_SOUND       { $<m_node>$ = ctree0( GlobalSound ); }
	| TOKEN_GET_DEBUG		{ $<m_node>$ = ctree0( GlobalDebug ); }
	| TOKEN_GET_TIMER		{ $<m_node>$ = ctree0( GlobalTimer ); }
	| TOKEN_GET_INPUT		{ $<m_node>$ = ctree0( GlobalInput ); }
	| TOKEN_GET_TELEMETRY	{ $<m_node>$ = ctree0( GlobalTelemetry ); }
	;

base_const
	: integer_const			{ $<m_node>$ = $<m_node>1; }
	| float_const			{ $<m_node>$ = $<m_node>1; }
	| bool_const			{ $<m_node>$ = $<m_node>1; }
	| name_const			{ $<m_node>$ = $<m_node>1; }
	| string_const			{ $<m_node>$ = $<m_node>1; }
	| null_const			{ $<m_node>$ = ctree0( NullConst ); }
	;

integer_const
	: integer				{ $<m_node>$ = ctree0( IntConst ); $<m_node>$->m_value.m_integer = $<m_integer>1; }
	;

float_const
	: float					{ $<m_node>$ = ctree0( FloatConst ); $<m_node>$->m_value.m_float = $<m_float>1; }
	;

bool_const
	: bool					{ $<m_node>$ = ctree0( BoolConst ); $<m_node>$->m_value.m_bool = $<m_bool>1; }
	;

name_const
	: name					{ $<m_node>$ = ctree0( NameConst ); $<m_node>$->m_value.m_string = $<m_string>1;  }
	;

string_const
	: string				{ $<m_node>$ = ctree0( StringConst ); $<m_node>$->m_value.m_string = $<m_string>1; }
	;

////////////////////////////////
// TERMINALS
//////////////////////////////// 

basic_type
	: TOKEN_TYPE_STRING	{ $<m_string>$ = GetTypeName<String>().AsString() }
	| TOKEN_TYPE_BYTE	{ $<m_string>$ = GetTypeName<Uint8>().AsString() }
	| TOKEN_TYPE_BOOL	{ $<m_string>$ = GetTypeName<Bool>().AsString() }
	| TOKEN_TYPE_INT	{ $<m_string>$ = GetTypeName<Int32>().AsString() }
	| TOKEN_TYPE_FLOAT	{ $<m_string>$ = GetTypeName<Float>().AsString() }
	| TOKEN_TYPE_NAME	{ $<m_string>$ = GetTypeName<CName>().AsString(); }
	| TOKEN_CLASS_TYPE	{ $<m_string>$ = $<m_token>1; }
	| TOKEN_ENUM_TYPE	{ $<m_string>$ = $<m_token>1; }
	;

integer
	: TOKEN_INTEGER
		{
			if( !Red::System::StringToInt( $<m_integer>$, $<m_token>1.AsChar(), nullptr, Red::System::BaseAuto ) )
			{
				if( errno == ERANGE )
				{
					yyerror( "Integer Overflow" );
				}
				else
				{
					yyerror( "Token is not a number" );
				}
				YYERROR;
			}
	
			$<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1;
		}
	;

float
	: TOKEN_FLOAT		{ $<m_float>$ = static_cast< Float >( Red::System::StringToDouble( $<m_token>1.AsChar() ) ); $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

name
	: TOKEN_NAME		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

string
	: TOKEN_STRING		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

bool
	: TOKEN_BOOL_TRUE	{ $<m_bool>$ = true; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	| TOKEN_BOOL_FALSE	{ $<m_bool>$ = false; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

ident
	: TOKEN_IDENT		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	; 

null_const
	: TOKEN_NULL		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

%%