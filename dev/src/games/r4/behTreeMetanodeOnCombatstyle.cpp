/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeMetanodeOnCombatstyle.h"

RED_DEFINE_STATIC_NAME( EBehaviorGraph )

Bool CBehTreeMetanodeDecorateOnCombatstyle::CheckCondition( CBehTreeSpawnContext& context ) const
{
	return ( m_combatStyleId == context.GetVal( m_behaviorGraphVarName, -1 ) );
}

String CBehTreeMetanodeDecorateOnCombatstyle::GetNodeCaption() const
{
	CEnum* enumType = SRTTI::GetInstance().FindEnum( CNAME( EBehaviorGraph ) );
	if ( !enumType )
	{
		return TXT("Missing EBehaviorGraph enum");
	}

	CName enumName;
	if ( !enumType->FindName( m_combatStyleId, enumName ) )
	{
		enumName = CNAME( Failed );
	}
	return String::Printf( TXT("Decorate if %s == %s"), m_behaviorGraphVarName.AsChar(), enumName.AsChar() );
}