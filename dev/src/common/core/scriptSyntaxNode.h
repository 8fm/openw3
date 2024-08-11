/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "namesPool.h"
#include "string.h"
#include "scriptFileContext.h"
#include "names.h"
#include "hashmap.h"
#include "staticarray.h"

/// Syntax node types
RED_DECLARE_NAME( SyntaxNop );
RED_DECLARE_NAME( SyntaxCode );
RED_DECLARE_NAME( SyntaxListItem );
RED_DECLARE_NAME( SyntaxIntConst );
RED_DECLARE_NAME( SyntaxBreakpoint );
RED_DECLARE_NAME( SyntaxFloatConst );
RED_DECLARE_NAME( SyntaxBoolConst );
RED_DECLARE_NAME( SyntaxStringConst );
RED_DECLARE_NAME( SyntaxNameConst );
RED_DECLARE_NAME( SyntaxNullConst );
RED_DECLARE_NAME( SyntaxEnumConst );
RED_DECLARE_NAME( SyntaxOperatorCall );
RED_DECLARE_NAME( SyntaxAssign );
RED_DECLARE_NAME( SyntaxConditional );
RED_DECLARE_NAME( SyntaxArrayElement );
RED_DECLARE_NAME( SyntaxNew );
RED_DECLARE_NAME( SyntaxDelete );
RED_DECLARE_NAME( SyntaxFuncCall );
RED_DECLARE_NAME( SyntaxScopeDefault );
RED_DECLARE_NAME( SyntaxScopeSuper );
RED_DECLARE_NAME( SyntaxScopeParent );
RED_DECLARE_NAME( SyntaxScopeVirtualParent );
RED_DECLARE_NAME( SyntaxIfThen );
RED_DECLARE_NAME( SyntaxIfThenElse );
RED_DECLARE_NAME( SyntaxSwitch );
RED_DECLARE_NAME( SyntaxSwitchCase );
RED_DECLARE_NAME( SyntaxDefaultCase );
RED_DECLARE_NAME( SyntaxConstructor );
RED_DECLARE_NAME( SyntaxFor );
RED_DECLARE_NAME( SyntaxWhile );
RED_DECLARE_NAME( SyntaxDoWhile );
RED_DECLARE_NAME( SyntaxReturn );
RED_DECLARE_NAME( SyntaxEnter );
RED_DECLARE_NAME( SyntaxContinue );
RED_DECLARE_NAME( SyntaxBreak );
RED_DECLARE_NAME( SyntaxThisValue );
RED_DECLARE_NAME( SyntaxIdent );
RED_DECLARE_NAME( SyntaxCast );
RED_DECLARE_NAME( SyntaxNoCast );
RED_DECLARE_NAME( SyntaxTypedEqual );
RED_DECLARE_NAME( SyntaxTypedNotEqual );
RED_DECLARE_NAME( SyntaxBoolToByte )
RED_DECLARE_NAME( SyntaxBoolToInt )
RED_DECLARE_NAME( SyntaxBoolToFloat )
RED_DECLARE_NAME( SyntaxBoolToString )
RED_DECLARE_NAME( SyntaxByteToBool )
RED_DECLARE_NAME( SyntaxByteToInt )
RED_DECLARE_NAME( SyntaxByteToFloat )
RED_DECLARE_NAME( SyntaxByteToString )
RED_DECLARE_NAME( SyntaxIntToBool )
RED_DECLARE_NAME( SyntaxIntToByte )
RED_DECLARE_NAME( SyntaxIntToFloat )
RED_DECLARE_NAME( SyntaxIntToString )
RED_DECLARE_NAME( SyntaxIntToEnum )
RED_DECLARE_NAME( SyntaxFloatToBool )
RED_DECLARE_NAME( SyntaxFloatToByte )
RED_DECLARE_NAME( SyntaxFloatToInt )
RED_DECLARE_NAME( SyntaxFloatToString )
RED_DECLARE_NAME( SyntaxNameToBool )
RED_DECLARE_NAME( SyntaxNameToString )
RED_DECLARE_NAME( SyntaxStringToBool )
RED_DECLARE_NAME( SyntaxStringToByte )
RED_DECLARE_NAME( SyntaxStringToInt )
RED_DECLARE_NAME( SyntaxStringToFloat )
RED_DECLARE_NAME( SyntaxObjectToBool )
RED_DECLARE_NAME( SyntaxObjectToString )
RED_DECLARE_NAME( SyntaxEnumToString )
RED_DECLARE_NAME( SyntaxEnumToInt )
RED_DECLARE_NAME( SyntaxDynamicCast )
RED_DECLARE_NAME( SyntaxArrayClear );
RED_DECLARE_NAME( SyntaxArrayGrow );
RED_DECLARE_NAME( SyntaxArrayPushBack );
RED_DECLARE_NAME( SyntaxArrayPopBack );
RED_DECLARE_NAME( SyntaxArrayInsert );
RED_DECLARE_NAME( SyntaxArrayErase );
RED_DECLARE_NAME( SyntaxArrayEraseFast );
RED_DECLARE_NAME( SyntaxArrayRemove );
RED_DECLARE_NAME( SyntaxArrayFindFirst );
RED_DECLARE_NAME( SyntaxArrayFindLast );
RED_DECLARE_NAME( SyntaxArrayContains );
RED_DECLARE_NAME( SyntaxArraySize );
RED_DECLARE_NAME( SyntaxArrayResize );
RED_DECLARE_NAME( SyntaxArrayLast );
RED_DECLARE_NAME( SyntaxSavePoint );
RED_DECLARE_NAME( SyntaxGlobalGame );
RED_DECLARE_NAME( SyntaxGlobalPlayer );
RED_DECLARE_NAME( SyntaxGlobalCamera );
RED_DECLARE_NAME( SyntaxGlobalHud );
RED_DECLARE_NAME( SyntaxGlobalSound );
RED_DECLARE_NAME( SyntaxGlobalDebug );
RED_DECLARE_NAME( SyntaxGlobalTimer );
RED_DECLARE_NAME( SyntaxGlobalInput );
RED_DECLARE_NAME( SyntaxGlobalTelemetry );

/// Enums
enum EScriptTreeNode { E_Tree };
enum EScriptListNode { E_List };
enum EScriptOpNode   { E_Operator };

class CScriptSyntaxNode;
class CScriptFunctionCompiler;
class CScriptCodeNode;
class CScriptCodeNodeCompilationPool;
class CFunction;
class CProperty;
class CEnum;

/// Value of syntax node
struct ScriptSyntaxNodeValue
{
	String					m_string;
	Uint32					m_dword;
	Int32					m_integer;
	Bool					m_bool;
	Float					m_float;
	CScriptSyntaxNode*		m_node;
	CFunction*				m_function;
	CProperty*				m_property;
	CClass*					m_structure;
	IRTTIType*				m_type;

	ScriptSyntaxNodeValue()
	:	m_dword( 0 )
	,	m_integer( 0 )
	,	m_bool( false )
	,	m_float( 0.0f )
	,	m_node( NULL )
	,	m_function( NULL )
	,	m_property( NULL )
	,	m_structure( NULL )
	,	m_type( NULL )
	{
	}


};

/// Type of syntax node
struct ScriptSyntaxNodeType
{
	Bool			m_isAssignable;		//!< This is an assignable type ( L-value )
	Bool			m_isNull;			//!< Hack for NULL :)
	Bool			m_isFromProperty;	//!< Value is given from property
	IRTTIType*		m_type;				//!< Low level RTTI type

	//! Constructor ( void type )
	ScriptSyntaxNodeType()
	:	m_isAssignable( false )
	,	m_isFromProperty( false )
	,	m_isNull( false )
	,	m_type( NULL )
	{}

	//! Is the a void type ?
	Bool IsVoid() const;

	//! Is targeted type an array
	Bool IsArray() const;

	//! Is targeted type is a pointer
	Bool IsPointer() const;

	//! Convert to string
	String ToString() const;

	//! Get the class in case of object types
	CClass* GetPtrClass() const;

	//! Get the struct in case of structure types
	CClass* GetStruct() const;

	//! Initialize as NULL
	void InitNULL();

	//! Initialize as simple type
	void InitSimple( CName typeName, Bool isAssignable = false );

	//! Init as a given type
	void InitSimple( IRTTIType* type, Bool isAssignable = false );

	//! Init as a pointer to class
	void InitPointer( CClass* typeClass, Bool isAssignable = false );

public:
	//! Check if type are compatible
	static Bool CheckCompatibleTypes( const ScriptSyntaxNodeType& srcType, const ScriptSyntaxNodeType& destType );

	//! Get cost of casting between types
	static Int32 GetCastCost( const ScriptSyntaxNodeType& srcType, const ScriptSyntaxNodeType& destType );

	//! Get the syntax node used for type casting
	static CName GetCastNode( const ScriptSyntaxNodeType& srcType, const ScriptSyntaxNodeType& destType );
};

/// Syntax node of script function code
class CScriptSyntaxNode
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );

public:
	typedef TDynArray< CScriptCodeNode*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > CodeNodes;
	typedef TDynArray< CScriptSyntaxNode*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > SyntaxNodes;

	static const Uint32					MAX_TREE_CHILDREN = 5;

	CScriptFileContext					m_context;							//!< Related place in the source code
	CName								m_type;								//!< Type of node
	CName								m_operator;							//!< Type of the operator
	ScriptSyntaxNodeValue				m_value;							//!< Stored value
	ScriptSyntaxNodeType				m_valueType;						//!< Type of stored value
	CScriptSyntaxNode*					m_parent;							//!< Parent syntax node
	CScriptSyntaxNode*					m_children[ MAX_TREE_CHILDREN ];	//!< Tree children
	CScriptSyntaxNode*					m_defaultSwitch;					//!< Default case for switch
	CScriptCodeNode*					m_breakLabel;						//!< Break label for loops
	CScriptCodeNode*					m_continueLabel;					//!< Continue label for loops
	CodeNodes							m_switchLabels;						//!< Remember label jumps (case:) for switch instruction
	CScriptCodeNode*					m_switchDefaultLabel;				//!< Remember default label jump (default:)
	SyntaxNodes							m_list;								//!< Linearized children tree
	
public:
	//! Tree constructor
	CScriptSyntaxNode( EScriptTreeNode, const CScriptFileContext& context, CName type, CScriptSyntaxNode* a = NULL, CScriptSyntaxNode* b = NULL, CScriptSyntaxNode* c = NULL, CScriptSyntaxNode* d = NULL, CScriptSyntaxNode* e = NULL );

	//! List constructor
	CScriptSyntaxNode( EScriptListNode, const CScriptFileContext& context, CName type, CScriptSyntaxNode* listRoot );

	//! Operator constructor
	CScriptSyntaxNode( EScriptOpNode, const CScriptFileContext& context, CName operatorType, CScriptSyntaxNode* left, CScriptSyntaxNode* right );

#ifdef RED_LOGGING_ENABLED
	//! Write syntax tree to log
	void Print( Uint32 printLevel = 0, const Char* ChildOrList = TXT( "T" ) );
#endif

	//! Delete node
	void Release( Bool recursive );  

public:
	//! Determine and check types of syntax nodes, handles casting
	Bool CheckNodeTypes( CScriptFunctionCompiler* compiler );

	static void InitialiseNodeCheckers();
	static void RegisterGlobalKeyword( const CName& nodeType, const String& className, const String& scriptKeyword );

private:
	struct SGlobalKeyword
	{
		CName nodeType;
		String className;
		String scriptKeyword;
	};

	typedef Bool  ( CScriptSyntaxNode::*TNodeCheckFunc )( CScriptFunctionCompiler* );
	static THashMap< CName, TNodeCheckFunc > m_nodeCheckers;
	static THashMap< CName, SGlobalKeyword > m_globalKeywords;

	static Bool m_nodeCheckersInitialised;

	Bool CheckNodeTypeDummy( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeBreakpoint( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeThis( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeNullConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeIntConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeFloatConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeBoolConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeStringConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeNameConst( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeAssign( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeSuper( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeDefault( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeParent( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeIdentifier( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeIdentifierClass( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeIdentifierFunction( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType );
	Bool CheckNodeTypeIfThen( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeIfThenElse( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeConditional( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeLoopWhile( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeLoopFor( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeBreak( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeContinue( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeReturn( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeArrayElement( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeSwitch( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeSwitchCase( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeSwitchCaseDefault( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeCallFunction( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeCallOperator( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeCallCommon( CScriptFunctionCompiler* compiler, const CFunction* function );
	Bool CheckNodeTypeNew( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeDelete( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeCast( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeEnter( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeSavePoint( CScriptFunctionCompiler* compiler );
	Bool CheckNodeTypeGlobalKeyword( CScriptFunctionCompiler* compiler );

	//! Emit error
	void EmitError( CScriptFunctionCompiler* compiler, const Char* errorString, ... );

public:
	struct SControlPath
	{
	public:
		SControlPath();
		~SControlPath();
		Bool CheckControlPaths( CScriptFunctionCompiler* compiler, Bool returnValueRequired );
		SControlPath* GetEndNode();

		CScriptSyntaxNode* m_node;
		SControlPath* m_next;
		SControlPath* m_prev;

		SControlPath* m_branchIf;
		SControlPath* m_branchElse;
	};

	typedef TStaticArray< SControlPath, 4096 > TControlPathPool;

	SControlPath* MapControlPaths( TControlPathPool& pool, SControlPath* previousNode = nullptr );
	SControlPath* MapControlPathNodeChildren( TControlPathPool& pool );
	void MapControlPathNodeSwitch( TControlPathPool& pool, SControlPath* currentPath );

	//! Generate code nodes from this syntax node 
	CScriptCodeNode* GenerateCode( CScriptCodeNodeCompilationPool& pool );

	//! Match types of node, will try to insert casting node
	Bool MatchNodeType( CScriptSyntaxNode*& nodePtr, CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& destType, Bool explicitCast, Bool functionParam ) const;

	//! Find property in scope of a type
	static CProperty* FindPropertyInScope( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType, const String& name );

	//! Find function in scope of a type
	static const CFunction* FindFunctionInScope( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType, const String& name );

	//! Find state entry function in given class
	static const CFunction* FindEntryFunctionInClass( CClass* compiledClass, const String& name );

	//! Search for enum value
	static CEnum* FindEnumValue( CScriptFunctionCompiler* compiler, const ScriptSyntaxNodeType& scopeType, const String& name, Int32& value );

	//! Find best operator function to use
	static const CFunction* FindBestOperator( CName operation, const ScriptSyntaxNodeType& LType, const ScriptSyntaxNodeType& RType );

	//! Check if a function supports the SavePoints system
	static Bool HasSavepointsSupport( const CFunction* function );

	static Bool FindLatentFunctions( CScriptSyntaxNode* nodePtr );

public:
	//! Handle dynamic array function node
	Bool HandleDynamicArrayFunction( CScriptFunctionCompiler* compiler );

	//! Handle static array function node
	Bool HandleStaticArrayFunction( CScriptFunctionCompiler* compiler );

	//! Handle structure constructor node
	Bool HandleStructureConstructor( CClass* structure, CScriptFunctionCompiler* compiler );
};
