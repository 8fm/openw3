/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idConnector.h"
#include "../../common/core/factory.h"
#include "../../common/engine/localizationManager.h"


IMPLEMENT_ENGINE_CLASS( SVoiceTagWrapper )
IMPLEMENT_ENGINE_CLASS( SIDConnectorLine )
IMPLEMENT_ENGINE_CLASS( SIDConnectorPackDefinition )
IMPLEMENT_ENGINE_CLASS( CIDConnectorPack )

//------------------------------------------------------------------------------------------------------------------
// Dialog connector pack resource (a list of connectors)
//------------------------------------------------------------------------------------------------------------------
CIDConnectorPack::CIDConnectorPack()
{

}

void CIDConnectorPack::OnPostLoad()
{
	RefreshVoiceovers(); 
}

void CIDConnectorPack::OnPreSave()
{
	TBaseClass::OnPreSave();

#ifndef NO_EDITOR
	SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );
#endif
}

void CIDConnectorPack::RefreshVoiceovers()
{
#ifndef NO_EDITOR
	if ( m_matchingVoiceTags.Empty() )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		m_lines[ i ].m_exampleVoiceoverFile = GetVoiceoverFileNameForLine( i, m_matchingVoiceTags[ 0 ].m_voiceTag );
	}
#endif
}

void CIDConnectorPack::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		for ( Uint32 k = 0; k < m_matchingVoiceTags.Size(); ++k )
		{
			localizedStrings.PushBack( LocalizedStringEntry( &m_lines[ i ].m_text, TXT("Dialog connector"), this, GetVoiceoverFileNameForLine( i, m_matchingVoiceTags[ k ].m_voiceTag ), GetStringKeyForLine( i ) ) );
		}
	}
}

String CIDConnectorPack::GetVoiceoverFileNameForLine( Uint32 lineIndex, CName voiceTag ) const
{
	R6_ASSERT( m_matchingVoiceTags.Exist( SVoiceTagWrapper( voiceTag ) ) );
	return String::Printf( TXT("%s_ID_%s_%09ld"), GCommonGame->GetGamePrefix(), voiceTag.AsString().AsChar(), m_lines[ lineIndex ].m_text.GetIndex() );
}

String CIDConnectorPack::GetStringKeyForLine( Uint32 lineIndex ) const
{
	// TODO: do we even need this?
	return String::Printf( TXT("idconnector%09ld"), m_lines[ lineIndex ].m_text.GetIndex() );
}

String CIDConnectorPack::GetStringInCurrentLanguageForLine( Uint32 lineIndex ) const
{
	return m_lines[ lineIndex ].m_text.GetString();
}

void CIDConnectorPack::GetVoiceoverFileNamesForLine( Uint32 lineIndex, TDynArray< String > outFineNames ) const
{
	for ( Uint32 i = 0; i < m_matchingVoiceTags.Size(); ++i )
	{
		outFineNames[ i ].PushBack( GetVoiceoverFileNameForLine( lineIndex, m_matchingVoiceTags[ i ].m_voiceTag ) );
	}
}

Bool CIDConnectorPack::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
#if VER_IDLINES_REFACTOR > VER_MINIMAL
	if ( propertyName == CNAME( lines ) )
	{
		TDynArray< LocalizedString > oldTypeLines;
		if ( readValue.AsType( oldTypeLines ) )
		{
			if ( !oldTypeLines.Empty() )
			{
				m_lines.Resize( oldTypeLines.Size() );
				for ( Uint32 i = 0; i < oldTypeLines.Size(); ++i )
				{
					m_lines[ i ].m_text = oldTypeLines[ i ];
				}
				return true;
			}
		}
	}
#endif
	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

void CIDConnectorPack::OnPropertyPostChange( IProperty* prop )
{
	RefreshVoiceovers();

	TBaseClass::OnPropertyPostChange( prop );
}


//------------------------------------------------------------------------------------------------------------------
// Factory 
//------------------------------------------------------------------------------------------------------------------
class CIDConnectorPackResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CIDConnectorPackResourceFactory, IFactory, 0 );

public:
	CIDConnectorPackResourceFactory()
	{
		m_resourceClass = ClassID< CIDConnectorPack >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		return ::CreateObject< CIDConnectorPack >( options.m_parentObject );
	}
};

BEGIN_CLASS_RTTI( CIDConnectorPackResourceFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CIDConnectorPackResourceFactory )
