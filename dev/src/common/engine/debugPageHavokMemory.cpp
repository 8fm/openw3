/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

/// Can we expand given memory category ?
#ifdef USE_HAVOK_ANIMATION
static inline Bool IsHavokMemoryCategoryExpandable( Uint32 category )
{
	for ( Uint32 i=category+1; i<HK_MEMORY_CLASS_MAX; ++i )
	{
		if ( CHavokMemoryManager::GetParentMemoryClass( i ) == category )
		{
			return true;
		}
	}

	return false;
}

static inline const Char* GetHavokMemoryCategoryName( Uint32 category )
{
	return CHavokMemoryManager::GetMemoryClassName( category );
}

/// Special check box option for toggling show flag on and off
class CDebugCheckBoxHavokMemory : public IDebugCheckBox
{
protected:
	Uint32			m_memoryClass;
	Color			m_highlightColor;
	Float			m_highlightFade;
	Int32				m_curMemory;
	Int32				m_curAllocations;
	Int32				m_totalMemory;
	Int32				m_totalAllocations;

public:
	CDebugCheckBoxHavokMemory( IDebugCheckBox* parent, Uint32 memoryClass )
		: IDebugCheckBox( parent, GetHavokMemoryCategoryName( memoryClass ), IsHavokMemoryCategoryExpandable( memoryClass ), false )
		, m_memoryClass( memoryClass )
		, m_highlightColor( Color::WHITE )
		, m_highlightFade( 0.0f )
		, m_curMemory( 0 )
		, m_curAllocations( 0 )
		, m_totalMemory( 0 )
		, m_totalAllocations( 0 )
	{
		// Get memory
		m_curMemory = CHavokMemoryManager::GetMemoryPerClass( m_memoryClass );
		m_curAllocations = CHavokMemoryManager::GetAllocationsPerClass( m_memoryClass );

		// Create children
		for ( Uint32 i=m_memoryClass+1; i<HK_MEMORY_CLASS_MAX; ++i )
		{
			if ( CHavokMemoryManager::GetParentMemoryClass( i ) == m_memoryClass )
			{
				new CDebugCheckBoxHavokMemory( this, i );
			}
		}
	}

	//! Calculate drawing color
	virtual void CalculateColor( Color& color, Bool isSelected )
	{
		if ( isSelected )
		{
			color = Color::YELLOW;
		}
		else if ( m_highlightFade > 0.0f )
		{
			color = m_highlightColor;
		}
		else
		{
			color = Color::WHITE;
		}
	}

	//! Render comment
	virtual void OnRenderComment( CRenderFrame* frame, Uint32 x, Uint32 y, const Color& color, const RenderOptions& options )
	{
		// Margin
		x += 200;

		if ( m_children.Empty() )
		{
			// Current allocations
			//frame->AddDebugScreenFormatedText( x+1, y+1, Color::BLACK, TXT("%i"), m_curAllocations );
			//frame->AddDebugScreenFormatedText( x, y, color, TXT("%i"), m_curAllocations );
			//x += 80;
			frame->AddDebugScreenFormatedText( x+1, y+1, Color::BLACK, TXT("%1.2f"), m_curMemory / 1024.0f );
			frame->AddDebugScreenFormatedText( x, y, color, TXT("%1.2f"), m_curMemory / 1024.0f );
			x += 80;
		}
		else
		{
			// Totals
			//frame->AddDebugScreenFormatedText( x+1, y+1, Color::BLACK, TXT("%i"), m_totalAllocations );
			//frame->AddDebugScreenFormatedText( x, y, color, TXT("%i"), m_totalAllocations );
			//x += 80;
			frame->AddDebugScreenFormatedText( x+1, y+1, Color::BLACK, TXT("%1.2f"), m_totalMemory / 1024.0f );
			frame->AddDebugScreenFormatedText( x, y, color, TXT("%1.2f"), m_totalMemory / 1024.0f );
			x += 80;
		}
	}

	//! Tick the crap
	virtual void OnTick( Float timeDelta )
	{
		// Pass to base class
		IDebugCheckBox::OnTick( timeDelta );

		// Fade...
		if ( m_highlightFade > 0.0f )
		{
			m_highlightFade -= timeDelta;
			if ( m_highlightFade < 0.0f )
			{
				m_highlightFade = 0.0f;
			}
		}

		// Read current stats
		const Int32 oldMem = m_curMemory;
		m_curMemory = CHavokMemoryManager::GetMemoryPerClass( m_memoryClass );
		m_curAllocations = CHavokMemoryManager::GetAllocationsPerClass( m_memoryClass );

		// Any change
		if ( m_curMemory < oldMem )
		{
			m_highlightColor = Color::GREEN;
			m_highlightFade = 1.0f;
		}
		else if ( m_curMemory > oldMem )
		{
			m_highlightColor = Color::RED;
			m_highlightFade = 1.0f;
		}

		// Reset totals
		m_totalAllocations = m_curAllocations;
		m_totalMemory = m_curMemory;

		// Accumulate totals
		for ( Uint32 i=0; i<m_children.Size(); ++i )
		{
			CDebugCheckBoxHavokMemory* mem = static_cast< CDebugCheckBoxHavokMemory* >( m_children[i] );
			m_totalAllocations += mem->m_totalAllocations;
			m_totalMemory += mem->m_totalMemory;
		}
	}
};

/// Debug page with memory status
class CDebugPageHavokMemoryStats : public IDebugPage
{
private:
	CDebugOptionsTree*		m_tree;

public:
	CDebugPageHavokMemoryStats()
		: IDebugPage( TXT("Havok Memory") )
		, m_tree( NULL )
	{};

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();
	}

	~CDebugPageHavokMemoryStats()
	{
		delete m_tree;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		// Create tree
		if ( !m_tree )
		{
			const Uint32 width = frame->GetFrameOverlayInfo().m_width - 100;
			const Uint32 height = frame->GetFrameOverlayInfo().m_height - 100;
			m_tree = new CDebugOptionsTree( 50, 50, width, height, this );

			// Create the root memory classes
			CDebugCheckBoxHavokMemory* root = new CDebugCheckBoxHavokMemory( NULL, HK_MEMORY_CLASS_ROOT );
			root->Expand( true );
			m_tree->AddRoot( root );
		}

		// Render tree		
		m_tree->OnRender( frame );		
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// Send the event
		if ( m_tree )
		{
			if ( m_tree->OnInput( key, action, data ) )
			{
				return true;
			}
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		// Update crap
		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
		}
	}
};

void CreateDebugPageHavokMemory()
{
	IDebugPage* page = new CDebugPageHavokMemoryStats();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}
#endif

#endif
