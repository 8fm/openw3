/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace Config
{
	/// Helper class - holds a hierarchy of IConfigVars in their groups, used for easier browsing
	class CConfigVarHierarchy
	{
	public:
		class Group;

		class Entry
		{
		public:
			IConfigVar*			m_var;
			Group*				m_group;

			Entry( IConfigVar* var, Group* group );
		};

		class Group
		{
		public:
			Group*								m_parent;
			StringAnsi							m_name;
			TDynArray< Group*, MC_Temporary >	m_children;
			TDynArray< Entry*, MC_Temporary >	m_entries;

		public:
			Group( Group* parent, const StringAnsi& name );
			~Group();

			void Sort();
		};

		RED_INLINE Group* GetRoot() const { return m_root; }

		CConfigVarHierarchy();
		~CConfigVarHierarchy();

		void Reset();

	private:
		Group*		m_root;
	};

	/// Helper class - builds a hierarchy of config vars in their groups for easier browsing
	class CConfigVarHierarchyBuilder
	{
	public:
		CConfigVarHierarchyBuilder( CConfigVarHierarchy& outHierarchy );
		void BuildFromRegistry( const class CConfigVarRegistry& registry, const AnsiChar* nameFilter="", const Uint32 includedFlags = 0, const Uint32 excludedFlags = 0 );

	private:
		CConfigVarHierarchy*		m_hierarchy;

		CConfigVarHierarchy::Group* GetGroup( CConfigVarHierarchy::Group* parent, const StringAnsi& name );
	};

} // Config