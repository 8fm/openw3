/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include <atldbcli.h>
#include <map>
#include <sstream>

using namespace std;

class CReviewDBConnection
{
public:
	CReviewDBConnection();
	~CReviewDBConnection();

	CDataSource m_dataSource;
	CSession m_session;
	bool m_connected;
};

wstring escapeSingleQuotes(const wstring &input);
wstring tokenReplacer(const wstring &input, std::map<wstring, wstring> &params, std::map<wstring, bool> &paramsQuote, bool unicode = false);
wstring tokenReplacerWithoutSingleQuotes(const wstring &input, std::map<wstring, wstring> &params, std::map<wstring, bool> &paramsQuote, bool unicode = false);

int PerformIdSelectCommand( const WCHAR* command, CReviewDBConnection* connection, unsigned int *id );
int PerformUpdateCommand( const WCHAR* command, CReviewDBConnection* connection );
int PerformSelectCommand( const WCHAR* command, CReviewDBConnection* connection, CCommand< CDynamicAccessor >& query, unsigned int& rowCount );

int AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, const WCHAR* value );
void AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, unsigned int value );
void AppendParameter( std::map< wstring, wstring >& params, std::map< wstring, bool >& paramQuotes, const wstring& key, double value );
