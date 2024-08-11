/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageResource.h"
#include "baseTree.h"

#include "../core/diskFile.h"

const Uint32 foliageResourceQuadtreeStorageVersion = 0;
const Uint32 foliageResourceArrayStorageVersion = 1;
const Uint32 foliageResource3DBBoxVersion = 2;
const Uint32 foliageResourceNewBBoxVersion = 3;

const Uint32 foliageResourceCurrentVersion = foliageResourceNewBBoxVersion;

IMPLEMENT_ENGINE_CLASS( CFoliageResource );

namespace
{
	template < typename T, EMemoryClass m, RED_CONTAINER_POOL_TYPE c, typename Pred >
	void MoveIf( TDynArray< T, m, c >& source, TDynArray< T, m, c >& dest, Pred pred )
	{
		source.Erase( 
			RemoveIf( source.Begin(), source.End(), [ pred, &dest ]( const T& obj ) 
			{ 
				if ( pred( obj ) ) { 
					dest.PushBack( obj ); 
					return true; 
				} 
				return false; 
			} ),
			source.End()
		);
	}
}


SFoliageInstanceGroup::SFoliageInstanceGroup()
{}

SFoliageInstanceGroup::SFoliageInstanceGroup( const SFoliageInstanceGroup& rhs )
	: baseTree( rhs.baseTree )
	, instances( rhs.instances )
{
}

SFoliageInstanceGroup::SFoliageInstanceGroup( SFoliageInstanceGroup&& rhs )
	: baseTree( Move( rhs.baseTree ) )
	, instances( Move( rhs.instances ) )
{
}

SFoliageInstanceGroup& SFoliageInstanceGroup::operator = ( const SFoliageInstanceGroup& rhs )
{
	SFoliageInstanceGroup( rhs ).SwapWith( *this );
	return *this;
}

SFoliageInstanceGroup& SFoliageInstanceGroup::operator = ( SFoliageInstanceGroup&& rhs )
{
	SFoliageInstanceGroup( Move( rhs ) ).SwapWith( *this );
	return *this;
}

void SFoliageInstanceGroup::SwapWith( SFoliageInstanceGroup& other )
{
	Swap( baseTree, other.baseTree );
	instances.SwapWith( other.instances );
}

IFile & operator<<( IFile & file, SFoliageInstanceGroup & group )
{
	file << group.baseTree;
	file << group.instances;
	return file;
}

CFoliageResource::CFoliageResource()
	:	m_version( 0 )
{}

CFoliageResource::~CFoliageResource()
{}

#ifndef NO_EDITOR_RESOURCE_SAVE
void CFoliageResource::OnResourceSavedInEditor()
{
	m_version = foliageResourceCurrentVersion;
	TBaseClass::OnResourceSavedInEditor();
}
#endif

void CFoliageResource::OnPreSave()
{
	TBaseClass::OnPreSave();
	SanitizeInstances();

	RemoveInstanceOutsideBox();

	RebuildInstanceGroupContainer();
	RecomputeBBox();
}

void CFoliageResource::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if( file.IsGarbageCollector() )
	{
		for( auto iter = m_baseTreeContainer.Begin(), end = m_baseTreeContainer.End(); iter != end; ++iter )
		{
			file << *iter;
		}
	}
	else if( m_version > foliageResourceQuadtreeStorageVersion )
	{
		file << m_treeInstanceContainer;
		file << m_grassInstanceContainer;
	}
}

void CFoliageResource::OnPostLoad()
{
	TBaseClass::OnPostLoad();	

	SanitizeInstances();

	PopulateBaseTreeContainer( m_treeInstanceContainer );
	PopulateBaseTreeContainer( m_grassInstanceContainer );
	
	if( m_version < foliageResourceNewBBoxVersion )
	{
		RecomputeBBox();
	}

	m_version = foliageResourceCurrentVersion;
}

void CFoliageResource::RecomputeBBox()
{	
	Box bbox = Box::EMPTY;

	for( auto iter = m_treeInstanceContainer.Begin(), end = m_treeInstanceContainer.End(); iter != end; ++iter )
	{
		const SFoliageInstanceGroup & group = *iter;
		const FoliageInstanceContainer & instances = group.instances;
		const CSRTBaseTree * baseTree = group.baseTree.Get();
		const Box baseTreeBox = baseTree->GetBBox();
		for( auto instanceIter = instances.Begin(), instanceEnd = instances.End(); instanceIter != instanceEnd; ++instanceIter )
		{
			const Float uniformScale = instanceIter->GetScale();
			const Vector scaleVector( uniformScale, uniformScale, uniformScale );
			Box instanceBox = baseTreeBox * scaleVector;
			instanceBox += instanceIter->GetPosition();
			bbox.AddBox( instanceBox );
		}
	}

	m_bbox = bbox;
}

void CFoliageResource::PopulateBaseTreeContainer( const InstanceGroupContainer & container )
{
	for( InstanceGroupContainer::const_iterator iter = container.Begin(), end = container.End(); iter != end; ++iter )
	{
		m_baseTreeContainer.PushBackUnique( iter->baseTree );
	}
}

Bool CFoliageResource::CanInsertInstance( const SFoliageInstance & instance ) const
{
	Box2 box2d = Box2( Vector2( m_gridbbox.Min.X, m_gridbbox.Min.Y ), Vector2( m_gridbbox.Max.X, m_gridbbox.Max.Y ) ); 
	return box2d.Contains( Vector2( instance.GetPosition().X, instance.GetPosition().Y ) );
}

Bool CFoliageResource::InsertInstance( BaseTreeHandle baseTree, const SFoliageInstance & instance )
{
	if( CanInsertInstance( instance ) )
	{
		SFoliageInstanceGroup & group = AcquireFoliageInstanceGroup( baseTree );
		group.instances.PushBack( instance );
		
		return true;
	}

	return false;
}

SFoliageInstanceGroup & CFoliageResource::AcquireFoliageInstanceGroup( BaseTreeHandle baseTree )
{
	InstanceGroupContainer & container = baseTree->IsGrassType() ? m_grassInstanceContainer : m_treeInstanceContainer;
	InstanceGroupContainer::iterator iter = FindIf( container.Begin(), container.End(), [=]( const SFoliageInstanceGroup & group ){ return group.baseTree == baseTree; } ); 
	if( iter == container.End() )
	{
		SFoliageInstanceGroup group;
		group.baseTree = baseTree;
		container.PushBack( group );
		m_baseTreeContainer.PushBackUnique(baseTree);
		return container.Back();
	}
	else
	{
		return *iter;
	}
}

Uint32 CFoliageResource::ReplaceBaseTree( const CSRTBaseTree* oldBaseTree, const CSRTBaseTree* newBaseTree, FoliageInstanceFilter filter, const Float* resetScale )
{
	if ( oldBaseTree == newBaseTree )
	{
		return 0; // nothing to do here
	}

	RebuildInstanceGroupContainer();

	InstanceGroupContainer & container = oldBaseTree->IsGrassType() ? m_grassInstanceContainer : m_treeInstanceContainer;

	InstanceGroupContainer::iterator iter = FindIf( container.Begin(), container.End(), [=]( const SFoliageInstanceGroup & group ){ return group.baseTree == oldBaseTree; } );
	
	if ( iter == container.End() )
	{
		return 0;
	}

	FoliageInstanceContainer instancesWithNewTree;

	// move aside, as acquiring may invalidate the container
	MoveIf( iter->instances, instancesWithNewTree, filter );

	if ( iter->instances.Empty() )
	{
		container.Erase( iter ); // for the same reason erase before iter gets invalidated
		m_baseTreeContainer.Remove( oldBaseTree );
	}

	if ( resetScale )
	{
		for ( SFoliageInstance& inst : instancesWithNewTree )
		{
			inst.SetScale( *resetScale );
		}
	}

	if ( newBaseTree != nullptr )
	{
		SFoliageInstanceGroup & group = AcquireFoliageInstanceGroup( newBaseTree );
		group.instances.PushBack( instancesWithNewTree );
	}

	return instancesWithNewTree.Size();
}

void CFoliageResource::RemoveBaseTree( const CSRTBaseTree * baseTree, InstanceGroupContainer & container )
{
	container.Erase( 
		RemoveIf( container.Begin(), container.End(), [=]( const SFoliageInstanceGroup & group ){ return group.baseTree == baseTree; } ), 
		container.End() );

	m_baseTreeContainer.Remove( baseTree );
}

void CFoliageResource::RemoveInstance( BaseTreeHandle baseTree, const SFoliageInstance & instance )
{
	SFoliageInstanceGroup & group = AcquireFoliageInstanceGroup( baseTree );
	SFoliageInstanceGroup::Instances & instances = group.instances;
	auto iter = Find( instances.Begin(), instances.End(), instance );
	if( iter != instances.End() )
	{
		instances.EraseFast( iter );
	}
}

Bool IsBaseTreeValid( const CSRTBaseTree * baseTree )
{
	if( !baseTree )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( Foliage ), TXT("Streamed foliage bucket with a NULL base tree. It can happen if SRTs were moved or removed from depot. Put them in old depot location and try again.") );
		return false;
	}

	return true;
}

const CFoliageResource::InstanceGroupContainer & CFoliageResource::GetAllTreeInstances() const
{
	return m_treeInstanceContainer;
}

const CFoliageResource::InstanceGroupContainer & CFoliageResource::GetAllGrassInstances() const
{
	return m_grassInstanceContainer;
}

const CFoliageResource::BaseTreeContainer & CFoliageResource::GetAllBaseTree() const
{
	return m_baseTreeContainer;
}

void CFoliageResource::GetInstancesFromArea( const CSRTBaseTree* baseTree, const Box & box, FoliageInstanceContainer& outInstances )
{
	const InstanceGroupContainer & container = baseTree->IsGrassType() ? m_grassInstanceContainer : m_treeInstanceContainer;
	auto iter = FindIf( container.Begin(), container.End(), [=]( const SFoliageInstanceGroup & group ){ return group.baseTree == baseTree; } );
	if( iter!= container.End() )
	{
		const FoliageInstanceContainer & instances = iter->instances;
		for( auto instanceIter = instances.Begin(), instanceEnd = instances.End(); instanceIter != instanceEnd; ++instanceIter )
		{
			const SFoliageInstance & instance = *instanceIter;
			if( box.Contains2D( instance.GetPosition() ) )
			{
				outInstances.PushBack( instance );
			}
		}
	}
}

void CFoliageResource::GetInstancesFromArea( const CSRTBaseTree* baseTree, const Vector & center, Float radius, FoliageInstanceContainer& outInstances )
{
	const Float sqRadius = radius * radius;
	const InstanceGroupContainer & container = baseTree->IsGrassType() ? m_grassInstanceContainer : m_treeInstanceContainer;
	auto iter = FindIf( container.Begin(), container.End(), [=]( const SFoliageInstanceGroup & group ){ return group.baseTree == baseTree; } );
	if( iter!= container.End() )
	{
		const FoliageInstanceContainer & instances = iter->instances;
		for( auto instanceIter = instances.Begin(), instanceEnd = instances.End(); instanceIter != instanceEnd; ++instanceIter )
		{
			const SFoliageInstance & instance = *instanceIter;
			const Vector dist = instance.GetPosition() - center;
			if ( dist.SquareMag3() < sqRadius )
			{
				outInstances.PushBack( instance );
			}
		}
	}
}

void CFoliageResource::SetGridBox( const Box & box )
{
	if( box != m_gridbbox )
	{
		m_gridbbox = box;
		RemoveInstanceOutsideBox();
	}
}

const Box & CFoliageResource::GetGridBox() const
{
	return m_gridbbox;
}

const Box & CFoliageResource::GetInstancesBoundingBox() const
{
	return m_bbox;
}

void CFoliageResource::SanitizeInstances()
{
	for ( Int32 i=m_treeInstanceContainer .SizeInt()-1; i>=0; --i )
	{
		if ( !m_treeInstanceContainer[i].baseTree )
		{
			m_treeInstanceContainer.RemoveAt(i);
		}
	}

	for ( Int32 i=m_grassInstanceContainer.SizeInt()-1; i>=0; --i )
	{
		if ( !m_grassInstanceContainer[i].baseTree )
		{
			m_grassInstanceContainer.RemoveAt(i);
		}
	}

	MoveIf( m_treeInstanceContainer, m_grassInstanceContainer, 
		[]( const SFoliageInstanceGroup& group ){ return group.baseTree->IsGrassType(); } );

	MoveIf( m_grassInstanceContainer, m_treeInstanceContainer, 
		[]( const SFoliageInstanceGroup& group ){ return !group.baseTree->IsGrassType(); } );
}

void CFoliageResource::RebuildInstanceGroupContainer()
{
	InstanceGroupContainer treeInstances  = Move( m_treeInstanceContainer );
	InstanceGroupContainer grassInstances = Move( m_grassInstanceContainer );

	for( const SFoliageInstanceGroup& iter : treeInstances )
	{
		SFoliageInstanceGroup & group = AcquireFoliageInstanceGroup( iter.baseTree );
		group.instances.PushBack( iter.instances );
	}

	for( const SFoliageInstanceGroup& iter : grassInstances )
	{
		SFoliageInstanceGroup & group = AcquireFoliageInstanceGroup( iter.baseTree );
		group.instances.PushBack( iter.instances );
	}
}

void CFoliageResource::RemoveInstanceOutsideBox()
{
	for( SFoliageInstanceGroup& group : m_treeInstanceContainer )
	{
		SFoliageInstanceGroup::Instances & instances = group.instances;

		instances.Erase( 
			RemoveIf( instances.Begin(), instances.End(), [this]( const SFoliageInstance& instance ){ return !CanInsertInstance( instance ); }), 
			instances.End() );
	}

	for( SFoliageInstanceGroup& group : m_grassInstanceContainer )
	{
		SFoliageInstanceGroup::Instances & instances = group.instances;

		instances.Erase( 
			RemoveIf( instances.Begin(), instances.End(), [this]( const SFoliageInstance& instance ){ return !CanInsertInstance( instance ); }), 
			instances.End() );
	}
}
