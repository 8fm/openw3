#include "build.h"
#include "quest.h"
#include "questStartBlock.h"
#include "questGraph.h"

#include "../core/factory.h"
#include "../core/diskFile.h"

IMPLEMENT_ENGINE_CLASS( CQuest );

CQuest::CQuest()
{
}

CQuestStartBlock* CQuest::GetInput() const
{
	TDynArray< CGraphBlock* >& blocks = GetGraph()->GraphGetBlocks();

	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( blocks[ i ] != NULL && blocks[ i ]->IsExactlyA< CQuestStartBlock >() )
		{
			return SafeCast< CQuestStartBlock >( blocks[ i ] );
		}
	}
	return NULL;
}

String CQuest::GetFileName() const
{
	CDiskFile* questFile = GetFile();
	String questFileName;

	if ( questFile )
	{
		questFileName = questFile->GetDepotPath() + TXT( "\\" ) + questFile->GetFileName();
	}

	return questFileName;
}

//////////////////////////////////////////////////////////////////////////

class CQuestFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CQuestFactory, IFactory, 0 );

public:
	CQuestFactory()
	{
		m_resourceClass = ClassID< CQuest >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CQuestFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CQuestFactory );

CResource* CQuestFactory::DoCreate( const FactoryOptions& options )
{
	CQuest *quest = ::CreateObject< CQuest >( options.m_parentObject );
	return quest;
}
