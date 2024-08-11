/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/aiStorage.h"

struct SDynamicClue;

class CBehTreeCluePathData
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeCluePathData );

public:
	
	CBehTreeCluePathData()	
		: m_maxClues( 1 )
	{}

	class CInitializer : public CAIStorageItem::CInitializer
	{
	public:
		CInitializer()
			: CAIStorageItem::CInitializer()
		{}
	
		CName GetItemName() const override;
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};

	void SetMaxClues( Uint32 maxClues );
	void LeaveClue( const THandle< CEntityTemplate >& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation );

private:

	typedef TQueue< SDynamicClue* > TClues;
	TClues	m_clues;	
	Uint32	m_maxClues;
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeCluePathData );
END_CLASS_RTTI();

class CBehTreeCluePathDataPtr : public TAIStoragePtr< CBehTreeCluePathData >
{
	typedef TAIStoragePtr< CBehTreeCluePathData > Super;

public:

	CBehTreeCluePathDataPtr( CAIStorage* storage );

	CBehTreeCluePathDataPtr()
		: Super()
	{}

	CBehTreeCluePathDataPtr( const CBehTreeCluePathDataPtr& p )
		: Super( p )
	{}

	CBehTreeCluePathDataPtr( CBehTreeCluePathDataPtr&& p )
		: Super( Move( p ) )
	{}
};