/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace CollisionMem
{
	typedef TDynArray< CStaticMeshComponent * >			TStaticMeshComponentArray;

	typedef THashMap< CMesh *, TStaticMeshComponentArray >	TMeshMap;
	typedef TMeshMap::const_iterator					TMeshMapConstIterator;

	struct SingleScaleData
	{
		Vector						scale;				//!< Unique collision scale
		Uint32						collisionMemSize;	//!< Size of compiled collision mesh for this scale in bytes
		TStaticMeshComponentArray	arr;				//!< Static mesh components with the same scale
	};

	typedef TDynArray< SingleScaleData >			TSingleScaleDataArray;
	typedef TSingleScaleDataArray::iterator			TSingleScaleDataArrayIterator;

	struct UniqueMeshData
	{
		CMesh *					pMesh;				//!< Mesh used by stored (and scale grouped) static mesh components
		Uint32					collisionMemSize;	//!<
		TSingleScaleDataArray	arr;				//!< Scale grouped static mesh components using stored mesh
	};

	typedef TDynArray< UniqueMeshData >	TUniqueMeshInfoArray;


	//** *******************************
	//
	class CCollisionMemToolModel
	{
	private:

		TUniqueMeshInfoArray	m_UniqueMeshArray;

	public:

		void	ReadWorldNodeData							( CStaticMeshComponent * pStaticMeshComponent, TMeshMap & meshMap );
		void	PrepareUniqueMeshData						( const TMeshMap & mehsMap );

		Uint32	GetTotalCollisionMemory						() const;

		const TUniqueMeshInfoArray &	GetUniqueMeshArray	() const;

	private:

		void	ProcessScales			( const TStaticMeshComponentArray & inArr, TSingleScaleDataArray & scalesArray, Uint32 & totalMemSize ) const;
		void	GroupScales				( TSingleScaleDataArray & arr ) const;

		Vector	GetCollsionScale		( const CStaticMeshComponent * pSMC ) const;
	};
};
