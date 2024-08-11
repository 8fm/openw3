/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageInstance.h"

void SFoliageInstance::Serialize( IFile & file )
{
	if( file.GetVersion() < VER_FOLIAGE_DATA_OPTIMIZATION )
	{
		Vector quaternion;
		file << quaternion;
		file << m_position;
		file << m_scale;
		SetQuaternion( quaternion );
	}
	else
	{
		file << m_position;
		file << m_scale;
		file << m_z << m_w;
	}
}

void SFoliageInstance::MultiplySize( Float multiplier )
{
	m_scale = m_scale * multiplier;
}

Float SFoliageInstance::GetNormalizedScale() const
{
	return Max( Min( m_scale, 20.0f ), 0.2f );
}

void SFoliageInstance::SetPosition( const Vector & position )
{
	m_position = position;
}

void SFoliageInstance::SetQuaternion( const Vector & quaternion )
{
	m_z = quaternion.Z;
	m_w = quaternion.W;
}

void SFoliageInstance::SetScale( Float scale )
{
	m_scale = scale;
}

IFile & operator<<( IFile & file, SFoliageInstance & instance )
{
	instance.Serialize( file );
	return file;
}

bool operator==( const SFoliageInstance & left, const SFoliageInstance & right )
{
	return Vector( left.GetPosition() ) == Vector( right.GetPosition() ); 
}

bool operator!=( const SFoliageInstance & left, const SFoliageInstance & right )
{
	return !( left == right ); 
}

FoliageTreeInstances::FoliageTreeInstances()
{}

FoliageTreeInstances::FoliageTreeInstances( FoliageTreeInstances && value )
	:	tree( std::move( value.tree ) ),
		instances( std::move( value.instances ) ),
		box( std::move( value.box ) )
{}

FoliageTreeInstances & FoliageTreeInstances::operator=( FoliageTreeInstances && value )
{
	FoliageTreeInstances( std::move( value ) ).Swap( *this );
	return *this;
}

void FoliageTreeInstances::Swap( FoliageTreeInstances & value )
{
	::Swap( tree, value.tree );
	instances.SwapWith( value.instances );
	::Swap( box, value.box );
}

SFoliageUpdateRequest::SFoliageUpdateRequest()
{}

SFoliageUpdateRequest::SFoliageUpdateRequest( SFoliageUpdateRequest && value )
	:	addRequestContainer( std::move( value.addRequestContainer ) ),
		removeRequestContainer( std::move( value.removeRequestContainer ) )
{}

SFoliageUpdateRequest & SFoliageUpdateRequest::operator=( SFoliageUpdateRequest && value )
{
	SFoliageUpdateRequest( std::move( value ) ).Swap( value );
	return *this;
}

void SFoliageUpdateRequest::Swap( SFoliageUpdateRequest & value )
{
	addRequestContainer.SwapWith( value.addRequestContainer );
	removeRequestContainer.SwapWith( value.removeRequestContainer );
}

