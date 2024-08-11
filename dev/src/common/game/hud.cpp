/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashPlayer.h"
#include "../../common/engine/flashMovie.h"

#include "flashScriptSupport.h"
#include "guiManager.h"
#include "flashBindingAdapter.h"
#include "flashMovieAdapter.h"
#include "flashHudAdapter.h"
#include "hudModule.h"

#include "hudResource.h"
#include "hud.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CHud );

//////////////////////////////////////////////////////////////////////////
// CHud
//////////////////////////////////////////////////////////////////////////
CHud::CHud()
	: m_hudResource( nullptr )
	, m_flashMovieAdapter( nullptr )
	, m_flashHudAdapter( nullptr )
	, m_scriptedFlashObjectPool( nullptr )
	, m_scriptedFlashSprite( nullptr )
	, m_scriptedFlashValueStorage( nullptr )
	, m_isInTick( false )
	, m_isAttached( true )
{
}

void CHud::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_hudResource )
		{
			file << m_hudResource;
		}

		for ( THudModuleMap::const_iterator it = m_hudModuleMap.Begin(); it != m_hudModuleMap.End(); ++it )
		{
			CHudModule* hudModule = it->m_second;
			file << hudModule;
		}
	}

	TBaseClass::OnSerialize( file );
}

void CHud::OnFinalize()
{	
	Cleanup();

	TBaseClass::OnFinalize();
}

void CHud::Cleanup()
{
	// Cleanup HUD modules
	for ( THudModuleMap::const_iterator it = m_hudModuleMap.Begin(); it != m_hudModuleMap.End(); ++it )
	{
		CHudModule* hudModule = it->m_second;
		ASSERT( hudModule );
		
		// TBD: Might want to unload the module Flash if the Witcherscript has any wanted cleanup logic
		if ( hudModule )
		{
			hudModule->Discard();
		}
	}
	m_hudModuleMap.ClearFast();

	delete m_scriptedFlashObjectPool;
	m_scriptedFlashObjectPool = nullptr;	
	RED_FATAL_ASSERT( ! m_scriptedFlashValueStorage.Get(), "Handle should be null" );
	RED_FATAL_ASSERT( ! m_scriptedFlashSprite.Get(), "Handle should be null" );

	// Cleanup HUD
	if ( m_flashHudAdapter )
	{
		m_flashHudAdapter->Release();
		m_flashHudAdapter = nullptr;
	}

	if ( m_flashMovieAdapter )
	{
		VERIFY( m_flashMovieAdapter->Release() == 0 );
		m_flashMovieAdapter = nullptr;
	}

	if ( m_flashMovie )
	{
		VERIFY( m_flashMovie->Release() == 0 );
		m_flashMovie = nullptr;
	}

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	VERIFY( guiManager && guiManager->UnregisterForTick( this ) );

	if ( m_hudResource )
	{
		// Don't call discard. Could be in the editor window.
		m_hudResource = nullptr;
	}
}

Bool CHud::RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		return guiManager->RegisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

Bool CHud::UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		return guiManager->UnregisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

void CHud::SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKeys( keysToSet );
	}
}

void CHud::ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKeys( keysToClear );
	}
}

void CHud::SetIgnoreKey( EInputKey keyToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKey( keyToSet );
	}
}

void CHud::ClearIgnoreKey( EInputKey keyToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKey( keyToClear );
	}
}

void CHud::ClearAllIgnoreKeys()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearAllIgnoreKeys();
	}
}

Bool CHud::CollectForTick( CHudModule* hudModule )
{
	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	ASSERT( hudModule );
	if ( ! hudModule )
	{
		return false;
	}

	ASSERT( hudModule->GetParent() == this );

	return m_hudModuleTickList.PushBackUnique( hudModule );
}

Bool CHud::Init( CGuiManager* guiManager )
{
	ASSERT( guiManager );
	if ( ! guiManager )
	{
		return false;
	}

	ASSERT( ! m_guiManagerHandle.Get() );
	if ( m_guiManagerHandle.Get() )
	{
		GUI_ERROR( TXT("Already initialized with a guiManager!") );
		return false;
	}

	ASSERT( m_hudResource );
	if ( ! m_hudResource )
	{
		GUI_ERROR( TXT("No HUD resource!") );
		return false;
	}

	ASSERT( ! m_flashMovie );
	if ( m_flashMovie )
	{
		GUI_ERROR( TXT("Already initialized with a flash movie!") );
		return false;
	}

	const TSoftHandle< CSwfResource >& hudFlashSwf = m_hudResource->GetHudFlashSwf();

	SFlashMovieInitParams context;
	context.m_layer = DEFAULT_HUD_LAYER;
	// Flash movie starts with refcount 1.
	m_flashMovie = GGame->GetFlashPlayer()->CreateMovie( hudFlashSwf, context );
	if ( ! m_flashMovie )
	{
		GUI_ERROR( TXT("Could not create HUD flash movie '%ls'"), hudFlashSwf.GetPath().AsChar() );
		return false;
	}

	m_flashMovieAdapter = new CFlashMovieAdapter( m_flashMovie );
	VERIFY( m_flashMovieAdapter->Init() );
	VERIFY( guiManager->RegisterForTick( this ) );

	m_guiManagerHandle = guiManager;

	m_scriptedFlashObjectPool = new CScriptedFlashObjectPool;

	return true;
}

Bool CHud::InitWithFlashSprite( const CFlashValue& flashSprite )
{
	ASSERT( ! m_flashHudAdapter );
	if ( m_flashHudAdapter )
	{
		GUI_ERROR( TXT("HUD Flash already registered!") );
		return false;
	}

	m_flashHudAdapter = new CFlashHudAdapter( m_flashMovie, flashSprite, this );
	if ( ! m_flashHudAdapter->Init() )
	{
		GUI_ERROR( TXT("Failed to init Flash HUD adapter") );
		return false;
	}

	m_scriptedFlashSprite = new CScriptedFlashSprite( *m_scriptedFlashObjectPool, flashSprite, TXT("[Hud]") );

	CFlashValueStorage* flashValueStorage = m_flashHudAdapter->GetFlashValueStorage();
	ASSERT( flashValueStorage );
	m_scriptedFlashValueStorage = new CScriptedFlashValueStorage( *m_scriptedFlashObjectPool, flashValueStorage, m_flashMovie );

	return true;
}

Bool CHud::IsInitWithFlashSprite() const
{
	return m_flashHudAdapter != nullptr;
}

CGuiObject* CHud::GetChild( const String& childName )
{
	CHudModule* hudModule = nullptr;
	m_hudModuleMap.Find( childName, hudModule );
	return hudModule;
}

Bool CHud::CreateHudModule( const String& moduleName, Int32 userData /*=-1*/ )
{
	PC_SCOPE( CreateHudModule );

	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return nullptr;
	}

	ASSERT( m_flashHudAdapter );
	if ( ! m_flashHudAdapter )
	{
		GUI_ERROR( TXT("Flash doesn't appear to be initalized when trying to create HUD module '%ls'."), moduleName.AsChar() );
		return false;
	}

	SHudModuleDesc hudModuleDesc;
	if ( ! FindHudModuleDescByName( moduleName, hudModuleDesc  ) )
	{
		GUI_ERROR( TXT("No such module '%ls'"), moduleName.AsChar() );
		return false;
	}

	const CName& moduleClassName = hudModuleDesc.m_moduleClassName;

	if ( m_hudModuleMap.KeyExist( moduleName ) )
	{
		GUI_ERROR( TXT("Module '%ls' already loaded!"), moduleName.AsChar() );
		return false;
	}

	const CClass* hudModuleClass = nullptr;
	if ( ! m_hudModuleClassMap.Find( moduleClassName, hudModuleClass ) )
	{
		HALT( "HUD module class mapping not found!" );
		return false;
	}
	ASSERT( hudModuleClass );
	if ( ! hudModuleClass )
	{
		return false;
	}

	CHudModule* hudModule = ::CreateObject< CHudModule >( const_cast< CClass* >( hudModuleClass ), this, OF_Transient );
	ASSERT( hudModule );
	if ( ! hudModule )
	{
		GUI_ERROR( TXT("Failed to create object for HUD module '%ls'"), moduleName.AsChar() );
		return false;
	}
	
	const CHudModuleResourceBlock* resourceBlock = hudModuleDesc.m_moduleResourceBlock.Get();
	ASSERT( resourceBlock );
	if ( ! resourceBlock )
	{
		GUI_ERROR( TXT("Can't create HUD module '%ls' as no resource block is associated with!"), moduleName.AsChar() );
		hudModule->Discard();
		return false;
	}

	SHudModuleInitParams hudModuleInitParams( moduleName, resourceBlock->IsGameInputListener() );
	VERIFY( hudModule->Init( this, hudModuleInitParams ) );
	
	if ( ! m_hudModuleMap.Insert( moduleName, hudModule ) )
	{
		GUI_ERROR( TXT("Duplicate HUD module name '%ls' for class '%ls'"), hudModuleDesc.m_moduleName.AsChar(),
			hudModuleDesc.m_moduleClassName.AsString().AsChar() );

		hudModule->Discard();
		return false;
	}

	if ( ! m_flashHudAdapter->LoadModuleAsync( moduleName, userData ) )
	{
		GUI_ERROR( TXT("Failed to load module '%ls'"), moduleName.AsChar() );
		hudModule->Discard();
		return false;
	}

	return true;
}

//TODO: Discarding a module that hasn't registered Flash yet
Bool CHud::DiscardHudModule( const String& moduleName, Int32 userData /*=-1*/ )
{
	PC_SCOPE( DiscardHudModule );

	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	CHudModule* hudModule = nullptr;
	if ( ! m_hudModuleMap.Find( moduleName, hudModule ) )
	{
		GUI_ERROR( TXT("HUD module '%ls' is not loaded"), moduleName.AsChar() );
		return false;
	}
	
	// Unload Flash first then discard hudModule CObject in case it gets any ideas to send events in its death throes
	ASSERT( m_flashHudAdapter );
	if ( ! m_flashHudAdapter )
	{
		GUI_ERROR( TXT("Flash doesn't appear to be initalized when trying to discard HUD module '%ls'."), moduleName.AsChar() );
		return false;
	}
	m_flashHudAdapter->UnloadModuleAsync( moduleName, userData );

	VERIFY( m_hudModuleMap.Erase( moduleName ) );

	(void)m_hudModuleTickList.Remove( hudModule );

	hudModule->Discard();

	return true;
}

Bool CHud::FindHudModuleDescByName( const String& moduleName, SHudModuleDesc& outHudModuleDesc )
{
	PC_SCOPE( FindHudModuleDescByName );

	return m_hudModuleDescMap.Find( moduleName, outHudModuleDesc );
}

void CHud::Tick( const Float timeDelta )
{
	PC_SCOPE( Tick );

	struct STickScope : private Red::System::NonCopyable
	{
		Bool& m_isInTickMember;

		STickScope( Bool& isInTickMember )
			: m_isInTickMember( isInTickMember )
		{
			m_isInTickMember = true;
		}

		~STickScope()
		{
			m_isInTickMember = false;
		}
	};	

	STickScope tickScope( m_isInTick );

	if ( ! m_isAttached )
	{
		return;
	}

	if ( ! m_flashHudAdapter )
	{
		return;
	}

	for ( THudModuleList::const_iterator it = m_hudModuleTickList.Begin(); it != m_hudModuleTickList.End(); ++it )
	{
		CHudModule* hudModule = *it;
		hudModule->Tick( timeDelta );
	}

	CallEvent( CNAME(OnTick), timeDelta );
}

//TBD: State if loading flashMovie async
//Also, need to null out handles to proxies so can't invoke and build up
//events in the movieclip
void CHud::Attach()
{
	if ( ! m_isAttached )
	{
		if ( m_flashMovie )
		{
			m_flashMovie->Attach();
		}
		m_isAttached = true;
	}
}

void CHud::Detach()
{
	if ( m_isAttached )
	{
		if ( m_flashMovie )
		{
			m_flashMovie->Detach();
		}
		m_isAttached = false;
	}
}

void CHud::funcCreateHudModule( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, moduleName, String::EMPTY );
	GET_PARAMETER_OPT( Int32, userData, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashHudAdapter );
	if ( ! m_flashHudAdapter )
	{
		GUI_ERROR( TXT("HUD flash not available") );
		return;
	}

	CreateHudModule( moduleName );
}

void CHud::funcDiscardHudModule( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, moduleName, String::EMPTY );
	GET_PARAMETER_OPT( Int32, userData, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashHudAdapter );
	if ( ! m_flashHudAdapter )
	{
		GUI_ERROR( TXT("HUD flash not available") );
		return;
	}

	DiscardHudModule( moduleName );
}

void CHud::funcGetHudFlash( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashSprite );
	if ( ! m_scriptedFlashSprite )
	{
		GUI_ERROR( TXT("HUD flash not available") );
	}

	RETURN_HANDLE( CScriptedFlashSprite, m_scriptedFlashSprite );
}

void CHud::funcGetHudFlashValueStorage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashValueStorage );
	if ( ! m_scriptedFlashValueStorage )
	{
		GUI_ERROR( TXT("HUD Flash value storage not available") );
	}

	RETURN_HANDLE( CScriptedFlashValueStorage, m_scriptedFlashValueStorage );
}

void CHud::funcGetHudModule( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, moduleName, String::EMPTY );
	FINISH_PARAMETERS;

	CHudModule* hudModule = nullptr;

	CGuiObject* child = GetChild( moduleName );
	if ( child && child->IsInitWithFlashSprite() )
	{
		hudModule = static_cast< CHudModule* >( child );
	}

	RETURN_HANDLE( CHudModule, hudModule );
}

CHud* CHud::CreateObjectFromResource( CHudResource* hudResource, CObject* parent )
{
	PC_SCOPE( CreateObjectFromResource );

	ASSERT( hudResource );
	if ( ! hudResource )
	{
		GUI_ERROR( TXT("Can't create HUD from NULL GUI resource!") );
		return nullptr;
	}

	ASSERT( parent );
	if ( ! parent )
	{
		GUI_ERROR( TXT("No CObject parent passed to HUD!") );
		return nullptr;
	}

	CClass* hudClass = SRTTI::GetInstance().FindClass( hudResource->GetHudClass() );
	ASSERT( hudClass && hudClass->IsA< CHud >() );
	if ( ! hudClass || ! hudClass->IsA< CHud >() )
	{
		GUI_ERROR( TXT("GUI resource does not contain a HUD template!" ) );
		return nullptr;
	}

	ASSERT( hudClass && ! hudClass->IsAbstract() &&
		! hudClass->IsAlwaysTransient() &&
		! hudClass->IsEditorOnly() );
	if ( ! hudClass )
	{
		GUI_ERROR( TXT("HUD base template class is NULL!") );
		return nullptr;
	}
	if ( hudClass->IsAbstract() )
	{
		GUI_ERROR( TXT("HUD base template class is abstract!") );
		return nullptr;
	}
	if ( hudClass->IsAlwaysTransient() )
	{
		GUI_ERROR( TXT("HUD base template class is always transient") );
		return nullptr;
	}
	if ( hudClass->IsEditorOnly() )
	{
		GUI_ERROR( TXT("HUD base template class is editor only") );
		return nullptr;
	}

	CHud* newHudObj = ::CreateObject< CHud >( hudClass, parent, OF_Transient );
	ASSERT( newHudObj );
	if ( ! newHudObj )
	{
		GUI_ERROR( TXT("Failed to create HUD object!") );
		return nullptr;
	}

	// Gather module names and create class mappings
	const TDynArray< CGraphBlock* >& graphBlocks = hudResource->GetResourceBlocks();
	for ( TDynArray< CGraphBlock* >::const_iterator it = graphBlocks.Begin(); it != graphBlocks.End(); ++it )
	{
		CGraphBlock* block = *it;
		CHudModuleResourceBlock* hudBlock = SafeCast< CHudModuleResourceBlock >( block );
		if ( ! hudBlock )
		{
			GUI_ERROR( TXT("HUD resource contains unexpected graph blocks!") );
			newHudObj->Discard();
			return nullptr;
		}

		const CName hudModuleClassName = hudBlock->GetModuleClass();
		const CClass* hudModuleClass = SRTTI::GetInstance().FindClass( hudModuleClassName );
		if ( ! hudModuleClass )
		{
			GUI_ERROR( TXT("HUD module class not found for name '%ls'"), hudModuleClassName.AsString().AsChar() );
			newHudObj->Discard();
			return nullptr;
		}
		if ( ! hudModuleClass->IsA< CHudModule >() )
		{
			GUI_ERROR( TXT("HUD module class is not based on CHudModule '%ls'"), hudModuleClassName.AsString().AsChar() );
			newHudObj->Discard();
			return nullptr;
		}
		if ( hudModuleClass->IsNative() && hudModuleClass->IsAbstract() ) // Allow "script" abstract. I.e., don't create directly in scripts through "new"
		{
			GUI_ERROR( TXT("HUD module class '%ls' is native and abstract!"), hudModuleClassName.AsString().AsChar() );
			newHudObj->Discard();
			return nullptr;

		}
		if ( hudModuleClass->IsEditorOnly() )
		{
			GUI_ERROR( TXT("HUD module class '%ls' is editor-only!"), hudModuleClassName.AsString().AsChar() );
			newHudObj->Discard();
			return nullptr;
		}		
		if ( ! newHudObj->m_hudModuleDescMap.Insert( hudBlock->GetModuleName(), SHudModuleDesc( hudBlock->GetModuleName(), hudBlock->GetModuleClass(), hudBlock ) ) )
		{
			GUI_ERROR( TXT("HUD resource contains duplicate module names for HUD module name '%ls' class '%ls'!"), hudBlock->GetModuleName().AsChar(), hudBlock->GetModuleClass().AsString().AsChar() );
			newHudObj->Discard();
			return nullptr;
		}

		newHudObj->m_hudModuleClassMap.Insert( hudModuleClassName, hudModuleClass );
	}

	newHudObj->m_hudResource = hudResource;

	return newHudObj;
}

