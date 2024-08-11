/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sortedmap.h"

/// Storage helper for config vars
namespace Config
{
	class CParseHelper
	{
	public:
		static Bool IsIdent( const StringAnsi& str );
		static Bool IsNumber( const StringAnsi& str );
		static Bool IsString( const StringAnsi& str );
		static Bool IsCharAlpha( const AnsiChar chr );
		static Bool IsCharNum( const AnsiChar chr );
		static Bool IsCharAlphaNum( const AnsiChar chr );
	};

	/// Saving/Loading of console config variables
	class CConfigVarStorage
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	public:

		typedef Uint32 TNameHash;

		struct Entry
		{
			StringAnsi		m_name;
			String			m_value;

			RED_FORCE_INLINE Entry( const AnsiChar* name = "", const String& value = String::EMPTY )
				: m_name( name )
				, m_value( value )
			{}

			RED_FORCE_INLINE Entry( const Entry& other )
				: m_name( other.m_name )
				, m_value( other.m_value )
			{}

			RED_FORCE_INLINE Entry( Entry&& other )
			{
				m_name = Move( other.m_name );
				m_value = Move( other.m_value );
			}

			RED_FORCE_INLINE Entry& operator=( const Entry& other )
			{
				if ( this != &other )
				{
					m_name = other.m_name;
					m_value = other.m_value;
				}
				return *this;
			}

			RED_FORCE_INLINE Entry& operator=( Entry&& other )
			{
				if ( this != &other )
				{
					m_name = Move( other.m_name );
					m_value = Move( other.m_value );
				}
				return *this;
			}
		};

		struct Group
		{
			DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
		public:

			StringAnsi							m_name;
			TSortedMap< TNameHash, Entry >		m_entries;

			Group( const AnsiChar* name )
				: m_name( name )
			{}
		};


		CConfigVarStorage();
		~CConfigVarStorage();

		// find entry for given group/var 
		Bool GetEntry( const AnsiChar* groupName, const AnsiChar* varName, String& outValue ) const;

		// set entry for given group/vars
		Bool SetEntry( const AnsiChar* groupName, const AnsiChar* varName, const String& value );

		// remove group
		Bool RemoveGroup( const AnsiChar* groupName );

		// remove entry in group
		Bool RemoveEntry( const AnsiChar* groupName, const AnsiChar* varName );

		// clear all
		void Clear();

		// filter the values from this group using a given base
		Bool FilterDifferences( const CConfigVarStorage& base, CConfigVarStorage& outDifference ) const;

		// load settings from file
		Bool Load( const String& absoluteFilePath );

		// save settings to file, filter out settings that are the same as in base
		Bool Save( const String& absoluteFilePath );

		// load settings from string, filter out settings that are the same as in base
		Bool LoadFromString( const StringAnsi& content );

		// save settings to string, filter out settings that are the same as in base
		Bool SaveToString( StringAnsi& output ) const;

		// is this storage modified ?
		Bool IsModified() const;

	private:
		
		typedef TSortedMap< TNameHash, Group* >		TGroups;
		TGroups		m_groups;
		Bool		m_isModified;

		static TNameHash CalcHash( const AnsiChar* text );
		static TNameHash CalcHash( const StringAnsi& text );
	};

} // Console