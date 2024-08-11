#include "build.h"
#include "dlcMounter.h"
#include "saveFileDLCMounter.h"
#include "dlcDefinition.h"
#include "../engine/localizationManager.h"

IMPLEMENT_ENGINE_CLASS( CDLCDefinition );

CDLCDefinition::CDLCDefinition()
	: m_initiallyEnabled( true )
	, m_visibleInDLCMenu( true )
	, m_requiredByGameSave( false )
{
}

void CDLCDefinition::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// collect localized names
	if ( file.IsWriter() && file.IsCooker() )
	{
		CSpeechCollector* speechCollector = file.QuerySpeechCollector();
		if( speechCollector )
		{
			if ( !m_localizedNameKey.Empty() )
				speechCollector->ReportStringKey( m_localizedNameKey );

			if ( !m_localizedDescriptionKey.Empty() )
				speechCollector->ReportStringKey( m_localizedDescriptionKey );
		}
	}
}

void CDLCDefinition::GetContentMounters( TDynArray< THandle< IGameplayDLCMounter > >& outMounters ) const
{
	outMounters.Reserve( outMounters.Size() + m_mounters.Size() );

	for ( auto* content : m_mounters )
	{
		outMounters.PushBack( content );
	}
}

Bool CDLCDefinition::CanStartInStandaloneMode() const
{
	for ( auto content : m_mounters )
	{
		if ( CSaveFileDLCMounter* savMounter = Cast< CSaveFileDLCMounter > ( content ) )
		{
			return savMounter->IsValid();
		}
	}

	return false;
}

IFile* CDLCDefinition::CreateStarterFileReader() const
{
	for ( auto content : m_mounters )
	{
		if ( CSaveFileDLCMounter* savMounter = Cast< CSaveFileDLCMounter > ( content ) )
		{
			if ( savMounter->IsValid() )
			{
				return savMounter->CreateStarterFileReader();
			}
		}
	}

	return nullptr;
}

#ifndef NO_EDITOR
bool CDLCDefinition::DoAnalyze( CAnalyzerOutputList& outputList )
{
	for ( auto* mounter : m_mounters )
	{
		if ( mounter )
		{
			mounter->DoAnalyze( outputList );
		}
		else
		{
			ERR_GAME( TXT("[DLC] Empty/Missing DLC mounter in DLC %ls. DLC may be broken."), GetDepotPath().AsChar() );
		}
	}

	return true;
}
#endif

