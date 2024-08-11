#ifndef _CVIRTUAL_CHARACTER_CONTROLLER_H_
#define _CVIRTUAL_CHARACTER_CONTROLLER_H_

//////////////////////////////////////////////////////////////////////////

#include "physicsCharacterWrapper.h"
#include "entityTemplateParams.h"
#include "showFlags.h"

//////////////////////////////////////////////////////////////////////////

class CVirtualCharacterController
{
	friend class CCharacterControllersManager;
	friend class CPhysicsCharacterWrapper;

public:
    CVirtualCharacterController( const CName& name, const CName& boneName, const Vector& localOffset, const Float height, const Float radius, class CPhysicsCharacterWrapper* parent );

    virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

    RED_INLINE Float GetCurrentHeight() const { return m_height; }

	RED_INLINE Float GetCurrentRadius() const { return m_virtualRadius; }
    RED_INLINE Float GetVirtualRadius() const { return m_virtualRadius; }
    RED_INLINE Float GetPhysicalRadius() const { return m_physicalRadius; }
    RED_INLINE void SetVirtualRadius( Float newRadius ){ m_virtualRadius = newRadius; }
    RED_INLINE void ResetVirtualRadius() { m_virtualRadius = m_physicalRadius; }
	RED_INLINE Bool IsEnabled() const { return m_enabled; }
	RED_INLINE Bool IsCollidable() const { return m_collisions; }
	RED_INLINE void SetLocalOffsetInModelSpace( const Bool inModelSpace ) { m_localOffsetInModelSpace = inModelSpace; }
	RED_INLINE void SetCollisionGrabber( const Bool grab, const Uint64 groupMask = 0 ) { m_collisionGrabber = grab; m_collisionGrabberGroupMask = groupMask; }

	void UpdateGlobalPosition( const Matrix& parentMatrixWS, const Matrix& acMatrix );
    Vector GetGlobalPosition() const;
    RED_INLINE Vector GetCenterPosition() const;
	void GetGlobalBoundingBox( Box& box ) const;

    RED_INLINE const CName& GetName() const { return m_name; }
	RED_INLINE CName GetBoneName() const { return m_boneName; }
	RED_INLINE void SetBoneIndex( Int32 boneIndex ) { m_boneIndex = boneIndex; }
	RED_INLINE Int32 GetBoneIndex() const { return m_boneIndex; }

	// additional stuff
	RED_INLINE void SetEnabled( const Bool enabled ) { m_enabled = enabled; };
	RED_INLINE void SetTargetable( const Bool targetable );
	RED_INLINE void EnableCollisions( const CName& eventName );
	RED_INLINE void EnableCollisionResponse( const Bool response );
	RED_INLINE void EnableAdditionalRaycast( const CName& eventName, const Vector& vec );

//     void ForceSetPosition( const Vector& newPos )		{ m_localOffset = m_parentController->GetPosition() - newPos; }
//     void ForceMovePosition( const Vector& newPos )		{ m_localOffset = m_parentController->GetPosition() - newPos; }
//     void ForceMoveToPosition( const Vector& newPos )	{ m_localOffset = m_parentController->GetPosition() - newPos; }
//     void Teleport( const Vector& newPos )				{ m_localOffset = m_parentController->GetPosition() - newPos; }

private:
    CName								m_name;
	CName								m_boneName;
	Int32								m_boneIndex;
    Vector						        m_localOffset;
	Vector								m_globalPosition;
    Float					            m_height;
    Float							    m_physicalRadius;
    Float								m_virtualRadius;
	CName								m_collisionsEventName;
	Vector								m_additionalRaycastCheck;
	CName								m_additionalRaycastCheckEventName;

	Bool								m_enabled;
	Bool								m_targetable;
	Bool								m_collisions;
	Bool								m_collisionResponse;
	Bool								m_collisionGrabber;
	Uint64								m_collisionGrabberGroupMask;
	Bool								m_localOffsetInModelSpace;

    class CPhysicsCharacterWrapper*		m_parentController;
};

//////////////////////////////////////////////////////////////////////////
// Inline
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

Vector CVirtualCharacterController::GetCenterPosition() const
{
	return GetGlobalPosition() + Vector( 0, 0, m_height*0.5f );
}

//////////////////////////////////////////////////////////////////////////
//
// set controller targetable
void CVirtualCharacterController::SetTargetable( const Bool targetable )
{
	m_targetable = targetable;
}
//////////////////////////////////////////////////////////////////////////
//
// enable controller collision solver
void CVirtualCharacterController::EnableCollisions( const CName& eventName )
{
	m_collisions = true;
	m_collisionsEventName = eventName;
}
//////////////////////////////////////////////////////////////////////////
//
// enable collision response
void CVirtualCharacterController::EnableCollisionResponse( const Bool response )
{
	m_collisionResponse = response;
}
//////////////////////////////////////////////////////////////////////////
//
// enable addition raycast for controller
void CVirtualCharacterController::EnableAdditionalRaycast( const CName& eventName, const Vector& vec )
{
	m_additionalRaycastCheck = vec;
	m_additionalRaycastCheckEventName = eventName;
}

//////////////////////////////////////////////////////////////////////////

#endif
