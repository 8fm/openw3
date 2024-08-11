/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __JSON_FILE_H__
#define __JSON_FILE_H__

// TODO: remove this nasty dependency on JSON crap
#include "../../../external/rapidjson/include/rapidjson/prettywriter.h"
#include "../../../external/rapidjson/include/rapidjson/reader.h"
#include "../../../external/rapidjson/include/rapidjson/filestream.h"
#include "../../../external/rapidjson/include/rapidjson/document.h"

// JSON based file crap class - move to Core when become a burden
class JSONFileHelper
{
public:
	// stream out writer for JSON
	template< Uint32 BUF_SIZE >
	class JSONStreamWriter
	{
	public:
		JSONStreamWriter(IFile* file)
			: m_file(file)
			, m_pos(0)
		{}

		~JSONStreamWriter()
		{
			Flush();
		}

		void Flush()
		{
			if ( m_pos > 0 )
			{
				m_file->Serialize( m_buf, m_pos );
				m_pos = 0;
			}
		}

		void Put(const AnsiChar ch)
		{
			if (m_pos == BUF_SIZE)
				Flush();

			m_buf[ m_pos++ ] = ch;
		}

	private:
		IFile*		m_file;

		AnsiChar	m_buf[ BUF_SIZE ];
		Uint32		m_pos;
	};

	// TODO: integrate our allocator if needed
	typedef rapidjson::GenericDocument< rapidjson::UTF8< Red::System::AnsiChar > > JSONDocument;
	typedef rapidjson::GenericValue< rapidjson::UTF8< Red::System::AnsiChar > > JSONValue;

	// TODO: move the writer interface to Core
	typedef JSONStreamWriter< 4096 > JSONFileStream;
	typedef rapidjson::PrettyWriter< JSONFileStream > JSONWriter;

	static StringAnsi GetAttrStr( const JSONValue& val, const AnsiChar* txt, const AnsiChar* defaultText = "" )
	{
		if ( !val.HasMember(txt) )
			return defaultText;

		const JSONValue& member = val[txt];
		if ( !member.IsString() )
			return defaultText;

		return member.GetString();
	}

	static CName GetAttrName( const JSONValue& val, const AnsiChar* txt, const CName defaultText = CName() )
	{
		if ( !val.HasMember(txt) )
			return defaultText;

		const JSONValue& member = val[txt];
		if ( !member.IsString() )
			return defaultText;

		return CName( ANSI_TO_UNICODE( member.GetString() ) );
	}

	static TDynArray< CName > GetAttrNameArray( const JSONValue& val, const AnsiChar* txt )
	{
		TDynArray< CName > ret;

		if ( !val.HasMember(txt) )
			return ret;

		const JSONValue& member = val[txt];
		if ( !member.IsArray() )
			return ret;

		const Uint32 count = member.Size();
		for ( auto it = member.Begin(); it != member.End(); ++it )
		{
			const JSONValue& arrayMember = (*it);
			if ( arrayMember.IsString() )
			{
				CName name( ANSI_TO_UNICODE( arrayMember.GetString() ) );
				if ( name )
				{
					ret.PushBack( name );
				}
			}
		}

		return ret;
	}

	static Int32 GetAttrInt( const JSONValue& val, const AnsiChar* txt, const Int32 defaultValue = 0 )
	{
		if ( !val.HasMember(txt) )
			return defaultValue;

		const JSONValue& member = val[txt];
		if ( !member.IsInt() )
			return defaultValue;

		return member.GetInt();
	}

	static Float GetAttrFloat( const JSONValue& val, const AnsiChar* txt, const Float defaultValue = 0.0 )
	{
		if ( !val.HasMember(txt) )
			return defaultValue;

		const JSONValue& member = val[txt];
		if ( !member.IsDouble() )
			return defaultValue;

		return (Float)member.GetDouble();
	}

	static Double GetAttrDouble( const JSONValue& val, const AnsiChar* txt, const Double defaultValue = 0.0 )
	{
		if ( !val.HasMember(txt) )
			return defaultValue;

		const JSONValue& member = val[txt];
		if ( !member.IsDouble() )
			return defaultValue;

		return member.GetDouble();
	}

};

#endif // __JSON_FILE_H__