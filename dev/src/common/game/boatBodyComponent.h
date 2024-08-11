#pragma once

//////////////////////////////////////////////////////////////////////////

#include "gameTypeRegistry.h"
#include "..\engine\rigidMeshComponent.h"

//////////////////////////////////////////////////////////////////////////

//#define CORRECT_BOAT_MASSES

//////////////////////////////////////////////////////////////////////////

class CBoatBodyComponent : public CRigidMeshComponent
{
    DECLARE_ENGINE_CLASS( CBoatBodyComponent, CRigidMeshComponent, 0 );

public:
    CBoatBodyComponent(void);

    virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
    virtual void OnTickPrePhysics( Float timeDelta );
	virtual void OnTick( Float timeDelta );

private:
    // Script events
    void funcTriggerCutsceneStart( CScriptStackFrame& stack, void* result );
    void funcTriggerCutsceneEnd( CScriptStackFrame& stack, void* result );
	
	// Setup component when in cutscene
	void SetupTransformInCutscene();

private:
    Bool 							m_isInCutsceneMode;
    Matrix                          m_cutscenePrevBoneTransform;
    Matrix                          m_cutsceneInitialBoneTransform;
    Int16                           m_cutsceneBoneIndex;
    Bool                            m_cutsceneAddedTick;
	Bool							m_delayedSwitchWrapperToKinematic;
	Bool							m_delayedSwitchWrapperToDynamic;

#ifdef CORRECT_BOAT_MASSES
    Bool                            m_firstTickMassChange;
#endif

public:

	virtual Bool HACK_ForceUpdateTransformWithGlobalPose( const Matrix& matrixWorldSpace ) override { return false; }

/// DEBUG
// public:
//     virtual void OnDetached( CWorld* world );
//     virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
//     TDynArray<CComponent>   m_toMove;
//     Bool m_isFirstTick;
// 
//     Float m_timeout;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CBoatBodyComponent );
    PARENT_CLASS( CRigidMeshComponent );

    NATIVE_FUNCTION( "TriggerCutsceneStart", funcTriggerCutsceneStart );
    NATIVE_FUNCTION( "TriggerCutsceneEnd", funcTriggerCutsceneEnd );

    PROPERTY_EDIT( m_cutsceneBoneIndex, TXT("Index of bone used in cutscene mode to move the boat") );

END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////