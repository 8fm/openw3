/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __SCRIPT_STUDIO_BISONTYPES_H__
#define __SCRIPT_STUDIO_BISONTYPES_H__

#include "fileStubs.h"
#include "lexer/context.h"

/// Structure to pass around parser parameters
struct YYSTYPE
{
	//! Constructor
	YYSTYPE()
		: m_dword( 0 )
		, m_integer( 0 )
		, m_float( 0.0f )
		, m_bool( false )
	{
	}

	//! Destructor
	~YYSTYPE()
	{
		// m_value is NOT destroyed here... !
	}

	//! Data
	wstring					m_string;
	unsigned int			m_dword;
	int						m_integer;
	bool					m_bool;
	float					m_float;
	wstring					m_typeName;
	SSFileContext			m_context;
	wstring					m_token;
	SSFlagList				m_flags;
	SSFlagList				m_idents;
};

#include RED_EXPAND_AND_STRINGIFY(PROJECT_PLATFORM\PROJECT_CONFIGURATION\fileParser_bison.cxx.h)

#endif // __SCRIPT_STUDIO_BISONTYPES_H__
