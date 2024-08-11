/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES
#ifndef NO_PERFCOUNTERS

#include "debugWindowsManager.h"
#include "debugCheckBox.h"
#include "debugPageManagerBase.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"

class CStatNode
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );
public:
	CProfilerStatBox m_statBox;
	CStatNode* m_parent;
	CStatNode* m_child;
	CStatNode* m_sibling;

	CStatNode(CPerfCounter* counter, CStatNode* parent = NULL)
		:m_statBox( counter ),
		m_parent( parent ),
		m_child( NULL ),
		m_sibling( NULL )
	{
		//recursive
		if (counter->GetFirstChild())
			m_child = new CStatNode( counter->GetFirstChild(), this );
		if (counter->GetSibling())
			m_sibling = new CStatNode( counter->GetSibling(), parent );
	}

	~CStatNode()
	{
		CStatNode* node = m_child;
		while(node)
		{
			CStatNode* t = node->m_sibling;
			delete node;
			node = t;
		}
	}

	void Tick()
	{
		m_statBox.Tick();
		CStatNode* node = m_child;
		while(node)
		{
			node->Tick();
			node = node->m_sibling;
		}
	}
};

/// Special check box option for toggling show flag on and off
class CDebugCheckBoxProfiler : public IDebugCheckBox
{
protected:
	CStatNode*		m_statNode;
	Bool			m_isActive;

public:
	static String StripPath(CStatNode* statNode)
	{
		if ( !statNode->m_statBox.GetPerfCounter()->GetName() || !strlen( statNode->m_statBox.GetPerfCounter()->GetName() ) || statNode->m_statBox.GetPerfCounter()->GetName()[0] == ' ' )
		{
			ASSERT( false, TXT("Empty perf counter name!") );
			return TXT("");
		}
		const Char* str = ANSI_TO_UNICODE( statNode->m_statBox.GetPerfCounter()->GetName() );
		static const String separator = L"\\";
		//special case for Roots!
		Int32 rootIndex = -1;
		for ( Uint32 i=0; i<CProfiler::GetThreadCount();++i)
		{
			if (statNode->m_statBox.GetPerfCounter()==CProfiler::GetThreadRoot(i))
			{
				rootIndex = i;
				break;
			}
		}
		TDynArray<String> path = String(str).Split(separator);
		if (rootIndex>-1)
		{
			return String::Printf(TXT("%s||%d"),path.Back().AsChar(),rootIndex);
		}
		return path.Back();
	}

public:
	CDebugCheckBoxProfiler( IDebugCheckBox* parent, CStatNode* statNode, Bool canExpand )
		: IDebugCheckBox( parent, StripPath(statNode), canExpand, true )
		, m_statNode( statNode )
	{
	};

	void SetProfiler(CStatNode* statNode)
	{
		m_statNode = statNode;
	}

	//! Focus was lost
	virtual void OnLostFocus()
	{
		CDebugCheckBoxProfiler* parent = static_cast<CDebugCheckBoxProfiler*>( GetParent() );
		if ( parent )
		{
			parent->m_isActive = false;
		}
	}

	//! Focus was gained
	virtual void OnGetFocus()
	{
		CDebugCheckBoxProfiler* parent = static_cast<CDebugCheckBoxProfiler*>( GetParent() );
		if ( parent )
		{
			parent->m_isActive = true;
		}
	}

	//! Is this item checked ?
	virtual Bool IsChecked() const
	{
		return false;
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		//grab stats
		double time = m_statNode->m_statBox.GetAverageTime();
		double maxTime = m_statNode->m_statBox.GetMaxTime();
		double hitCount = m_statNode->m_statBox.GetAverageHitCount();
		double percent = 0;
		double percentMax = 0.0f;
		if (!m_statNode->m_parent)
		{
			//adjust for children
			CStatNode* child = m_statNode->m_child;
			while (child)
			{
				time += child->m_statBox.GetAverageTime();
				maxTime += child->m_statBox.GetMaxTime();
				child = child->m_sibling;
			}
		}
		else if (m_statNode->m_parent->m_statBox.GetAverageTime()>0.00001)
		{
			double avg = m_statNode->m_parent->m_statBox.GetAverageTime();
			percent = 100*time/avg;
			percentMax = 100*maxTime/avg;
		}

		CDebugCheckBoxProfiler* parent = static_cast<CDebugCheckBoxProfiler*>(GetParent());
		if (parent && parent->m_isActive)
		{
			static Int32 width = 750;
			static Int32 height = 12;
			static Int32 offsetX = 0;
			static Int32 offsetY = -9;
			static Color green(0,120,0);
			static Color red(120,0,0);
			static Color gray(60,60,60);

			// Draw background bar
			Int32 barWidth = Clamp< Int32 >( (Int32)( (percentMax/100.0) * width ), 0, width );
			frame->AddDebugRect( x+offsetX, y+offsetY, barWidth, height, red );

			// Draw background bar
			barWidth = Clamp< Int32 >( (Int32)( (percent/100.0) * width ), 0, width );
			frame->AddDebugRect( x+offsetX, y+offsetY, barWidth, height, green );

			// Draw frame
			frame->AddDebugFrame( x+offsetX, y+offsetY, width, height, gray );
		}

		Color color = Color::WHITE;
		if ( this == options.m_selected ) color = Color::YELLOW;
		frame->AddDebugScreenText( x+160, y, String::Printf(TXT("     (%.2lf)%%    ms(%.3lf/%.3lf)   c(%.0lf)"), percent, time, maxTime, hitCount), color );

		IDebugCheckBox::OnRender(frame,x,y,counter,options);
	}

	//! Toggle state of this item, only for check items
	virtual void OnToggle()
	{
	}

	Bool OnReset()
	{
		CProfiler::Reset();
		return true;
	}

	//! Handle input
	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
	{
		// Enter
		if ( action == IACT_Press )
		{
			switch(key)
			{
			case IK_X:
			case IK_Pad_X_SQUARE:
				// Toggle
				if ( OnReset() )
					return true;
				break;
			}
		}

		// Not handled
		return IDebugCheckBox::OnInput( key, action, data );
	}
};

/// Special check box option for toggling show flag on and off
class CCategoryCheckBoxProfiler : public IDebugCheckBox
{
protected:
	TDynArray<CStatNode*> m_statBoxes;

public:
	CCategoryCheckBoxProfiler( IDebugCheckBox* parent, const String& name, CStatNode* counter, Bool canExpand )
		: IDebugCheckBox( parent, name, canExpand, true )		
	{
		AddCounter(counter);
	};

	void AddCounter(CStatNode* counter)
	{
		if (counter)
			m_statBoxes.PushBack( counter );
	}

	void ClearCounters()
	{
		m_statBoxes.ClearFast();
	}

	//! Is this item checked ?
	virtual Bool IsChecked() const
	{
		return false;
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		if (m_statBoxes.Size())
		{
			double time = 0;
			double hitCount = 0;
			double self_time = 0;
			for (Uint32 i = 0;i<m_statBoxes.Size();++i)
			{
				//grab stats
				time += m_statBoxes[i]->m_statBox.GetAverageTime();
				hitCount += m_statBoxes[i]->m_statBox.GetAverageHitCount();
				//adjust for children
				CStatNode* child = m_statBoxes[i]->m_child;
				while (child)
				{
					self_time-=child->m_statBox.GetAverageTime();
					child = child->m_sibling;
				}
			}
			self_time += time;			
			
			Color color = Color::WHITE;
			if ( this == options.m_selected ) color = Color::YELLOW;
			frame->AddDebugScreenText( x+160, y, String::Printf(TXT("   ms(%.3lf) self(%.3lf)   c(%.0lf)"), time, self_time, hitCount), color );
		}

		IDebugCheckBox::OnRender(frame,x,y,counter,options);
	}

	//! Toggle state of this item, only for check items
	virtual void OnToggle()
	{		
	}

	Bool OnReset()
	{
		CProfiler::Reset();
		return true;
	}

	//! Handle input
	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
	{
		// Enter
		if ( action == IACT_Press )
		{
			switch(key)
			{
			case IK_X:
			case IK_Pad_X_SQUARE:
				// Toggle
				if ( OnReset() )
					return true;
				break;
			}
		}

		// Not handled
		return IDebugCheckBox::OnInput( key, action, data );
	}
};

/// Debug page that can toggle profilers on/off
class CDebugPageProfilers : public IDebugPage
{
protected:
	CDebugOptionsTree*	m_hTree; //<! hierarchical tree
	CDebugOptionsTree*	m_cTree; //<! category tree (flat)
	CDebugOptionsTree*	m_activeTree;
	TDynArray<CStatNode*> m_statBoxes;
	Int32 m_profCount;

public:
	CDebugPageProfilers()
		: IDebugPage( TXT("Profilers") )
		, m_hTree( NULL )
		, m_cTree( NULL )
		, m_profCount(-1)
	{
		EnumerateProfilers();
		m_activeTree = m_hTree;
	};

	~CDebugPageProfilers()
	{
		delete m_hTree;
		delete m_cTree;
		m_activeTree = NULL;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		frame->AddDebugScreenText( 55, 65, TXT("Press TAB or Y/Triangle-Button to switch between profiler views."), Color::YELLOW );
		if ( m_activeTree != NULL )
		{
			m_activeTree->OnRender( frame );
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_Counters );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( (key == IK_Tab || key == IK_Pad_Y_TRIANGLE)&& action == IACT_Press)
		{
			if (m_activeTree == m_hTree)
				m_activeTree = m_cTree;
			else
				m_activeTree = m_hTree;
			return true;
		}
		// Send the event
		if ( m_activeTree != NULL && m_activeTree->OnInput( key, action, data ) )
		{
			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		// Enumerate profilers
		EnumerateProfilers();

		// Tick statistics
		for (Uint32 i =0;i<m_statBoxes.Size();++i)
			m_statBoxes[i]->Tick();

		if ( m_activeTree )
		{
			m_activeTree->OnTick( timeDelta );
		}
	}

private:

	IDebugCheckBox* GetDCBProfiler(CStatNode* statNode, IDebugCheckBox* checkParent, bool canExpand)
	{
		String name = CDebugCheckBoxProfiler::StripPath(statNode);
		IDebugCheckBox* ret = NULL;
		if (!checkParent)
		{
			ret = m_hTree->FindRootByName(name);
			if (!ret)
			{
				ret = new CDebugCheckBoxProfiler( checkParent, statNode, canExpand );
				m_hTree->AddRoot( ret );
			}
			else
			{
				static_cast<CDebugCheckBoxProfiler*>(ret)->SetProfiler(statNode);
			}
		}
		else
		{
			ret = checkParent->FindChildByName(name);
			if (!ret)
			{
				ret = new CDebugCheckBoxProfiler( checkParent, statNode, canExpand );
			}
			else
			{
				static_cast<CDebugCheckBoxProfiler*>(ret)->SetProfiler(statNode);
			}
		}
		return ret;
	}

	void EnumerateProfilers(CStatNode* statNode, IDebugCheckBox* checkParent)
	{
		bool canExpand = statNode->m_child != NULL;
		IDebugCheckBox* checkBox = GetDCBProfiler(statNode, checkParent, canExpand);		
		//breadth first
		if (statNode->m_sibling)
		{
			EnumerateProfilers( statNode->m_sibling, checkParent );
		}
		//then depth
		if (canExpand)
		{
			EnumerateProfilers( statNode->m_child, checkBox );
		}
	}

	void ClearCategoryProfilers(IDebugCheckBox* boxNode)
	{
		static_cast<CCategoryCheckBoxProfiler*>(boxNode)->ClearCounters();
		//recursively for other nodes
		for (Uint32 i = 0; i<boxNode->GetChildren().Size();++i)
		{
			ClearCategoryProfilers(boxNode->GetChildren()[i]);
		}
	}

	void EnumerateCategoryProfilers(CStatNode* statNode)
	{
		IDebugCheckBox* parent = NULL;
		IDebugCheckBox* node;

		//split name
		if (statNode->m_parent)
		{
			String name = ANSI_TO_UNICODE( statNode->m_statBox.GetPerfCounter()->GetName() );
			TDynArray<String> parts = name.Split(TXT("\\"));		
			//find/create category path
			if (parts.Size()>1)
			{
				parent = m_cTree->FindRootByName(parts[0]);
				if (!parent)
				{
					//create parent
					parent = new CCategoryCheckBoxProfiler( NULL, parts[0], NULL, true );
					m_cTree->AddRoot( parent );
				}
				for (Uint32 i=1;i<parts.Size()-1;++i)
				{
					node = parent->FindChildByName(parts[i]);
					if (!node)
					{
						//create parent
						node = new CCategoryCheckBoxProfiler( parent, parts[i], NULL, true );					
					}
					parent = node;
				}

				//add counter, check if it exist
				node = parent->FindChildByName(parts[parts.Size()-1]);
				if (!node)
				{
					//create parent
					node = new CCategoryCheckBoxProfiler( parent, parts[parts.Size()-1], statNode, false );					
				}
				else
				{
					((CCategoryCheckBoxProfiler*)node)->AddCounter(statNode);
				}
			}
			else if ( parts.Size() )
			{
				//add counter, check if it exist
				node = m_cTree->FindRootByName(parts[0]);
				if (!node)
				{
					//create parent
					m_cTree->AddRoot(new CCategoryCheckBoxProfiler( NULL, parts[0], statNode, false ));
				}
				else
				{
					((CCategoryCheckBoxProfiler*)node)->AddCounter(statNode);
				}
			}
		}
		//recursively for other nodes
		CStatNode* snode = statNode->m_child;
		while (snode)
		{
			EnumerateCategoryProfilers( snode );
			snode = snode->m_sibling;
		}
	}

	void EnumerateProfilers()
	{
		if ( m_profCount == (Int32)CProfiler::GetCountersCount() )
		{
			return;
		}

		EnumerateStatBoxes();

		if(!m_hTree)
		{
			m_hTree = new CDebugOptionsTree( 55, 80, 1230, 600, this );
		}

		m_profCount = static_cast< Int32 >( CProfiler::GetCountersCount() );

		// Enumerate profilers
		for (Uint32 i=0; i<CProfiler::GetThreadCount();++i)
			EnumerateProfilers( m_statBoxes[i], NULL );

		if(!m_cTree)
		{
			m_cTree = new CDebugOptionsTree( 55, 80, 1230, 600, this );
		}
		for (Uint32 i=0; i<m_cTree->m_roots.Size();++i)
			ClearCategoryProfilers( m_cTree->m_roots[i] );		

		// Enumerate category profilers
		for (Uint32 i=0; i<CProfiler::GetThreadCount();++i)
			EnumerateCategoryProfilers( m_statBoxes[i] );
	}

	void EnumerateStatBoxes()
	{
		//free hierarchy
		for (Uint32 i =0;i<m_statBoxes.Size();++i)
			delete m_statBoxes[i];
		m_statBoxes.Clear();
		//create statbox hierarchy
		for (Uint32 i=0; i<CProfiler::GetThreadCount();++i)
			m_statBoxes.PushBack( new CStatNode( CProfiler::GetThreadRoot(i), NULL ) );
	}
};

void CreateDebugPageProfilers()
{
	IDebugPage* page = new CDebugPageProfilers();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
#endif