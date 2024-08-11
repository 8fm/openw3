/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiPositioning.h"
#include "aiStorage.h"

class CBehTreePositioningRequest
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreePositioningRequest )
protected:
	CPositioningFilterRequest::Ptr			m_request;
	Float									m_validFor;

	void									LazyCreate();
public:
	CBehTreePositioningRequest()
		: m_validFor( 0.f )																		{}
	~CBehTreePositioningRequest()																{}

	CPositioningFilterRequest*				operator->()										{ LazyCreate(); return m_request.Get(); }
	CPositioningFilterRequest&				operator*()											{ LazyCreate(); return *m_request.Get(); }

	CPositioningFilterRequest*				GetQuery()											{ LazyCreate(); return m_request.Get(); }
	void									SetValidFor( Float f )								{ m_validFor = f; }
	Bool									IsValid( Float localTime ) const					{ return m_validFor >= localTime; }

	class CInitializer : public CAIStorageItem::CNamedInitializer
	{
		typedef CAIStorageItem::CNamedInitializer Super;
	public:
		CInitializer( CName name )
			: Super( name )																		{}
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};
};

BEGIN_CLASS_RTTI( CBehTreePositioningRequest )
END_CLASS_RTTI()

struct CBehTreePositioningRequestPtr : public TAIStoragePtr< CBehTreePositioningRequest >
{
	typedef TAIStoragePtr< CBehTreePositioningRequest > Super;
public:
	CBehTreePositioningRequestPtr( CAIStorage* storage, CName itemName )
		: Super( CBehTreePositioningRequest::CInitializer( itemName ), storage )				{}
};