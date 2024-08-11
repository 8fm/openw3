#pragma once

////////////////////////////////////////////////////////////////////////
// Class hierarchy mapper is data structure that after its created,
// lets you quickly iterate over given class subclasses.
struct SClassHierarchyMappingInfo
{
	CClass*			m_class;
	CClass*			m_parentClass;
	Int16			m_inheritanceLevel;
	mutable Int16	m_privateData;
	String			m_className;
	String			m_parentClassName;
	inline Bool operator<( const SClassHierarchyMappingInfo& c ) const;
};

class CClassHierarchyMapper : public TSortedArray< SClassHierarchyMappingInfo >
{
public:
	class CClassNaming
	{
	public:
		virtual void GetClassName( CClass* classId, String& outName ) const;
	};

	void GetRootClasses( iterator& itBeginOut, iterator& itEndOut );
	void GetDerivedClasses( const SClassHierarchyMappingInfo& classInfo, iterator& itBeginOut, iterator& itEndOut );

	static void MapHierarchy( CClass* baseClass, CClassHierarchyMapper& output, const CClassNaming& classNaming = CClassNaming(), Bool includeBaseClass = false );
};


////////////////////////////////////////////////////////////////////////
// CClassHierarchyContextMenuBuilder is template for creation
// of left-click context menu that selects a subclasses of given
// class. Consult use cases from beh tree editor.
struct CClassHierarchyContextMenuBuilderDefaultConfiguration
{
public:
	enum eConstants
	{
		BASE_ID = 0,
		PRECOMPUTE_AVAILABILITY = 0
	};
	Bool ClassIsAvailable( CClass* classId )
	{
		return !classId->IsAbstract();
	}
};

template < class Configurator >
class CClassHierarchyContextMenuBuilder
{
public:
	CClassHierarchyContextMenuBuilder( wxEvtHandler* me, CClassHierarchyMapper& mapper, const wxObjectEventFunction& fun, Configurator& conf )
		: m_me( me )
		, m_mapper( mapper )
		, m_command( fun )
		, m_configuration( conf ) {}

	Bool PrecomputeAvailability( CClassHierarchyMapper::iterator itBegin, CClassHierarchyMapper::iterator itEnd )
	{
		Bool available = false;
		for ( auto it = itBegin; it != itEnd; ++it )
		{
			CClassHierarchyMapper::iterator subitBegin;
			CClassHierarchyMapper::iterator subitEnd;
			m_mapper.GetDerivedClasses( *it, subitBegin, subitEnd );

			Bool childrenAvailable = false;
			if ( subitBegin != subitEnd )
			{
				childrenAvailable = PrecomputeAvailability( subitBegin, subitEnd ) || childrenAvailable;
			}
			it->m_privateData = childrenAvailable;
			available = available || childrenAvailable || m_configuration.ClassIsAvailable( it->m_class );
		}
		return available;
	}

	void SpawnMenus( wxMenu* menuCreate, wxMenu* menuMain )
	{
		auto itBegin = m_mapper.Begin();
		auto itEnd = m_mapper.End();
		m_mapper.GetRootClasses( itBegin, itEnd );
		if ( Configurator::PRECOMPUTE_AVAILABILITY )
		{
			PrecomputeAvailability( itBegin, itEnd );
		}
		SpawnMenus( itBegin, itEnd, menuCreate, menuMain );
	}
	void SpawnMenus( CClassHierarchyMapper::iterator itBegin, CClassHierarchyMapper::iterator itEnd, wxMenu* menuLevel, wxMenu* menuMain )
	{
		for ( auto it = itBegin; it != itEnd; ++it )
		{
			CClass* blockClass = it->m_class;

			CClassHierarchyMapper::iterator subitBegin;
			CClassHierarchyMapper::iterator subitEnd;
			m_mapper.GetDerivedClasses( *it, subitBegin, subitEnd );

			Uint32 id = Configurator::BASE_ID + ( it - m_mapper.Begin() );

			if ( m_configuration.ClassIsAvailable( blockClass ) )
			{
				wxMenuItem* item = menuLevel->Append( id, it->m_className.AsChar() );
				menuMain->Connect( id, wxEVT_COMMAND_MENU_SELECTED, m_command, NULL, m_me );
			}

			if ( subitBegin != subitEnd &&
				( !Configurator::PRECOMPUTE_AVAILABILITY || it->m_privateData ) )
			{
				// menu has submenu
				wxMenu* subMenu = new wxMenu;
				SpawnMenus( subitBegin, subitEnd, subMenu, menuMain );
				wxMenuItem* item = menuLevel->Append( id, wxString(it->m_className.AsChar()) + wxString( TXT(" subs") ), subMenu );
			}
		}
	}

	wxEvtHandler*					m_me;
	CClassHierarchyMapper&			m_mapper;
	wxObjectEventFunction			m_command;
	Configurator&					m_configuration;
};