#include "build.h"

#include "classHierarchyMapper.h"


Bool SClassHierarchyMappingInfo::operator<( const SClassHierarchyMappingInfo& c ) const
{
	return
		m_inheritanceLevel < c.m_inheritanceLevel ? true :
		m_inheritanceLevel > c.m_inheritanceLevel ? false :
		m_parentClass < c.m_parentClass ? true :
		m_parentClass > c.m_parentClass ? false :
		m_className < c.m_className;
}
void CClassHierarchyMapper::CClassNaming::GetClassName( CClass* classId, String& outName ) const
{
	outName = classId->GetName().AsString();
}

void CClassHierarchyMapper::GetRootClasses( iterator& itBeginOut, iterator& itEndOut )
{
	itBeginOut = Begin();
	iterator it = itBeginOut;
	for ( iterator end = End(); it != end && it->m_inheritanceLevel == 0; ++it )
	{}
	itEndOut = it;
}
void CClassHierarchyMapper::GetDerivedClasses( const SClassHierarchyMappingInfo& classInfo, iterator& itBeginOut, iterator& itEndOut )
{
	SClassHierarchyMappingInfo checkInfo;
	checkInfo.m_class = 0;
	checkInfo.m_parentClass = classInfo.m_class;
	checkInfo.m_inheritanceLevel = classInfo.m_inheritanceLevel + 1;
	auto it = UpperBound( Begin(), End(), checkInfo );
	if ( it != End() && it->m_parentClass == classInfo.m_class )
	{
		itBeginOut = it;
		for ( iterator end = End(); it != end && it->m_parentClass == classInfo.m_class; ++it )
		{}
		itEndOut = it;
	}
	else
	{
		itBeginOut = End();
		itEndOut = End();
	}
}
void CClassHierarchyMapper::MapHierarchy( CClass* baseClass, CClassHierarchyMapper& output, const CClassNaming& classNaming, Bool includeBaseClass /*= false*/ )
{
	// I could use EnumDerivedClasses but looking at the code that might do all my computations extremally slow
	// insteady my algorithm takes all derived classes, their direct parents, and then marks them out level after level until they are done
	TDynArray< CClass* > allClasses;

	// read all derived classes
	SRTTI::GetInstance().EnumClasses( baseClass, allClasses, NULL, true );

	if ( includeBaseClass == false )
	{
		// Make sure the specified base class is not included in the output
		allClasses.Remove( baseClass );
	}
	Uint32 classesCount = allClasses.Size();


	// sort the list so we can find pointer to a class in log(n) time (will be needed later on)
	struct PredAllClasses
	{
		Bool operator()( CClass* c1, CClass* c2 ) const
		{
			return c1 < c2;
		}
	} predAllClasses;
	::Sort( allClasses.Begin(), allClasses.End(), predAllClasses );
	
	// start filling up the output
	output.Resize( classesCount );
	Uint32 processedClasses = 0;
	for ( Uint32 i = 0; i != classesCount; ++i )
	{
		output[ i ].m_class = allClasses[ i ];
		output[ i ].m_parentClass = allClasses[ i ]->GetBaseClass();

		classNaming.GetClassName( output[ i ].m_class, output[ i ].m_className );
		classNaming.GetClassName( output[ i ].m_parentClass, output[ i ].m_parentClassName );

		// initial test
		if ( output[ i ].m_class == baseClass )
		{
			output[ i ].m_inheritanceLevel = 0;
			++processedClasses;
		}
		else if ( output[ i ].m_parentClass == baseClass )
		{
			output[ i ].m_inheritanceLevel = 1;
			++processedClasses;
		}
		else
		{
			output[ i ].m_inheritanceLevel = -1;
		}
	}

	// calculate inheritance level 
	while ( processedClasses < classesCount )
	{
		// process all classes
		for ( Uint32 i = 0; i != classesCount; ++i )
		{
			// look for "unset" ones
			if ( output[ i ].m_inheritanceLevel == -1 )
			{
				// check if their parent classes was set
				auto it = ::LowerBound( allClasses.Begin(), allClasses.End(), output[ i ].m_parentClass, predAllClasses );
				// find parent class index in 'allClasses', because it still has same order as 'output'
				ASSERT( it != allClasses.End() && *it == output[ i ].m_parentClass );
				
				Int32 parentIndex = it - allClasses.Begin();
				// check if parent class inheritance level was allready computed
				if ( output[ parentIndex ].m_inheritanceLevel != -1 )
				{
					output[ i ].m_inheritanceLevel = output[ parentIndex ].m_inheritanceLevel + 1;
					++processedClasses;
				}
				
				
			}
		}
	}

	// all is set - sort the output
	output.Sort();
}
