/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/resource.h"
#include "../core/classBuilder.h"
#include "../core/uniqueBuffer.h"
#include "engineTypeRegistry.h"
#include "renderObject.h"

class CReloadBaseTreeInfo
{
public:
	CResource*	m_resourceToReload;
	String		m_editor;

public:
	CReloadBaseTreeInfo( CResource* resource, const String& editor )
		: m_resourceToReload( resource )
		, m_editor( editor )
	{};
};

enum EBaseTreeType : Int32
{
	BaseTreeTypeGrass,
	BaseTreeTypeTree,
	BaseTreeTypeUnknown
};

/// Speed Tree SRT in memory
class CSRTBaseTree : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSRTBaseTree, CResource, "srt", "SRT tree" );

public:
	static const Int32		s_maxSRTBufferSize = 1024 * 1024 * 5;

public:

	CSRTBaseTree();
	virtual ~CSRTBaseTree();

	// Reload resource
	virtual Bool Reload(Bool confirm);
	void ReleaseTextures();

	//! Serialize object ( binary form )
	virtual void OnSerialize( IFile &file );

	//! Resource post loading
	virtual void OnPostLoad();

#ifndef NO_RESOURCE_COOKING
	//! Cook resource for use in game
	virtual void OnCook( class ICookerFramework& cooker ) override;
	static void ReportUsedTextures( const AnsiChar* dataBuffer, Uint32 dataSize, TDynArray< String >& outList );
#endif

	//! SRT file data buffer
	const void*		GetSRTData() const;
	Uint32			GetSRTDataSize() const;
	void			SetSRTData( Red::UniqueBuffer buffer );

	//! Get render tree object
	Bool CreateRenderObject() const;
	RED_MOCKABLE RenderObjectHandle	GetRenderObject() const;
	RED_MOCKABLE RenderObjectHandle AcquireRenderObject() const;
	//! Expose bbox for engine side tests
	const Box& GetBBox() const;
	//! Expose bbox for engine side tests
	void SetBBox( const Box& box ) const;

	void SetTreeType( EBaseTreeType type ) const;
	EBaseTreeType GetType() const;
	Bool IsGrassType() const;


private:

	void SerializeBuffer( IFile & file );

	mutable RenderObjectHandle m_loadedTree; //!< Render side tree representation
	mutable Box m_bbox; //!< bounding box of render tree
	mutable EBaseTreeType m_type;

	Red::UniqueBuffer m_buffer;
};

BEGIN_CLASS_RTTI( CSRTBaseTree );
	PARENT_CLASS( CResource );
END_CLASS_RTTI();
