/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "..\physics\physicsWrapper.h"
#include "..\core\dataBuffer.h"

struct BoneInfo
{
	StringAnsi m_boneName;
	Matrix m_initLocalPose;
	Int16 m_parentIndex;
};

class CPhysicsRagdollWrapper : public CPhysicsWrapperInterface
{
protected:
	DataBuffer						m_buffer;
	void*							m_collection;
	Vector							m_previousPosition;
	void*							m_aggregate;

	CPhysicsRagdollWrapper() 
		: CPhysicsWrapperInterface()
		, m_previousPosition( Vector::ZEROS )
		, m_aggregate( nullptr )
 {}

public:
	virtual Int32 GetBoneIndex( Int32 actorIndex ) const { return -1; }
	virtual Bool DisableRagdollParts( const TDynArray< Uint32 > & boneIndices ) = 0;
	virtual Bool GetCurrentPositionAndDeltaPosition( Vector & outCurrentPosition, Vector & outDeltaPosition ) const = 0;
	virtual const Float GetRagdollStoppedFactor() { return 0.0f; }
	virtual void SampleBonesModelSpace( TDynArray<Matrix>& poseInOut ) const = 0;
	virtual void SyncToAnimation( const TDynArray<Matrix>& bones, const Matrix& localToWorld ) = 0;
	Float GetMotionIntensity();

};
