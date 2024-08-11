/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_FOLIAGE_INSTANCE_H_
#define _ENGINE_FOLIAGE_INSTANCE_H_

struct SFoliageInstance;

typedef TDynArray< SFoliageInstance, MC_FoliageInstances, MemoryPool_FoliageData > FoliageInstanceContainer;

struct SFoliageInstance
{
public:

	void Serialize( IFile & file );

	void MultiplySize( Float multiplier );
	
	Float GetNormalizedScale() const;
	Vector3 GetUpVector() const;
	Vector3 GetRightVector() const;
	
	void SetQuaternion( const Vector & quaternion );
	Vector GetQuaterion() const;

	const Vector3 & GetPosition() const;
	void SetPosition( const Vector & position );

	void SetScale( Float scale );
	Float GetScale() const;

private:

	Vector3 m_position;	
	Float m_scale;
	Float m_z;
	Float m_w;
};

IFile & operator<<( IFile & file, SFoliageInstance & instance );
bool operator==( const SFoliageInstance & left, const SFoliageInstance & right );
bool operator!=( const SFoliageInstance & left, const SFoliageInstance & right );

struct FoliageTreeInstances : Red::System::NonCopyable // Only "movable"
{
	FoliageTreeInstances();
	FoliageTreeInstances( FoliageTreeInstances && value );
	FoliageTreeInstances & operator=( FoliageTreeInstances && value );
	void Swap( FoliageTreeInstances & value );

	RenderObjectHandle tree;
	FoliageInstanceContainer instances;
	Box box;
};

typedef TDynArray< FoliageTreeInstances, MC_FoliageInstances, MemoryPool_FoliageData > FoliageAddInstancesContainer;
typedef TPair< RenderObjectHandle, Box > FoliageRemoveInstances;
typedef TDynArray< FoliageRemoveInstances, MC_FoliageInstances > FoliageRemoveInstancesContainer;

struct SFoliageUpdateRequest : Red::System::NonCopyable
{
	SFoliageUpdateRequest();
	SFoliageUpdateRequest( SFoliageUpdateRequest && value );
	SFoliageUpdateRequest & operator=( SFoliageUpdateRequest && value );
	void Swap( SFoliageUpdateRequest & value );

	FoliageAddInstancesContainer addRequestContainer;
	FoliageRemoveInstancesContainer removeRequestContainer; 
};

#include "foliageInstance.inl"

#endif 
