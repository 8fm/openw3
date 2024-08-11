
#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::CollectAnimationsFromScript( const String& absFilePath )
{
	// TODO Optimize!!!

	String fileData;

	GFileManager->LoadFileToString( absFilePath, fileData, true );

	const Uint32 size = fileData.GetLength();

	String funcName = TXT(".ActionPlaySlotAnimation");

	size_t currPos = 0;
	size_t startOffset = 0;
	while ( 1 )
	{
		if ( !fileData.FindSubstring( funcName, currPos,  false, startOffset ) )
		{
			return;
		}

		currPos += funcName.GetLength();

		Int32 start = -1;
		Int32 end = -1;

		for ( Uint32 i=currPos; i<size; ++i )
		{
			if ( start == -1 )
			{
				if ( fileData.TypedData()[ i ] == ',' )
				{
					start = i+1;
				}
			}
			else if ( end == -1 )
			{
				if ( fileData.TypedData()[ i ] == ',' || fileData.TypedData()[ i ] == ')' )
				{
					end = i;
					break;
				}
			}
		}

		if ( start != -1 && end != -1 && start != size && end != size )
		{
			String str = fileData.MidString( start, end - start );
			str.Trim();

			if ( str.TypedData()[ 0 ] == '\'' && str.TypedData()[ str.GetLength() - 1 ] == '\'' )
			{
				SExternalAnim anim;
				anim.m_animation = str.MidString( 1, str.GetLength() - 2 );
				anim.m_owner = TXT("Script");
				anim.m_desc = String::Printf( TXT("Resource %s"), absFilePath.AsChar() );

				m_externalAnims.PushBack( anim );
			}
			else
			{
				m_todoList.AddTask( new EdAnimationReporterScriptPlaySlotAnim( absFilePath ) );
			}

			currPos = end;
		}
	}
}
