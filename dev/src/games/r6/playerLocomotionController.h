/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "..\..\common\game\moveLocomotion.h"
#include "directMovementLocomotionSegment.h"



class CPlayerLocomotionController : public CMoveLocomotion::IController
{
	CMoveLSDirect*		m_movementSegment;
	Bool				m_isAttached;


public:

	CPlayerLocomotionController();


	virtual void OnSegmentFinished( EMoveStatus status )	override;
	virtual void OnControllerAttached( )					override;
	virtual void OnControllerDetached( )					override;

	void AddTranslation						( const Vector& translation );
	void AddRotation						( const EulerAngles& rotaion );
	void ResetTranslationAndRotation		( );

	Bool IsAttached() const;
};













RED_INLINE void CPlayerLocomotionController::AddTranslation( const Vector& translation )
{
	R6_ASSERT( m_movementSegment );
	m_movementSegment->AddTranslation( translation );
}




RED_INLINE void CPlayerLocomotionController::AddRotation( const EulerAngles& rotaion )
{
	R6_ASSERT( m_movementSegment );
	m_movementSegment->AddRotation( rotaion );
}




RED_INLINE void CPlayerLocomotionController::ResetTranslationAndRotation()
{
	R6_ASSERT( m_movementSegment );
	m_movementSegment->ResetTranslationAndRotation();
}




RED_INLINE Bool CPlayerLocomotionController::IsAttached() const
{
	return m_isAttached;
}


