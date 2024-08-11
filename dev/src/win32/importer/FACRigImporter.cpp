/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "FACFormat.h"
#include "../../common/core/importer.h"
#include "../../common/engine/skeleton.h"

class CFACRigImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CFACRigImporter, IImporter, 0 );

public:
	CFACRigImporter();
	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CFACRigImporter )
	PARENT_CLASS(IImporter)
	END_CLASS_RTTI()
IMPLEMENT_ENGINE_CLASS(CFACRigImporter);

CFACRigImporter::CFACRigImporter()
{
	m_resourceClass = ClassID< CSkeleton >();
	m_formats.PushBack( CFileFormat( TXT("fac"), TXT("FAC file") ) );
}

CResource* CFACRigImporter::DoImport( const ImportOptions& options )
{
	CSkeleton::FactoryInfo info;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CSkeleton >( options.m_existingResource );

	FILE* f = _wfopen(options.m_sourceFilePath.AsChar(),TXT("rb"));
	if(!f){return NULL;}

	fac::cface head;
	head.read( f );
	fclose(f);

	// Skeleton
	{
		info.m_bones.Resize(head.skeleton.numbones);

		const UINT numBones = head.skeleton.numbones;
		for (UINT i=0;i<numBones;++i)
		{
			CSkeleton::FactoryInfo::BoneImportInfo& boneInfo = info.m_bones[i];

			{
				const int numChars = head.skeleton.names[i].numchars;
				char* name = new char[numChars+1];
				Red::System::MemoryCopy(name, head.skeleton.names[i].data, numChars);
				boneInfo.m_name = StringAnsi(name);
				delete [] name;
			}

			boneInfo.m_lockTranslation = false;

			RED_ASSERT( head.skeleton.parents[i] <= MAXINT16, TXT("parentindex overflow while importing, skeleton will be broken") );

			boneInfo.m_parentIndex = static_cast<Uint16>( head.skeleton.parents[i] );

			if (boneInfo.m_parentIndex==-1)
			{
				boneInfo.m_referencePose = RedQsTransform::IDENTITY;
			}
			else
			{
				RedMatrix3x3 rotation;
				rotation.Row0.X = head.skeleton.transforms[i].row1[0];
				rotation.Row0.Y = head.skeleton.transforms[i].row1[1];
				rotation.Row0.Z = head.skeleton.transforms[i].row1[2];
				rotation.Row1.X = head.skeleton.transforms[i].row2[0];
				rotation.Row1.Y = head.skeleton.transforms[i].row2[1];
				rotation.Row1.Z = head.skeleton.transforms[i].row2[2];
				rotation.Row2.X = head.skeleton.transforms[i].row3[0];
				rotation.Row2.Y = head.skeleton.transforms[i].row3[1];
				rotation.Row2.Z = head.skeleton.transforms[i].row3[2];

				RedVector4 pos;
				pos.X = head.skeleton.transforms[i].pos[0]*0.01f;
				pos.Y = head.skeleton.transforms[i].pos[1]*0.01f;
				pos.Z = head.skeleton.transforms[i].pos[2]*0.01f;
				pos.W =1.0f;

				boneInfo.m_referencePose.Rotation.ConstructFromMatrix(rotation);
				boneInfo.m_referencePose.Translation = pos;
				boneInfo.m_referencePose.Scale = RedVector4::ONES;
			}
		}

		CSkeleton* retVal = CSkeleton::Create( info );
		return retVal;
	}
}
