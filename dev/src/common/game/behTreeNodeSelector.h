#pragma once

#include "behTreeNodeComposite.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"

class CBehTreeNodeChoiceInstance;
class CBehTreeNodeSelectorInstance;

////////////////////////////////////////////////////////////////////////
// Choice node
// On activation selects child behavior (
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeChoiceDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeChoiceDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodeChoiceInstance, Choice );
protected:
	Bool				m_useScoring;
	Bool				m_selectRandom;
	Bool				m_forwardChildrenCompletness;


	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeChoiceDefinition()
		: m_useScoring( false )
		, m_selectRandom( false )
		, m_forwardChildrenCompletness( true )						{}

	virtual String GetNodeCaption() const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeChoiceDefinition );
	PARENT_CLASS( IBehTreeNodeCompositeDefinition );
	PROPERTY_EDIT( m_useScoring, TXT("Use scoring evaluation or if not, accept first available.") );
	PROPERTY_EDIT( m_selectRandom, TXT("Select random available/highest priority child insteady of left-most.") );
	PROPERTY_EDIT( m_forwardChildrenCompletness, TXT("Forward children completness or remain active") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Choice instance class
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeChoiceInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;
protected:
	Bool					m_forwardChildrenCompletness;
public:
	typedef CBehTreeNodeChoiceDefinition Definition;
	CBehTreeNodeChoiceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_forwardChildrenCompletness( def.m_forwardChildrenCompletness ) {}
};

template< class SelectionMethod >
class TBehTreeNodeChoiceInstance : public CBehTreeNodeChoiceInstance, public SelectionMethod
{
	typedef CBehTreeNodeChoiceInstance Super;
	friend SelectionMethod;
	
public:
	typedef CBehTreeNodeChoiceInstance::Definition Definition;
	
	TBehTreeNodeChoiceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, SelectionMethod()												{}

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool Activate() override;
	void OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome ) override;
};

template < class SelectionMethod >
class TBehTreeNodeEvaluatingChoiceInstance : public TBehTreeNodeChoiceInstance< SelectionMethod >
{
	typedef TBehTreeNodeChoiceInstance< SelectionMethod > Super;
public:
	TBehTreeNodeEvaluatingChoiceInstance( const CBehTreeNodeChoiceInstance::Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )							{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;
};


//////////////////////////////////////////////////////////////////////////
//// Evaluating choice definition
//////////////////////////////////////////////////////////////////////////
class CBehTreeNodeEvaluatingChoiceDefinition : public CBehTreeNodeChoiceDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeEvaluatingChoiceDefinition, CBehTreeNodeChoiceDefinition, CBehTreeNodeChoiceInstance, EvaluatingChoice );

public:
	CBehTreeNodeEvaluatingChoiceDefinition () {}

	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	virtual String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeEvaluatingChoiceDefinition  );
	PARENT_CLASS( CBehTreeNodeChoiceDefinition );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Selector node
// Frequently check for active object
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeSelectorDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectorDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodeSelectorInstance, Selector );
protected:
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	Float				m_checkFrequency;
	Bool				m_useScoring;
	Bool				m_selectRandom;
	Bool				m_forwardChildrenCompletness;

public:
	CBehTreeNodeSelectorDefinition()
		: m_checkFrequency( 2.f )
		, m_useScoring( false )
		, m_selectRandom( false )
		, m_forwardChildrenCompletness( false )							{}

	virtual String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeSelectorDefinition);
	PARENT_CLASS(IBehTreeNodeCompositeDefinition);
	PROPERTY_EDIT( m_checkFrequency, TXT("Frequency of automated decision reevaluations") );
	PROPERTY_EDIT( m_useScoring, TXT("Use scoring evaluation or if not, accept first available.") );
	PROPERTY_EDIT( m_selectRandom, TXT("Select random available/highest priority child insteady of left-most.") );
	PROPERTY_EDIT( m_forwardChildrenCompletness, TXT("If set to true, selector will 'complete' itself when children is completed(insteady of reevaluation).") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// General selector instance class
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectorInstance : public IBehTreeNodeCompositeInstance
{
protected:
	Float				m_nextTest;
	Float				m_checkFrequency;
	Bool				m_forwardChildrenCompletness;

public:
	typedef CBehTreeNodeSelectorDefinition Definition;

	CBehTreeNodeSelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeCompositeInstance( def, owner, context, parent )
		, m_nextTest( 0.f )
		, m_checkFrequency( def.m_checkFrequency )
		, m_forwardChildrenCompletness( def.m_forwardChildrenCompletness )
	{}

	Bool OnEvent( CBehTreeEvent& e ) override;
	void MarkDirty() override;
	void MarkParentSelectorDirty() override;
};

template< class SelectionMethod >
class TBehTreeNodeSelectorInstance : public CBehTreeNodeSelectorInstance, public SelectionMethod
{
	friend SelectionMethod;
	typedef CBehTreeNodeSelectorInstance Super;
public:
	typedef typename CBehTreeNodeSelectorInstance::Definition Definition;

	TBehTreeNodeSelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, SelectionMethod()												{}

	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
};

template< class SelectionMethod >
class TBehTreeNodeEvaluatingSelectorInstance : public TBehTreeNodeSelectorInstance< SelectionMethod >
{
	typedef TBehTreeNodeSelectorInstance< SelectionMethod > Super;
public:
	TBehTreeNodeEvaluatingSelectorInstance( const CBehTreeNodeSelectorInstance::Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )							{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	void Deactivate() override;
};



//////////////////////////////////////////////////////////////////////////
//// Evaluating selector definition
//////////////////////////////////////////////////////////////////////////
class CBehTreeNodeEvaluatingSelectorDefinition : public CBehTreeNodeSelectorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeEvaluatingSelectorDefinition, CBehTreeNodeSelectorDefinition, CBehTreeNodeSelectorInstance, EvaluatingSelector );

public:
	CBehTreeNodeEvaluatingSelectorDefinition() {}

	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	virtual String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeEvaluatingSelectorDefinition  );
	PARENT_CLASS( CBehTreeNodeSelectorDefinition );
END_CLASS_RTTI();


namespace Selector
{
	enum EThinkResult
	{
		THINK_FAILED	= 0,
		THINK_COOL		= 1,
		THINK_DELAYED	= 2,
	};

	////////////////////////////////////////////////////////////////////////
	// Selector caching strategies
	////////////////////////////////////////////////////////////////////////
	struct FEmptyCachingStrategy
	{
	protected:
		typedef IBehTreeNodeInstance Node;
		enum { USE_CACHED_THINK_OUTPUT = false, CACHE_EVALUATION_VALUE = false };

		RED_INLINE Bool HasCachedThinkOutput( Node* node ) const						{ ASSERT( false ); ASSUME( false ); return false; }
		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output )				{ ASSUME( false ); }
		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output, Int32 score )	{ ASSUME( false ); }
		RED_INLINE void ClearCachedOutput( Node* node )									{ ASSUME( false ); }
		RED_INLINE Int32 CachedThinkOutput( Node* node ) const							{ ASSERT( false ); ASSUME( false ); return -1; }
		RED_INLINE Int32 CachedScore( Node* node ) const								{ ASSERT( false ); ASSUME( false ); return -1; }
	};

	struct FCachingAvailabilityStrategy : public FEmptyCachingStrategy
	{
	private:
		typedef FEmptyCachingStrategy Super;
	protected:
		enum { USE_CACHED_THINK_OUTPUT = true };

		Float	m_lastSelectionTime;
		Int32	m_lastBestChild;

		FCachingAvailabilityStrategy()
			: Super()
			, m_lastSelectionTime( -1.f )
			, m_lastBestChild( IBehTreeNodeCompositeInstance::INVALID_CHILD ) {}

		RED_INLINE Bool HasCachedThinkOutput( Node* node ) const						{ return m_lastSelectionTime >= node->GetOwner()->GetLocalTime(); }
		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output )				{ m_lastSelectionTime = node->GetOwner()->GetLocalTime(); m_lastBestChild = output; }
		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output, Int32 score )	{ SetCachedThinkOutput( node, output ); }
		RED_INLINE void ClearCachedOutput( Node* node )									{ m_lastSelectionTime = 0.f; }
		RED_INLINE Int32 CachedThinkOutput( Node* node ) const							{ ASSERT( HasCachedThinkOutput( node ) ); return m_lastBestChild; }
	};

	struct FCachingScoreStrategy : public FCachingAvailabilityStrategy
	{
	private:
		typedef FCachingAvailabilityStrategy Super;
	protected:
		enum { CACHE_EVALUATION_VALUE = true };

		Int32			m_lastScore;

		FCachingScoreStrategy()
			: Super()
			, m_lastScore( -1 ) {}

		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output )				{ Super::SetCachedThinkOutput( node, output ); ASSERT( false ); ASSUME( false ); }
		RED_INLINE void SetCachedThinkOutput( Node* node, Int32 output, Int32 score )	{ Super::SetCachedThinkOutput( node, output ); m_lastScore = score; }
		RED_INLINE Int32 CachedScore( Node* node ) const								{ ASSERT( HasCachedThinkOutput( node ) ); return m_lastScore; }
	};

	template < class TCachingStrategy >
	struct FSelectorFindAvailable : public TCachingStrategy
	{
		typedef TCachingStrategy TBaseClass;

		RED_INLINE Uint32 SelectChild( IBehTreeNodeCompositeInstance* owner )
		{
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				if ( TBaseClass::HasCachedThinkOutput( owner ) )
				{
					return TBaseClass::CachedThinkOutput( owner );
				}
			}
			Uint32 bestChild = IBehTreeNodeCompositeInstance::INVALID_CHILD;
			const auto& children = owner->GetChildren();
			for ( Uint32 i = 0, n = children.Size(); i < n; ++i )
			{
				if ( children[ i ]->IsAvailable() )
				{
					bestChild = i;
					break;
				}
			}
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				TBaseClass::SetCachedThinkOutput( owner, bestChild );
			}
			return bestChild;
		}
	};


	////////////////////////////////////////////////////////////////////////
	// Selector class selection methods
	////////////////////////////////////////////////////////////////////////
	template < class TCachingStrategy >
	struct FSelectorFindBestScore : public TCachingStrategy
	{
		typedef TCachingStrategy TBaseClass;

		Uint32 SelectChild( IBehTreeNodeCompositeInstance* owner )
		{
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				if ( TBaseClass::HasCachedThinkOutput( owner ) )
				{
					return TBaseClass::CachedThinkOutput( owner );
				}
			}
			Int32 bestScore = -1;
			Uint32 bestChild = IBehTreeNodeCompositeInstance::INVALID_CHILD;
			const auto& children = owner->GetChildren();
			for ( Uint32 i = 0, n = children.Size(); i < n; ++i )
			{
				Int32 eval = children[ i ]->Evaluate();
				if ( eval > bestScore )
				{
					bestScore = eval;
					bestChild = i;
				}
			}
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				TBaseClass::SetCachedThinkOutput( owner, bestChild, bestScore );
			}
			return bestChild;
		}
	};

	template < class TCachingStrategy >
	struct FSelectorFindRandomAvailable : public TCachingStrategy
	{
		typedef TCachingStrategy TBaseClass;

		Uint32 SelectChild( IBehTreeNodeCompositeInstance* owner )
		{
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				if ( TBaseClass::HasCachedThinkOutput( owner ) )
				{
					return TBaseClass::CachedThinkOutput( owner );
				}
			}

			const auto& children = owner->GetChildren();

			{
				Uint32 currChild = owner->GetActiveChildIndex();
				if( owner->IsActive() && currChild != IBehTreeNodeCompositeInstance::INVALID_CHILD )
				{
					if ( children[ currChild ]->IsAvailable() )
					{
						return currChild;
					}
				}
			}

			Uint32 bestChild = IBehTreeNodeCompositeInstance::INVALID_CHILD;
			Float availableCount = 0.f;

			for ( Uint32 i = 0, n = children.Size(); i < n; ++i )
			{
				if ( children[ i ]->IsAvailable() )
				{
					if ( availableCount == 0.f )
					{
						availableCount = 1.f;
						bestChild = i;
					}
					else
					{
						availableCount += 1.f;
						Float chance = 1.f / availableCount;
						if ( GEngine->GetRandomNumberGenerator().Get< Float >() < chance )
						{
							bestChild = i;
						}

					}
				}
			}
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				TBaseClass::SetCachedThinkOutput( owner, bestChild );
			}
			return bestChild;
		}
	};

	template < class TCachingStrategy >
	struct FSelectorFindRandomBestScore : public TCachingStrategy
	{
		typedef TCachingStrategy TBaseClass;

		Uint32 SelectChild( IBehTreeNodeCompositeInstance* owner )
		{
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				if ( TBaseClass::HasCachedThinkOutput( owner ) )
				{
					return TBaseClass::CachedThinkOutput( owner );
				}
			}

			const Uint32 currChild = owner->GetActiveChildIndex();

			Int32 bestScore = -1;
			Bool bestScoreLocked = true;
			Float bestScoreCount = 0.f;

			Uint32 bestChild = IBehTreeNodeCompositeInstance::INVALID_CHILD;

			const auto& children = owner->GetChildren();
			for ( Uint32 i = 0, n = children.Size(); i < n; ++i )
			{
				auto* childNode = children[ i ];
				Int32 eval = childNode->Evaluate();
				if ( eval > bestScore )
				{
					bestScore = eval;
					bestChild = i;
					bestScoreCount = 1.f;
					bestScoreLocked = i == currChild;
				}
				else if ( eval == bestScore && !bestScoreLocked )
				{
					if ( i == currChild )
					{
						bestChild = i;
						bestScoreLocked = true;
					}
					else
					{
						bestScoreCount += 1.f;
						Float chance = 1.f / bestScoreCount;
						if ( GEngine->GetRandomNumberGenerator().Get< Float >() < chance )
						{
							bestChild = i;
						}
					}
				}
			}
			if ( TBaseClass::USE_CACHED_THINK_OUTPUT )
			{
				TBaseClass::SetCachedThinkOutput( owner, bestChild, bestScore );
			}
			return bestChild;
		}
	};

	template < class FSelector >
	struct Logic : public FSelector
	{
		RED_INLINE EThinkResult Think( IBehTreeNodeCompositeInstance* node, Uint32& activeChild )
		{
			Uint32 bestChild = FSelector::SelectChild( node );

			if ( bestChild != activeChild )
			{
				if ( activeChild != IBehTreeNodeCompositeInstance::INVALID_CHILD )
				{
					if ( !node->GetChildren()[ activeChild ]->Interrupt() )
					{
						return Selector::THINK_DELAYED;
					}
				}
				if ( bestChild != IBehTreeNodeCompositeInstance::INVALID_CHILD && node->GetChildren()[ bestChild ]->Activate() )
				{
					activeChild = bestChild;
					return Selector::THINK_COOL;
				}
				else
				{
					activeChild = IBehTreeNodeCompositeInstance::INVALID_CHILD;
					return Selector::THINK_FAILED;
				}
			}
			return (bestChild != IBehTreeNodeCompositeInstance::INVALID_CHILD)
				? Selector::THINK_COOL
				: Selector::THINK_FAILED;
		}
	};

};		// namespace Selector



typedef TBehTreeNodeChoiceInstance< Selector::Logic< Selector::FSelectorFindAvailable< Selector::FEmptyCachingStrategy > > >							CBehTreeNodeFirstAvailableChoice;
typedef TBehTreeNodeChoiceInstance< Selector::Logic< Selector::FSelectorFindBestScore< Selector::FEmptyCachingStrategy > > >							CBehTreeNodeBestScoreChoice;
typedef TBehTreeNodeChoiceInstance< Selector::Logic< Selector::FSelectorFindRandomAvailable< Selector::FEmptyCachingStrategy > > >						CBehTreeNodeRandomAvailableChoice;
typedef TBehTreeNodeChoiceInstance< Selector::Logic< Selector::FSelectorFindRandomBestScore< Selector::FEmptyCachingStrategy > > >						CBehTreeNodeRandomBestScoreChoice;

typedef TBehTreeNodeEvaluatingChoiceInstance< Selector::Logic< Selector::FSelectorFindAvailable< Selector::FCachingAvailabilityStrategy > > >			CBehTreeNodeEvaluatingFirstAvailableChoice;
typedef TBehTreeNodeEvaluatingChoiceInstance< Selector::Logic< Selector::FSelectorFindBestScore< Selector::FCachingScoreStrategy > > >					CBehTreeNodeEvaluatingBestScoreChoice;
typedef TBehTreeNodeEvaluatingChoiceInstance< Selector::Logic< Selector::FSelectorFindRandomAvailable< Selector::FCachingAvailabilityStrategy > > >		CBehTreeNodeEvaluatingRandomAvailableChoice;
typedef TBehTreeNodeEvaluatingChoiceInstance< Selector::Logic< Selector::FSelectorFindRandomBestScore< Selector::FCachingScoreStrategy > > >			CBehTreeNodeEvaluatingRandomBestScoreChoice;

typedef TBehTreeNodeSelectorInstance< Selector::Logic< Selector::FSelectorFindAvailable< Selector::FEmptyCachingStrategy > > >							CBehTreeNodeFirstAvailableSelector;
typedef TBehTreeNodeSelectorInstance< Selector::Logic< Selector::FSelectorFindBestScore< Selector::FEmptyCachingStrategy > > >							CBehTreeNodeBestScoreSelector;
typedef TBehTreeNodeSelectorInstance< Selector::Logic< Selector::FSelectorFindRandomAvailable< Selector::FEmptyCachingStrategy > > >					CBehTreeNodeRandomAvailableSelector;
typedef TBehTreeNodeSelectorInstance< Selector::Logic< Selector::FSelectorFindRandomBestScore< Selector::FEmptyCachingStrategy > > >					CBehTreeNodeRandomBestScoreSelector;

typedef TBehTreeNodeEvaluatingSelectorInstance< Selector::Logic< Selector::FSelectorFindAvailable< Selector::FCachingAvailabilityStrategy > > >			CBehTreeNodeEvaluatingFirstAvailableSelector;
typedef TBehTreeNodeEvaluatingSelectorInstance< Selector::Logic< Selector::FSelectorFindBestScore< Selector::FCachingScoreStrategy > > >				CBehTreeNodeEvaluatingBestScoreSelector;
typedef TBehTreeNodeEvaluatingSelectorInstance< Selector::Logic< Selector::FSelectorFindRandomAvailable< Selector::FCachingAvailabilityStrategy > > >	CBehTreeNodeEvaluatingRandomAvailableSelector;
typedef TBehTreeNodeEvaluatingSelectorInstance< Selector::Logic< Selector::FSelectorFindRandomBestScore< Selector::FCachingScoreStrategy > > >			CBehTreeNodeEvaluatingRandomBestScoreSelector;
