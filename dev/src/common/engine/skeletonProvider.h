/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/enum.h"
#include "../core/allocArray.h"
#include "renderSkinningData.h"


/// Skeleton data provider for skinning
class ISkeletonDataProvider
{
public:
	/// Bone info
	struct BoneInfo
	{
		CName		m_name;		//!< Bone name
		Int32		m_parent;	//!< Parent bone index

		// Constructor
		RED_INLINE BoneInfo()
			: m_parent( -1 )
		{};
	};

	// Bone data
	struct SBonesData : private Red::System::NonCopyable
	{
		const Int16*				m_boneIndices;
		Uint32						m_numBones;
		const Matrix*				m_rigMatrices;
		const Float*				m_vertexEpsilons;
		void*						m_outMatricesArray;
		ESkinningDataMatrixType		m_outMatricesType;
		Box&						m_outBoxMS;

		SBonesData( Box& boxMS )
			: m_boneIndices( nullptr )
			, m_numBones( 0 )
			, m_rigMatrices( nullptr )
			, m_vertexEpsilons( nullptr )
			, m_outMatricesArray( nullptr )
			, m_outMatricesType( SDMT_4x4 )
			, m_outBoxMS( boxMS )
		{}
	};
public:
	ISkeletonDataProvider() {};
	virtual ~ISkeletonDataProvider()  {};

	// Get the runtime cache index
	virtual Uint32 GetRuntimeCacheIndex() const = 0;

	// Create skeleton mapping for another skeleton
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const = 0;

	// Get bones number in the skeleton
	virtual Int32 GetBonesNum() const = 0;

	// Find bone by name
	virtual Int32 FindBoneByName( const Char* name ) const = 0;

	// Find bone by name
	virtual Int32 FindBoneByName( const CName& name ) const { return FindBoneByName( name.AsString().AsChar() ); }

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const = 0;

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const = 0;

	// Calc bone matrix in model space
	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const = 0;

	// Get bone matrix in model space
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const = 0;

	// Get bone matrix in world space
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const = 0;

	// Get bone matrices in the model space. vertexEpsilons can be NULL, if not used (treat as though 0's)
	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const = 0;
};
