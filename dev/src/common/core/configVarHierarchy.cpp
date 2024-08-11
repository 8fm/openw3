/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "configVar.h"
#include "configVarRegistry.h"
#include "configVarHierarchy.h"

namespace Config
{

	//------

	CConfigVarHierarchy::Entry::Entry( IConfigVar* var, Group* group )
		: m_var( var )
		, m_group( group )
	{
		RED_FATAL_ASSERT( var != nullptr, "Variable should not be empty" );
		RED_FATAL_ASSERT( group != nullptr, "Group should not be empty" );
	}

	CConfigVarHierarchy::Group::Group( Group* parent, const StringAnsi& name )
		: m_parent( parent )
		, m_name( name )
	{
		if ( parent )
			parent->m_children.PushBack( this );
	}

	CConfigVarHierarchy::Group::~Group()
	{
		m_children.ClearPtr();
		m_entries.ClearPtr();
	}

	void CConfigVarHierarchy::Group::Sort()
	{
		::Sort( m_entries.Begin(), m_entries.End(), []( Entry* a, Entry* b ) { return Red::StringCompareNoCase( a->m_var->GetName(), b->m_var->GetName() ) < 0; } );
		::Sort( m_children.Begin(), m_children.End(), []( Group* a, Group* b ) { return Red::StringCompareNoCase( a->m_name.AsChar(), b->m_name.AsChar() ) < 0; } );

		for ( auto child : m_children )
			child->Sort();
	}

	CConfigVarHierarchy::CConfigVarHierarchy()
	{
		m_root = new Group( nullptr, "ConfigRoot" );
	}

	CConfigVarHierarchy::~CConfigVarHierarchy()
	{
		Reset();
	}

	void CConfigVarHierarchy::Reset()
	{
		if ( m_root )
		{
			delete m_root;
			m_root = nullptr;
		}
	}

	//------

	CConfigVarHierarchyBuilder::CConfigVarHierarchyBuilder( CConfigVarHierarchy& outHierarchy )
		: m_hierarchy( &outHierarchy )
	{
	}

	void CConfigVarHierarchyBuilder::BuildFromRegistry( const class CConfigVarRegistry& registry, const AnsiChar* nameFilter/*=nullptr*/, const Uint32 includedFlags /*= 0*/, const Uint32 excludedFlags /*= 0*/ )
	{
		// get all entries from the registry
		TDynArray< IConfigVar* > allVars;
		registry.EnumVars( allVars, "", nameFilter, includedFlags, excludedFlags );

		// build the groups
		for ( IConfigVar* var : allVars )
		{
			const AnsiChar* groupName = var->GetGroup();

			// split the path and create the directories
			CConfigVarHierarchy::Group* curGroup = m_hierarchy->GetRoot();
			const AnsiChar* start = groupName;
			const AnsiChar* cur = groupName;
			do
			{
				if ( *cur == '/' || *cur == 0 )
				{
					const Uint32 length = (const Uint32) (cur - start);
					if ( !length )
					{
						curGroup = nullptr;
						break;
					}

					// create sub directory
					const StringAnsi groupPartName( start, length );
					curGroup = GetGroup( curGroup, groupPartName );
					if ( !curGroup )
						break;

					// start new segment
					start = cur+1;
				}
			}
			while ( *cur++ );

			// no group created - probably invalid path or sth
			RED_ASSERT( curGroup != nullptr, TXT("Invalid config group for var '%ls' in group '%ls'"), ANSI_TO_UNICODE(var->GetName()), ANSI_TO_UNICODE(var->GetGroup()) );
			if ( curGroup == nullptr )
				continue;

			// add the entry to the group
			CConfigVarHierarchy::Entry* entry = new CConfigVarHierarchy::Entry( var, curGroup );
			curGroup->m_entries.PushBack( entry );
		}

		// sort the lists
		m_hierarchy->GetRoot()->Sort();
	}

	CConfigVarHierarchy::Group* CConfigVarHierarchyBuilder::GetGroup( CConfigVarHierarchy::Group* parent, const StringAnsi& name )
	{
		for ( CConfigVarHierarchy::Group* child : parent->m_children )
		{
			if ( child->m_name == name )
				return child;
		}

		return new CConfigVarHierarchy::Group( parent, name ); // this auto registeres in the parent->m_children list
	}

} // Config
