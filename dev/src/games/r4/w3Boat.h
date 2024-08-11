#pragma once
#include "..\..\common\game\gameplayentity.h"
#include "..\..\common\engine\vectorDamper.h"

//////////////////////////////////////////////////////////////////////////

class W3Boat : public CGameplayEntity
{
    DECLARE_ENGINE_CLASS( W3Boat, CGameplayEntity, 0 )
public:
    W3Boat();
    virtual ~W3Boat();

    virtual void OnTick( Float deltaTime );
    virtual Bool Teleport( const Vector& position, const EulerAngles& rotation );
    virtual Bool Teleport( CNode* node, Bool applyRotation = true );

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

    virtual void OnDestroyed( CLayer* layer );

	// Called after streamable components have been streamed in and their own OnStreamIn calls have been made
	virtual void OnStreamIn() override;

	// Called before streamable components are to be streamed out, before the components' own OnStreamOut is called
	virtual void OnStreamOut() override;

    void SetRawPlacementNoScale( const Matrix& newPlacement ) override;

    virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
    struct SDBGTeleportDestination 
    {
        SDBGTeleportDestination()
        {
            tickCreated = -1.0f;
        }

        Matrix pose;
        Box bbox;
        Float tickCreated;
    };
    static TDynArray<SDBGTeleportDestination> s_dbgPlacements;
//     virtual void OnAttached( CWorld* world );
//     virtual void OnDetached( CWorld* world );

	void funcHasDrowned( CScriptStackFrame& stack, void* result );
	void funcSetHasDrowned( CScriptStackFrame& stack, void* result );
	void funcSetTeleportedFromOtherHUB( CScriptStackFrame& stack, void* result );

	RED_INLINE Bool HasDrowned() const { return m_hasDrowned; }

private:
    Vector  AlignPositionToWaterLevel( const Vector& posToFix );

    struct STeleportLocation
    {
        Matrix pose;
        Box bbox;
    };

    static Bool IsOverlaping2D( const Box& a, const Box& b );
    static Box ConvertToGlobal( const Box& localBBox, const Matrix& pose );
    static Bool IsLocationOverlapingOnPhysx( const CEntity* callerPtr, const Vector& targetPosition, const Matrix& targetRotation, const Box& targetBBox, TDynArray<CEntity*>& outOverlapingEntities );
    static Bool IsLocationOverlapingOnTeleportLocations( const CEntity* callerPtr, const Vector& targetPosition, const Matrix& targetRotation, const Box& targetBBox );
    static THashMap<const CEntity*, STeleportLocation> s_teleportLocations;
    
private:
    Int32   m_numTeleportTicks;
    Matrix  m_teleportPose;

    // Damping physics position update
    Matrix                          m_prevTransform;
    Bool                            m_firstUpdate;

    CVectorDamper                   m_damperX;
    CVectorDamper                   m_damperY;
    CVectorDamper                   m_damperZ;
    CVectorDamper                   m_damperT;

	Bool							m_teleportedFromOtherHUB;
	Bool							m_hasDrowned;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( W3Boat );
    PARENT_CLASS( CGameplayEntity );
	PROPERTY_SAVED( m_teleportedFromOtherHUB );
	NATIVE_FUNCTION( "HasDrowned", funcHasDrowned );
	NATIVE_FUNCTION( "SetHasDrowned", funcSetHasDrowned );
	NATIVE_FUNCTION( "SetTeleportedFromOtherHUB", funcSetTeleportedFromOtherHUB );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
