/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptCompiledCode.h"
#include "scriptFileContext.h"
#include "scriptSyntaxNode.h"

class CScriptCodeGenerator;

/// A single binary code node
class CScriptCodeNode
{
public:
	EScriptOpcode			m_opcode;			//!< Node to execute opcode
	Uint16					m_offset;			//!< Write offset
	CScriptFileContext		m_context;			//!< Source context
	ScriptSyntaxNodeValue	m_value;			//!< Internal value
	CScriptCodeNode*		m_next;				//!< Next node in list
	CScriptCodeNode*		m_label;			//!< Label target
	CScriptCodeNode*		m_skipLabel;		//!< Label target for skip-able blocks

public:
	CScriptCodeNode();
	CScriptCodeNode( const CScriptFileContext& context, EScriptOpcode opcode );
	~CScriptCodeNode();

	//! Generate final bytecode
	void GenerateCode( CScriptCodeGenerator& generator );

public:
	//! Glue given list to our tail
	static CScriptCodeNode* Glue( CScriptCodeNode* left, CScriptCodeNode* right );

public:
	//! Print to log
	void PrintMe();
	void PrintTree();

	static const Char* GetOpcodeName( EScriptOpcode opcode );
};
