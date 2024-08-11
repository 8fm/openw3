#include "build.h"
#include "idThreadActivator.h"

#include "idCondition.h"

#include "../../common/core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CIDActivator )
IMPLEMENT_RTTI_ENUM( EIDPriority )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CIDActivator::CIDActivator()
	: m_condition		( NULL )
	, m_priorityDefault	( IDP_Irrelevant )
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDActivator::OnInitInstance( InstanceBuffer& data ) const
{
	data[ i_isLocked ] = 0;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDActivator::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_isLocked;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CIDActivator::IsFulfilled( InstanceBuffer& data, Uint32	dialogInstanceID ) const
{
	// Locking a thread makes it impossible to activate
	if( data[i_isLocked] )
	{
		return false;
	}

	// If there is no condition, the thread is valid
	if( m_condition	== NULL || !IsValidObject( m_condition ) )
	{
		return true;
	}	

	// Check the quest condition
	Bool l_QuestConditionResult	= false;

	l_QuestConditionResult		= m_condition->IsFulfilled( dialogInstanceID );

	return l_QuestConditionResult;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void	CIDActivator::SetLocked( InstanceBuffer& data, Bool _locked ) const
{	
	data[i_isLocked]	= _locked;			
}
