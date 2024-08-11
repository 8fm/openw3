#pragma once


namespace PathLib
{
////////////////////////////////////////////////////////////////////////////
// Binary tree nodes are part of internal bin tree implementation
class CNavmesh;

namespace BinTree
{
	struct BinTreeNodeBase;
}

////////////////////////////////////////////////////////////////////////////
// Binary tree for fast vertical ray tests
////////////////////////////////////////////////////////////////////////////
class CNavmeshBinTree
{
public:
	typedef Uint16 TriangleIndex;
	static const Uint32 MIN_NODE_TRIANGLES = 4;

	CNavmeshBinTree()
		: m_rootNode( NULL )
		, m_dataSize( 0 )													{}
	~CNavmeshBinTree();

	void CreateTree( CNavmesh* navmesh );									// creates non-optimized tree
	void Clear();															// reset everything

	Bool IsInitialized() const												{ return m_dataSize > 0; }
	Uint32 GetDataSize() const												{ return m_dataSize; }

	// Data processing pre/post (de)serialization
	void SetDataBuffer( void* data, Uint32 dataSize );

	TriangleIndex FindTriangle(const Vector2& pos, Float fZMin, Float fZMax, const CNavmesh* navmesh) const;	// thats what bin tree is for
	Bool ComputeAverageHeight(const Vector3& bboxMin, const Vector3& bboxMax, const CNavmesh* navmesh, Float& zAverage, Float& zMin, Float& zMax);
	//void FindTrianglesInBBox(const Vector3& bboxMin, const Vector3& bboxMax, const CNavmesh* navmesh, TDynArray< TriangleIndex >& outTriangles) const;	// additional test
	Bool ContainsTriangle(TriangleIndex nTri);								// for debug purposes
	TriangleIndex GetClosestTriangleInside(const Box& bbox, CNavmesh* navmesh );
	//void GetTrianglesInside(const Box& bbox,TDynArray< TriangleIndex >& aOut, CNavmesh* pNavmesh); // TODO
protected:
	template < class Functor >
	RED_INLINE void IterateTrianglesInBBox(const Vector3& bboxMin, const Vector3& bboxMax, const CNavmesh* navmesh, Functor& functor);

	BinTree::BinTreeNodeBase*		m_rootNode;
	Uint32							m_dataSize;
};

};			// namespace PathLib