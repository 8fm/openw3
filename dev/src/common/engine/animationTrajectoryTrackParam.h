
#pragma once
#include "skeletalAnimationEntry.h"

class CSkeletalAnimationTrajectoryTrackParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CSkeletalAnimationTrajectoryTrackParam );

public:
	Bool								m_editorOnly;
	TDynArray< CName >					m_names;
	TDynArray< TDynArray< Vector > >	m_datas;

public:
	CSkeletalAnimationTrajectoryTrackParam();

	virtual Bool EditorOnly() const override { return false; }

#ifndef NO_EDITOR
	void SetEditorOnly( Bool flag ) { m_editorOnly = flag; }
#endif
};

BEGIN_CLASS_RTTI( CSkeletalAnimationTrajectoryTrackParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
	PROPERTY_RO( m_editorOnly, TXT("Editor only") );
	PROPERTY_RO( m_names, TXT("Names") );
	PROPERTY( m_datas );
END_CLASS_RTTI();
