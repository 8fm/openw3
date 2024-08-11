/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "attachment.h"

class SMaterialOverrideContext;

/// Base material override attachment
class CMaterialOverrideAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CMaterialOverrideAttachment, IAttachment, 0 );

public:
	//! Ctor
	CMaterialOverrideAttachment();

	//! Dtor
	virtual ~CMaterialOverrideAttachment();

	//! Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );
	
public:
	//! Get material override component that we are attached to.
	virtual const CMaterialOverrideComponent* GetMaterialOverrideComponent() const;

	//! Tests wheather given context conforms attachment's settings.
	virtual Bool IsConformed( const SMaterialOverrideContext &context );
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialOverrideAttachment, IAttachment );


/// Material override attachment
class CMaterialOverrideAttachmentSelectByChunk : public CMaterialOverrideAttachment
{
	DECLARE_ENGINE_CLASS( CMaterialOverrideAttachmentSelectByChunk, CMaterialOverrideAttachment, 0 );
public:
	TDynArray< Uint32 > m_chunkIndices;
	
public:
	//! Tests wheather given context conforms attachment's settings.
	virtual Bool IsConformed( const SMaterialOverrideContext &context );
};

BEGIN_CLASS_RTTI( CMaterialOverrideAttachmentSelectByChunk )
	PARENT_CLASS( CMaterialOverrideAttachment );
	PROPERTY_EDIT( m_chunkIndices, TXT("Chunks which materials should get overrided.") );
END_CLASS_RTTI();

