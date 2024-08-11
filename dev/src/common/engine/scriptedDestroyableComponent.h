/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "rigidMeshComponent.h"

enum EDestroyWay
{
    DW_Random,
    DW_Timed,
    DW_OnContact,
    DW_OnDistance
};

BEGIN_ENUM_RTTI( EDestroyWay );
    ENUM_OPTION_DESC( TXT("Random"), DW_Random );
    ENUM_OPTION_DESC( TXT("Timed"), DW_Timed );
    ENUM_OPTION_DESC( TXT("OnContact"), DW_OnContact );
    ENUM_OPTION_DESC( TXT("OnDistance"), DW_OnDistance );
END_ENUM_RTTI();

class CScriptedDestroyableComponent: public CRigidMeshComponent
{
    DECLARE_ENGINE_CLASS( CScriptedDestroyableComponent, CRigidMeshComponent, 0 );

protected:
    EDestroyWay m_destroyWay;

    float       m_distanceValue;
    float       m_destroyTime;
    float       m_destroyTimeDuration;
    //float       m_timeBetweenDestroys;

    int         m_orderID;
public:
    CScriptedDestroyableComponent();

private:
    void funcGetDestroyWay( CScriptStackFrame& stack, void* result );
    void funcGetDistanceValue( CScriptStackFrame& stack, void* result );
    //void funcGetDestroyOrderID( CScriptStackFrame& stack, void* result );
    void funcGetDestroyAtTime( CScriptStackFrame& stack, void* result );
    void funcGetDestroyTimeDuration( CScriptStackFrame& stack, void* result );
    //void funcGetTimeBetweenDestroys( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CScriptedDestroyableComponent );
    PARENT_CLASS( CRigidMeshComponent );
    PROPERTY_EDIT( m_destroyWay, TXT("Destroy way") );
    NATIVE_FUNCTION( "GetDestroyWay", funcGetDestroyWay );
    NATIVE_FUNCTION( "GetDistanceValue", funcGetDistanceValue );
    //NATIVE_FUNCTION( "GetDestroyOrderID", funcGetDestroyOrderID );
    NATIVE_FUNCTION( "GetDestroyAtTime", funcGetDestroyAtTime );
    NATIVE_FUNCTION( "GetDestroyTimeDuration", funcGetDestroyTimeDuration );
    //NATIVE_FUNCTION( "GetTimeBetweenDestroys", funcGetTimeBetweenDestroys );
END_CLASS_RTTI();
