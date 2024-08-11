#pragma once

class CStorySceneActorsEyesTracker
{
public:
	static const CName& EYE_PLACER_L_NAME;
	static const CName& EYE_PLACER_R_NAME;

private:
	THandle< CActor >	m_actor;
	Int32				m_placerEyeL;
	Int32				m_placerEyeR;
	Int32				m_updateID;
	Vector				m_cachedPosition;

public:
	CStorySceneActorsEyesTracker();

	void Init( CEntity* e );

public:
	Vector GetEyesPosition( Int32 updateID );

	static const Vector GetActorEyePosWS( const CActor* actor );
	static const Matrix GetActorMimicWorldMatrix( const Matrix& actorWS, CMimicComponent* mimic );
};
