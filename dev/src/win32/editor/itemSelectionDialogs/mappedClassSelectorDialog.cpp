#include "build.h"
#include "mappedClassSelectorDialog.h"

void CEdMappedClassSelectorDialog::Populate()
{
	for( Uint32 i = 0, n = m_hierarchy.Size(); i < n; ++i )
	{
		CClass* currClass = m_hierarchy[ i ].m_class;
		Bool isSelected = m_defaultSelected == currClass;
		Bool isSelectable = IsSelectable( currClass );

		if( m_hierarchy[ i ].m_parentClass == m_root )
		{
			AddItem( m_hierarchy[ i ].m_className, currClass, isSelectable, -1, isSelected );
		}
		else
		{
			AddItem( m_hierarchy[ i ].m_className, currClass, m_hierarchy[ i ].m_parentClassName, isSelectable, -1, isSelected );
		}
	}
}

Bool CEdMappedClassSelectorDialog::IsSelectable( CClass* classId ) const
{
	return !classId->IsAbstract();
}