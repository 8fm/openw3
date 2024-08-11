//**
//* 
//* Copyright © 2014 CD Projekt Red. All Rights Reserved.
//*

#pragma once
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "../../common/core/core.h"
#include "../../common/core/stringLocale.h"
#include "../../common/core/tokenizer.h"
#include "../../common/core/names.h"

struct CommandParameters
{
	CommandParameters()
		: m_windowed( false )
		, m_saveReferences( false )
		, m_marginOfError( 0 )
		, m_height( 0 )
		, m_width( 0 )
		, m_duration( 0 )
	{
	}

	Bool    m_saveReferences; // Save new reference images
	Bool	m_windowed;
	Int32	m_width;
	Int32	m_height;
	Float   m_marginOfError;  // X% difference
	Int32	m_duration;		  // seconds
	String  m_test;			  // specific test to run
};

struct CommandLineArguments
{
	CommandLineArguments() { };
	CommandParameters m_params;
};

void ExtractCommandLineArguments( const Char* commandLine, CommandLineArguments & result );