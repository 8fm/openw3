
#include "build.h"
#include "languageSelector.h"

#include "editorExternalResources.h"
#include "../../common/core/2darray.h"
#include "../../common/core/depot.h"

CEdLanguageSelector::CEdLanguageSelector( CPropertyItem* item, EType type )
	: CEdMappedSelectionEditor( item )
	, m_type(type)
{}


void CEdLanguageSelector::FillMap( TDynArray< TPair< String, String > >& map )
{
	const Char* const csvFile = m_type == eText ? TEXT_LANGUAGES_CSV : SPEECH_LANGUAGES_CSV;

	if ( THandle< C2dArray > list = LoadResource< C2dArray >( csvFile ) )
	{
		for ( Uint32 i = 0; i < list->GetNumberOfRows(); ++i )
		{
			String languageName = list->GetValue( TXT("Language"), i );
			String displayName = list->GetValue( TXT("DisplayName"), i );
			displayName = String::Printf(TXT("%ls (%ls)"), displayName.AsChar(), languageName.AsChar() );
			map.PushBack( MakePair( languageName, displayName ) );
		}
	}
}
