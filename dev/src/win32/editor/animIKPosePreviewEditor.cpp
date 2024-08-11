
#include "build.h"
#include "animIKPosePreviewEditor.h"
#include "../../common/engine/virtualAnimation.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/engine/controlRigDefinition.h"
#include "../../common/engine/controlRig.h"
#include "../../common/engine/skeleton.h"

AnimIKPosePreviewItem::AnimIKPosePreviewItem( AnimIKPosePreviewEditor* editor, ETCrEffectorId effector, Int32 boneIndex )
	: m_editor( editor )
	, m_boneIndex( boneIndex )
	, m_effectorId( effector )
	, m_isSet( false )
{

}

IPreviewItemContainer* AnimIKPosePreviewItem::GetItemContainer() const
{
	return m_editor;
}

void AnimIKPosePreviewItem::Refresh()
{

}

void AnimIKPosePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_isSet = true;

	m_editor->SetPositionFromPreview( m_effectorId, newPos );
}

void AnimIKPosePreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	
}

void AnimIKPosePreviewItem::SetReferencePose( const Matrix& transformLS, const Matrix& transformMS )
{
	if ( !m_isSet )
	{
		InternalEditor_SetPosition( transformMS.GetTranslation() );
	}
}

void AnimIKPosePreviewItem::SetInitialTransformation( const Vector& posMS, const EulerAngles& rotMS )
{
	InternalEditor_SetPosition( posMS );
	InternalEditor_SetRotation( rotMS );

	m_isSet = true;
}

//////////////////////////////////////////////////////////////////////////

AnimIKPosePreviewEditor::AnimIKPosePreviewEditor( CWorld* world )
	: m_world( world )
	, m_data( NULL )
	, m_show( false )
	, m_skeleton( NULL )
{

}

CWorld* AnimIKPosePreviewEditor::GetPreviewItemWorld() const
{
	return m_world;
}

void AnimIKPosePreviewEditor::Set( VirtualAnimationPoseIK& data )
{
	ASSERT( !m_data );
	m_data = &data;

	m_show = true;
	ShowItems( true );
}

void AnimIKPosePreviewEditor::Reset()
{
	m_data = NULL;

	m_show = false;
	ShowItems( false );
}

void AnimIKPosePreviewEditor::ShowItems( Bool flag )
{
	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		ASSERT( m_items[ i ] );
		ASSERT( m_items[ i ]->GetComponent() );

		m_items[ i ]->SetVisible( flag );
	}
}

void AnimIKPosePreviewEditor::SetPositionFromPreview( ETCrEffectorId effector, const Vector& positionMS )
{
	if ( m_data )
	{
		Int32 place = m_data->m_ids.GetIndex( effector );
		if ( place == -1 )
		{
			m_data->m_ids.Grow( 1 );
			m_data->m_positionsMS.Grow( 1 );
			m_data->m_rotationsMS.Grow( 1 );
			m_data->m_weights.Grow( 1 );

			place = m_data->m_ids.SizeInt() - 1;

			m_data->m_ids[ place ] = effector;
			m_data->m_rotationsMS[ place ] = EulerAngles::ZEROS;
			m_data->m_weights[ place ] = 1.f;
		}

		m_data->m_positionsMS[ place ] = positionMS;
	}
}

void AnimIKPosePreviewEditor::FillPose( const CSkeleton* skeleton )
{
	m_skeleton = skeleton;

	ClearItems();

	if ( m_skeleton && m_skeleton->GetControlRigDefinition() && m_skeleton->GetDefaultControlRigPropertySet() )
	{
		const TCrDefinition* crDef = m_skeleton->GetControlRigDefinition();
		if ( crDef )
		{
			if ( crDef->m_indexHandL != -1 )
			{
				const Int32 index = crDef->m_indexHandL;

				AnimIKPosePreviewItem* item = new AnimIKPosePreviewItem( this, TCrEffector_HandL, index );
				item->Init( TXT("Hand L") );
				item->SetSize( IPreviewItem::PS_Normal );
				item->SetColor( Color( 0, 255, 0 ) );
				item->SetVisible( m_show );

				const Matrix boneMat = m_skeleton->GetBoneMatrixMS( index );

				item->InternalEditor_SetPosition( boneMat.GetTranslation() );
				item->InternalEditor_SetRotation( boneMat.ToEulerAngles() );

				AddItem( item );
			}

			if ( crDef->m_indexHandR != -1 )
			{
				const Int32 index = crDef->m_indexHandR;

				AnimIKPosePreviewItem* item = new AnimIKPosePreviewItem( this, TCrEffector_HandR, index );
				item->Init( TXT("Hand R") );
				item->SetSize( IPreviewItem::PS_Normal );
				item->SetColor( Color( 0, 255, 0 ) );
				item->SetVisible( m_show );

				const Matrix boneMat = m_skeleton->GetBoneMatrixMS( index );

				item->InternalEditor_SetPosition( boneMat.GetTranslation() );
				item->InternalEditor_SetRotation( boneMat.ToEulerAngles() );

				AddItem( item );
			}
		}
	}
}
void AnimIKPosePreviewEditor::UpdatePosePre( Uint32 boneNumIn, AnimQsTransform* bonesOut )
{
	if ( !m_skeleton )
	{
		return;
	}

	ASSERT( m_items.Size() == boneNumIn );

	const Uint32 size = Min( m_items.Size(), boneNumIn );

	SBehaviorGraphOutput pose;
	pose.Init( boneNumIn, 0 );

	for ( Uint32 i=0; i<size; ++i )
	{
		pose.m_outputPose[ i ] = bonesOut[ i ];
	}
	TDynArray< AnimQsTransform > buffer;
	buffer.Resize( boneNumIn );

	pose.GetBonesModelSpace( m_skeleton, buffer );

	for ( Uint32 i=0; i<size; ++i )
	{
		AnimIKPosePreviewItem* item = static_cast< AnimIKPosePreviewItem* >( m_items[ i ] );
		
		const Int32 boneIndex = item->GetBoneIndex();

		AnimQsTransform& boneTrans = bonesOut[ boneIndex ];
		AnimQsTransform& boneTransMS = buffer[ boneIndex ];

#ifdef USE_HAVOK_ANIMATION
		Matrix boneMat;
		Matrix boneMatMS;
		HavokTransformToMatrix_Renormalize( boneTrans, &boneMat );
		HavokTransformToMatrix_Renormalize( boneTransMS, &boneMatMS );
#else
		Matrix boneMat;
		RedMatrix4x4 conversionMatrix = boneTrans.ConvertToMatrixNormalized();
		boneMat = reinterpret_cast< const Matrix& >( conversionMatrix );

		Matrix boneMatMS;

		conversionMatrix = boneTransMS.ConvertToMatrixNormalized();
		boneMatMS = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		item->SetReferencePose( boneMat, boneMatMS );
	}
}

void AnimIKPosePreviewEditor::UpdatePosePost( Uint32 boneNumIn, AnimQsTransform* bonesOut )
{
	
}
