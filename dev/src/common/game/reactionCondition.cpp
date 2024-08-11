/**
* Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "reactionCondition.h"
#include "interestPointComponent.h"
#include "potentialField.h"

IMPLEMENT_ENGINE_CLASS( IReactionCondition );
IMPLEMENT_ENGINE_CLASS( CReactionOrCondition );
IMPLEMENT_ENGINE_CLASS( CReactionAndCondition );
IMPLEMENT_ENGINE_CLASS( CReactionScriptedCondition );

/////////////////////////////////////////////////////////////////

CReactionScriptedCondition::CReactionScriptedCondition()
{

}

Bool CReactionScriptedCondition::Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance )
{
	Bool res = true;
	CallFunctionRet( this, CNAME( Perform ), THandle< CNode >( const_cast<CNode*>(source) ), THandle< CNode >( const_cast<CNode*>(target) ), THandle< CInterestPointInstance >( instance ), res );
	return res;
}

/////////////////////////////////////////////////////////////////

Bool CReactionOrCondition::Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance )
{
	for ( TDynArray<IReactionCondition*>::iterator it = m_conditions.Begin(); it != m_conditions.End(); ++it )
		if ((*it)->Perform(source, target, instance))
			return true;
	return false;
}


/////////////////////////////////////////////////////////////////

Bool CReactionAndCondition::Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance )
{
	for ( TDynArray<IReactionCondition*>::iterator it = m_conditions.Begin(); it != m_conditions.End(); ++it )
		if (!(*it)->Perform(source, target, instance))
			return false;
	return true;
}
