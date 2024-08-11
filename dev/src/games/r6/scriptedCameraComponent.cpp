/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scriptedCameraComponent.h"

IMPLEMENT_ENGINE_CLASS( CScriptedCameraComponent );



//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicStartEntering( Float timeToComplete )
{
	TBaseClass::LogicStartEntering( timeToComplete );

	CallEvent( CNAME( OnComponentStartEntering ), timeToComplete );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicCompleteEntering()
{
	TBaseClass::LogicCompleteEntering();

	CallEvent( CNAME( OnComponentCompleteEntering ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicStartLeaving( Float timeToComplete )
{
	TBaseClass::LogicStartLeaving( timeToComplete );

	CallEvent( CNAME( OnComponentStartLeaving ), timeToComplete );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicCompleteLeaving( )
{
	TBaseClass::LogicCompleteLeaving();

	CallEvent( CNAME( OnComponentCompleteLeaving ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicUpdate( Float timeDelta )
{
	TBaseClass::LogicUpdate( timeDelta );

	CallEvent( CNAME( OnComponentTick ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CScriptedCameraComponent::LogicInitialize()
{
	TBaseClass::LogicInitialize();

	CallEvent( CNAME( OnInitialize ) );
}