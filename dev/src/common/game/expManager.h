/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/core/kdTree.h"
#include "expTreeMediator.h"
#include "expOracle.h"
#include "expCooked.h"
#include "../core/scopedPtr.h"

class IExploration;		

class ExpManager : public ExpFilter< MAX_EXPLORATIONS >
{
#ifndef NO_RESOURCE_COOKING
	friend class CExplorationCookingContext;
#endif

	struct SIndexRange 
	{ 
		Int32 m_min, m_max;
		SIndexRange() {}
		SIndexRange( Int32 mi, Int32 ma ) : m_min( mi ), m_max( ma ) {}
		SIndexRange( Uint64 data ) { *this = *( ( SIndexRange* ) &data ); }
		RED_INLINE Bool IsValid() const { return m_min >= 0 && m_min <= m_max; }
		RED_INLINE operator Uint64() const { return *( ( Uint64* ) this ); }
	};


	typedef TDynArray< IExploration*, MC_Gameplay >			TExpList;
	typedef TDynArray< CCookedExploration, MC_Gameplay >	TExpCookedList;
	typedef TSortedMap< CGUID, Uint64 >						TExpLayerMap;

	ExpTreeMediator		m_treeMediator;
	ExpTree*			m_tree;
	Red::TScopedPtr< ExpOracle > m_oracle;

	TExpList			m_exps;
	TExpList			m_dynamicExps;
	TExpCookedList		m_cookedList;
	TExpLayerMap		m_layerMap;

	Bool				m_isCooked;

public:
	ExpManager();
	virtual ~ExpManager();

	void OnShutdownAtGameEnd();

public:
	Bool LoadCookedData( const DataBuffer* cookedData );

	ExpOracle* GetOracle() const { return m_oracle.Get(); }

	void AddExplorationByComponent( CExplorationComponent* e );
	void RemoveExplorationByComponent( CExplorationComponent* e );

	void OnLayerAttached( const CLayer* layer );
	void OnLayerDetached( const CLayer* layer );

	void AddDynamicExploration( IExploration* e );
	void RemoveDynamicExploration( IExploration* e );

	Bool QueryExplorationSync( SExplorationQueryToken & token, const CEntity* ent ) const;
	Bool QueryExplorationFromObjectSync( SExplorationQueryToken & token, const CEntity* ent, const CEntity* object ) const;

public:
	virtual IExpExecutor* CreateTransition( const IExploration* form, const IExploration* to, ExecutorSetup& setup ) const;
	virtual const IExploration* GetNNForExploration( const IExploration* exploration, ExpRelativeDirection dir, const CEntity* entity ) const;

	void FindNN( Vector const & worldPos, const IExploration* withoutExp, IExplorationList& out ) const;

private:
	void AddExplorationToList( IExploration* e );
	void Clear();
	void RebuildTree( Uint8* cookedTreeData, Uint32 cookedTreeDataSize );
	SIndexRange FindRangeForLayer( const CLayer* layer ) const;

	void CollectExplorationFromObject( const CEntity* object, IExplorationList& out ) const;

	RED_INLINE Int32 NumExps() const { return m_isCooked ? m_cookedList.SizeInt() : m_exps.SizeInt(); }
	RED_INLINE const IExploration* Exp( Int32 idx ) const { return m_isCooked ? &m_cookedList[ idx ] : m_exps[ idx ]; }
};
