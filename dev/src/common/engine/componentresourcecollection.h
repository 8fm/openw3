/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_COMPONENT_RESOURCE_COLLECTION_H_
#define _RED_COMPONENT_RESOURCE_COLLECTION_H_
#ifdef USE_RED_RESOURCEMANAGER

#include "../core/rawresourcecollection.h"

namespace Red
{
	namespace System
	{
		class GUID;
	};
};


class IFile;
class DataBuffer;

class CComponentResourceCollection
{
public:
	typedef TDynArray< CComponent*, MC_ResourceBuffer > AddedComponentCollection;
	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	CComponentResourceCollection() 
		: m_ownerGUID( Red::System::GUID::ZERO )
		, m_rawResourceCollection( nullptr )
	{
	}

	CComponentResourceCollection( const Red::System::GUID& guid );
	
	RED_INLINE void operator=( const CComponentResourceCollection& resourceCollection )
	{
		m_ownerGUID = resourceCollection.m_ownerGUID;
		m_rawResourceCollection = resourceCollection.m_rawResourceCollection;
		m_attached = resourceCollection.m_attached;
		m_addedComponentObjs = resourceCollection.m_addedComponentObjs;
	}

	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	~CComponentResourceCollection();

	//////////////////////////////////////////////////////////////////////////
	// Public Methods
	//////////////////////////////////////////////////////////////////////////

	// Initialize the resource collection.
	void Initialize();

	void Destroy();

	// Serialize to the supplied IFile all relevant data.
	void Serialize( IFile& file );

	// Adds a data buffer object.
	void AddDataBuffer( const DataBuffer& dataBuffer );
	
	// Returns a data buffer object.
	const DataBuffer& GetDataBuffer( const Uint32 index );

	RED_INLINE const Red::System::GUID& GetOwner()
	{
		return m_ownerGUID;
	}

	RED_INLINE const Uint32 GetDataBufferCount() const
	{
		return m_rawResourceCollection->GetDataBufferCount();
	}

	RED_INLINE void SetAttached( Bool attached )
	{
		m_attached = attached;
	}

	RED_INLINE Bool GetAttached() const
	{
		return m_attached;
	}

	RED_INLINE void StoreAttachedComponentPtr( CComponent* component )
	{		
		m_addedComponentObjs.PushBackUnique( component );
	}

	RED_INLINE void ClearAddedComponents()
	{
		m_addedComponentObjs.ClearFast();
	}

	RED_INLINE const AddedComponentCollection& GetAttachedComponents() const
	{
		return m_addedComponentObjs;
	}
private:
	
	Red::System::GUID										m_ownerGUID;
	Red::Core::ResourceManagement::CRawResourceCollection*	m_rawResourceCollection;
	AddedComponentCollection								m_addedComponentObjs;
	Bool													m_attached;
};

#endif // USE_RED_RESOURCEMANAGER
#endif // !_RED_COMPONENT_RESOURCE_COLLECTION_H_