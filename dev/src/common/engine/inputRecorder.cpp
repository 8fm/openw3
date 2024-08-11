#include "build.h"

#ifndef NO_TEST_FRAMEWORK

#include "inputRecorder.h"
#include "debugConsole.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( SRecordedInput );

CInputRecorder::CInputRecorder( const String& testName )
	: m_testName( testName )
{
	m_startTick = GEngine->GetCurrentEngineTick();
}

CInputRecorder::~CInputRecorder()
{
}

void CInputRecorder::ProcessInput( Uint64 currentRelativeTick, const BufferedInput& input )
{
	if ( GDebugConsole->IsVisible() )
	{
		return;
	}

	for( Uint32 i = 0; i < input.Size(); ++i )
	{
		SRecordedInput serializableStruct = input[ i ];

		// Keys to ignore
		if
		(
			!(
				serializableStruct.key == IK_F10 ||
				serializableStruct.key == IK_Tilde
			)
		)
		{
			AddToMap( m_rawInput, currentRelativeTick, serializableStruct );
		}
	}
}

void CInputRecorder::OnSave()
{
#ifndef NO_RESOURCE_IMPORT
	SaveRawInput();
#endif // NO_RESOURCE_IMPORT
}

#ifndef NO_RESOURCE_IMPORT

void CInputRecorder::SaveRawInput()
{
	IFile* file = GFileManager->CreateFileWriter( GFileManager->GetDataDirectory() + TXT("test_cases/") + m_testName + TXT(".rawinput"), FOF_AbsolutePath );
	ASSERT( file );

	Uint32 rawInputSize = m_rawInput.Size();
	*file << rawInputSize;

	for( TRawInputMap::iterator iter = m_rawInput.Begin(); iter != m_rawInput.End(); ++iter )
	{
		*file << iter->m_first;
		iter->m_second.Serialize( *file );
	}

	delete file;
}

#endif // NO_RESOURCE_IMPORT

#endif // NO_TEST_FRAMEWORK
