
#pragma once

class CSkeletalAnimationStepClipParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CSkeletalAnimationStepClipParam );

	TDynArray< Float >		m_syncPoints;

public:
	CSkeletalAnimationStepClipParam();

	const TDynArray< Float >& GetSyncPoints() const { return m_syncPoints; }

	virtual Bool EditorOnly() const override { return true; }
};

BEGIN_CLASS_RTTI( CSkeletalAnimationStepClipParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
	PROPERTY_EDIT( m_syncPoints, TXT("Sync points") );
END_CLASS_RTTI();
