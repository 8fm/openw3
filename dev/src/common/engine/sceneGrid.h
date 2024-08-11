/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once 
#include "../core/hashset.h"

class CRenderFrame;
class CBeamTree;
class CHierarchicalGrid;

class IHierarchicalGridElement
{
	friend class CHierarchicalGrid;
	friend class CHierarchicalGridNode;

	Int32 m_hierarchialTestIndex;		//!< Marks if this component was tested for visibility

public:
	RED_INLINE virtual const Box& GetBoundingBox() const = 0;
};

class CHierarchicalGridNode
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	friend class CHierarchicalGrid;
	friend class CVisibilityContext;

protected:
	Int32										m_size[2];
	CHierarchicalGridNode*					m_parent;
	CHierarchicalGridNode*					m_children;
	TDynArray< IHierarchicalGridElement* >	m_components;

public:
	CHierarchicalGridNode( Int32 xSize = 0, Int32 ySize = 0 );

	void Insert( IHierarchicalGridElement* component, Int32 level, Float xMin, Float xMax, Float yMin, Float yMax, CHierarchicalGrid* gridContext );
	void Remove( IHierarchicalGridElement* component, THashMap< IHierarchicalGridElement*, TDynArray< CHierarchicalGridNode* > >& componentToNodeMapping );

	void Find( const Box& box, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& outComponents ) const;
	void Find( const Box& box, Float xMin, Float xMax, Float yMin, Float yMax, THashSet< IHierarchicalGridElement* >& outComponents ) const;
	void Find( const Vector &point, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& outComponents ) const;
	void FindClosest( const Vector& point, Float xMin, Float xMax, Float yMin, Float yMax, IHierarchicalGridElement*& outComponents, Float& outDistance ) const;

	void GenerateFragments( CRenderFrame* frame, Color color, Int32 count, Float xMin, Float xMax, Float yMin, Float yMax ) const;
	void CollectVisibility( const CBeamTree* beamTree, const Uint32 beamTreeNode, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const;
	void CollectComponents( const CBeamTree* beamTree, Bool recurse, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const;

protected:
	void AddComponentsFromThisNode( THashSet< IHierarchicalGridElement* >& visibleComponents ) const;
	void AddComponentsFromThisNodeRecursive( THashSet< IHierarchicalGridElement* >& visibleComponents ) const;

	Bool InsertToSubNodes( IHierarchicalGridElement* component, Int32 level, Float xMin, Float xMax, Float yMin, Float yMax, CHierarchicalGrid* gridContext );
	void Split( const Int32 xSize, const Int32 ySize );
	void Merge( CHierarchicalGrid* gridContext );
};

class CHierarchicalGrid
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	friend class CHierarchicalGridNode;
	friend class CVisibilityContext;

protected:
	Int32																				m_maxLevels;
	Int32																				m_minComponents;
	Int32																				m_maxComponents;
	Box																				m_boundingBox;
	CHierarchicalGridNode*															m_rootNode;
	THashMap< IHierarchicalGridElement*, TDynArray< CHierarchicalGridNode* > >		m_componentToNodesMapping;

private:
	static Int32				st_hierarchialTestIndex;
	static Red::Threads::CMutex	st_collectVisbilityLock;
	static Red::Threads::CMutex	st_insertUpdateRemoveLock;

public:
	CHierarchicalGrid( const Float gridSize, const Int32 xSize, const Int32 ySize, const Int32 maxLevels = 4, const Int32 minComponents = 10, const Int32 maxComponents = 10 );
	~CHierarchicalGrid();

	void						Insert( IHierarchicalGridElement* component );
	void						Remove( IHierarchicalGridElement* component );
	void						Update( IHierarchicalGridElement* component );

	void						Find( const Box& box, THashSet< IHierarchicalGridElement* >& outComponents ) const;
	void						Find( const Box& box, TDynArray< IHierarchicalGridElement* >& outComponents ) const;
	void						Find( const Vector &point, TDynArray< IHierarchicalGridElement* > &outComponents ) const;
	void						Find( const Sphere& sphere, TDynArray< IHierarchicalGridElement* >& outComponents ) const;
	IHierarchicalGridElement*	FindClosest( const Vector& point ) const;

	void	CollectVisibility( const CBeamTree* beamTree, const Uint32 beamTreeNode, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const;

	void	GenerateFragments( CRenderFrame* frame, Color color ) const;
};