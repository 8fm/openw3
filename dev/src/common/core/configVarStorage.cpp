/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scopedPtr.h"
#include "configVarStorage.h"
#include "fileSys.h"
#include "fileStringReader.h"
#include "fileStringWriter.h"

namespace Config
{
/******** Parse Helper ********/

Bool CParseHelper::IsIdent(const StringAnsi& str)
{
	Uint32 length = str.GetLength();

	if( length > 0 && IsCharNum( str[0] ) )		// Identifiers can't begin with number
		return false;

	for( Uint32 i=0; i<length; ++i )
	{
		if( IsCharAlphaNum( str[i] ) == false )
			return false;
	}

	return true;
}

Bool CParseHelper::IsString(const StringAnsi& str)
{
	if( IsNumber( str ) == true )
		return false;
	if( IsIdent( str ) == true )
		return false;

	return true;
}

Bool CParseHelper::IsNumber(const StringAnsi& str)
{
	Uint32 length = str.GetLength();

	Uint32 startIdx = 0;
	if( length > 0 && str[0] == '-' )		// First character can be minus
		startIdx++;

	for( Uint32 i=startIdx; i<length; ++i )
	{
		// Is not - is num or dot with previous character as num (so you can't type '.0')
		if( IsCharNum( str[i] ) == false && !(str[i] == '.' && i>0 && IsCharNum( str[i-1] )) )
			return false;
	}

	return true;
}

Bool CParseHelper::IsCharAlpha(const AnsiChar chr)
{
	if ( (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || ( chr == '_' ) )
	{
		return true;
	}

	return  false;
}

Bool CParseHelper::IsCharNum(const AnsiChar chr)
{
	if ( (chr >= '0' && chr <= '9') )
	{
		return true;
	}

	return  false;
}

Bool CParseHelper::IsCharAlphaNum(const AnsiChar chr)
{
	if ( (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9') || ( chr == '_' ) )
	{
		return true;
	}

	return  false;
}

/******** Storage ********/

CConfigVarStorage::CConfigVarStorage()
{
}


CConfigVarStorage::~CConfigVarStorage()
{
	m_groups.ClearPtr();
}

Bool CConfigVarStorage::GetEntry( const AnsiChar* groupName, const AnsiChar* varName, String& outValue ) const
{
	Group* group = nullptr;
	if ( m_groups.Find( CalcHash( groupName ), group ) )
	{
		const Entry* entry = group->m_entries.FindPtr( CalcHash( varName ) );
		if ( entry )
		{
			outValue = entry->m_value;
			return true;
		}
	}

	return false;
}

Bool CConfigVarStorage::SetEntry( const AnsiChar* groupName, const AnsiChar* varName, const String& value )
{
	// request group
	Group* group = nullptr;
	const Uint32 groupHash = CalcHash( groupName );
	if ( !m_groups.Find( groupHash, group ) )
	{
		group = new Group( groupName );
		m_groups.Insert( groupHash, group );
	}

	// request entry
	const Uint32 entryHash = CalcHash( varName );
	Entry* entry = group->m_entries.FindPtr( entryHash );
	if ( entry )
	{
		if ( entry->m_value != value )
		{
			entry->m_value = value;
			m_isModified = true;
		}
	}
	else
	{
		group->m_entries.Insert( entryHash, Entry( varName, value ) );
		m_isModified = true;
	}

	return true;
}

Bool CConfigVarStorage::RemoveGroup( const AnsiChar* groupName )
{
	return true;
}

Bool CConfigVarStorage::RemoveEntry( const AnsiChar* groupName, const AnsiChar* varName )
{
	return true;
}

Bool CConfigVarStorage::FilterDifferences( const CConfigVarStorage& base, CConfigVarStorage& outDifference ) const
{
	for ( auto it = m_groups.Begin(); it != m_groups.End(); ++it )
	{
		const auto& groupName = (*it).m_second->m_name;
		for ( auto jt = (*it).m_second->m_entries.Begin(); jt != (*it).m_second->m_entries.End(); ++jt )
		{
			const auto& varName = (*jt).m_second.m_name;
			const auto& varValue = (*jt).m_second.m_value;

			// get base value
			String baseValue;
			if ( base.GetEntry( groupName.AsChar(), varName.AsChar(), baseValue ) )
			{
				// compare values
				if ( baseValue == varValue )
					continue;
			}

			// store in the difference mapping
			outDifference.SetEntry( groupName.AsChar(), varName.AsChar(), varValue );
		}
	}

	// done
	return true;
}

Bool CConfigVarStorage::Load( const String& absoluteFilePath )
{
	String contentUnicode;
	Bool result = GFileManager->LoadFileToString( absoluteFilePath, contentUnicode, true );
	if( result == false )
		return false;

	StringAnsi contentAnsi = UNICODE_TO_ANSI( contentUnicode.AsChar() );
	result = LoadFromString( contentAnsi );
	if( result == false )
	{
		ERR_CORE( TXT("Parsing ini file failed: %ls"), absoluteFilePath.AsChar() );
		return false;
	}

	// done
	return true;
}

Bool CConfigVarStorage::Save( const String& absoluteFilePath )
{
	// Save configs to string
	StringAnsi contentAnsi = "";
	if( SaveToString( contentAnsi ) == false )
		return false;

	String contentUnicode = ANSI_TO_UNICODE( contentAnsi.AsChar() );

	Bool result = GFileManager->SaveStringToFile( absoluteFilePath, contentUnicode );
	if( result == false )
		return false;

	return true;
}

Bool CConfigVarStorage::LoadFromString(const StringAnsi& content)
{
	// Create text reader (simple parser)
	Red::CAnsiStringFileReader reader( content );

	while ( !reader.EndOfFile() )
	{
		// parse comment
		if ( reader.ParseKeyword( ";" ) )
		{
			reader.SkipCurrentLine();
			continue;
		}

		// parse section header
		if ( !reader.ParseKeyword( "[") )
		{
			ERR_CORE( TXT("Parsing ini file: Expected '[' at line %d"), reader.GetLine() );
			return false;
		}

		// parse group name
		StringAnsi groupName;
		if ( !reader.ParseIdent( groupName ) )
		{
			ERR_CORE( TXT("Parsing ini file: Expected group name at line %d"), reader.GetLine() );
			return false;
		}

		// parse section tail
		if ( !reader.ParseKeyword( "]") )
		{
			ERR_CORE( TXT("Parsing ini file: Expected ']' at line %d in section '%ls'"), reader.GetLine(), ANSI_TO_UNICODE( groupName.AsChar() ) );
			return false;
		}

		// create the group
		Group* group = nullptr;
		const TNameHash groupHash = CalcHash( groupName );
		if ( !m_groups.Find( groupHash, group ) )
		{
			group = new Group( groupName.AsChar() );
			m_groups.Insert( groupHash, group );
		}		

		// read the options
		while ( !reader.EndOfFile() )
		{
			// parse comment
			if ( reader.ParseKeyword( ";" ) )
			{
				reader.SkipCurrentLine();
				continue;
			}

			// read the entry name
			StringAnsi entryName;
			if ( !reader.ParseIdent( entryName ) )
			{
				// end of the list
				break;
			}

			// parse the 'equal' sign
			if ( !reader.ParseKeyword( "=") )
			{
				ERR_CORE( TXT("Parsing ini file: Expected '=' at line %d in section '%ls', entry '%ls'"), 
					reader.GetLine(), 
					ANSI_TO_UNICODE( groupName.AsChar() ), ANSI_TO_UNICODE( entryName.AsChar() ) );
				return false;
			}

			// parse the value - as a token, should be in the same line
			StringAnsi entryValue;
			reader.ParseToken( entryValue, /* line break */ false );

			// find existing entry of create new one
			const TNameHash nameHash = CalcHash( entryName );
			Entry* entry = group->m_entries.FindPtr( nameHash );
			if ( entry )
			{
				if ( entry->m_name != entryName )
				{
					ERR_CORE( TXT("Parsing ini file: Hash collision between '%ls' and '%ls' at line %d in section '%ls'"), 
						ANSI_TO_UNICODE( entryName.AsChar() ), ANSI_TO_UNICODE( entry->m_name.AsChar() ),
						reader.GetLine(), ANSI_TO_UNICODE( groupName.AsChar() ) );
					return false;
				}

				// TODO: the whole engine is suffering from String vs StringAnsi problem...
				entry->m_value = ANSI_TO_UNICODE( entryValue.AsChar() );
			}
			else
			{
				// Create new entry
				group->m_entries.Insert( nameHash, Entry( entryName.AsChar(), ANSI_TO_UNICODE( entryValue.AsChar() ) ) );
			}
		}
	}
	
	return true;
}

Bool CConfigVarStorage::SaveToString(StringAnsi& output) const
{
	// process groups - the save order is stable (because we use TSortedMap)
	// do not save empty groups
	for ( auto it = m_groups.Begin(); it != m_groups.End(); ++it )
	{
		const Group* group = (*it).m_second;

		// empty ?
		if ( group->m_entries.Empty() )
			continue;

		// header
		output += StringAnsi::Printf( "[%s]\r\n", group->m_name.AsChar() );

		// values
		for ( auto jt = group->m_entries.Begin(); jt != group->m_entries.End(); ++jt )
		{
			const Entry& entry = (*jt).m_second;
			if ( !entry.m_name.Empty() )
			{
				StringAnsi ansiValue = UNICODE_TO_ANSI( entry.m_value.AsChar() );
				if( CParseHelper::IsString( ansiValue ) == true )
				{
					// Save as string, within quotation characters
					ansiValue = "\"" + ansiValue + "\"";
					output += StringAnsi::Printf( "%s=%s\r\n", entry.m_name.AsChar(), ansiValue.AsChar() );
				}
				else
				{
					// Save as number or identifier, without quotation characters
					output += StringAnsi::Printf( "%s=%s\r\n", entry.m_name.AsChar(), ansiValue.AsChar() );
				}
			}
		}
	}

	// saved
	return true;
}

CConfigVarStorage::TNameHash CConfigVarStorage::CalcHash( const AnsiChar* text )
{
	return Red::CalculateHash32( text );
}

CConfigVarStorage::TNameHash CConfigVarStorage::CalcHash( const StringAnsi& text )
{
	return Red::CalculateHash32( text.AsChar() );
}

void CConfigVarStorage::Clear()
{
	m_groups.Clear();
	m_isModified = true;
}

} // Console
