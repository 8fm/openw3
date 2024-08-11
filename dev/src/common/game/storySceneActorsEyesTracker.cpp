
#include "build.h"
#include "storySceneActorsEyesTracker.h"
#include "../engine/mimicComponent.h"

const CName& CStorySceneActorsEyesTracker::EYE_PLACER_L_NAME( CNAME( placer_left_eye ) );
const CName& CStorySceneActorsEyesTracker::EYE_PLACER_R_NAME( CNAME( placer_right_eye ) );

CStorySceneActorsEyesTracker::CStorySceneActorsEyesTracker()
	: m_updateID( -1 )
	, m_placerEyeL( -1 )
	, m_placerEyeR( -1 )
	, m_cachedPosition( Vector::ZERO_3D_POINT )
{

}

void CStorySceneActorsEyesTracker::Init( CEntity* e )
{
	m_actor = nullptr;

	if ( CActor* a = Cast< CActor >( e ) )
	{
		if ( const CMimicComponent* mimic = a->GetMimicComponent() )
		{
			m_placerEyeL = mimic->FindBoneByName( EYE_PLACER_L_NAME );
			m_placerEyeR = mimic->FindBoneByName( EYE_PLACER_R_NAME );

			if ( m_placerEyeL != -1 && m_placerEyeR != -1 )
			{
				m_actor = a;
			}
		}
	}
}

Vector CStorySceneActorsEyesTracker::GetEyesPosition( Int32 updateID )
{
	if ( updateID != m_updateID )
	{
		if ( const CActor* e = m_actor.Get() )
		{
			if ( const CMimicComponent* mimic = e->GetMimicComponent() )
			{
				const Matrix eyeL = mimic->GetBoneMatrixWorldSpace( m_placerEyeL );
				const Matrix eyeR = mimic->GetBoneMatrixWorldSpace( m_placerEyeR );

				m_cachedPosition = Lerp( 0.5f, eyeL.GetTranslationRef(), eyeR.GetTranslationRef() );
			}
		}

		m_updateID = updateID;
	}

	return m_cachedPosition;
}

const Vector CStorySceneActorsEyesTracker::GetActorEyePosWS( const CActor* actor )
{
	if ( const CMimicComponent* mimic = actor->GetMimicComponent() )
	{
		const Int32 placerEyeL = mimic->FindBoneByName( EYE_PLACER_L_NAME );
		const Int32 placerEyeR = mimic->FindBoneByName( EYE_PLACER_R_NAME );

		if ( placerEyeL != -1 && placerEyeR != -1 )
		{
			if ( const CHardAttachment* att = mimic->GetTransformParent() )
			{
				if ( const CAnimatedComponent* parent = Cast< const CAnimatedComponent >( att->GetParent() ) )
				{
					const CName slotName = att->GetParentSlotName();
					if ( slotName != CName::NONE )
					{
						const Int32 slotIdx = parent->FindBoneByName( slotName );
						if ( slotIdx != -1 )
						{
							const Vector eyeL_pWS = mimic->GetBoneMatrixWorldSpace( placerEyeL ).GetTranslation();
							const Vector eyeR_pWS = mimic->GetBoneMatrixWorldSpace( placerEyeR ).GetTranslation();
							const Vector ppos = Lerp( 0.5f, eyeL_pWS, eyeR_pWS );

							const Vector eyeL_MS = mimic->GetBoneMatrixModelSpace( placerEyeL ).GetTranslation();
							const Vector eyeR_MS = mimic->GetBoneMatrixModelSpace( placerEyeR ).GetTranslation();
							const Vector posMS = Lerp( 0.5f, eyeL_MS, eyeR_MS );

							const Matrix slotMS = parent->GetBoneMatrixModelSpace( slotIdx );
							
							const Matrix& l2w = parent->GetThisFrameTempLocalToWorld();
							const Matrix slotWS = Matrix::Mul( l2w, slotMS );

							const Vector ret = slotWS.TransformPoint( posMS );
							return ret;
						}
					}
				}
			}
		}
	}

	return Vector::ZERO_3D_POINT;
}

const Matrix CStorySceneActorsEyesTracker::GetActorMimicWorldMatrix( const Matrix& actorWS, CMimicComponent* mimic )
{
	if ( const CHardAttachment* att = mimic->GetTransformParent() )
	{
		if ( const CAnimatedComponent* parent = Cast< const CAnimatedComponent >( att->GetParent() ) )
		{
			const CName slotName = att->GetParentSlotName();
			if ( slotName != CName::NONE )
			{
				const Int32 slotIdx = parent->FindBoneByName( slotName );
				if ( slotIdx != -1 )
				{
					const Matrix slotMS = parent->GetBoneMatrixModelSpace( slotIdx );
					const Matrix slotWS = Matrix::Mul( actorWS, slotMS );

					return slotWS;
				}
			}
		}
	}

	return Matrix::IDENTITY;
}
