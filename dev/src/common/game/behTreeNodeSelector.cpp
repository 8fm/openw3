#include "build.h"
#include "behTreeNodeSelector.h"

////////////////////////////////////////////////////////////////////////
// GetNodeCaption()
////////////////////////////////////////////////////////////////////////
String CBehTreeNodeChoiceDefinition::GetNodeCaption() const
{
	return m_useScoring
		? TXT("Choice [score]")
		: TXT("Choice");
}

String CBehTreeNodeEvaluatingChoiceDefinition::GetNodeCaption() const
{
	return m_useScoring
		? TXT("EvaluatingChoice [score]")
		: TXT("EvaluatingChoice");
}

String CBehTreeNodeSelectorDefinition::GetNodeCaption() const
{
	return m_useScoring
		? TXT("Selector [score]")
		: TXT("Selector");
}

String CBehTreeNodeEvaluatingSelectorDefinition::GetNodeCaption() const
{
	return m_useScoring
		? TXT("EvaluatingSelector [score]")
		: TXT("EvaluatingSelector");
}

////////////////////////////////////////////////////////////////////////
// TBehTreeNodeChoiceInstance
////////////////////////////////////////////////////////////////////////
template< class SelectionMethod >
Bool TBehTreeNodeChoiceInstance< SelectionMethod >::Activate()
{
	if( this->Think( this, m_activeChild ) == Selector::THINK_FAILED )
	{
		DebugNotifyActivationFail();
		return false;	
	}
	
	return Super::Activate();
}

template< class SelectionMethod >
void TBehTreeNodeChoiceInstance< SelectionMethod >::OnSubgoalCompleted( eTaskOutcome outcome )
{
	m_activeChild = INVALID_CHILD;
	if ( m_forwardChildrenCompletness )
	{
		Complete( outcome );
	}
	else
	{
		if ( this->Think( this, m_activeChild ) == Selector::THINK_FAILED )
		{
			Complete( BTTO_FAILED );
		}
	}
}

////////////////////////////////////////////////////////////////////////
// TBehTreeNodeEvaluatingChoiceInstance
////////////////////////////////////////////////////////////////////////

template< class SelectionMethod >
Bool TBehTreeNodeEvaluatingChoiceInstance< SelectionMethod >::IsAvailable()
{
	if ( SelectionMethod::SelectChild( this ) != Super::INVALID_CHILD )
	{
		return Super::IsAvailable();
	}

	Super::DebugNotifyAvailableFail();
	return false;
}

template< class SelectionMethod >
Int32 TBehTreeNodeEvaluatingChoiceInstance< SelectionMethod >::Evaluate()
{
	Int32 bestChild = SelectionMethod::SelectChild( this );
	if ( SelectionMethod::CACHE_EVALUATION_VALUE )
	{
		return SelectionMethod::CachedScore( this );
	}
	else
	{
		if ( bestChild != Super::INVALID_CHILD )
		{
			return this->m_children[ bestChild ]->Evaluate();
		}

		Super::DebugNotifyAvailableFail();
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectorInstance
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeSelectorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( m_activeChild != INVALID_CHILD )
	{
		if ( m_children[ m_activeChild ]->OnEvent( e ) )
		{
			m_nextTest = 0.f;
			return true;
		}
	}
	return false;
}
void CBehTreeNodeSelectorInstance::MarkDirty()
{
	m_nextTest = 0.f;
}
void CBehTreeNodeSelectorInstance::MarkParentSelectorDirty()
{
	m_nextTest = 0.f;
}

////////////////////////////////////////////////////////////////////////
// TBehTreeNodeSelectorInstance
////////////////////////////////////////////////////////////////////////
template< class SelectionMethod >
void TBehTreeNodeSelectorInstance< SelectionMethod >::Update()
{
	Float currentTime = m_owner->GetLocalTime();
	if ( m_activeChild == INVALID_CHILD || m_nextTest < currentTime )
	{
		m_nextTest = currentTime + m_checkFrequency;
		switch ( this->Think( this, m_activeChild ) )
		{
		case Selector::THINK_FAILED:
			if ( m_forwardChildrenCompletness )
			{
				Complete( BTTO_FAILED );
				return;
			}
			m_nextTest = 0.f;
			return;
		case Selector::THINK_DELAYED:
			m_nextTest = 0.f;
			break;
		case Selector::THINK_COOL:
			break;
		default:
			ASSERT( false );
			ASSUME( false );
		}
	}
	Super::Update();
}
template< class SelectionMethod >
Bool TBehTreeNodeSelectorInstance< SelectionMethod >::Activate()
{
	Float currentTime = m_owner->GetLocalTime();
	m_nextTest = currentTime + m_checkFrequency;
	if ( this->Think( this, m_activeChild ) != Selector::THINK_FAILED )
	{
		if ( SelectionMethod::USE_CACHED_THINK_OUTPUT )
		{
			SelectionMethod::ClearCachedOutput( this );
		}
		
		return Super::Activate();
	}
	else if ( m_forwardChildrenCompletness )
	{
		Super::DebugNotifyActivationFail();
		return false;
	}
	m_nextTest = 0.f;
	return Super::Activate();
}

template< class SelectionMethod >
void TBehTreeNodeSelectorInstance< SelectionMethod >::OnSubgoalCompleted( eTaskOutcome outcome )
{
	m_nextTest = 0.f;
	m_activeChild = INVALID_CHILD;
	if ( m_forwardChildrenCompletness )
	{
		Complete( outcome );
	}
	else
	{
		if ( this->Think( this, m_activeChild ) == Selector::THINK_COOL )
		{
			m_nextTest = m_owner->GetLocalTime() + m_checkFrequency;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// TBehTreeNodeEvaluatingSelectorInstance
//////////////////////////////////////////////////////////////////////////
template < class SelectionMethod >
Bool TBehTreeNodeEvaluatingSelectorInstance< SelectionMethod >::IsAvailable()
{
	if ( SelectionMethod::SelectChild( this ) != Super::INVALID_CHILD )
	{
		return Super::IsAvailable();
	}

	Super::DebugNotifyAvailableFail();
	return false;
}

template < class SelectionMethod >
Int32 TBehTreeNodeEvaluatingSelectorInstance< SelectionMethod >::Evaluate()
{
	Int32 bestChild = SelectionMethod::SelectChild( this );
	if ( SelectionMethod::CACHE_EVALUATION_VALUE )
	{
		return SelectionMethod::CachedScore( this );
	}
	else
	{
		if ( bestChild != Super::INVALID_CHILD )
		{
			return this->m_children[ bestChild ]->Evaluate();
		}

		Super::DebugNotifyAvailableFail();
		return -1;
	}
}

template < class SelectionMethod >
void TBehTreeNodeEvaluatingSelectorInstance< SelectionMethod >::Deactivate()
{
	SelectionMethod::ClearCachedOutput( this );

	Super::Deactivate();
}
//template <> class TBehTreeNodeSelectorInstance< FSelectorFindAvailable >;
//template <> class TBehTreeNodeSelectorInstance< FSelectorFindBestScore >;



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeChoiceDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeCompositeInstance* CBehTreeNodeChoiceDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_useScoring )
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeRandomBestScoreChoice( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeBestScoreChoice( *this, owner, context, parent );
		}

	}
	else
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeRandomAvailableChoice( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeFirstAvailableChoice( *this, owner, context, parent );
		}
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeEvaluatingChoiceDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance* CBehTreeNodeEvaluatingChoiceDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_useScoring )
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeEvaluatingRandomBestScoreChoice( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeEvaluatingBestScoreChoice( *this, owner, context, parent );
		}

	}
	else
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeEvaluatingRandomAvailableChoice( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeEvaluatingFirstAvailableChoice( *this, owner, context, parent );
		}
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeEvaluatingSelectorDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeCompositeInstance* CBehTreeNodeEvaluatingSelectorDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_useScoring )
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeEvaluatingRandomBestScoreSelector( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeEvaluatingBestScoreSelector( *this, owner, context, parent );
		}

	}
	else
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeEvaluatingRandomAvailableSelector( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeEvaluatingFirstAvailableSelector( *this, owner, context, parent );
		}
	}
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectorDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance* CBehTreeNodeSelectorDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_useScoring )
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeRandomBestScoreSelector( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeBestScoreSelector( *this, owner, context, parent );
		}

	}
	else
	{
		if ( m_selectRandom )
		{
			return new CBehTreeNodeRandomAvailableSelector( *this, owner, context, parent );
		}
		else
		{
			return new CBehTreeNodeFirstAvailableSelector( *this, owner, context, parent );
		}
	}
}