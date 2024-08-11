#pragma once
#include "attachment.h"

//////////////////////////////////////////////////////////////////////////
/// Dependency attachment spawn info
class DependencyAttachmentSpawnInfo : public AttachmentSpawnInfo
{
public:
    DependencyAttachmentSpawnInfo();
};

//////////////////////////////////////////////////////////////////////////
/// Dependency attachment
class CDependencyAttachment : public IAttachment
{
    DECLARE_ENGINE_CLASS( CDependencyAttachment, IAttachment, 0 )

public:
    virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CDependencyAttachment );
    PARENT_CLASS( IAttachment );

END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////