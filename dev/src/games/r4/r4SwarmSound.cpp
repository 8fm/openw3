#include "build.h"
#include "r4SwarmSound.h"
#include "flyingCrittersAI.h"
#include "flyingCrittersParams.h"
#include "flyingSwarmGroup.h"
#include "../../common/core/feedback.h"

////////////////////////////////////////////////////////////
//  CGroupStateSwarmSoundFilter

CGroupStateSwarmSoundFilter::CGroupStateSwarmSoundFilter()
	: CBaseSwarmSoundFilter( )
	, m_groupState( CName::NONE )
{

}

Bool CGroupStateSwarmSoundFilter::FilterFlyingGroup( const CFlyingSwarmGroup & group )const
{
	if ( group.m_currentGroupState == m_groupState )
	{
		return true;
	}
	return false;
}

Bool CGroupStateSwarmSoundFilter::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const
{
	return true;
}
Bool CGroupStateSwarmSoundFilter::FilterBoid( const CBaseCritterAI & baseAI )const
{
	return true;
}

Bool CGroupStateSwarmSoundFilter::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )
{
	// First filling generic attributesstatic_cast
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( groupState_name ) )
		{
			const Uint32 groupStateIndex = static_cast< const CFlyingCritterLairParams * >(boidLairParams)->GetGroupStateIndexFromName( att.m_attributeValueAsCName );
			if ( groupStateIndex == (Uint32)-1 )
			{
				GFeedback->ShowError( TXT("Boid XML Error: sound filter unrekognized boidState: %s"), att.m_attributeValueAsCName.AsString().AsChar() );
				return false;
			}
			m_groupState = att.m_attributeValueAsCName;
		}
		else 
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound filter unrekognized attribute: %s"), att.m_attributeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// CR4SwarmSoundConfig


CBaseSwarmSoundFilter*const CR4SwarmSoundConfig::CreateXmlFromXmlAtt( const SCustomNodeAttribute & att )
{
	if ( att.m_attributeName == CNAME( groupState_name ) )
	{
		return new CGroupStateSwarmSoundFilter();
	}
	return CSwarmSoundConfig::CreateXmlFromXmlAtt( att );
}

Bool CR4SwarmSoundConfig::FilterGroup( const CFlyingSwarmGroup & group )const
{
	if ( m_rootFilter == NULL )
	{
		return false;
	}

	return m_rootFilter->FilterFlyingGroup( group ); 
}