/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeCluePathData.h"
#include "../../common/game/behTreeInstance.h"
#include "dynamicClueStorage.h"
#include "focusModeController.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeCluePathData );

////////////////////////////////////////////////////////////////////////

void CBehTreeCluePathData::SetMaxClues( Uint32 maxClues )
{
	m_maxClues = Max( m_maxClues, maxClues );
}

void CBehTreeCluePathData::LeaveClue( const THandle< CEntityTemplate>& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation )
{
	CFocusModeController* fm = GCommonGame->GetSystem< CFocusModeController >();
	if ( fm == nullptr )
	{
		return;
	}
	CDynamicClueStorage* storage = fm->GetDynamicClueStorage();
	while ( m_clues.Size() > m_maxClues )
	{
		storage->Remove( m_clues.Front() );
		m_clues.Pop();
	}
	m_clues.Push( storage->Add( templ, layer, position, rotation ) );
}

////////////////////////////////////////////////////////////////////////

CBehTreeCluePathDataPtr::CBehTreeCluePathDataPtr( CAIStorage* storage )
	: Super( CBehTreeCluePathData::CInitializer(), storage )
{
}

////////////////////////////////////////////////////////////////////////

CName CBehTreeCluePathData::CInitializer::GetItemName() const
{
	return CNAME( CluePathData );
}

void CBehTreeCluePathData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
}

IRTTIType* CBehTreeCluePathData::CInitializer::GetItemType() const
{
	return CBehTreeCluePathData::GetStaticClass();
}