/**
* Copyright © 2015 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once

namespace Core
{
	class CommandLineArguments
	{
	public:
		Bool m_useBundles;
		Bool m_splitCook;

		CommandLineArguments()
#ifndef NO_EDITOR
		:	m_useBundles( false )
		,	m_splitCook( false )
#else
		:	m_useBundles( true )
		,	m_splitCook( true )
#endif
		{
		}

		void Parse( const Char* commandLine )
		{
			Bool useBundles	= Red::System::StringSearch( commandLine, TXT( "-usebundles" ) ) != nullptr;
			Bool useLoose	= Red::System::StringSearch( commandLine, TXT( "-loose" ) ) != nullptr;

			if( useLoose )
			{
				m_useBundles = false;
			}
			else if( useBundles )
			{
				m_useBundles = true;
			}

			if( m_useBundles )
			{
				Bool notsplit = Red::System::StringSearch( commandLine, TXT( "-nosplitcook" ) ) != nullptr;
				Bool split = Red::System::StringSearch( commandLine, TXT( "-splitcook" ) ) != nullptr;

				if( notsplit )
				{
					m_splitCook = false;
				}
				else if( split )
				{
					m_splitCook = true;
				}
			}
			else
			{
				m_splitCook = false;
			}
		}
	};
}
