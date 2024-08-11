%{
// Some yacc (bison) defines
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

/// Disable warning
#pragma warning( disable: 4244 )	// conversion from 'int' to 'short', possible loss of data
#pragma warning( disable: 4065 )	// switch statement contains 'default' but no 'case' labels
#pragma warning( disable: 4702 )	// unreachable code

/// The parsing token
#include "fileParserType.h"
#include "fileParser.h"
#include "lexer/tokenStream.h"
#include "../../common/redSystem/log.h"

/// Parser context
struct FileParserContext
{
	SSTokenStream*			m_tokens;
	SSFileParser*			m_parser;
	
	FileParserContext()
		: m_parser( nullptr )
		, m_tokens( nullptr )
	{};
};

FileParserContext GFilePaser;

// Reset
void GInitFileParser( SSTokenStream* tokens, SSFileParser* parser )
{
	// Bind data
	GFilePaser.m_tokens = tokens;
	GFilePaser.m_parser = parser;
}
  
// Error function
int yyerror( const char *msg )
{
	// Emit error
	wxString errorString;
	errorString.Printf( wxT("%s, near '%s'"), wxString( msg, wxConvLocal ).c_str(), yylval.m_token.c_str() );
	GFilePaser.m_parser->EmitError( yylval.m_context, errorString.wc_str() );
	
	// Continue parsing
	// TODO: thing about error resolve conditions
	return 0;
}

// Token reader
int yylex()
{
	SSToken token;
	if ( GFilePaser.m_tokens->PopToken( token ) )
	{
		yylval.m_context = token.m_context;
		yylval.m_token = token.m_text;
		return token.m_token;
	}
	else
	{
		return 0;
	}
}

%}

/* ------------------------------------------------------------------
   Token definitions
   ------------------------------------------------------------------ */

/* Expect 0 shift/reduce conflicts */
%expect 0

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
%token TOKEN_CLASS_TYPE		// Special, function code only

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
// PROGRAM
////////////////////////////////

program_file
	: file_declaration_list
	; 
	
////////////////////////////////
// DECLARATIONS
////////////////////////////////
 
file_declaration_list
	: file_declaration_list file_declaration
	| /* empty */
	;
 
file_declaration
	: struct_dcl optional_semicolon
	| enum_dcl optional_semicolon
	| class_dcl optional_semicolon
	| state_dcl optional_semicolon
	| function_dcl optional_semicolon
	;

////////////////////////////////
// COMMON
////////////////////////////////

import_flag
	: TOKEN_IMPORT			{ $<m_flags>$ = wxT("import"); }
	| /* empty */			{ }
	;

class_flag
	: TOKEN_IMPORT			{ $<m_flags>$ = wxT("import"); }
	| TOKEN_ABSTRACT		{ $<m_flags>$ = wxT("abstract"); }
	| TOKEN_STATEMACHINE	{ $<m_flags>$ = wxT("statemachine"); }
	;

class_flags
	: class_flag class_flags { $<m_flags>$ = $<m_flags>1 + $<m_flags>2; }
	| /* empty */			 { }
	;

access_flag
	: TOKEN_PRIVATE			{ $<m_flags>$ = wxT("private"); }
	| TOKEN_PROTECTED		{ $<m_flags>$ = wxT("protected"); }
	| TOKEN_PUBLIC			{ $<m_flags>$ = wxT("public"); }
	| /* empty */
	;

function_flag_final
	: TOKEN_FINAL			{ $<m_flags>$ = wxT("finale"); }
	| /* empty */
	;

function_flag_latent
	: TOKEN_LATENT			{ $<m_flags>$ = wxT("latent"); }
	| /* empty */			
	;

function_flag_type
	: TOKEN_EXEC			{ $<m_flags>$ = wxT("exec"); }
	| TOKEN_TIMER			{ $<m_flags>$ = wxT("timer"); }
	| TOKEN_SCENE			{ $<m_flags>$ = wxT("storyscene"); }
	| TOKEN_QUEST			{ $<m_flags>$ = wxT("quest"); }
	| TOKEN_REWARD			{ $<m_flags>$ = wxT("reward"); }
	| TOKEN_CLEANUP			{ $<m_flags>$ = wxT("cleanup"); }
	| TOKEN_ENTRY			{ $<m_flags>$ = wxT("entry"); }
	| /* empty */			
	;

var_flag_const
	: TOKEN_CONST			{ $<m_flags>$ = wxT("const"); }
	| /* empty */
	;

var_flag_editable
	: TOKEN_EDITABLE		{ $<m_flags>$ = wxT("editable"); }
	| /* empty */
	;

var_flag_inlined
	: TOKEN_INLINED			{ $<m_flags>$ = wxT("inlined"); }
	| /* empty */
	;

var_flag_saved
	: TOKEN_SAVED			{ $<m_flags>$ = wxT("saved"); }
	| /* empty */
	;

////////////////////////////////
// STATE
////////////////////////////////

state_dcl
	: state_header '{' class_elem_list '}' { GFilePaser.m_parser->EndClass( $<m_context>4 ); GFilePaser.m_parser->PopContext(); }
	;
 
state_header
	: class_flags TOKEN_STATE ident TOKEN_IN ident						{ GFilePaser.m_parser->StartState( $<m_context>2, $<m_string>3, $<m_string>5, $<m_flags>1, wxT("") ); }
	| class_flags TOKEN_STATE ident TOKEN_IN ident TOKEN_EXTENDS ident	{ GFilePaser.m_parser->StartState( $<m_context>2, $<m_string>3, $<m_string>5, $<m_flags>1, $<m_string>7 ); }
	;
	
////////////////////////////////
// CLASS
////////////////////////////////
 
class_dcl
	: class_header '{' class_elem_list '}' { GFilePaser.m_parser->EndClass( $<m_context>4 ); GFilePaser.m_parser->PopContext(); }
	;
 
class_header
	: class_flags TOKEN_CLASS ident class_extends { GFilePaser.m_parser->StartClass( $<m_context>2, $<m_string>3, $<m_string>4, $<m_flags>1 ); }
	;
 
class_elem_list
	: class_elem_list class_element
	| /* empty */
	;
 
class_element
	: var_dcl semicolon
	| autobind_dcl semicolon
	| function_dcl
	| defaults_dcl
	| hint_dcl
	; 
 
class_extends
	: TOKEN_EXTENDS ident	{ $<m_string>$ = $<m_string>2; }
	| /* empty */			{ $<m_string>$ = wxT(""); }
	; 

////////////////////////////////
// VARIABLES
////////////////////////////////
	
var_common_flags
	:	import_flag access_flag { $<m_flags>$ = $<m_flags>1 + $<m_flags>2; }
	;

var_flags
	:	var_common_flags	
		var_flag_const
		var_flag_editable
		var_flag_inlined
		var_flag_saved
		{  $<m_flags>$ = $<m_flags>1 + $<m_flags>2 + $<m_flags>3 + $<m_flags>4 + $<m_flags>5; }
	;

var_dcl
	:	var_flags
		TOKEN_VAR
		function_param_ident_list ':' full_type
		{
			vector< wstring > idents = $<m_idents>3.m_flags;
			for ( size_t i = 0; i < idents.size(); ++i )
			{
				GFilePaser.m_parser->AddProperty( $<m_context>2, idents[i], $<m_flags>1, $<m_typeName>5, false );
			}
		}
	;

////////////////////////////////
// AUTO BIND
////////////////////////////////

autobind_dcl
	: autobind_flags TOKEN_AUTOBIND ident ':' ident '=' autobind_binding
	{
		GFilePaser.m_parser->AddProperty( $<m_context>2, $<m_string>3, $<m_flags>1, $<m_string>5, false );
	}
	;

autobind_flags
	: var_common_flags autobind_optional_flag  { $<m_flags>$ = $<m_flags>1 + $<m_flags>2; }
	;

autobind_optional_flag
	: TOKEN_OPTIONAL { $<m_flags>$ = wxT("out"); }
	| /* empty */    {  }
	;

autobind_binding
	: name			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; }
	| string		{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; }
	| TOKEN_ANY		{ $<m_string>$ = TXT("$any"); $<m_context>$ = $<m_context>1; }
	| TOKEN_SINGLE	{ $<m_string>$ = TXT("$single"); $<m_context>$ = $<m_context>1; }
	;

////////////////////////////////
// STRUCT
////////////////////////////////

struct_dcl
	:  struct_info '{' struct_elem_list '}' { GFilePaser.m_parser->EndStruct( $<m_context>4 ); GFilePaser.m_parser->PopContext(); }
	;
 
struct_info
	: import_flag TOKEN_STRUCT ident { GFilePaser.m_parser->StartStruct( $<m_context>3, $<m_string>3, $<m_flags>1 ); }
	;
 
struct_elem_list
	: struct_element struct_elem_list 
	| /* empty */
	;
  
struct_element
	: var_dcl semicolon
	| defaults_dcl
	| hint_dcl
	; 

////////////////////////////////
// ENUM
////////////////////////////////

enum_dcl
	: TOKEN_ENUM enum_info '{' enum_name_list optional_comma '}'	{ GFilePaser.m_parser->PopContext(); }
	;

enum_info
	: ident { GFilePaser.m_parser->StartEnum( $<m_context>1, $<m_string>1 ); }
	;
 
enum_name_list 
	: enum_name_list ',' enum_name	{  }
	| enum_name						{  }
	;
 
enum_name
	: ident						{ GFilePaser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1 ); }
	| ident	'=' integer			{ GFilePaser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1, $<m_integer>3 ); }
	| ident	'=' '-' integer		{ GFilePaser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1, -$<m_integer>4 ); }
	;

////////////////////////////////
// DEFAULTS
////////////////////////////////

defaults_dcl
	: TOKEN_DEFAULTS '{' defaults_val_list '}'
	| TOKEN_DEFAULT default_val	
	;
	
defaults_val_list
	: default_val defaults_val_list
	| /* empty */
	;
	
default_val
	: ident '=' script_unnamed_value semicolon
	| ident '=' '-' integer semicolon
	| ident '=' '-' float semicolon
	;

////////////////////////////////
// HINT
////////////////////////////////

hint_dcl
	: TOKEN_HINT hint_val
	;

hint_val
	: ident '=' string semicolon
	;
	
////////////////////////////////
// SCRIPT VALUE
////////////////////////////////

script_value
	: script_named_value
	| script_unnamed_value
	;	
	
script_named_value	
	: ident '=' script_unnamed_value
	;

script_unnamed_value
	: basic_value
	| script_compound_value
	| script_object_value
	;

script_compound_value
	: script_compound_value_header script_compound_value_list_def '}'
	; 

script_compound_value_header
	: '{'
	;

script_compound_value_list_def
	: script_compound_value_list
	| /* empty */
	;
	
script_compound_value_list
	: script_compound_value_list ',' script_value
	| script_value
	;

script_object_value
	: script_object_value_header script_object_value_list_def	 ')'
	;

script_object_value_header
	: '('
	| ident '('
	;

script_object_value_list_def	
	: script_object_value_list
	| /* empty */
	;

script_object_value_list
	: script_object_value_list ',' script_named_value
	| script_named_value
	;

////////////////////////////////
// FUNCTION
////////////////////////////////

function_dcl
	: function_header
	;

function_header
	: function_inner_header '(' function_params ')' function_ret_value function_tail 
	;

function_ret_value
	: ':' full_type		{ GFilePaser.m_parser->SetReturnValueType( $<m_typeName>2 ); }
	| /* empty */
	;

function_inner_header
	:	import_flag
		access_flag
		function_flag_final
		function_flag_latent
		function_flag_type
		TOKEN_FUNCTION
		ident
		{
			GFilePaser.m_parser->StartFunction
			(
				$<m_context>6,
				$<m_string>7,
				$<m_flags>1 +
				$<m_flags>2 +
				$<m_flags>3 +
				$<m_flags>4 +
				$<m_flags>5
			);
		}
	| TOKEN_EVENT ident { GFilePaser.m_parser->StartFunction( $<m_context>1, $<m_string>2, SSFlagList( wxT( "event" ) ) ); }
	;

function_params
	: function_param_list 
	| /* empty */
	;

function_param_list
	: function_param ',' function_param_list
	| function_param
	;

function_param
	: function_param_flag_list function_param_ident_list ':' full_type {
		vector<wstring> idents = $<m_idents>2.m_flags;
		for ( size_t i=0; i<idents.size(); ++i )
		{
			GFilePaser.m_parser->AddProperty( $<m_context>4, idents[i], $<m_flags>1, $<m_typeName>4, true );
		}
	};

function_param_ident_list
	: ident ',' function_param_ident_list	{ $<m_idents>$ = SSFlagList( $<m_string>1 ) + $<m_idents>3;	}
	| ident									{ $<m_idents>$ = $<m_string>1; }
	; 

function_param_flag_list
	: function_param_flag function_param_flag_list     { $<m_flags>$ = $<m_flags>1 + $<m_string>2; }
	| /* empty */                                      { $<m_flags>$ = SSFlagList(); } 
	;

function_param_flag
	: TOKEN_OUT                { $<m_string>$ = wxT("out"); }
	| TOKEN_OPTIONAL           { $<m_string>$ = wxT("optional"); }
	;

function_tail
	: '{' function_vars_dcl '}'		{ GFilePaser.m_parser->EndFunction( $<m_context>3 ); GFilePaser.m_parser->PopContext(); }
	| semicolon						{ GFilePaser.m_parser->EndFunction( $<m_context>1 ); GFilePaser.m_parser->PopContext(); }
	;

function_vars_dcl
	: TOKEN_VAR function_var_dcl function_var_dcl_right_side ';' function_vars_dcl
	| /* empty */	{ GFilePaser.m_tokens->SkipFunctionBody(); yyclearin; }
	;

function_var_dcl
	: function_var_single_dcl ',' function_var_dcl
	| function_var_single_dcl
	;

function_var_single_dcl
	: function_param_ident_list ':' full_type
	{
		vector<wstring> idents = $<m_idents>1.m_flags;
		for ( size_t i=0; i<idents.size(); i++ )
		{
			SSFlagList emptyFlags;
			GFilePaser.m_parser->AddProperty( $<m_context>3, idents[i], emptyFlags, $<m_typeName>3, false );
		}
	}
	;

function_var_dcl_right_side
	: '=' { GFilePaser.m_tokens->SkipFunctionPropertyInitialization(); yyclearin; }
	| /* empty */
	;

////////////////////////////////
// Brought in from scriptFunctionParser
//////////////////////////////// 

expression
	: assignment_expresion
	;

assignment_expresion 
	: conditional_expression
	| conditional_expression '=' assignment_expresion
	| conditional_expression assignment_operator assignment_expresion
	;

assignment_operator  
	: TOKEN_OP_IADD
	| TOKEN_OP_ISUB
	| TOKEN_OP_IMUL
	| TOKEN_OP_IDIV
	| TOKEN_OP_IAND
	| TOKEN_OP_IOR
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression TOKEN_OP_LOGIC_OR logical_and_expression
	;
 
logical_and_expression
	: inclusive_or_expression
	| logical_and_expression TOKEN_OP_LOGIC_AND inclusive_or_expression
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	;

equality_expression
	: relational_expression
	| equality_expression equality_operator relational_expression
	;

equality_operator
	: TOKEN_OP_EQUAL
	| TOKEN_OP_NOTEQUAL
	;

relational_expression
	: addtive_expression
	| relational_expression relational_operator addtive_expression
	;

relational_operator
	: '<'
	| '>'
	| TOKEN_OP_GREQ
	| TOKEN_OP_LEEQ
	;

addtive_expression
	: multiplicative_expression
	| addtive_expression '+' multiplicative_expression
	| addtive_expression '-' multiplicative_expression
	;

multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression
	| multiplicative_expression '/' cast_expression
	| multiplicative_expression '%' cast_expression
	;

cast_expression
	: unary_expression
	| '(' basic_type ')' cast_expression
	;

unary_expression
	: postfix_expression
	| '-' postfix_expression
	| '!' postfix_expression
	| allocation_expression
	;

allocation_expression
	: TOKEN_NEW ident TOKEN_IN postfix_expression
	| TOKEN_NEW ident
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
	| postfix_expression '(' function_params ')'
	| postfix_expression '.' base_ident
	;
	
primary_expression
	: base_ident
	| basic_value
	| TOKEN_THIS
	| '(' expression ')'
	;

base_ident 
	: ident
	| TOKEN_DEFAULT
	| TOKEN_SUPER
	| TOKEN_PARENT
	| TOKEN_VIRTUAL_PARENT
	;

////////////////////////////////
// TERMINALS
//////////////////////////////// 

full_type
	: basic_type	 	{ $<m_typeName>$ = $<m_typeName>1; }
	| array_type		{ $<m_typeName>$ = $<m_typeName>1; }
	| static_array_type	{ $<m_typeName>$ = $<m_typeName>1; }
	| ident				{ $<m_typeName>$ = $<m_string>1; }
	;

basic_type
	: TOKEN_TYPE_STRING	{ $<m_typeName>$ = wxT("string"); }
	| TOKEN_TYPE_BYTE	{ $<m_typeName>$ = wxT("byte"); }
	| TOKEN_TYPE_BOOL	{ $<m_typeName>$ = wxT("bool"); }
	| TOKEN_TYPE_INT	{ $<m_typeName>$ = wxT("int"); }
	| TOKEN_TYPE_FLOAT	{ $<m_typeName>$ = wxT("float"); }
	| TOKEN_TYPE_NAME	{ $<m_typeName>$ = wxT("name"); }
	| TOKEN_TYPE_VOID	{ $<m_typeName>$ = wxT("void"); }
	;

array_type
	: TOKEN_ARRAY '<' full_type '>' { $<m_typeName>$ = wstring( wxT("@") ) + $<m_typeName>3; }
	;

static_array_type
	: full_type '[' integer ']' { $<m_typeName>$ = wstring( wxT("@") ) + $<m_typeName>1; }
	;

basic_value
	: integer			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| float				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| bool				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| string			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| name				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| ident				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| null				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	;

integer
	: TOKEN_INTEGER { RED_VERIFY( Red::System::StringToInt( $<m_integer>$, $<m_token>1.c_str(), nullptr, Red::System::BaseAuto ), TXT( "Could not convert script token into integer" ) ); $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

float
	: TOKEN_FLOAT { $<m_float>$ = _wtof( $<m_token>1.c_str() ); $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

name
	: TOKEN_NAME { $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

string
	: TOKEN_STRING { $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

bool
	: TOKEN_BOOL_TRUE { $<m_bool>$ = true; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	| TOKEN_BOOL_FALSE { $<m_bool>$ = false; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

ident
	: TOKEN_IDENT { $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

null
	: TOKEN_NULL { $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

semicolon 
	: ';'
	;

optional_semicolon
	: ';'
	| /* empty */
	;

optional_comma
	: ','
	| /* empty */
	;

%%