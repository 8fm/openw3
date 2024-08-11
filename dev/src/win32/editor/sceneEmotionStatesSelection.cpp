#include "build.h"
#include "sceneEmotionStatesSelection.h"
#include "editorExternalResources.h"

/*void CSceneEmotionStatesSelection::FillChoices()
{	
	C2dArray* emotionArray = LoadResource< C2dArray >( EMOTION_STATES_TABLE );
	ASSERT( emotionArray );

	if ( emotionArray )
	{
		// Fill temporary array with voice tags
		for ( Uint32 i=0; i<emotionArray->GetNumberOfRows(); i++ )
		{
			String emotionTag = emotionArray->GetValue( 0, i );
			m_ctrlChoice->AppendString( emotionTag.AsChar() );
		}
	}
}
*/