
#include "build.h"
#include "animFKPosePreviewEditor.h"
#include "../../common/engine/virtualAnimation.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/engine/skeleton.h"

AnimFKPosePreviewItem::AnimFKPosePreviewItem( AnimFKPosePreviewEditor* editor, Int32 id )
	: m_editor( editor )
	, m_id( id )
	, m_referencePoseTranslationLS( Vector::ZERO_3D_POINT )
	, m_referencePoseRotationLS( EulerAngles::ZEROS )
	, m_referencePoseTranslationMS( Vector::ZERO_3D_POINT )
	, m_referencePoseRotationMS( EulerAngles::ZEROS )
{

}

IPreviewItemContainer* AnimFKPosePreviewItem::GetItemContainer() const
{
	return m_editor;
}

void AnimFKPosePreviewItem::Refresh()
{

}

void AnimFKPosePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{

}

void AnimFKPosePreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	m_editor->SetRotationFromPreview( m_id, m_referencePoseRotationMS, prevRot, newRot );
}

void AnimFKPosePreviewItem::SetReferencePose( const Matrix& transformLS, const Matrix& transformMS )
{
	m_referencePoseTranslationLS = transformLS.GetTranslation();
	m_referencePoseRotationLS = transformLS.ToEulerAngles();

	m_referencePoseTranslationMS = transformMS.GetTranslation();
	m_referencePoseRotationMS = transformMS.ToEulerAngles();
}

void AnimFKPosePreviewItem::SetPose( const Vector& positionLS )
{
	
}

void AnimFKPosePreviewItem::SetPose( const EulerAngles& rotationLS )
{
	
}

//////////////////////////////////////////////////////////////////////////

AnimFKPosePreviewEditor::AnimFKPosePreviewEditor( CWorld* world )
	: m_world( world )
	, m_data( NULL )
	, m_show( false )
	, m_skeleton( NULL )
{

}

CWorld* AnimFKPosePreviewEditor::GetPreviewItemWorld() const
{
	return m_world;
}

void AnimFKPosePreviewEditor::Set( VirtualAnimationPoseFK& data )
{
	ASSERT( !m_data );
	m_data = &data;

	m_show = true;
	ShowItems( true );
}

void AnimFKPosePreviewEditor::Reset()
{
	m_data = NULL;

	m_show = false;
	ShowItems( false );
}

void AnimFKPosePreviewEditor::ShowItems( Bool flag )
{
	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		ASSERT( m_items[ i ] );
		ASSERT( m_items[ i ]->GetComponent() );

		m_items[ i ]->SetVisible( flag );
	}
}

void AnimFKPosePreviewEditor::SetRotationFromPreview( Int32 id, const EulerAngles& refRot, const EulerAngles& prevRot, const EulerAngles& newRot )
{
	if ( m_data )
	{
		Int32 place = -1;

		for ( Int32 i=0; i<m_data->m_indices.SizeInt(); ++i )
		{
			if ( m_data->m_indices[ i ] == id )
			{
				place = i;
				break;
			}
		}

		if ( place == -1 )
		{
			m_data->m_indices.PushBack( id );
			m_data->m_transforms.Grow( 1 );
			place = m_data->m_indices.SizeInt() - 1;

			m_data->m_transforms[ place ].Identity();
		}

		ASSERT( place >= 0 );
		ASSERT( place < m_data->m_indices.SizeInt() );
		ASSERT( m_data->m_transforms.Size() < m_data->m_indices.Size() );

		EngineQsTransform& transform = m_data->m_transforms[ place ];
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform& hkTrans = reinterpret_cast< hkQsTransform& >( transform );

		EulerAngles deltaRot = EulerAngles::AngleDistance( refRot, newRot );

		hkQsTransform transToAdd( hkQsTransform::IDENTITY );
		EulerAnglesToHavokQuaternion( deltaRot, transToAdd.m_rotation );

		//hkTrans.setMul( hkTrans, transToAdd );
		hkTrans = transToAdd;
#else
		RedQsTransform& redTrans = reinterpret_cast< RedQsTransform& >( transform );

		EulerAngles deltaRot = EulerAngles::AngleDistance( refRot, newRot );

		RedQsTransform transToAdd( RedQsTransform::IDENTITY );
		RedMatrix4x4 conversionMatrix = BuildFromQuaternion( transToAdd.Rotation.Quat );
		RedEulerAngles tempConv = conversionMatrix.ToEulerAnglesFull();
		deltaRot = reinterpret_cast< const EulerAngles& >( tempConv );

		//hkTrans.setMul( hkTrans, transToAdd );
		redTrans = transToAdd;
#endif
	}
}

void AnimFKPosePreviewEditor::FillPose( const CSkeleton* skeleton )
{
	m_skeleton = skeleton;

	ClearItems();

	if ( m_skeleton )
	{
		for ( Int32 index=0; index<m_skeleton->GetBonesNum(); ++index )
		{
			String boneName = m_skeleton->GetBoneName( (Uint32)index );

			AnimFKPosePreviewItem* item = new AnimFKPosePreviewItem( this, index );
			item->Init( boneName );
			item->SetSize( IPreviewItem::PS_Tiny );
			item->SetColor( Color( 0, 255, 0 ) );
			item->SetVisible( m_show );

			const Matrix boneMat = m_skeleton->GetBoneMatrixMS( index );

			item->InternalEditor_SetPosition( boneMat.GetTranslation() );
			item->InternalEditor_SetRotation( boneMat.ToEulerAngles() );

			AddItem( item );
		}
	}
}

void AnimFKPosePreviewEditor::UpdatePosePre( Uint32 boneNumIn, AnimQsTransform* bonesOut )
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
		AnimFKPosePreviewItem* item = static_cast< AnimFKPosePreviewItem* >( m_items[ i ] );

		AnimQsTransform& boneTrans = bonesOut[ i ];
		AnimQsTransform& boneTransMS = pose.m_outputPose[ i ];

#ifdef USE_HAVOK_ANIMATION
		Matrix boneMat;
		Matrix boneMatMS;

		HavokTransformToMatrix_Renormalize( boneTrans, &boneMat );
		HavokTransformToMatrix_Renormalize( boneTransMS, &boneMatMS );

		item->SetReferencePose( boneMat, boneMatMS );
#else
		RedMatrix4x4 conversionMatrix = boneTrans.ConvertToMatrixNormalized();
		Matrix boneMat = reinterpret_cast< const Matrix& >( conversionMatrix );

		conversionMatrix = boneTransMS.ConvertToMatrixNormalized();
		Matrix boneMatMS = reinterpret_cast< const Matrix& >( conversionMatrix );

		item->SetReferencePose( boneMat, boneMatMS );
#endif
	}
}

void AnimFKPosePreviewEditor::UpdatePosePost( Uint32 boneNumIn, AnimQsTransform* bonesOut )
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
		AnimFKPosePreviewItem* item = static_cast< AnimFKPosePreviewItem* >( m_items[ i ] );
		AnimQsTransform& boneTrans = buffer[ i ];

#ifdef USE_HAVOK_ANIMATION
		Matrix boneMat;
		HavokTransformToMatrix_Renormalize( boneTrans, &boneMat );
#else
		RedMatrix4x4 conversionMatrix = boneTrans.ConvertToMatrixNormalized();
		Matrix boneMat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		item->InternalEditor_SetPosition( boneMat.GetTranslation() );
		//item->InternalEditor_SetRotation( boneMat.ToEulerAngles() );
	}
}
