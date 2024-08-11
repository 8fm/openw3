#pragma once


#include "../core/mathUtils.h"

// NOTICE ITS NOT MEANT TO BE INCLUDED IN ANY OTHER HEADER FILES (just some cpp ones)
#include "pathlibBinTree.h"
#include "pathlibNavgraph.h"

namespace PathLib
{

namespace KD
{

typedef Vector2 TPosition;
enum EDimms { DIMMS = 2 };

struct KDBox3;

struct KDBox2
{
	enum eResetState { RESET };

	typedef Vector2 Pos;

	//FIXME>>>>> Unions can't have members with non-trivial ctors
#ifndef RED_COMPILER_CLANG
	union 
	{
		struct
		{
#endif // ! RED_COMPILER_CLANG
			Vector2			Min;
			Vector2			Max;
#ifndef RED_COMPILER_CLANG
		};
		struct
		{
			Vector2			m_vec[2];
		};
	};
#endif // ! RED_COMPILER_CLANG

	RED_INLINE KDBox2();
	RED_INLINE KDBox2( eResetState );
	RED_INLINE KDBox2( const Vector2& min, const Vector2& max );
	RED_INLINE KDBox2( const Box& bbox );

	RED_INLINE Bool Intersect( const KDBox2& box ) const;
	RED_INLINE Bool Intersect( const KDBox3& box ) const;

	RED_INLINE Bool Encompass( const KDBox2& box ) const;
	RED_INLINE Bool Encompass( const KDBox3& box ) const;

	RED_INLINE Bool PointTest( const Vector2& v ) const;
	RED_INLINE Bool PointTest( const Vector3& v ) const;
};

struct KDBox3
{
	enum eResetState { RESET };

	typedef Vector3 Pos;

	union 
	{
		struct
		{
			Vector3			Min;
			Vector3			Max;
		};
		struct
		{
			Vector3			m_vec[2];
		};
	};

	RED_INLINE KDBox3();
	RED_INLINE KDBox3( eResetState );
	RED_INLINE KDBox3( const Vector3& min, const Vector3& max );
	RED_INLINE KDBox3( const Box& bbox );

	RED_INLINE Bool Intersect( const KDBox2& box ) const;
	RED_INLINE Bool Intersect( const KDBox3& box ) const;

	RED_INLINE Bool Encompass( const KDBox2& box ) const;
	RED_INLINE Bool Encompass( const KDBox3& box ) const;

	RED_INLINE Bool PointTest( const Vector2& v ) const;
	RED_INLINE Bool PointTest( const Vector3& v ) const;
};

template < Int32 DIM >
struct TKDBox;

template <> struct TKDBox< 2 > { typedef KDBox2 KDBox; };
template <> struct TKDBox< 3 > { typedef KDBox3 KDBox; };

typedef TKDBox< 2 >::KDBox KDBox;

class CCel
{
private:
	typedef CNavNode*				ElementType;
	typedef TDynArray< CNavNode* >	NodeList;
	TDynArray< Uint8 >				m_binTree;
	NodeList						m_navNodes;
	KDBox							m_celBox;
	Bool							m_constructed;
public:
	struct MultiNodesTestContext : public  BinTree::FindBinTreeNClosestElementsContext< CNavNode::Id >
	{
		typedef BinTree::FindBinTreeNClosestElementsContext< CNavNode::Id > Super;
		MultiNodesTestContext( Float maxDistSq, Uint32 elementsToFind, CNavNode::Id* outElements )
			: Super( maxDistSq, elementsToFind, outElements )				{}

		void StoreOutput( const CNavNode* navNode, Uint32 index )			{ m_elementsFound[ index ] = navNode->GetFullId(); }
	};

	CCel()
		: m_constructed( false )											{}
	~CCel()																	{}

	Bool						IsConstructed() const						{ return m_constructed; }
	Bool						IsEmpty() const								{ return m_navNodes.Empty(); }
	const KDBox&				GetBoundings() const						{ return m_celBox; }

	void						Initialize( const KDBox& bbox );
	Bool						RequestConstruct();
	void						Invalidate();

	void						CollectNode( CNavNode* node )				{ m_navNodes.PushBack( node ); }
	void						CollectionDone();

	void						NodeCreated( CNavNode* node );
	void						NodeRemoved( CNavNode* node );

	template < class Acceptor >
	RED_INLINE CNavNode*		FindClosestNode( const Vector2& pos, Float& closestDistSq, Acceptor& acceptor );
	template < class Acceptor >
	RED_INLINE Uint32			FindNClosestNodes( const Vector2& pos, Float& closestDistSq, Acceptor& acceptor, MultiNodesTestContext& context );
	template < class Functor >
	RED_INLINE Bool				IterateNodes( const KDBox& kdBox, Functor& functor );
};

class CNodeMap : public Red::System::NonCopyable
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_PathLib );

private:
	TDynArray< CCel >				m_grid;
	CNavGraph&						m_navgraph;
	KDBox							m_gridBounds;
	Int16							m_celsX;
	Int16							m_celsY;
	Vector2							m_celSize;
	Float							m_defaultTest;
	AreaRegionId					m_regionId;
	Bool							m_isInitialized;
	Bool							m_isValid;

	RED_INLINE Uint32			GetCelIndex( Int16 x, Int16 y );
	RED_INLINE CCel&			GetCel( Int16 x, Int16 y );
	RED_INLINE void				ComputeCelBox( Int16 x, Int16 y, KDBox& outCelBox );
	RED_INLINE void				GetCelCenter( Int16 x, Int16 y, Vector2& outCelCenter );
	RED_INLINE void				PosToCoord( const Vector2& pos, Int16& x, Int16& y );

	void						InitializeCelMap();
public:
	CNodeMap( CNavGraph& navgraph, AreaRegionId regionId )
		: m_navgraph( navgraph )
		, m_isInitialized( false )
		, m_isValid( false )
		, m_regionId( regionId )													{}

	struct DefaultAcceptor : public Red::System::NonCopyable
	{
		static CNavNode*		InvalidElement()									{ return nullptr; }
	};

	struct FilteringAcceptor : public DefaultAcceptor
	{
		NodeFlags				m_forbiddenFlags;

		FilteringAcceptor( NodeFlags forbiddenFlags )
			: m_forbiddenFlags( forbiddenFlags )									{}

		RED_INLINE Bool			CheckFlags( CNavNode* node ) const					{ return !node->HaveAnyFlag( m_forbiddenFlags ); }
	};

	struct Filtering2DAcceptor : public FilteringAcceptor
	{
		Filtering2DAcceptor( const Vector3& pos, NodeFlags forbiddenFlags )
			: FilteringAcceptor( forbiddenFlags )
			, m_pos( pos )															{}

		RED_INLINE Float		DistSq( CNavNode* node )							{ return (m_pos - node->GetPosition()).SquareMag(); }

		RED_INLINE Float		operator()( CNavNode* node )						{ if ( !CheckFlags( node ) ) return FLT_MAX; return (m_pos.AsVector2() - node->GetPosition().AsVector2()).SquareMag(); }
		Vector3					m_pos;
	};

	struct Filtering3DAcceptor : public FilteringAcceptor
	{
		Filtering3DAcceptor( const Vector3& pos, NodeFlags forbiddenFlags )
			: FilteringAcceptor( forbiddenFlags )
			, m_pos( pos )															{}

		RED_INLINE Float		DistSq( CNavNode* node )							{ return (m_pos - node->GetPosition()).SquareMag(); }

		RED_INLINE Float operator()( CNavNode* node )								{ if ( !CheckFlags( node ) ) return FLT_MAX; return (m_pos - node->GetPosition()).SquareMag(); }
		Vector3					m_pos;
	};

	struct DefaultPosAcceptor : public DefaultAcceptor
	{
		DefaultPosAcceptor( const Vector3& pos )
			: m_pos( pos )															{}

		RED_INLINE Float		Dist3DSq( CNavNode* node )							{ return (m_pos - node->GetPosition()).SquareMag(); }
		RED_INLINE Float		Dist2DSq( CNavNode* node )							{ return (m_pos.AsVector2() - node->GetPosition().AsVector2()).SquareMag(); }

		Vector3					m_pos;
	};

	struct Default2DAcceptor : public DefaultPosAcceptor
	{
		Default2DAcceptor( const Vector3& pos )
			: DefaultPosAcceptor( pos )												{}

		RED_INLINE Float		operator()( CNavNode* node )						{ return (m_pos.AsVector2() - node->GetPosition().AsVector2()).SquareMag(); }
	};

	struct Default3DAcceptor : public DefaultPosAcceptor
	{
		Default3DAcceptor( const Vector3& pos )
			: DefaultPosAcceptor( pos )												{}

		RED_INLINE Float operator()( CNavNode* node )								{ return (m_pos - node->GetPosition()).SquareMag(); }
	};

	void						Initialize();
	Bool						IsInitialized() const								{ return m_isInitialized; }
	void						Clear();

	void						Populate();
	Bool						IsPopulated() const									{ return m_isValid; }
	void						Invalidate();
	void						CompactData();

	void						NodeCreated( CNavNode* node );
	void						NodeRemoved( CNavNode* node );

	// central initialization process
	void						PreCenteralInitialization( CNavGraph& navgraph, AreaRegionId regionId );
	void						CentralInitializationBBoxUpdate( CNavNode& node );
	void						CentralInitilzationComputeCelMap();
	void						CentralInitializationCollectNode( CNavNode& node );
	void						PostCentralInitialization();


	void						WriteToBuffer( CSimpleBufferWriter& writer );
	Bool						ReadFromBuffer( CSimpleBufferReader& reader );

	CNavGraph&					GetNavgraph() const									{ return m_navgraph; }

	template < class Acceptor >
	RED_INLINE CNavNode*		FindClosestNode( const Vector3& pos, Float& maxDist, Acceptor& acceptor );
	template < class Acceptor >
	RED_INLINE Uint32			FindNClosestNodes( const Vector3& pos, Float& maxDist, Acceptor& acceptor, Uint32 outNodesMaxCount, CNavNode::Id* outNodes );
	template < class Acceptor >
	RED_INLINE CNavNode*		FindClosestNodeInBoundings( const Vector3& pos, const Box& boundings, Float& maxDist, Acceptor& acceptor );
	template < class Handler >
	RED_INLINE void				IterateNodes( const Box& boundings, Handler& functor );
};

};		// namespace KD

};		// namespace PathLib

#include "pathlibKDTree.inl"
