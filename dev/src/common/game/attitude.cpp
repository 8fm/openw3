/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../core/gatheredResource.h"
#include "../core/depot.h"
#include "../core/xmlFileReader.h"
#include "../core/xmlWriter.h"

IMPLEMENT_RTTI_ENUM( EAIAttitude );
IMPLEMENT_ENGINE_CLASS( SAIAttitudeDummy );

const Uint32 CAttitudes::WRONG_INDEX = NumericLimits< Uint32 >::Max();

CGatheredResource resAttitudeGroups( ATTITUDE_GROUPS_CSV, RGF_Startup );

EAIAttitude CAttitudes::GetDefaultAttitude()
{
	return AIA_Friendly;
}

EAIAttitude CAttitudes::AttitudeFromString( const String& attitude )
{
	if ( attitude == ToString( AIA_Friendly ) )
	{
		return AIA_Friendly;
	}
	else if ( attitude == ToString( AIA_Neutral ) )
	{
		return AIA_Neutral;
	}
	else if ( attitude == ToString( AIA_Hostile ) )
	{
		return AIA_Hostile;
	}
	else
	{
		ASSERT( !TXT("AttitudeFromString(): Unknown attitude") );
		return GetDefaultAttitude();
	}
}

Bool CAttitudes::SetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude, bool updateTree )
{
	if ( srcGroup == dstGroup )
	{
		return false;
	}

	AttitudeGroup* pSrcGroup = m_attitudeGroups.FindPtr( srcGroup );
	AttitudeGroup* pDstGroup = m_attitudeGroups.FindPtr( dstGroup );
	if ( pSrcGroup == NULL || pDstGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), srcGroup.AsString().AsChar(), dstGroup.AsString().AsChar() );
		return false;
	}

	Uint32 index = GetCacheIndex( pSrcGroup->m_index, pDstGroup->m_index );
	m_attitudes.Set( index, (Uint32)attitude );
	m_isCustomAttitude.Set( index, 1 );
	if ( updateTree )
	{
		UpdateChildrenAttitude( pSrcGroup, pDstGroup, attitude );
		UpdateChildrenAttitude( pDstGroup, pSrcGroup, attitude );
	}
	return true;
}

Bool CAttitudes::GetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude /* out */, Bool& isCustom /* out */ ) const
{
	if( srcGroup == dstGroup )
	{
		attitude = AIA_Friendly;
		isCustom = false;
		return true;
	}

	Uint32 index = GetCacheIndex( srcGroup, dstGroup );
	if ( index == WRONG_INDEX )
	{
		attitude = GetDefaultAttitude();
		isCustom = false;
		return false;
	}

	attitude = (EAIAttitude)m_attitudes.Get( index );
	isCustom = m_isCustomAttitude.Get( index ) > 0;
	return true;
}

Bool CAttitudes::GetAttitudeWithParents( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude /* out */, Bool& isCustom /* out */,
										 CName& srcGroupParent /* out */, CName& dstGroupParent /* out */ ) const
{
	if( srcGroup == dstGroup )
	{
		attitude = AIA_Friendly;
		isCustom = false;
		srcGroupParent = dstGroupParent = srcGroup;
		return false;
	}

	Uint32 index = GetCacheIndex( srcGroup, dstGroup );
	if ( index == WRONG_INDEX )
	{
		attitude = GetDefaultAttitude();
		isCustom = false;
		srcGroupParent = srcGroup;
		dstGroupParent = dstGroup;
		return false;
	}

	attitude = (EAIAttitude)m_attitudes.Get( index );
	isCustom = m_isCustomAttitude.Get( index ) > 0;
	if ( isCustom )
	{
		srcGroupParent = srcGroup;
		dstGroupParent = dstGroup;
	}
	else
	{
		AttitudeGroup* pSrcGroupParent = NULL;
		AttitudeGroup* pDstGroupParent = NULL;
		if ( GetInheritanceRoot( srcGroup, dstGroup, pSrcGroupParent, pDstGroupParent ) )
		{
			srcGroupParent = pSrcGroupParent->m_name;
			dstGroupParent = pDstGroupParent->m_name;
		}
		else
		{
			srcGroupParent = CName::NONE;
			dstGroupParent = CName::NONE;
		}
	}
	return true;
}

Uint32 CAttitudes::GetAttitudes( TDynArray< CName > & srcGroups /* out */, TDynArray< CName > & dstGroups /* out */, TDynArray< EAIAttitude > & attitudes /* out */ ) const
{
	ASSERT( srcGroups.Size() == dstGroups.Size() );
	ASSERT( srcGroups.Size() == attitudes.Size() );

	for ( THashMap< CName, AttitudeGroup >::const_iterator i = m_attitudeGroups.Begin(); i != m_attitudeGroups.End(); ++i)
	{
		THashMap< CName, AttitudeGroup >::const_iterator j = i;
		++j;
		for ( ; j != m_attitudeGroups.End(); ++j )
		{
			Uint32 index = GetCacheIndex( i->m_second.m_index, j->m_second.m_index );
			if ( m_isCustomAttitude.Get( index ) )
			{
				srcGroups.PushBack( i->m_first );
				dstGroups.PushBack( j->m_first );
				attitudes.PushBack( (EAIAttitude)m_attitudes.Get( index ) );
			}
		}
	}
	return attitudes.Size();
}

Bool CAttitudes::RemoveAttitude( const CName& srcGroup, const CName& dstGroup, bool updateTree )
{
	if ( srcGroup == dstGroup )
	{
		return false;
	}

	AttitudeGroup* pSrcGroup = m_attitudeGroups.FindPtr( srcGroup );
	AttitudeGroup* pDstGroup = m_attitudeGroups.FindPtr( dstGroup );
	if ( pSrcGroup == NULL || pDstGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), srcGroup.AsString().AsChar(), dstGroup.AsString().AsChar() );
		return false;
	}

	Uint32 index = GetCacheIndex( pSrcGroup->m_index, pDstGroup->m_index );
	m_isCustomAttitude.Set( index, 0 );
	AttitudeGroup* pSrcGroupParent = NULL;
	AttitudeGroup* pDstGroupParent = NULL;
	EAIAttitude newAttitude = GetDefaultAttitude();
	if ( GetInheritanceRoot( srcGroup, dstGroup, pSrcGroupParent, pDstGroupParent ) )
	{
		// if one group is parent to another, they're friendly to each other by default
		if ( pSrcGroup == pDstGroupParent || pDstGroup == pSrcGroupParent )
		{
			newAttitude = AIA_Friendly;
		}
		else
		{
			newAttitude = (EAIAttitude)m_attitudes.Get( GetCacheIndex( pSrcGroupParent->m_index, pDstGroupParent->m_index ) );
		}
	}
	m_attitudes.Set( index, newAttitude );
	if ( updateTree )
	{
		UpdateChildrenAttitude( pSrcGroup, pDstGroup, newAttitude);
		UpdateChildrenAttitude( pDstGroup, pSrcGroup, newAttitude);
	}
	return true;
}

void CAttitudes::ClearAllAttitudes()
{
	m_attitudes.Clear();
	m_isCustomAttitude.Clear();
	m_attitudeGroups.Clear();
}

Bool CAttitudes::IsCustomAttitude( const CName& srcGroup, const CName& dstGroup ) const
{
	Uint32 index = GetCacheIndex( srcGroup, dstGroup );
	if ( index == WRONG_INDEX )
	{
		return false;
	}
	return m_isCustomAttitude.Get( index ) > 0;
}

Uint32 CAttitudes::GetAttitudeGroups( TDynArray< CName > & attitudeGroups )
{
	attitudeGroups.Resize( m_attitudeGroups.Size() );
	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		attitudeGroups[ it->m_second.m_index ] = it->m_first;
	}
	return attitudeGroups.Size();
}

Bool CAttitudes::AttitudeGroupExists( const CName& group ) const
{
	return m_attitudeGroups.FindPtr( group ) != NULL;
}

Bool CAttitudes::GetParentForGroup( const CName& group, CName& parent ) const
{
	const AttitudeGroup* pGroup = m_attitudeGroups.FindPtr( group );
	if ( pGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find %s in attitude groups list"), group.AsString().AsChar() );
		parent = CName::NONE;
		return false;
	}

	if ( pGroup->m_parent != NULL )
	{
		parent = pGroup->m_parent->m_name;
		return true;
	}
	else
	{
		parent = CName::NONE;
		return false;
	}
}

Bool CAttitudes::SetParentForGroup( const CName& group, const CName& parent, bool updateTree )
{
	AttitudeGroup* pGroup = m_attitudeGroups.FindPtr( group );
	AttitudeGroup* pParent = m_attitudeGroups.FindPtr( parent );
	if ( pGroup == NULL || pParent == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), group.AsString().AsChar(), parent.AsString().AsChar() );
		return false;
	}

	if ( pGroup->m_parent != NULL )
	{
		pGroup->m_parent->m_children.Remove( pGroup );
	}
	pGroup->m_parent = pParent;
	pParent->m_children.PushBack( pGroup );

	if ( updateTree )
	{
		CopyInheritedAttitudes( pGroup, pParent );
		UpdateChildrenAttitudes( pGroup );
	}

	return true;
}

Bool CAttitudes::RemoveParentForGroup( const CName& group, bool updateTree )
{
	AttitudeGroup* pGroup = m_attitudeGroups.FindPtr( group );
	if ( pGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find %s in attitude groups list"), group.AsString().AsChar() );
		return false;
	}

	if ( pGroup->m_parent != NULL )
	{
		pGroup->m_parent->m_children.Remove( pGroup );
		pGroup->m_parent = NULL;
	}

	if ( updateTree )
	{
		ResetInheritedAttributes( pGroup );
		ResetChildrenAttitudes( pGroup );
	}

	return true;
}

Uint32 CAttitudes::GetParentGroups( TDynArray< CName > & groups, TDynArray< CName > & parents ) const
{
	ASSERT( groups.Size() == parents.Size() );

	for ( THashMap< CName, AttitudeGroup >::const_iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( it->m_second.m_parent != NULL )
		{
			groups.PushBack( it->m_first );
			parents.PushBack( it->m_second.m_parent->m_name );
		}
	}
	return groups.Size();
}

Bool CAttitudes::IsParentForGroup( const CName& child, const CName& parent ) const
{
	if ( child == parent )
	{
		return false;
	}

	const AttitudeGroup* pChild = m_attitudeGroups.FindPtr( child );
	const AttitudeGroup* pParent = m_attitudeGroups.FindPtr( parent );
	if ( pChild == NULL || pParent == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), child.AsString().AsChar(), parent.AsString().AsChar() );
		return false;
	}

	return IsParentForGroup( pChild, pParent );
}

Bool CAttitudes::IsParentForGroup( const AttitudeGroup* child, const AttitudeGroup* parent ) const
{
	ASSERT( child != NULL && parent != NULL );

	if ( child == parent )
	{
		return false;
	}
	while ( child != NULL )
	{
		if ( child->m_parent == parent)
		{
			return true;
		}
		child = child->m_parent;
	}
	return false;
}

Bool CAttitudes::GetAllParents( const CName& group, TDynArray< CName > & parents ) const
{
	const AttitudeGroup* pGroup = m_attitudeGroups.FindPtr( group );
	if ( pGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find %s in attitude groups list"), group.AsString().AsChar() );
		return false;
	}

	TDynArray< AttitudeGroup* > tmpParents;
	GetAllParents( pGroup, tmpParents );
	for ( TDynArray< AttitudeGroup* >::iterator it = tmpParents.Begin(); it != tmpParents.End(); ++it )
	{
		parents.PushBack( (*it)->m_name );
	}
	return parents.Size() > 0;
}

Bool CAttitudes::GetAllParents( const AttitudeGroup* group, TDynArray< AttitudeGroup* > & parents ) const
{
	ASSERT( group != NULL );

	AttitudeGroup* parent = group->m_parent;
	while ( parent != NULL )
	{
		parents.PushBack( parent );
		parent = parent->m_parent;
	}
	return parents.Size() > 0;
}

Bool CAttitudes::GetAllChildren( const CName& group, TDynArray< CName > & children ) const
{
	const AttitudeGroup* pGroup = m_attitudeGroups.FindPtr( group );
	if ( pGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find %s in attitude groups list"), group.AsString().AsChar() );
		return false;
	}

	TDynArray< const AttitudeGroup* > tmpChildren;
	GetAllChildren( pGroup, tmpChildren );
	for ( TDynArray< const AttitudeGroup* >::iterator it = tmpChildren.Begin(); it != tmpChildren.End(); ++it )
	{
		children.PushBack( (*it)->m_name );
	}
	return children.Size() > 0;
}

Bool CAttitudes::GetAllChildren( const AttitudeGroup* group, TDynArray< const AttitudeGroup* > & children ) const
{
	ASSERT( group != NULL );

	for ( TDynArray< AttitudeGroup* >::const_iterator it = group->m_children.Begin(); it != group->m_children.End(); ++it )
	{
		children.PushBack( *it );
		GetAllChildren( *it, children );
	}
	return children.Size() > 0;
}

Bool CAttitudes::CanAttitudeCreateConflict( const CName& srcGroup, const CName& dstGroup, CName& srcConflictGroup , CName& dstConflictGroup )
{
	TDynArray< CName > children;
	TDynArray< CName > parents;
	
	if ( IsParentForGroup( srcGroup, dstGroup ) || IsParentForGroup( dstGroup, srcGroup ) )
	{
		return false;
	}

	GetAllChildren( srcGroup, children );
	GetAllParents( dstGroup, parents );

	for ( TDynArray< CName >::iterator it = children.Begin(); it != children.End(); ++it )
	{
		for ( TDynArray< CName >::iterator jt = parents.Begin(); jt != parents.End(); ++jt )
		{
			if ( *it != *jt && IsCustomAttitude( *it, *jt ) )
			{
				srcConflictGroup = *it;
				dstConflictGroup = *jt;
				return true;
			}
		}
	}

	children.Clear();
	parents.Clear();
	GetAllParents( srcGroup, parents );
	GetAllChildren( dstGroup, children );

	for ( TDynArray< CName >::iterator it = children.Begin(); it != children.End(); ++it )
	{
		for ( TDynArray< CName >::iterator jt = parents.Begin(); jt != parents.End(); ++jt )
		{
			if ( *it != *jt && IsCustomAttitude( *it, *jt ) )
			{
				srcConflictGroup = *jt;
				dstConflictGroup = *it;
				return true;
			}
		}
	}

	srcConflictGroup = CName::NONE;
	dstConflictGroup = CName::NONE;
	return false;
}

Bool CAttitudes::CanParenthoodCreateConflict( const CName& child, const CName& parent, CName& childConflictGroup , CName& parentConflictGroup )
{
	TDynArray< AttitudeGroup* > childConnections;
	TDynArray< AttitudeGroup* > parentConnections;

	AttitudeGroup* pChild = m_attitudeGroups.FindPtr( child );
	AttitudeGroup* pParent = m_attitudeGroups.FindPtr( parent );
	if ( pChild == NULL || pParent == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), child.AsString().AsChar(), parent.AsString().AsChar() );
		childConflictGroup = CName::NONE;
		parentConflictGroup = CName::NONE;
		return false;
	}

	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++ it)
	{
		if ( it->m_first != child && m_isCustomAttitude.Get( GetCacheIndex( pChild->m_index, it->m_second.m_index ) ) )
		{
			childConnections.PushBack( &it->m_second );
		}
		else if ( it->m_first != parent && m_isCustomAttitude.Get( GetCacheIndex( pParent->m_index, it->m_second.m_index ) ) )
		{
			parentConnections.PushBack( &it->m_second );
		}
	}

	for ( TDynArray< AttitudeGroup* >::iterator it = childConnections.Begin(); it != childConnections.End(); ++it )
	{
		for ( TDynArray< AttitudeGroup* >::iterator jt = parentConnections.Begin(); jt != parentConnections.End(); ++jt )
		{
			if ( IsParentForGroup( *jt, *it ) )
			{
				childConflictGroup = (*it)->m_name;
				parentConflictGroup = (*jt)->m_name;
				return true;
			}
		}
	}

	childConflictGroup = CName::NONE;
	parentConflictGroup = CName::NONE;
	return false;
}

void CAttitudes::InitAttitudesTree()
{
	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( it->m_second.m_parent == NULL )
		{
			UpdateAttitudesTree( &it->m_second );
		}
	}
}

Uint32 CAttitudes::RemoveUnusedGroups( TDynArray< String > & groups )
{
	Uint32 result = 0; // the number of removed elements

	TDynArray< CName > attitudeGroups;
	for ( TDynArray< String >::const_iterator i = groups.Begin(); i != groups.End(); ++i )
	{
		attitudeGroups.PushBack( CName(*i) );
	}

	TDynArray< CName > groupsToRemove;
	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( !attitudeGroups.Exist( it->m_first ) )
		{
			groupsToRemove.PushBack( it->m_first );
		}
	}

	// removed groups won't be "physically" removed from the list
	// they are marked as "non-custom" and removed from inheritance tree
	// thus, non-custom entries won't be saved
	for ( TDynArray< CName >::const_iterator it = groupsToRemove.Begin(); it != groupsToRemove.End(); ++it )
	{
		AttitudeGroup* group = m_attitudeGroups.FindPtr( *it );
		ASSERT( group != NULL );
		// marking all entries for given group "non-custom" and leaving cache untouched
		// (non-custom values won't be saved)
		for ( THashMap< CName, AttitudeGroup >::iterator jt = m_attitudeGroups.Begin(); jt != m_attitudeGroups.End(); ++jt )
		{
			if ( group->m_name != jt->m_second.m_name )
			{
				m_isCustomAttitude.Set( GetCacheIndex( group->m_index, jt->m_second.m_index ), 0 );
			}
		}
		// if group has parent, moving all its children to parent's children
		// otherwise reseting all children inherited attitudes to default
		AttitudeGroup* parent = group->m_parent;
		for ( TDynArray< AttitudeGroup* >::iterator child = group->m_children.Begin(); child != group->m_children.End(); ++child )
		{
			(*child)->m_parent = parent;
		}
		if ( parent != NULL )
		{
			group->m_parent = NULL;
			UpdateChildrenAttitudes( parent );
		}
		else
		{
			ResetChildrenAttitudes( group );
		}
		group->m_children.Clear();
	}

	return result;
}

Bool CAttitudes::LoadDataFromXml( const String& filePath )
{
	IFile* file = NULL;
	CDiskFile *diskFile = GDepot->FindFile( filePath );
	if( diskFile )
	{
		file = diskFile->CreateReader();
	}

	if ( !file )
	{
		ERR_GAME( TXT( "Could not open attitude config file '%ls'" ), filePath.AsChar() );
		return false;
	}
	CXMLFileReader* fileReader = new CXMLFileReader( *file );
	delete file;

	Bool result = LoadDataFromXml( fileReader );

	delete fileReader;
	if( result )
	{
		InitAttitudesTree();
	}
	return result;
}

Bool CAttitudes::LoadDataFromXmls( const TDynArray<String>& filePaths )
{
	CDiskFile* diskFile = nullptr;
	Bool result = false;
	for( const String& filePath : filePaths )
	{
		diskFile = GDepot->FindFile( filePath );
		if( !diskFile )
		{
			continue;
		}
		Red::TScopedPtr< IFile > file( diskFile->CreateReader() );
		if ( !file )
		{
			ERR_GAME( TXT( "Could not open attitude config file '%ls'" ), filePath.AsChar() );
			continue;
		}
		Red::TScopedPtr< CXMLFileReader > fileReader( new CXMLFileReader( *file ) );

		result |= LoadDataFromXml( fileReader.Get() );
	}
	if( result )
	{
		InitAttitudesTree();
	}
	return result;
}

Bool CAttitudes::LoadDataFromXml( CXMLReader* reader)
{
	if ( reader == NULL ) return false;

	if ( !reader->BeginNode( TXT("Attitudes") ) )
	{
		// bad XML attitudes file format
		return false;
	}

	while ( reader->BeginNode( TXT("Attitude") ) )
	{
		String srcGroup;
		String dstGroup;
		String attitudeValue;
		reader->Attribute( TXT("GroupName1"), srcGroup );
		reader->Attribute( TXT("GroupName2"), dstGroup );
		reader->Attribute( TXT("AttitudeValue"), attitudeValue );
		CName srcGroupName = CName( srcGroup );
		CName dstGroupName = CName( dstGroup );
		if ( AttitudeGroupExists( srcGroupName ) && AttitudeGroupExists( dstGroupName ) )
		{
			SetAttitude( srcGroupName, dstGroupName, AttitudeFromString( attitudeValue ), false );
		}
		reader->EndNode();
	}

	while ( reader->BeginNode( TXT("ParentGroup") ) )
	{
		String group;
		String parent;
		reader->Attribute( TXT("Group"), group );
		reader->Attribute( TXT("Parent"), parent );
		CName groupName = CName( group );
		CName parentName = CName( parent );
		if ( AttitudeGroupExists( groupName ) && AttitudeGroupExists( parentName ) )
		{
			SetParentForGroup( groupName, parentName, false );
		}
		reader->EndNode();
	}

	reader->EndNode();
	return true;
}

Bool CAttitudes::SaveDataToXml( CXMLWriter* writer )
{
	if ( writer == NULL ) return false;

	TDynArray< CName > srcGroups;
	TDynArray< CName > dstGroups;
	TDynArray< EAIAttitude > attitudes;
	Uint32 attitudesCount = GetAttitudes( srcGroups, dstGroups, attitudes );

	writer->BeginNode( TXT("Attitudes") );
	for ( Uint32 i = 0; i < attitudesCount; ++i )
	{
		String srcGroupName = srcGroups[i].AsString();
		String dstGroupName = dstGroups[i].AsString();
		String attitudeName = ToString( attitudes[i] );
		writer->BeginNode( TXT("Attitude") );
		writer->Attribute( TXT("GroupName1"), srcGroupName );
		writer->Attribute( TXT("GroupName2"), dstGroupName );
		writer->Attribute( TXT("AttitudeValue"), attitudeName );
		writer->EndNode();
	}

	srcGroups.Clear();
	dstGroups.Clear();
	Uint32 parentGroupsCount = GetParentGroups( srcGroups, dstGroups );
	for ( Uint32 i = 0; i < parentGroupsCount; ++i )
	{
		String groupName = srcGroups[i].AsString();
		String parentName = dstGroups[i].AsString();
		writer->BeginNode( TXT("ParentGroup") );
		writer->Attribute( TXT("Group"), groupName );
		writer->Attribute( TXT("Parent"), parentName );
		writer->EndNode();
	}

	writer->EndNode();
	writer->Flush();

	return true;
}

Bool CAttitudes::LoadAttitudeGroups()
{
	const C2dArray& attitudeGroupsArray = SAttitudesResourcesManager::GetInstance().Get2dArray();
	
	m_attitudeGroups.Clear();
	Uint32 index = 0;
	Uint32 rows = static_cast< Uint32 >( attitudeGroupsArray.GetNumberOfRows() );
	for ( Uint32 i = 0; i < rows; ++i )
	{
		String attitudeGroup = attitudeGroupsArray.GetValue( 0, i );
		if ( !attitudeGroup.Empty() && attitudeGroup != TXT("default") ) // skip empty rows and default group
		{
			CName name = CName( attitudeGroup );
			if ( m_attitudeGroups.Insert( name, AttitudeGroup( name, index ) ) )
			{
				index++;
			}
		}
	}

	Uint32 totalSize = GetCacheSize();
	ASSERT( totalSize > 0 );
	m_attitudes.Resize( totalSize );
	m_attitudes.SetZero();
	m_isCustomAttitude.Resize( totalSize );
	m_isCustomAttitude.SetZero();
	return true;
}

Bool CAttitudes::AttitudeGroupsLoaded() const
{
	return m_attitudeGroups.Size() > 0;
}

Bool CAttitudes::GetInheritanceRoot( const CName& srcGroup, const CName& dstGroup, AttitudeGroup*& srcGroupParent, AttitudeGroup*& dstGroupParent ) const
{
	srcGroupParent = const_cast< AttitudeGroup* >( m_attitudeGroups.FindPtr( srcGroup ) );
	AttitudeGroup* pDstGroup = const_cast< AttitudeGroup* >( m_attitudeGroups.FindPtr( dstGroup ) );
	while ( srcGroupParent != NULL && pDstGroup != NULL )
	{
		dstGroupParent = pDstGroup;
		while ( dstGroupParent != NULL )
		{
			if ( srcGroupParent == dstGroupParent )
			{
				return true;
			}
			if ( m_isCustomAttitude.Get( GetCacheIndex( srcGroupParent->m_index, dstGroupParent->m_index ) ) )
			{
				return true;
			}
			dstGroupParent = dstGroupParent->m_parent;
		}
		srcGroupParent = srcGroupParent->m_parent;
	}
	srcGroupParent = NULL;
	dstGroupParent = NULL;
	return false;
}

void CAttitudes::UpdateChildrenAttitude( AttitudeGroup* baseGroup, AttitudeGroup* otherGroup, EAIAttitude attitude )
{
	ASSERT( baseGroup != NULL && otherGroup != NULL );

	for ( TDynArray< AttitudeGroup* >::iterator it = baseGroup->m_children.Begin(); it != baseGroup->m_children.End(); ++it )
	{
		Uint32 index = GetCacheIndex( otherGroup->m_index, (*it)->m_index );
		if ( !m_isCustomAttitude.Get( index ) )
		{
			m_attitudes.Set( index, (Uint32)attitude );
			UpdateChildrenAttitude( *it, otherGroup, attitude );
		}
	}
}

void CAttitudes::UpdateChildrenAttitudes( AttitudeGroup* baseGroup )
{
	ASSERT( baseGroup != NULL );

	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( baseGroup->m_name == it->m_second.m_name )
		{
			continue;
		}
		EAIAttitude attitude = (EAIAttitude)m_attitudes.Get( GetCacheIndex( baseGroup->m_index, it->m_second.m_index ) );
		UpdateChildrenAttitude( baseGroup, &it->m_second, attitude );
	}
}

void CAttitudes::UpdateAttitudesTree( AttitudeGroup* baseGroup )
{
	ASSERT( baseGroup != NULL );

	for ( TDynArray< AttitudeGroup* >::iterator it = baseGroup->m_children.Begin(); it != baseGroup->m_children.End(); ++it )
	{
		CopyInheritedAttitudes( *it, baseGroup );
		UpdateAttitudesTree( *it );
	}
}

void CAttitudes::CopyInheritedAttitudes( AttitudeGroup* group, AttitudeGroup* parent )
{
	ASSERT( group != NULL && parent != NULL );

	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( it->m_first == group->m_name )
		{
			continue;
		}

		// we need to check if an attitude between current group and other group's parent
		// wasn't defined (is not custom) so we don't override it by inherited value
		AttitudeGroup* otherParent = it->m_second.m_parent;
		Bool hasParentWithCustomAttitude = false;
		while ( otherParent != nullptr && otherParent != group )
		{
			if ( m_isCustomAttitude.Get( GetCacheIndex( group->m_index, otherParent->m_index ) ) )
			{
				hasParentWithCustomAttitude = true;
				break;
			}
			otherParent = otherParent->m_parent;
		}
		if ( hasParentWithCustomAttitude )
		{
			continue;
		}

		Uint32 index = GetCacheIndex( group->m_index, it->m_second.m_index );
		// if not custom attitude then we overwrite it with parent value
		if ( !m_isCustomAttitude.Get( index ) )
		{
			// group is friendly to its parent by default
			if ( it->m_first == parent->m_name || it->m_second.m_parent == group )
			{
				m_attitudes.Set( index, (Uint32)AIA_Friendly );
			}
			else
			{
				m_attitudes.Set( index, m_attitudes.Get( GetCacheIndex( parent->m_index, it->m_second.m_index ) ) );
			}
		}
	}
}

void CAttitudes::ResetChildrenAttitude( AttitudeGroup* baseGroup, AttitudeGroup* otherGroup )
{
	ASSERT( baseGroup != NULL && otherGroup != NULL );

	Uint32 value = (Uint32)GetDefaultAttitude();
	for ( TDynArray< AttitudeGroup* >::iterator it = baseGroup->m_children.Begin(); it != baseGroup->m_children.End(); ++it )
	{
		Uint32 index = GetCacheIndex( baseGroup->m_index, (*it)->m_index );
		if ( !m_isCustomAttitude.Get( index ) )
		{
			m_attitudes.Set( index, value );
			ResetChildrenAttitude( *it, otherGroup );
		}
	}
}

void CAttitudes::ResetChildrenAttitudes( AttitudeGroup* baseGroup )
{
	ASSERT( baseGroup != NULL );

	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( baseGroup->m_name == it->m_second.m_name )
		{
			continue;
		}
		ResetChildrenAttitude( baseGroup, &it->m_second );
	}
}

void CAttitudes::ResetInheritedAttributes( AttitudeGroup* group )
{
	ASSERT( group != NULL );

	Uint32 defaultAttitude = (Uint32)GetDefaultAttitude();
	for ( THashMap< CName, AttitudeGroup >::iterator it = m_attitudeGroups.Begin(); it != m_attitudeGroups.End(); ++it )
	{
		if ( it->m_first == group->m_name )
		{
			continue;
		}
		Uint32 index = GetCacheIndex( group->m_index, it->m_second.m_index );
		if ( !m_isCustomAttitude.Get( index ) )
		{
			m_attitudes.Set( index, defaultAttitude );
		}
	}
}

Uint32 CAttitudes::GetCacheIndex( const CName& srcGroup, const CName& dstGroup ) const
{
	const AttitudeGroup* pSrcGroup = m_attitudeGroups.FindPtr( srcGroup );
	const AttitudeGroup* pDstGroup = m_attitudeGroups.FindPtr( dstGroup );
	if ( pSrcGroup == NULL || pDstGroup == NULL )
	{
		WARN_GAME( TXT("Couldn't find one of the following groups in attitude groups list: %s %s"), srcGroup.AsString().AsChar(), dstGroup.AsString().AsChar() );
		return WRONG_INDEX;
	}

	return GetCacheIndex( pSrcGroup->m_index, pDstGroup->m_index );
}

Uint32 CAttitudes::GetCacheIndex( Uint32 srcGroupIndex, Uint32 dstGroupIndex ) const
{
	RED_WARNING_ONCE( srcGroupIndex != dstGroupIndex, "srcGroupIndex should not be the same than dstGroupIndex" );
	ASSERT( srcGroupIndex != WRONG_INDEX );
	ASSERT( dstGroupIndex != WRONG_INDEX );

	if ( dstGroupIndex < srcGroupIndex )
	{
		Swap( srcGroupIndex, dstGroupIndex );
	}

	// values of two-dimensional table [n, n] are aligned in the following way:
	// [0, 1], [0, 2], ..., [0, n - 1], [1, 2], [1, 3], ..., [1, n - 1], ..., [n - 2, n - 1]
	// example for table [4, 4]:
	// [0, 1], [0, 2], [0, 3], [1, 2], [1, 3], [2, 3]
	// offset for the first coordinate x is calculated by the sum of following sequence:
	// (n - x) + (n - x + 1) + ... + (n - 1) = (n - x + n - 1) * x / 2
	// which is well defined also for x = 0 and x = 1
	// offset for the second coordinate y is calculated with the sum:
	// y - x - 1
	// the total number of values is equal to (n - 1) * n / 2
	Uint32 n = m_attitudeGroups.Size();
	ASSERT( n > 0 );
	Uint32 x = srcGroupIndex;
	Uint32 y = dstGroupIndex;
	return ( n - x + n - 1 ) * x / 2 + ( y - x - 1 );
}

Uint32 CAttitudes::GetCacheSize() const
{
	Uint32 n = m_attitudeGroups.Size();
	ASSERT( n > 0 );
	return n * (n - 1) / 2;
}

CAttitudesResourcesManager::CAttitudesResourcesManager(): C2dArraysResourcesManager( resAttitudeGroups )
{
}

#ifndef NO_EDITOR

Uint32 CAttitudesResourcesManager::GetAttitudeGroupsFormFile( const String& pathToGroupsFile, TDynArray< String > & attitudeGroups )
{
	Int32 foundIndex = (Int32)m_2dArrayFilePaths.GetIndex( pathToGroupsFile );
	C2dArray* attitudeGroupsArray = nullptr;
	if( foundIndex != -1 )
	{
		attitudeGroupsArray = m_2dArrays[foundIndex];		
	}
	else
	{
		attitudeGroupsArray = LoadResource< C2dArray >( pathToGroupsFile );
	}
	if( attitudeGroupsArray != nullptr )
	{
		Uint32 rows = static_cast< Uint32 >( attitudeGroupsArray->GetNumberOfRows() );
		for ( Uint32 i = 0; i < rows; ++i )
		{
			String attitudeGroup = attitudeGroupsArray->GetValue( 0, i );
			attitudeGroups.PushBackUnique( attitudeGroup );
		}
		return attitudeGroups.Size();
	}
	return 0;
}

void CAttitudesResourcesManager::Sync()
{
	for( const String& filePath : m_2dArrayFilePaths )
	{
		CDiskFile *diskFile = GDepot->FindFile( filePath );
		if ( diskFile == NULL )
		{
			ERR_GAME( TXT( "Could not open attitudes data file '%s'" ), filePath.AsChar() );
		}
		else if ( diskFile->IsNotSynced() )
		{
			diskFile->Sync();
		}
	}	
	Reload2dArray();
}
#endif // !NO_EDITOR
//////////////////////////////////////////////////////////////////////////

Bool SAttitudesAIProfilePred::operator()( CAIProfile *aiProfile ) const
{
	if ( aiProfile->GetAttitudeGroup() != CNAME( default ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}