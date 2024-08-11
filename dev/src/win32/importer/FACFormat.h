
#pragma once

#include <stdio.h>

#define NUMAREAS 16
namespace fac
{

	class transform3
	{
	public:
		inline transform3()
		{
			pos[0]=0; pos[1]=0; pos[2]=0;
			row1[0]=1; row1[1]=0; row1[2]=0;
			row2[0]=0; row2[1]=1; row2[2]=0;
			row3[0]=0; row3[1]=0; row3[2]=1;
		}
		float pos[3];
		float row1[3];
		float row2[3];
		float row3[3];
	};

	class qtransform
	{
	public:
		inline qtransform()
		{
			pos[0]=0; pos[1]=0; pos[2]=0;
			rot[0]=0; rot[1]=0; rot[2]=0; rot[3]=1;
		}
		float pos[3];
		float rot[4];
	};

	class fname
	{
	public:
		inline fname(){data=0;}
		inline ~fname()
		{
			if(data){delete data;}
		}
		inline void skip(FILE* f)
		{
			numchars=0;
			fread(&numchars,sizeof(int),1,f);
			fseek(f, sizeof(char)*numchars, SEEK_CUR);
		}
		inline void read(FILE* f)
		{
			numchars=0;
			fread(&numchars,sizeof(int),1,f);
			data = new char[numchars+1];
			fread(data,sizeof(char),numchars,f);
			data[numchars]='\0';
		}
		char* data;
		int numchars;
	};

	class fpose
	{
	public:
		fpose()
		{
			numweights=0;
			numemos=0;
			numnormals=0;
			weights = NULL;
			emos = NULL;
			normals = NULL;
		}
		~fpose()
		{
			if(weights){delete [] weights;}
			if(emos){delete [] emos;}
			if(normals){delete [] normals;}
		}
		void read(FILE* f)
		{
			int numn=0;
			name.read(f);
			fread(&numweights,sizeof(int),1,f);
			fread(&numemos,sizeof(int),1,f);
			fread(&numn,sizeof(int),1,f);
			float* nor = new float[numn];
			weights = new float[numweights];
			emos = new float[numemos];
			numnormals=NUMAREAS;
			normals = new float[numnormals];
			Red::System::MemorySet(normals,0,sizeof(float)*numnormals);
			fread(weights,sizeof(float)*numweights,1,f);
			fread(emos,sizeof(float)*numemos,1,f);
			fread(nor,sizeof(float)*numn,1,f);
			if(numn>NUMAREAS){numn=NUMAREAS;}
			Red::System::MemoryCopy(normals,nor,sizeof(float)*numn);
			delete [] nor;
		}
		int numweights;
		int numemos;
		int numnormals;
		float* weights;
		float* emos;
		float* normals;
		fname name;
	};

	class ftexture
	{
	public:
		inline ftexture(){data=NULL;}
		inline ~ftexture()
		{
			if(data){delete [] data;data=NULL;}
		}
		inline void skip(FILE* f)
		{
			resx = 0;
			resy = 0;
			bpp  = 0;
			name.skip(f);
			fread(&resx,sizeof(int),1,f);
			fread(&resy,sizeof(int),1,f);
			fread(&bpp,sizeof(int),1,f);
			int size = resx*resy*bpp*sizeof(char);
			fseek(f,size,SEEK_CUR);
		}
		inline void read(FILE* f)
		{
			resx = 0;
			resy = 0;
			bpp  = 0;
			name.read(f);
			fread(&resx,sizeof(int),1,f);
			fread(&resy,sizeof(int),1,f);
			fread(&bpp,sizeof(int),1,f);
			data = new char[resx*resy*bpp];
			fread(data,sizeof(char),resx*resy*bpp,f);
		}
		int resx;
		int resy;
		int bpp;
		char* data;
		fname name;
	};

	class fchunk
	{
	public:
		inline fchunk()
		{
			vertices=0;
			normals=0;
			mapping=0;
			bones=0;
			weights=0;
			indices=0;
			textures=0;
			rigmatrices=0;
			attachment=0;	
			vcolor=0;
		}
		inline ~fchunk()
		{
			if(vertices){delete [] vertices;}
			if(normals){delete [] normals;}
			if(mapping){delete [] mapping;}
			if(bones){delete [] bones;}
			if(weights){delete [] weights;}
			if(indices){delete [] indices;}
			if(rigmatrices){delete [] rigmatrices;}
			if(attachment){delete [] attachment;}
			if(textures){delete [] textures;}
			if(vcolor){delete [] vcolor;}
		}
		inline void skip(FILE* f)
		{
			numverts=0;
			numfaces=0;
			fread(&numverts,sizeof(int),1,f);
			fread(&numfaces,sizeof(int),1,f);
			int size = (sizeof(float)*20*numverts) + (sizeof(int)*3*numfaces);
			fseek(f, size, SEEK_CUR);
			numrigmatrices=0;
			fread(&numrigmatrices,sizeof(int),1,f);
			size =  ( sizeof(transform3)+sizeof(int) )*numrigmatrices;
			fseek(f, size, SEEK_CUR);
			numtextures=0;
			fread(&numtextures,sizeof(int),1,f);
			size =  ( sizeof(int) )*numtextures;
			fseek(f, size, SEEK_CUR);
		}
		inline void read(FILE* f)
		{
			numverts=0;
			numfaces=0;
			fread(&numverts,sizeof(int),1,f);
			fread(&numfaces,sizeof(int),1,f);
			vertices    = new float[numverts*3];
			normals     = new float[numverts*3];;
			mapping     = new float[numverts*2];
			bones       = new float[numverts*4];
			vcolor      = new float[numverts*4];
			weights     = new float[numverts*4];
			indices     = new int  [numfaces*3];
			fread(vertices,sizeof(float)*3,numverts,f);
			fread(normals,sizeof(float)*3,numverts,f);
			fread(mapping,sizeof(float)*2,numverts,f);
			fread(weights,sizeof(float)*4,numverts,f);
			fread(bones,sizeof(float)*4,numverts,f);
			fread(vcolor,sizeof(float)*4,numverts,f);
			fread(indices,sizeof(int)*3,numfaces,f);
			numrigmatrices=0;
			fread(&numrigmatrices,sizeof(int),1,f);
			rigmatrices = new transform3[numrigmatrices];
			fread(rigmatrices, sizeof(transform3),numrigmatrices,f);
			attachment = new int[numrigmatrices];
			fread(attachment,sizeof(int),numrigmatrices,f);
			numtextures=0;
			fread(&numtextures,sizeof(int),1,f);
			textures = new int[numtextures];
			fread(textures,sizeof(int),numtextures,f);
		}
		float* vertices;
		float* normals;
		float* mapping;
		float* vcolor;
		float* bones;
		float* weights;
		int*   indices;
		int*   textures;
		int numtextures;
		int numverts;
		int numfaces;
		transform3* rigmatrices;
		int* attachment;
		int numrigmatrices;
	};

	class fmesh
	{
	public:
		inline fmesh(){chunks=0;}
		inline ~fmesh()
		{
			if(chunks){delete [] chunks;}
		}
		inline void skip(FILE* f)
		{
			numchunks=0;
			fread(&numchunks,sizeof(int),1,f);
			for(int i=0;i<numchunks;i++)
			{
				chunks[i].skip(f);
			}
		}
		inline void read(FILE* f)
		{
			numchunks=0;
			fread(&numchunks,sizeof(int),1,f);
			chunks = new fchunk[numchunks];
			for(int i=0;i<numchunks;i++)
			{
				chunks[i].read(f);
			}
		}
		fchunk* chunks;
		int numchunks;
	};

	class fskeleton
	{
	public:
		inline fskeleton()
		{
			transforms=0;
			parents=0;
			names=0;	
		}
		inline ~fskeleton()
		{
			if(transforms){delete [] transforms;}
			if(parents){delete [] parents;}
			if(names){delete [] names;}
		}
		inline void skip(FILE* f)
		{
			numbones  = 0;
			fread(&numbones,sizeof(int),1,f);
			int size = ( sizeof(transform3) + sizeof(int) ) * numbones;
			fseek(f, size, SEEK_CUR);
			for(int i=0;i<numbones;i++)
			{
				names[i].skip(f);
			}
		}
		inline void read(FILE* f)
		{
			numbones  = 0;
			fread(&numbones,sizeof(int),1,f);
			transforms = new transform3[numbones];
			parents = new int[numbones];
			fread(transforms,sizeof(transform3),numbones,f);
			fread(parents,sizeof(int),numbones,f);
			names=new fname[numbones];
			for(int i=0;i<numbones;i++)
			{
				names[i].read(f);
			}
		}
		int numbones;
		transform3* transforms;
		int*   parents;
		fname* names;
	};

	class fmixer
	{
	public:
		inline fmixer()
		{
			mapping=0;
			poses=0;
			weights=0;
			names=0;	
		}
		inline ~fmixer()
		{
			if(mapping){delete [] mapping;}
			if(poses){delete [] poses;}
			if(weights){delete [] weights;}
			if(names){delete [] names;}
		}
		inline void skip(FILE* f)
		{
			numposes  = 0;
			fread(&numposes,sizeof(int),1,f);
			numbones  = 0;
			fread(&numbones,sizeof(int),1,f);
			int size = (sizeof(int)*numbones) +  (sizeof(qtransform)*numposes*numbones);
			fseek(f, size, SEEK_CUR);
			numnames  = 0;
			fread(&numnames,sizeof(int),1,f);
			for(int i=0;i<numnames;i++)
			{
				names[i].skip(f);
			}
			name.skip(f);
		}
		inline void read(FILE* f)
		{
			numposes  = 0;
			fread(&numposes,sizeof(int),1,f);
			numbones  = 0;
			fread(&numbones,sizeof(int),1,f);
			mapping = new int[numbones];
			fread(mapping, sizeof(int),numbones,f);
			poses = new qtransform[numposes*numbones];
			fread(poses, sizeof(qtransform),numposes*numbones,f);
			weights = new float[numposes];
			for(int i=0;i<numposes;i++){weights[i]=0.0f;}
			numnames  = 0;
			fread(&numnames,sizeof(int),1,f);
			names=new fname[numnames];
			for(int i=0;i<numnames;i++)
			{
				names[i].read(f);
			}
			name.read(f);
		}
		int numposes;
		int numbones;
		int numnames;
		int* mapping;
		qtransform *poses;
		float* weights;
		fname* names;
		fname name;
	};

	class cface
	{
	public:
		inline cface()
		{
			textures=0;
			mixers=0;
			poses=0;
			numposes=0;
			filters=0;
			numfilters=0;
		}
		inline ~cface()
		{
			if(textures){delete [] textures;}
			if(mixers){delete [] mixers;}
			if(poses){delete [] poses;}
			if(filters){delete [] filters;}
		}
		inline void read(FILE* f)
		{

				skeleton.read(f);
				mesh.read(f);
				numtextures=0;
				fread(&numtextures,sizeof(int),1,f);
				textures = new ftexture[numtextures];
				for(int i=0;i<numtextures;i++)
				{
					textures[i].read(f);
				}
				nummixers=0;
				fread(&nummixers,sizeof(int),1,f);
				mixers = new fmixer[nummixers];
				for(int i=0;i<nummixers;i++)
				{
					mixers[i].read(f);
				}
				fread(&inputs,sizeof(float), 4*NUMAREAS,f);
				fread(&outputs,sizeof(float),4*NUMAREAS,f);
				fread(&areasw,sizeof(float),   NUMAREAS,f);
				fread(&numposes,sizeof(int),1,f);
				poses = new fpose[numposes];
				for(int i=0;i<numposes;i++)
				{
					poses[i].read(f);
				}
				fread(&numfilters,sizeof(int),1,f);
				filters = new fpose[numfilters];
				for(int i=0;i<numfilters;i++)
				{
					filters[i].read(f);
				}
		}

		fmesh mesh;
		fskeleton skeleton;
		fmixer* mixers;
		int numtextures;
		int nummixers;
		fpose* poses;
		int numposes;
		fpose* filters;
		int numfilters;
		ftexture* textures;
		float inputs[NUMAREAS][4];
		float outputs[NUMAREAS][4];
		float areasw[NUMAREAS];
	};

};

//////////////////////////////////////////////////////////////////////////

namespace FacFormatImporter
{
	Bool ImportTrackPoses( const fac::cface& loadedData, TDynArray< SMimicTrackPose >& poses );
	Bool ImportFilterPoses( const fac::cface& loadedData, TDynArray< SMimicTrackPose >& poses );

	
	Bool InternalImportPoses( const fac::fpose* inputPoses, Int32 inputPosesNum, TDynArray< SMimicTrackPose >& poses, Float defaultValue, const String& namePattern = String::EMPTY );


};
