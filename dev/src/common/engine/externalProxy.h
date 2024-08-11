/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "attachment.h"
#include "component.h"

class CExternalProxyAttachment : public IAttachment
{
    DECLARE_ENGINE_CLASS( CExternalProxyAttachment, IAttachment, 0 )

    IAttachment *m_originalAttachment;

public:

    IAttachment *GetOriginalLink()                        { return m_originalAttachment; }
    void         SetOriginalLink(IAttachment *attachment) { m_originalAttachment = attachment; }

    // Serialization
	virtual void OnSerialize( IFile& file );

    virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
    {
        return TBaseClass::Init( parent, child, info );
    }
};

BEGIN_CLASS_RTTI( CExternalProxyAttachment );
	PARENT_CLASS( IAttachment );
	PROPERTY( m_originalAttachment );
END_CLASS_RTTI();

class CExternalProxyComponent : public CComponent
{
    DECLARE_ENGINE_CLASS( CExternalProxyComponent, CComponent, 0 );
    
	// not needed after VER_MINIMAL >= VER_EXTERNAL_PROXIES_USE_GUIDS
    CName m_originalClassName;

public:
	CExternalProxyComponent() : m_originalClassName(CName::NONE) {}

    virtual Bool IsManualCreationAllowed() const { return false; }

    // Serialization
	virtual void OnSerialize( IFile& file );

    void SuckDataFromDestination( CEntity &entity, CComponent &destination );
    void DumpDataToDestination  ( CEntity &entity, CComponent &destination );

    Bool IsProxyFor ( CComponent &component );

public:
    static CComponent *GetProxyIfNeeded( CEntity &entity, CComponent &component );
};

//DEFINE_SIMPLE_RTTI_CLASS( CExternalProxyComponent, CComponent );
BEGIN_CLASS_RTTI( CExternalProxyComponent );
	PARENT_CLASS( CComponent );
	// not needed after VER_MINIMAL >= VER_EXTERNAL_PROXIES_USE_GUIDS
	PROPERTY( m_originalClassName );
END_CLASS_RTTI();