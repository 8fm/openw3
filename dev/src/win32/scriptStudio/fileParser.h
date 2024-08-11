/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_PARSER_H__
#define __SCRIPT_STUDIO_PARSER_H__

struct SSLexicalData;

/// Script file parser
class SSFileParser
{
protected:
	SSFileStub*				m_stub;

	SSClassStub*			m_currentClass;		//!< Opened class
	SSClassStub*			m_currentStruct;	//!< Opened structure
	SSEnumStub*				m_currentEnum;		//!< Opened enum
	SSFunctionStub*			m_currentFunction;	//!< Opened function
	bool					m_hasError;			//!< Parsing error

public:
	SSFileParser();
	~SSFileParser();

	//! Parse the script file definitions
	bool ParseFile( SSFileStub* stub, const wstring& fileName, const wstring& code, const Red::Scripts::LexerDefinition& definition );

	//! Emit parsing error
	void EmitError( const SSFileContext& context, const wstring& text );

public:
	//! Start class definition
	void StartClass( const SSFileContext& context, const wstring& name, const wstring& extends, const SSFlagList& flags );

	//! Start state definition
	void StartState( const SSFileContext& context, const wstring& name, const wstring& parentClass, const SSFlagList& flags, const wstring& extends );

	//! Start structure definition
	void StartStruct( const SSFileContext& context, const wstring& name, const SSFlagList& flags );

	//! Start an enum definition
	void StartEnum( const SSFileContext& context, const wstring& name );

	//! Start function definition
	void StartFunction( const SSFileContext& context, const wstring& name, const SSFlagList& flags );

	//! Add class/struct/local function property
	void AddProperty( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& typeName, bool isFunctionParameter );

	//! Add enum option
	void AddEmumOption( const SSFileContext& context, const wstring& name );
	void AddEmumOption( const SSFileContext& context, const wstring& name, int value );

	//! Pop context
	void PopContext();

	//! Set function return value
	void SetReturnValueType( const wstring& retTypeName );

	//! End function
	void EndFunction( const SSFileContext& context );

	//! End class/struct
	void EndStruct( const SSFileContext& context );
	void EndClass( const SSFileContext& context );
	void EndClassStub( SSClassStub* stub, const SSFileContext& context );
};

#endif // __SCRIPT_STUDIO_PARSER_H__
