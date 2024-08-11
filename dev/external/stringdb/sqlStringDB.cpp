#define _HAS_EXCEPTIONS 0

#include <windows.h>
#include <atldbcli.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <sstream>


#include "sqlStringDB.h"
#include "sqlCommandsDefinitions.h"

using namespace std;

BOOL APIENTRY DllMain( HMODULE,
					  DWORD,
					  LPVOID
					  )
{
	return TRUE;
}

class sqlStringDbConnection
{
public:
	CDataSource ds;
	CSession session;
	bool initialized;

	sqlStringDbConnection()
	{
		initialized = false;
		OleInitialize( 0 );
	}
	~sqlStringDbConnection()
	{
		if( initialized )
		{
			session.Close();
			ds.Close();
		}
//		OleUninitialize();
	}
};


// This shouldn't be made global, but is a hack #1 to ensure proper dll unloading order
sqlStringDbConnection con;


DLL sqlStringDbConnection * sqlStringDbInit( const WCHAR* databaseAddress, const WCHAR* databaseName )
{
	//Hack #1
	//CoInitialize( 0 );

	//sqlStringDbConnection *connection = new sqlStringDbConnection();

	if( con.initialized )
	{
		// Sorry, only one connection at a time
		return NULL;
	}

	HRESULT hr;

	CDBPropSet rgPropertySet[1] = {DBPROPSET_DBINIT};

	// default address: "CDPRS-MSSQL\\sqlexpress"
	rgPropertySet[0].AddProperty(DBPROP_INIT_DATASOURCE, databaseAddress);
	rgPropertySet[0].AddProperty(DBPROP_INIT_CATALOG, databaseName);
	rgPropertySet[0].AddProperty(DBPROP_AUTH_INTEGRATED, L"SSPI");

	CLSID clsid = {0xc7ff16cL,0x38e3,0x11d0,
	{0x97,0xab,0x0,0xc0,0x4f,0xc2,0xad,0x98}};

	hr = con.ds.Open(clsid, rgPropertySet, 1);

	if( FAILED(hr) )
	{
		//delete connection;
		return NULL;

	}


	hr = con.session.Open( con.ds );

	if ( FAILED(hr) )
	{
		//delete connection;
		return NULL;
	}
	con.initialized = true;

	return &con;

}

DLL void sqlStringDbClose( sqlStringDbConnection *conn )
{
	if( !conn )
		return;

	if ( conn->initialized )
	{
		conn->session.Close();
		conn->ds.Close();
		conn->initialized = false;
	}
}

/*
WCHAR *replace(const WCHAR *str, const WCHAR *old, const WCHAR *newStr)
{
	WCHAR *ret, *r;
	const WCHAR *p, *q;
	size_t oldlen = wcslen(old);
	size_t count, retlen, newlen = wcslen(newStr);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = wcsstr(p, old)) != NULL; p = q + oldlen)
			count++;

		retlen = p - str + wcslen(p) + count * (newlen - oldlen);
	} 
	else
	{
		retlen = wcslen(str);
	}

	ret = (WCHAR*) malloc((retlen + 1)*sizeof(WCHAR));

	for (r = ret, p = str; (q = wcsstr(p, old)) != NULL; p = q + oldlen) {

		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, newStr, newlen);
		r += newlen;
	}
	wcscpy_s(r, retlen + 1 - (r - ret), p);

	return ret;
}*/


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

wstring tokenReplacer(const wstring &input, std::map<wstring, wstring> &params, std::map<wstring, bool> &paramsQuote, bool unicode = false)
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




DLL int sqlStringInsert( sqlStringDbConnection *sqlStringDbConn, unsigned int stringId, unsigned int lang, const WCHAR *text, const WCHAR *user )
{
	int retVal = 1;

	if( !sqlStringDbConn )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	if ( wcslen( text ) != 0 )
	{
		params[L"PARAM_TEXT"] = text;
		paramQuotes[L"PARAM_TEXT"] = true;
	}
	else
	{
		params[L"PARAM_TEXT"] = L"NULL";
		paramQuotes[L"PARAM_TEXT"] = false;
	}

	wstringstream wss1;
	wss1 << stringId;

	
	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	wstringstream wss2;
	wss2 << lang;

	params[L"PARAM_LANG"] = wss2.str();
	paramQuotes[L"PARAM_LANG"] = false;

	if( wcslen( user ) != 0)
	{
		params[L"PARAM_USER"] = user;
		paramQuotes[L"PARAM_USER"] = true;
	}
	else
	{
		params[L"PARAM_USER"] = L"NULL";
		paramQuotes[L"PARAM_USER"] = false;
	}


	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacer( insertQuery, params, paramQuotes, true );

	HRESULT hr = cmd.Open( sqlStringDbConn->session, 
		command.c_str(), 
		NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}
	cmd.Close();
	cmd.ReleaseCommand();


	return retVal;
}

DLL int sqlCopyStringWithoutInfo( sqlStringDbConnection *sqlStringDbConn, unsigned int sourceStringId, unsigned int destStringId )
{
	int retVal = 1;

	if( !sqlStringDbConn )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	wstringstream wss1;
	wss1 << sourceStringId;
	params[ L"PARAM_SOURCE_STRID" ] = wss1.str();
	paramQuotes[ L"PARAM_SOURCE_STRID" ] = false;

	wstringstream wss2;
	wss2 << destStringId;
	params[ L"PARAM_DEST_STRID" ] = wss2.str();
	paramQuotes[ L"PARAM_DEST_STRID" ] = false;

	wstring command = tokenReplacer( copyStringWithoutInfoQuery, params, paramQuotes );

	CCommand< CNoAccessor, CNoRowset > cmd;
	HRESULT hr = cmd.Open( sqlStringDbConn->session, command.c_str(), NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}

DLL int sqlStringInfoInsert( sqlStringDbConnection* sqlStringDbConn, unsigned int stringId, const WCHAR *resource, const WCHAR *property_name, const WCHAR* voiceover_name, const WCHAR* string_key )
{
	int retVal = 1;

	if( !sqlStringDbConn )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	wstringstream wss1;
	wss1 << stringId;

	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;


	if( wcslen( property_name ) != 0)
	{
		params[L"PARAM_PROPERTY_NAME"] = property_name;
		paramQuotes[L"PARAM_PROPERTY_NAME"] = true;
	}
	else
	{
		params[L"PARAM_PROPERTY_NAME"] = L"NULL";
		paramQuotes[L"PARAM_PROPERTY_NAME"] = false;
	}

	if( wcslen( resource ) != 0)
	{
		params[L"PARAM_RESOURCE"] = resource;
		paramQuotes[L"PARAM_RESOURCE"] = true;
	}
	else
	{
		params[L"PARAM_RESOURCE"] = L"NULL";
		paramQuotes[L"PARAM_RESOURCE"] = false;
	}

	if( wcslen( voiceover_name ) != 0)
	{
		params[L"PARAM_VOICEOVER_NAME"] = voiceover_name;
		paramQuotes[L"PARAM_VOICEOVER_NAME"] = true;
	}
	else
	{
		params[L"PARAM_VOICEOVER_NAME"] = L"NULL";
		paramQuotes[L"PARAM_VOICEOVER_NAME"] = false;
	}

	if ( wcslen( string_key ) != 0 )
	{
		params[L"PARAM_STRINGKEY"] = string_key;
		paramQuotes[L"PARAM_STRINGKEY"] = true;
	}
	else
	{
		params[L"PARAM_STRINGKEY"] = L"NULL";
		paramQuotes[L"PARAM_STRINGKEY"] = false;
	}


	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacer( insertInfoQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( sqlStringDbConn->session, 
		command.c_str(), 
		NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();


	return retVal;


}

DLL int sqlStringInfoUpdate( sqlStringDbConnection* connection, unsigned int stringId, const WCHAR *resource, const WCHAR *property_name, const WCHAR* voiceover_name, const WCHAR* string_key )
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	wstringstream wss1;
	wss1 << stringId;

	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;


	if( wcslen( property_name ) != 0)
	{
		params[L"PARAM_PROPERTY_NAME"] = property_name;
		paramQuotes[L"PARAM_PROPERTY_NAME"] = true;
	}
	else
	{
		params[L"PARAM_PROPERTY_NAME"] = L"NULL";
		paramQuotes[L"PARAM_PROPERTY_NAME"] = false;
	}

	if( wcslen( resource ) != 0)
	{
		params[L"PARAM_RESOURCE"] = resource;
		paramQuotes[L"PARAM_RESOURCE"] = true;
	}
	else
	{
		params[L"PARAM_RESOURCE"] = L"NULL";
		paramQuotes[L"PARAM_RESOURCE"] = false;
	}

	if( wcslen( voiceover_name ) != 0)
	{
		params[L"PARAM_VOICEOVER_NAME"] = voiceover_name;
		paramQuotes[L"PARAM_VOICEOVER_NAME"] = true;
	}
	else
	{
		params[L"PARAM_VOICEOVER_NAME"] = L"NULL";
		paramQuotes[L"PARAM_VOICEOVER_NAME"] = false;
	}

	if ( wcslen( string_key ) != 0 )
	{
		params[L"PARAM_STRINGKEY"] = string_key;
		paramQuotes[L"PARAM_STRINGKEY"] = true;
	}
	else
	{
		params[L"PARAM_STRINGKEY"] = L"NULL";
		paramQuotes[L"PARAM_STRINGKEY"] = false;
	}

	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacer( updateInfoQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( connection->session, 
		command.c_str(), 
		NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();


	return retVal;
}

bool checkFallback( sqlStringDbConnection *conn, unsigned int stringId, unsigned int lang )
{
	if( !conn )
	{
		return false;
	}

	bool ret = true;

	CCommand<CDynamicAccessor> rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;


	wstringstream wss1;
	wss1 << stringId;
	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	wstringstream wss2;
	wss2 << lang;

	params[L"PARAM_LANG"] = wss2.str();
	paramQuotes[L"PARAM_LANG"] = false;

	HRESULT hr = rs.Open( conn->session, tokenReplacer( isFallbackQuery, params, paramQuotes ).c_str() );

	if( FAILED(hr) )
	{
		return false;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long res = *(long*)rs.GetValue( 1 );
		if( res > 0 )
		{
			ret = false;
		}
		else
		{
			ret = true;
		}
	}
	else
	{
		ret = true;
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;

}




DLL sqlReadStruct * sqlStringRead( sqlStringDbConnection *conn, unsigned int stringId, unsigned int lang, bool deep )
{
	sqlReadStruct *ret = NULL;

	if( !conn )
	{
		return NULL;
	}
	CCommand<CDynamicAccessor> rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;


	wstringstream wss1;
	wss1 << stringId;
	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	wstringstream wss2;
	wss2 << lang;

	params[L"PARAM_LANG"] = wss2.str();
	paramQuotes[L"PARAM_LANG"] = false;



	HRESULT hr;
	if ( deep )
	{
		hr = rs.Open( conn->session, tokenReplacer( getTextQueryFallbackDeep, params, paramQuotes ).c_str() );
	}
	else
	{
		hr = rs.Open( conn->session, tokenReplacer( getTextQueryFallback, params, paramQuotes ).c_str() );
	}
	
	if( FAILED( hr ) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		WCHAR *res = (WCHAR*)rs.GetValue( 1 );
		size_t size = wcslen( res );
		ret = new sqlReadStruct();
		ret->textSize = size;
		ret->text = new WCHAR[size + 1];
		wcscpy_s( ret->text, size + 1, res);
	}

	rs.Close();   
	rs.ReleaseCommand();
	
	if( ret )
		ret->isFallback = checkFallback( conn, stringId, lang );

	return ret;
}

DLL sqlStringIdStruct *sqlStringReadId( sqlStringDbConnection *conn, const WCHAR* string_key )
{
	sqlStringIdStruct *ret = NULL;

	if( !conn )
	{
		return NULL;
	}
	CCommand< CDynamicAccessor > rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	params[L"PARAM_STRINGKEY"] = string_key;
	paramQuotes[L"PARAM_STRINGKEY"] = true;

	HRESULT hr = rs.Open( conn->session, tokenReplacer( getStringIdByStringKeyQuery, params, paramQuotes ).c_str() );

	if( FAILED( hr ) )
	{
		return NULL;
	}

	hr = rs.MoveFirst();
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		unsigned int res = *(unsigned int*)rs.GetValue( 1 );
		ret = new sqlStringIdStruct();
		ret->stringId = res;
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

DLL int sqlStringFreeReadId( sqlStringIdStruct *s )
{
	if( s )
	{
		delete s;
		return 1;
	}

	return -1;
}

DLL sqlStringKeyStruct *sqlStringReadStringKey( sqlStringDbConnection *conn, unsigned int stringId )
{
	sqlStringKeyStruct *ret = NULL;

	if( !conn )
	{
		return NULL;
	}
	CCommand< CDynamicAccessor > rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	wstringstream wss1;
	wss1 << stringId;

	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	HRESULT hr = rs.Open( conn->session, tokenReplacer( getStringKeyByStringIdQuery, params, paramQuotes ).c_str() );

	if( FAILED( hr ) )
	{
		return NULL;
	}

	hr = rs.MoveFirst();
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		ret = new sqlStringKeyStruct();

		WCHAR* text = (WCHAR*) rs.GetValue( 1 );
		size_t textSize = wcslen( text );
		
		ret->stringKey = new WCHAR[ textSize + 1 ];
		wcscpy_s( ret->stringKey, textSize + 1, text );
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

DLL int sqlStringFreeReadStringKey( sqlStringKeyStruct *s )
{
	if( s )
	{
		delete[] s->stringKey;
		delete s;
		return 1;
	}

	return -1;
}

DLL int sqlStringGetNextID( sqlStringDbConnection *conn, unsigned int *id )
{
	int ret = 1;

	if( !conn )
	{
		return -1;
	}
	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( conn->session, getNextIdQuery );

	if( FAILED(hr) )
	{
		return -1;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long res = *(long*)rs.GetValue( 1 );
		(*id) = static_cast<unsigned int>(res);
	}
	else
	{
		ret = -1;
	}

	hr = rs.Open( conn->session, getNextIdQuery2 );

	if( FAILED(hr) )
	{
		return -1;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long res = *(long*)rs.GetValue( 1 );
		if ( static_cast< unsigned int >( res ) > (*id) )
		{
			(*id) = static_cast< unsigned int >( res );
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

DLL int sqlStringFreeReadResult( sqlReadStruct *s )
{
	if( s )
	{
		delete[] s->text;
		delete s;
		return 1;
	}

	return -1;
}

DLL int sqlStringIsInDb( sqlStringDbConnection *conn, unsigned int stringId )
{
	int ret = 0;

	if( !conn )
	{
		return -1;
	}
	CCommand<CDynamicAccessor> rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;


	wstringstream wss1;
	wss1 << stringId;

	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	HRESULT hr = rs.Open( conn->session, tokenReplacer( getCountQuery, params, paramQuotes ).c_str() );

	if( FAILED(hr) )
	{
		return -1;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long res = *(long*)rs.GetValue( 1 );
		ret = static_cast<int>(res);
	}
	else
	{
		ret = -1;
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

DLL bool sqlStringHasInfo( sqlStringDbConnection* connection , unsigned int stringId )
{
	if ( connection == NULL )
	{
		return false;
	}

	wstringstream stringIdStream;
	stringIdStream << stringId;

	std::map< wstring, wstring > parameters;
	parameters[ L"PARAM_STRID" ] = stringIdStream.str();

	std::map< wstring, bool > parameterQuotes;
	parameterQuotes[ L"PARAM_LANG" ] = false;

	CCommand< CDynamicAccessor > query;
	wstring queryCommand = tokenReplacer( infoCountQuery, parameters, parameterQuotes );

	HRESULT queryResult = query.Open( connection->session, queryCommand.c_str() );
	if ( FAILED( queryResult ) == true )
	{
		return false;
	}

	bool returnValue = false; 

	queryResult = query.MoveFirst();
	if ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		long infoCount = *(long*) query.GetValue( 1 );
		returnValue = infoCount != 0;
	}

	query.Close();
	query.ReleaseCommand();

	return returnValue;
}

DLL int sqlStringFreeLanguagesResult( sqlLanguagesStruct *s )
{
	if( s )
	{
		for( unsigned int i = 0; i < s->languageNum; i++ )
		{
			delete[] s->languages[i];
		}
		delete[] s->languages;
		delete[] s->textSizes;

		delete s;
		
		return 1;
	}

	return -1;
}

DLL sqlLanguagesStruct * sqlStringReadLanguages( sqlStringDbConnection *conn )
{
	list< wstring > langs;
	list< unsigned int > langIds;

	sqlLanguagesStruct *ret = NULL;

	if( !conn )
	{
		return NULL;
	}

	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( conn->session, getLanguagesQuery );

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );

	while( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		WCHAR *res = (WCHAR*)rs.GetValue( 1 );
		wstring s( res );

		unsigned int id = *(unsigned int*)rs.GetValue( 2 );

		langs.push_back( s );
		langIds.push_back( id );
		hr = rs.MoveNext( );
	}

	rs.Close();   
	rs.ReleaseCommand();

	ret = new sqlLanguagesStruct();

	ret->languageNum = langs.size();
	ret->textSizes = new size_t[ ret->languageNum ];
	ret->languages = new WCHAR*[ ret->languageNum ];
	ret->langIds = new unsigned int[ ret->languageNum ];

	list<wstring>::iterator it;
	int i = 0;

	for( it = langs.begin(); it!= langs.end(); ++it )
	{
		size_t size = (*it).size();

		ret->textSizes[i] = size;
		ret->languages[i] = new WCHAR[size + 1];
		wcscpy_s( ret->languages[i], size + 1, (*it).c_str() );

		++i;
	}

	list<unsigned int>::iterator idIter;
	i = 0;
	for( idIter = langIds.begin(); idIter != langIds.end(); ++idIter, ++i )
	{
		ret->langIds[ i ] = *idIter;
	}

	return ret;
}

DLL sqlReadAllStringsStruct * sqlReadAllStrings( sqlStringDbConnection *conn, unsigned int lang )
{
	map< unsigned int, wstring > strings;
	map< unsigned int, bool > stringsNoFallbacks;
	list< unsigned int > allIds;

	sqlReadAllStringsStruct *ret = NULL;

	if( !conn )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	wstringstream wss2;
	wss2 << lang;

	params[L"PARAM_LANG"] = wss2.str();
	paramQuotes[L"PARAM_LANG"] = false;

	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( conn->session, getAllIdsQuery );

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );

	while( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long id = *(long*)rs.GetValue( 1 );

		allIds.push_back( static_cast<unsigned int>(id) );
		hr = rs.MoveNext( );
	}

	rs.Close();   
	rs.ReleaseCommand();

	hr = rs.Open( conn->session, tokenReplacer( getAllStringsQuery, params, paramQuotes ).c_str() );

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );

	while( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long id = *(long*)rs.GetValue( 1 );
		WCHAR *res = (WCHAR*)rs.GetValue( 2 );
		wstring s( res );

		strings[static_cast<unsigned int>(id)] = s;
		hr = rs.MoveNext( );
	}

	rs.Close();   
	rs.ReleaseCommand();

	hr = rs.Open( conn->session, tokenReplacer( areStringsFallbacksQuery, params, paramQuotes ).c_str() );

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );

	while( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long id = *(long*)rs.GetValue( 1 );
		stringsNoFallbacks[ static_cast<unsigned int>(id) ] = true;

		hr = rs.MoveNext( );
	}

	rs.Close();   
	rs.ReleaseCommand();


	ret = new sqlReadAllStringsStruct();

	ret->textNum = strings.size();
	ret->allIds = new unsigned int[ allIds.size() ];
	ret->idsNum = allIds.size();
	ret->textSizes = new size_t[ ret->textNum ];
	ret->textIds = new unsigned int[ ret->textNum ];
	ret->fallbacks = new bool[ ret->textNum ];
	ret->texts = new WCHAR*[ ret->textNum ];

	int i = 0;

	for( map<unsigned int, wstring>::iterator it = strings.begin(); it!= strings.end(); ++it )
	{
		size_t size = (*it).second.size();

		ret->textSizes[i] = size;
		ret->textIds[i] = (*it).first;
		ret->fallbacks[i] = stringsNoFallbacks.find( (*it).first ) == stringsNoFallbacks.end();
		ret->texts[i] = new WCHAR[size + 1];
		wcscpy_s( ret->texts[i], size + 1, (*it).second.c_str() );

		++i;
	}

	i = 0;

	for( list<unsigned int >::iterator it = allIds.begin(); it!= allIds.end(); ++it )
	{
		ret->allIds[i] = *it;
		++i;
	}

	return ret;
}

DLL int sqlStringFreeReadAllStringsResult( sqlReadAllStringsStruct *str )
{
	if( str )
	{
		for( unsigned int i = 0; i < str->textNum; i++ )
		{
			delete[] str->texts[i];
		}
		delete[] str->texts;
		delete[] str->textSizes;
		delete[] str->fallbacks;
		delete[] str->textIds;
		delete[] str->allIds;

		delete str;

		return 1;
	}

	return -1;
}

DLL sqlReadLanguagePackStruct * sqlReadLanguagePack( sqlStringDbConnection* connection, unsigned int stringId, unsigned int lang, bool deep )
{
	if ( connection == NULL )
	{
		return NULL;
	}
	
	sqlReadLanguagePackStruct* packSqlStruct = NULL;

	wstringstream stringIdStream;
	stringIdStream << stringId;
	
	wstringstream langStream;
	langStream << lang;



	std::map< wstring, wstring > parameters;
	parameters[ L"PARAM_STRID" ] = stringIdStream.str();
	parameters[ L"PARAM_LANG" ] = langStream.str();

	std::map< wstring, bool > parameterQuotes;
	parameterQuotes[ L"PARAM_STRID" ] = false;
	parameterQuotes[ L"PARAM_LANG" ] = false;

	CCommand< CDynamicAccessor > query;
	wstring queryCommand;
	if ( deep )
	{
		queryCommand = tokenReplacer( lanquagePackDataQueryDeep, parameters, parameterQuotes );
	}
	else
	{
		queryCommand = tokenReplacer( lanquagePackDataQuery, parameters, parameterQuotes );
	}

	HRESULT queryResult = query.Open( connection->session, queryCommand.c_str() );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	queryResult = query.MoveFirst();
	if ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		packSqlStruct = new sqlReadLanguagePackStruct();
		
		WCHAR* text = (WCHAR*) query.GetValue( 1 );
		size_t textSize = wcslen( text );
		packSqlStruct->text = new WCHAR[ textSize + 1 ];
		wcscpy_s( packSqlStruct->text, textSize + 1, text );

		WCHAR* voiceover = (WCHAR*) query.GetValue( 2 );
		size_t voiceoverSize = wcslen( voiceover );
		packSqlStruct->voiceoverName = new WCHAR[ voiceoverSize + 1 ];
		wcscpy_s( packSqlStruct->voiceoverName, voiceoverSize + 1, voiceover );

		WCHAR* lipsync = (WCHAR*) query.GetValue( 3 );
		size_t lipsyncSize = wcslen( lipsync );
		packSqlStruct->lipsyncName = new WCHAR[ lipsyncSize + 1 ];
		wcscpy_s( packSqlStruct->lipsyncName, lipsyncSize + 1, lipsync );
	}

	query.Close();
	query.ReleaseCommand();

	return packSqlStruct;
}

DLL int sqlStringFreeReadLanguagePackResult( sqlReadLanguagePackStruct* packStruct )
{
	if ( packStruct != NULL )
	{
		delete[] packStruct->text;
		delete[] packStruct->lipsyncName;
		delete[] packStruct->voiceoverName;

		delete packStruct;

		return 1;
	}

	return -1;
}

class sqlStringInternal
{
	WCHAR** buffers;

	size_t numBuffers;
	size_t bufferInUse;
	size_t firstFreeIndex;

	size_t allocationSize;

public:
	sqlStringInternal( size_t allocationSize );

	WCHAR* Copy( WCHAR* str );

	void Free();
};

sqlStringInternal::sqlStringInternal( size_t allocationSize )
:	buffers( nullptr )
,	numBuffers( 0 )
,	bufferInUse( -1 )
,	firstFreeIndex( allocationSize )
,	allocationSize( allocationSize )
{
}

WCHAR* sqlStringInternal::Copy( WCHAR* str )
{
	size_t length = wcslen( str );
	size_t lengthWithTerminator = length + 1;

	if( allocationSize - firstFreeIndex < lengthWithTerminator )
	{
		++bufferInUse;

		firstFreeIndex = 0;
	}

	if( bufferInUse == numBuffers )
	{
		++numBuffers;
		buffers = static_cast< WCHAR** >( realloc( buffers, sizeof( WCHAR** ) * numBuffers ) );
		buffers[ bufferInUse ] = static_cast< WCHAR* >( malloc( sizeof( WCHAR ) * allocationSize ) );
	}

	WCHAR* destination = buffers[ bufferInUse ] + firstFreeIndex;

	wcscpy_s( destination, allocationSize - firstFreeIndex, str );

	firstFreeIndex += lengthWithTerminator;

	return destination;
}

void sqlStringInternal::Free()
{
	for( size_t i = 0; i < numBuffers; ++i )
	{
		free( buffers[ i ] );
	}

	free( buffers );
}

DLL sqlReadAllLanguagePacksStruct* sqlReadAllLanguagePacks( sqlStringDbConnection* connection, unsigned int lang, const WCHAR* view )
{

	sqlReadAllLanguagePacksStruct* returnPackStruct = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}
	
	wstringstream langStream;
	langStream << lang;

	std::map< wstring, wstring > parameters;
	parameters[ L"PARAM_LANG" ] = langStream.str();
	parameters[ L"PARAM_VIEW" ] = view;

	std::map< wstring, bool > parameterQuotes;
	parameterQuotes[ L"PARAM_LANG" ] = false;
	parameterQuotes[ L"PARAM_VIEW" ] = false;

	CCommand< CDynamicAccessor > query;
	wstring queryCommand = tokenReplacer( allLanguagePacksQuery, parameters, parameterQuotes );

	// In ODBC, this is equivalent to specifying SQL_CURSOR_STATIC for the SQL_ATTR_CURSOR_TYPE attribute in a call to SQLSetStmtAttr.
	// This is to make sure that the database is not being changed while we're using the result. 
	CDBPropSet propertySet = DBPROPSET_ROWSET;
	propertySet.AddProperty( DBPROP_CANSCROLLBACKWARDS, true );
	propertySet.AddProperty( DBPROP_OTHERINSERT, false );
	propertySet.AddProperty( DBPROP_OTHERUPDATEDELETE, false );
	HRESULT queryResult = query.Open( connection->session, queryCommand.c_str(), &propertySet );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	returnPackStruct = new sqlReadAllLanguagePacksStruct();


	unsigned int rowCount = 0;
	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}


	returnPackStruct->packsCount = rowCount;
	returnPackStruct->ids = new unsigned int [ returnPackStruct->packsCount ];
	returnPackStruct->texts = new WCHAR* [ returnPackStruct->packsCount ];
	returnPackStruct->voiceovers = new WCHAR* [ returnPackStruct->packsCount ];
	returnPackStruct->lipsyncs = new WCHAR* [ returnPackStruct->packsCount ];
	returnPackStruct->stringKeys = new WCHAR* [ returnPackStruct->packsCount ];

	const size_t allocationSize = 1024 * 1024 * 10;
	returnPackStruct->internalData = new sqlStringInternal( allocationSize );

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		long id					= *( (long*) query.GetValue( 1 ) );
		WCHAR* text				= (WCHAR*) query.GetValue( 2 );
		WCHAR* voiceover		= (WCHAR*) query.GetValue( 3 );
		WCHAR* lipsync			= (WCHAR*) query.GetValue( 4 );
		WCHAR* key				= (WCHAR*) query.GetValue( 5 );

		returnPackStruct->ids[ currentRow ]			= (unsigned int) id;
		returnPackStruct->texts[ currentRow ]		= returnPackStruct->internalData->Copy( text );
		returnPackStruct->voiceovers[ currentRow ]	= returnPackStruct->internalData->Copy( voiceover );
		returnPackStruct->lipsyncs[ currentRow ]	= returnPackStruct->internalData->Copy( lipsync );
		returnPackStruct->stringKeys[ currentRow ]	= returnPackStruct->internalData->Copy( key );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return returnPackStruct;
}

DLL int sqlFreeReadAllLanguagePacksResult( sqlReadAllLanguagePacksStruct* packsStruct )
{
	if ( packsStruct != NULL )
	{
		packsStruct->internalData->Free();

		delete[] packsStruct->texts;
		delete[] packsStruct->lipsyncs;
		delete[] packsStruct->voiceovers;
		delete[] packsStruct->ids;
		delete[] packsStruct->stringKeys;

		delete packsStruct->internalData;

		delete packsStruct;

		return 1;
	}

	return -1;
}

DLL sqlReadAllStringsWithStringKeyStruct* sqlReadAllStringsWithStringKey( sqlStringDbConnection* connection )
{
	sqlReadAllStringsWithStringKeyStruct* returnPackStruct = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;

	// In ODBC, this is equivalent to specifying SQL_CURSOR_STATIC for the SQL_ATTR_CURSOR_TYPE attribute in a call to SQLSetStmtAttr.
	// This is to make sure that the database is not being changed while we're using the result. 
	CDBPropSet propertySet = DBPROPSET_ROWSET;
	propertySet.AddProperty( DBPROP_CANSCROLLBACKWARDS, true );
	propertySet.AddProperty( DBPROP_OTHERINSERT, false );
	propertySet.AddProperty( DBPROP_OTHERUPDATEDELETE, false );
	HRESULT queryResult = query.Open( connection->session, getAllStringsWithStringKey, &propertySet );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	returnPackStruct = new sqlReadAllStringsWithStringKeyStruct();


	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}


	returnPackStruct->packsCount = rowCount;
	returnPackStruct->ids = new unsigned int [ returnPackStruct->packsCount ];
	returnPackStruct->categories = new WCHAR* [ returnPackStruct->packsCount ];
	returnPackStruct->stringKeys = new WCHAR* [ returnPackStruct->packsCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		long id					= *( (long*) query.GetValue( 1 ) );

		WCHAR* category			= (WCHAR*) query.GetValue( 2 );
		size_t categorySize		= wcslen( category );

		WCHAR* stringKey		= (WCHAR*) query.GetValue( 3 );
		size_t stringKeySize	= wcslen( stringKey );
	
		returnPackStruct->ids[ currentRow ] = (unsigned int) id;
		
		returnPackStruct->categories[ currentRow ] = new WCHAR[ categorySize + 1 ];
		wcscpy_s( returnPackStruct->categories[ currentRow ], categorySize + 1, category );

		returnPackStruct->stringKeys[ currentRow ] = new WCHAR[ stringKeySize + 1 ];
		wcscpy_s( returnPackStruct->stringKeys[ currentRow ], stringKeySize + 1, stringKey );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return returnPackStruct;
}

DLL int sqlFreeReadAllStringsWithStringKeyResult( sqlReadAllStringsWithStringKeyStruct* packsStruct )
{
	if ( packsStruct != NULL )
	{
		for ( size_t i = 0; i < packsStruct->packsCount; ++i )
		{
			delete[] packsStruct->categories[i];
			delete[] packsStruct->stringKeys[i];
		}

		delete[] packsStruct->categories;
		delete[] packsStruct->stringKeys;
		delete[] packsStruct->ids;

		delete packsStruct;

		return 1;
	}

	return -1;
}

DLL sqlReadAllCategoriesStruct* sqlReadAllCategories( sqlStringDbConnection* connection, bool onlyStringsWithKeys )
{
	sqlReadAllCategoriesStruct* returnPackStruct = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	wstringstream queryStream;
	queryStream << getAllCategories;

	if( onlyStringsWithKeys )
	{
		queryStream << L' ';
		queryStream << getAllCategoriesKeysOnlyCondition;
	}

	CCommand< CDynamicAccessor > query;
	// In ODBC, this is equivalent to specifying SQL_CURSOR_STATIC for the SQL_ATTR_CURSOR_TYPE attribute in a call to SQLSetStmtAttr.
	// This is to make sure that the database is not being changed while we're using the result. 
	CDBPropSet propertySet = DBPROPSET_ROWSET;
	propertySet.AddProperty( DBPROP_CANSCROLLBACKWARDS, true );
	propertySet.AddProperty( DBPROP_OTHERINSERT, false );
	propertySet.AddProperty( DBPROP_OTHERUPDATEDELETE, false );
	HRESULT queryResult = query.Open( connection->session, queryStream.str().c_str(), &propertySet );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	returnPackStruct = new sqlReadAllCategoriesStruct();

	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}


	returnPackStruct->packsCount = rowCount;
	returnPackStruct->categories = new WCHAR* [ returnPackStruct->packsCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		WCHAR* category			= (WCHAR*) query.GetValue( 1 );
		size_t categorySize		= wcslen( category );

		returnPackStruct->categories[ currentRow ] = new WCHAR[ categorySize + 1 ];
		wcscpy_s( returnPackStruct->categories[ currentRow ], categorySize + 1, category );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return returnPackStruct;
}

DLL int sqlFreeReadAllCategoriesResult( sqlReadAllCategoriesStruct* packsStruct )
{
	if ( packsStruct != NULL )
	{
		for ( size_t i = 0; i < packsStruct->packsCount; ++i )
		{
			delete[] packsStruct->categories[i];
		}

		delete[] packsStruct->categories;

		delete packsStruct;

		return 1;
	}

	return -1;
}

DLL sqlStringInfoStruct* sqlGetStringInfo( sqlStringDbConnection* connection, unsigned int stringId )
{
	sqlStringInfoStruct *ret = NULL;

	if( !connection )
	{
		return NULL;
	}
	CCommand<CDynamicAccessor> rs;

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;


	wstringstream wss1;
	wss1 << stringId;
	params[L"PARAM_STRID"] = wss1.str();
	paramQuotes[L"PARAM_STRID"] = false;

	HRESULT hr = rs.Open( connection->session, tokenReplacer( getStringInfoForStringId, params, paramQuotes ).c_str() );

	if( FAILED( hr ) )
	{
		return NULL;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		ret = new sqlStringInfoStruct();

		WCHAR *res = (WCHAR*)rs.GetValue( 1 );
		size_t size = wcslen( res );
		
		ret->voiceoverName= new WCHAR[size + 1];
		wcscpy_s( ret->voiceoverName, size + 1, res);
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

DLL int sqlFreeGetStringInfoResult( sqlStringInfoStruct* infoStruct )
{
	if ( infoStruct != NULL )
	{
		delete [] infoStruct->voiceoverName;
		delete infoStruct;

		return 1;
	}
	return -1;
}

DLL sqlSearchResults* sqlSearchStrings( sqlStringDbConnection *connection, const sqlSearchParams& searchParams )
{
	sqlSearchResults* ret = NULL;

	if( !connection )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// Language where clause section
	wstringstream langStream;
	langStream << searchParams.lang;

	params[L"PARAM_LANG"] = langStream.str();
	paramQuotes[L"PARAM_LANG"] = false;

	// Text comparison where clause section
	wstringstream searchQueryStream;

	if( searchParams.searchKeys )
	{
		params[L"PARAM_TEXTFIELD"] = L"STRING_INFO.STRING_KEY";
	}
	else
	{
		params[L"PARAM_TEXTFIELD"] = L"LATEST_STRINGS.TEXT";
	}

	paramQuotes[L"PARAM_TEXTFIELD"] = false;

	params[L"PARAM_SEARCH"] = wstring( searchParams.searchQuery ) + L"%";
	paramQuotes[L"PARAM_SEARCH"] = true;

	params[L"PARAM_ORDERCOL"] = wstring( searchParams.orderBy );
	paramQuotes[L"PARAM_ORDERCOL"] = true;

	// Category filters
	wstringstream categoryStream;
	categoryStream << L"(";

	for( unsigned int i = 0; i < searchParams.numCategories; ++i )
	{
		if( i > 0 )
		{
			categoryStream << L" OR ";
		}

		categoryStream << L"STRING_INFO.PROPERTY_NAME = '";
		categoryStream << escapeSingleQuotes( searchParams.categories[ i ] );
		categoryStream << L"'";
	}

	categoryStream << L")";

	// Construct final SQL Query
	wstring lang = tokenReplacer( stringSearchLangCondition, params, paramQuotes );
	wstring search = tokenReplacer( stringSearchTextCondition, params, paramQuotes );
	wstring order = tokenReplacer( stringSearchOrderCondition, params, paramQuotes );

	wstringstream query;
	query << stringSearchQuery;
	query << lang;
	query << L" AND ";
	query << search;
	query << L" AND ";
	query << categoryStream.str();
	query << L" ";
	query << order;

	// Execute Query!
	CCommand<CDynamicAccessor> rs;
	// In ODBC, this is equivalent to specifying SQL_CURSOR_STATIC for the SQL_ATTR_CURSOR_TYPE attribute in a call to SQLSetStmtAttr.
	// This is to make sure that the database is not being changed while we're using the result. 
	CDBPropSet propertySet = DBPROPSET_ROWSET;
	propertySet.AddProperty( DBPROP_CANSCROLLBACKWARDS, true );
	propertySet.AddProperty( DBPROP_OTHERINSERT, false );
	propertySet.AddProperty( DBPROP_OTHERUPDATEDELETE, false );
	HRESULT hr = rs.Open( connection->session, query.str().c_str(), &propertySet );

	if( FAILED( hr ) )
	{
		return NULL;
	}

	// Count number of rows returned
	size_t rowCount = 0;

	hr = rs.MoveFirst();
	while ( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		++rowCount;
		hr = rs.MoveNext();
	}

	ret = new sqlSearchResults;
	ret->size = rowCount;

	ret->ids = new unsigned int[ rowCount ];
	ret->keys = new WCHAR*[ rowCount ];
	ret->texts = new WCHAR*[ rowCount ];

#define STRING_ID 1
#define STRING_TEXT 2
#define STRING_KEY 3

	unsigned int currentRow = 0;

	hr = rs.MoveFirst();
	while ( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		ret->ids[ currentRow ] = *(unsigned int*)rs.GetValue( STRING_ID );

		WCHAR* text = (WCHAR*) rs.GetValue( STRING_TEXT );
		size_t textSize = wcslen( text ) + 1;

		ret->texts[ currentRow ] = new WCHAR[ textSize ];
		wcscpy_s( ret->texts[ currentRow ], textSize, text );

		DBSTATUS status;

		rs.GetStatus( STRING_KEY, &status );

		if( status == DBSTATUS_S_ISNULL )
		{
			ret->keys[ currentRow ] = NULL;
		}
		else
		{
			WCHAR* key = (WCHAR*) rs.GetValue( STRING_KEY );
			size_t keySize = wcslen( key ) + 1;

			ret->keys[ currentRow ] = new WCHAR[ keySize ];
			wcscpy_s( ret->keys[ currentRow ], keySize, key );
		}

		++currentRow;
		hr = rs.MoveNext();
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}

DLL int sqlFreeSearchResults( sqlSearchResults* results )
{
	if( results != NULL )
	{
		delete results->ids;

		delete [] results->keys;

		delete [] results->texts;

		delete results;

		return 1;
	}

	return -1;
}
