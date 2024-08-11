/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#define _HAS_EXCEPTIONS 0

#include "HelperFunctions.h"

CReviewDBConnection::CReviewDBConnection()
{
	m_connected = false;
	OleInitialize( 0 );
}

CReviewDBConnection::~CReviewDBConnection()
{
	if( m_connected )
	{
		m_session.Close();
		m_dataSource.Close();
	}
}

// HELPER FUNCTIONS
wstring escapeSingleQuotes(const wstring &input)
{
	wstring output = L"";

	size_t inputSize = input.size();

	size_t counter = 0;

	while( counter < inputSize )
	{
		if( input[counter] != L'\'' )
		{
			output += input[counter];
		}
		else
		{
			output += L'\'';
			output += L'\'';
		}
		counter++;
	}
	return output;
}

wstring tokenReplacer(const wstring &input, std::map<wstring, wstring> &params, std::map<wstring, bool> &paramsQuote, bool unicode)
{
	wstring output = L"";
	WCHAR c;

	size_t inputSize = input.size();

	size_t counter = 0;

	while( counter < inputSize )
	{
		c = input[counter];
		if( c != L'%' )
		{
			output += c;
			counter++;
		}
		else
		{
			wstring token = L"";
			counter++;
			while( input[counter] != L'%' )
			{
				token += input[counter];
				counter++;
			}
			counter++;

			bool quote = paramsQuote[token];

			if(quote)
			{
				if (unicode)
				{
					output += L'N';
				}
				output += L'\'';
			}
			output += params[token];
			if(quote)
			{
				output += L'\'';
			}

		}
	}

	return output;
}

wstring tokenReplacerWithoutSingleQuotes(const wstring &input, std::map<wstring, wstring> &params, std::map<wstring, bool> &paramsQuote, bool unicode)
{
	wstring output = L"";
	WCHAR c;

	size_t inputSize = input.size();

	size_t counter = 0;

	while( counter < inputSize )
	{
		c = input[counter];
		if( c != L'%' )
		{
			output += c;
			counter++;
		}
		else
		{
			wstring token = L"";
			counter++;
			while( input[counter] != L'%' )
			{
				token += input[counter];
				counter++;
			}
			counter++;

			bool quote = paramsQuote[token];

			if(quote)
			{
				if (unicode)
				{
					output += L'N';
				}
				output += L'\'';
			}
			output += escapeSingleQuotes(params[token]);
			if(quote)
			{
				output += L'\'';
			}

		}
	}

	return output;
}

int PerformIdSelectCommand( const WCHAR* command, CReviewDBConnection* connection, unsigned int *id )
{
	int ret = 1;

	if( !connection )
	{
		return -1;
	}
	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( connection->m_session, command );

	if( FAILED(hr) )
	{
		return -1;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) )
	{
		if( hr != DB_S_ENDOFROWSET )
		{
			long res = *(long*)rs.GetValue( 1 );
			(*id) = static_cast<unsigned int>(res);
		}
		else
		{
			(*id) = 0;
			ret = 0;
		}
	}
	else
	{
		ret = -1;
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

int PerformUpdateCommand( const WCHAR* command, CReviewDBConnection* connection )
{
	CCommand<CNoAccessor, CNoRowset> cmd;
	HRESULT hr = cmd.Open( connection->m_session, command, NULL, NULL, DBGUID_DEFAULT, false );

	if( FAILED( hr ) )
	{
		return -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();
	return 1;
}

int PerformSelectCommand( const WCHAR* command, CReviewDBConnection* connection, CCommand< CDynamicAccessor >& query, unsigned int& rowCount )
{
	HRESULT queryResult = query.Open( connection->m_session, command );
	if ( FAILED( queryResult ) == true )
	{
		return -1;
	}

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}
	return 1;
}

int AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, const WCHAR* value )
{
	if ( wcslen( value ) != 0 )
	{
		params[key] = value;
		paramQuotes[key] = true;
		return 1;
	}
	
	return -1;
}

void AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, unsigned int value )
{
	wstringstream wss;
	wss << value;

	params[key] = wss.str();
	paramQuotes[key] = false;
}

void AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, double value )
{
	wstringstream wss;
	wss << value;

	params[key] = wss.str();
	paramQuotes[key] = false;
}

// MAIN DLL FUNCTION
BOOL APIENTRY DllMain( HMODULE, DWORD, LPVOID )
{
	return TRUE;
}

