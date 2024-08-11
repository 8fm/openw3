/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPageDynamicAttributes.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptingSystem.h"

IMPLEMENT_ENGINE_CLASS( CDebugAttributesManager );

#ifndef NO_DEBUG_PAGES

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#include "debugPageManagerBase.h"
#include "debugCheckBox.h"
#include "inputBufferedInputEvent.h"
#include "game.h"
#include "renderFrame.h"
#endif

CDebugEnumSelector::CDebugEnumSelector( IDebugCheckBox* parent, const String& name, const SAttribute& attr )
	: IAttributeChanger( attr )
	, IDebugChoice( parent, name )
	, m_index( 0 )
	, m_enum( NULL )
{
	SetSeparation( 200 );

	if( attr.IsValid() )
	{
		ASSERT( attr.m_property->GetType()->GetType() == RT_Enum );
		m_enum = (CEnum*) attr.m_property->GetType();

		// Get current value of Enum as Int32
		Int32 value = GetValue();

		CName valName = CName::NONE;
		// Find the name represented by the value
		if( m_enum->FindName( value, valName ) )
		{
			// Since enum may be represented as any value we must find the proper index
			const Uint32 size = m_enum->GetOptions().Size();
			for( Uint32 i = 0; i < size; ++i )
			{
				if( m_enum->GetOptions()[i] == valName )
				{
					m_index = i;
					break;
				}
			}
		}
	}
}

String CDebugEnumSelector::GetSelection() const
{
	if( !m_enum )
	{
		return TXT("Invalid Enum");
	}

	if( m_enum->GetOptions().Size() > m_index )
	{
		return m_enum->GetOptions()[m_index].AsString();
	}

	return TXT("Invalid Value");
}

void CDebugEnumSelector::OnNext()
{
	if( !m_enum )
	{
		return;
	}

	++m_index;

	if( m_index > m_enum->GetOptions().Size() - 1 )
	{
		m_index = 0;
	}
}

void CDebugEnumSelector::OnPrev()
{
	if( !m_enum )
	{
		return;
	}

	m_index == 0 ? m_index = m_enum->GetOptions().Size() - 1 : --m_index;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CDebugPageDynamicAttributes::CDebugPageDynamicAttributes()
	: IDebugPage( TXT("Dynamic Attributes") )
	, m_tree( NULL )
{
}

void CDebugPageDynamicAttributes::OnTick( Float timeDelta )
{
	IDebugPage::OnTick( timeDelta );

	if ( m_tree )
	{
		m_tree->OnTick( timeDelta );
	}
}

Bool CDebugPageDynamicAttributes::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	if( action == IACT_Press && key == IK_Enter )
	{
		GDebugWin::GetInstance().SetVisible(true);
	}
	return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

	return m_tree ? m_tree->OnInput( key, action, data ) : false;
}

void CDebugPageDynamicAttributes::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
#ifndef NO_DEBUG_WINDOWS
	String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

	frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
	frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

	frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
	frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

	frame->AddDebugScreenFormatedText( 70, 120, Color(255, 0, 0, 255), message.AsChar());

	frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
	return;
#endif

	if( m_tree ) m_tree->OnRender( frame );
}

RED_DEFINE_STATIC_NAME( CDebugPageDynamicAttributes )

void CDebugPageDynamicAttributes::OnPageShown()
{
	GGame->Pause( TXT( "CDebugPageDynamicAttributes" ) );

	if( !m_tree )
	{
		m_tree = new CDebugOptionsTree( 50, 50, 500, 500, this );
	}

	m_tree->Clear();

	THashMap< CName, CAttributeProvider >::iterator provEnd = m_providers.End();
	for( THashMap< CName, CAttributeProvider >::iterator provIt = m_providers.Begin(); provIt != provEnd; ++provIt )
	{
		IDebugCheckBox* group = new IDebugCheckBox( NULL, provIt->m_first.AsString(), true, false );
		m_tree->AddRoot( group );

		CAttributeProvider::Attributes& attrs = provIt->m_second.GetAttributes();
		CAttributeProvider::Attributes::iterator end = attrs.End();
		for( CAttributeProvider::Attributes::iterator it = attrs.Begin(); it != end; )
		{
			if( it->m_second.IsValid()  )
			{
				if( it->m_second.m_property->GetType()->GetType() == RT_Fundamental )
				{
					const CName typeName = it->m_second.m_property->GetType()->GetName();
					if( typeName == CNAME( Bool ) )
					{
						new CDebugBoolSelector( group, it->m_first.AsString(), it->m_second );
					}
					else if( typeName == CNAME( Int32 ) )
					{
						new CDebugTextBoxChanger<Int32>( group, it->m_first.AsString(), it->m_second );
					}
					else if( typeName == CNAME( Float ) )
					{
						new CDebugTextBoxChanger<Float>( group, it->m_first.AsString(), it->m_second );
					}
				}
				else if( it->m_second.m_property->GetType()->GetType() == RT_Enum )
				{
					new CDebugEnumSelector( group, it->m_first.AsString(), it->m_second );
				}

				++it;
			}
			else
			{
				CAttributeProvider::Attributes::iterator temp = it;
				++it;
				attrs.Erase( temp );
			}
		}
	}
}

void CDebugPageDynamicAttributes::OnPageHidden()
{
	if( m_tree->m_active )
	{
		m_tree->m_active->OnLostFocus();
	}

	GGame->Unpause( TXT( "CDebugPageDynamicAttributes" ) );
}

#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CDebugAttributesManager::Init()
{
#ifndef NO_DEBUG_WINDOWS
	m_page = new CDebugPageDynamicAttributes();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( m_page );
#endif

	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_DEBUG, this );
}

void CDebugAttributesManager::ShutDown()
{
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_DEBUG, NULL );
}

void CDebugAttributesManager::funcAddAttribute( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( CName, propName, CName::NONE );
	GET_PARAMETER_REF( THandle<IScriptable>, context, THandle<IScriptable>() );
	GET_PARAMETER_OPT( CName, groupName, CNAME( Default ) );
	FINISH_PARAMETERS;

#ifndef NO_DEBUG_PAGES
	IScriptable* obj = context.Get();
	ASSERT( obj && TXT("Function AddAttribute is called with NULL context!") );
	if( obj )
	{
		CProperty* prop = obj->GetClass()->FindProperty( propName );
		ASSERT( prop, TXT("AddAttribute - property '%ls' not found in '%ls'."), propName.AsString().AsChar(), obj->GetClass()->GetName().AsString().AsChar() );
		if( prop )
		{
			const ERTTITypeType propType = prop->GetType()->GetType();
			if( propType == RT_Simple || propType == RT_Enum || propType == RT_Fundamental)
			{
				RETURN_BOOL( m_page ? m_page->m_providers.GetRef( groupName ).AddAttribute( name, SAttribute( prop, context ) ) : false );
				return;
			}

			ASSERT( false, TXT("AddAttribute - only Simple and Enum properties are supported.") );
		}
	}
#endif

	RETURN_BOOL( false );
}

void CDebugAttributesManager::Clear()
{
#ifndef NO_DEBUG_PAGES
	if( m_page )
	{
		m_page->m_providers.Clear();
	}
#endif
}
