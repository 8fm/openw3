/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scriptedDestroyableComponent.h"
#include "../core/scriptStackFrame.h"
#include "entity.h"

IMPLEMENT_RTTI_ENUM( EDestroyWay );
IMPLEMENT_ENGINE_CLASS( CScriptedDestroyableComponent );

CScriptedDestroyableComponent::CScriptedDestroyableComponent()
{

}


void CScriptedDestroyableComponent::funcGetDestroyWay( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_STRUCT( EDestroyWay, m_destroyWay );
}

void CScriptedDestroyableComponent::funcGetDistanceValue( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_FLOAT(m_distanceValue);
}
/*
void CScriptedDestroyableComponent::funcGetDestroyOrderID( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_FLOAT(m_orderID);
}
*/
void CScriptedDestroyableComponent::funcGetDestroyAtTime( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_FLOAT(m_destroyTime);
}

void CScriptedDestroyableComponent::funcGetDestroyTimeDuration( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_FLOAT(m_destroyTimeDuration);
}
/*
void CScriptedDestroyableComponent::funcGetTimeBetweenDestroys( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;	
    RETURN_FLOAT(m_timeBetweenDestroys);
}*/
