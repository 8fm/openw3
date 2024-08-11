/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#if !defined( NO_DEBUG_PAGES ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

#include "../../common/engine/debugCommandBox.h"
#include "../../common/engine/debugPageParam.h"
#include "../../common/engine/debugChoice.h"
#include "../core/garbageCollector.h"

#include "objectsSnapshot.h"
#include "debugPageMemoryBudgetCommands.h"
#include "../engine/debugPage.h"

//////////////////////////////////////////////////////////////////////////

class CDebugClassChoice : public IDebugChoice
{
	Uint32		m_index;
	String		m_selected;

public:
	CDebugClassChoice( IDebugCheckBox* parent );

	virtual String GetSelection() const;

protected:
	virtual void OnNext();
	virtual void OnPrev();
};

#ifndef RED_FINAL_BUILD
class CDebugForceDebugCGCommandBox : public IDebugCommandBox
{
	const CDebugStringChoice* m_choice;

public:
	CDebugForceDebugCGCommandBox( IDebugCheckBox* parent, const CDebugStringChoice* choice )
		: IDebugCommandBox( parent, TXT("Run debug GC") )
		, m_choice( choice )
	{};

	virtual void Process()
	{
		String str = m_choice->GetSelection();
		CObject::PrintDependencies( CName( str ) );
	}
};
#endif
//////////////////////////////////////////////////////////////////////////

class CDebugPageMemoryBudget : public IDebugPage
{
private:
	CDebugPageConsoleLog					m_consoleLog;
	CDebugPageLog							m_log;
	CDebugPageFileLog						m_fileLog;

	CDebugOptionsTree*						m_rightTree;

	TDynArray< SMemorySnapshot, MC_Debug >	m_snapshots;
	CDebugSliderIntWithValParam*			m_sliderDelete;
	CDebugSliderIntWithValParam*			m_sliderCompareOne;
	CDebugSliderIntWithValParam*			m_sliderCompareTwo;

private:
	Uint32	m_allocs;
	Uint32	m_reallocs;
	Uint32	m_deallocs;
	Uint32	m_countedAllocs;
	Uint32	m_countedReallocs;
	Uint32	m_countedDeallocs;
	Uint32	m_frameCounter;

public:
	CDebugPageMemoryBudget();

	virtual void OnPageShown();

	virtual void OnTick( Float timeDelta );

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

public:
	void OnMemSnapshotAdded();

	void OnMemSnapshotDeleted();

	void OnMemSnapshotCompare();

private:
	void TickAllocStats();
	void RefreshMemSliders();
};

#endif
