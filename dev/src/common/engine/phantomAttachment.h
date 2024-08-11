/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "hardAttachment.h"

/// Special attachments that skins skinned mesh
class CPhantomAttachment : public CHardAttachment
{
	DECLARE_ENGINE_CLASS( CPhantomAttachment, CHardAttachment, 0 );
protected:
	Bool m_takeVertexSimulationPosition;

public:
	CPhantomAttachment() : m_takeVertexSimulationPosition( false ) {}
	virtual ~CPhantomAttachment();
	
public:
	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

	virtual void CalcAttachedLocalToWorld( Matrix& out ) const;

};

BEGIN_CLASS_RTTI( CPhantomAttachment );
	PARENT_CLASS( CHardAttachment );
	PROPERTY_EDIT( m_takeVertexSimulationPosition, TXT("") );
END_CLASS_RTTI();
