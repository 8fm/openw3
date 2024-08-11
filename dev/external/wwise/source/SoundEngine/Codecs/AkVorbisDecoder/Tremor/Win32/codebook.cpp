/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: basic codebook pack/unpack/code/decode operations

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codebook.h"
#include "misc.h"
#include "os.h"

#define CODEBOOK_DIM_MAX 8

void decode_map(codebook *s, oggpack_buffer *b, ogg_int32_t *v, int point);

/**** pack/unpack helpers ******************************************/
static int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static ogg_uint32_t decpack(long entry,long used_entry,long quantvals,
			    codebook *b,oggpack_buffer *opb,int maptype){
  ogg_uint32_t ret=0;
  int j;
  
  switch(b->dec_type){

  case 0:
    return (ogg_uint32_t)entry;

  case 1:
    if(maptype==1){
      /* vals are already read into temporary column vector here */
      for(j=0;j<b->dim;j++){
	ogg_uint32_t off=entry%quantvals;
	entry/=quantvals;
	ret|=((ogg_uint16_t *)(b->q_val))[off]<<(b->q_bits*j);
      }
    }else{
      for(j=0;j<b->dim;j++)
	ret|=oggpack_read(opb,b->q_bits)<<(b->q_bits*j);
    }
    return ret;
    
  case 2:
    for(j=0;j<b->dim;j++){
      ogg_uint32_t off=entry%quantvals;
      entry/=quantvals;
      ret|=off<<(b->q_pack*j);
    }
    return ret;

  case 3:
    return (ogg_uint32_t)used_entry;

  }
  return 0; /* silence compiler */
}

/* 32 bit float (not IEEE; nonnormalized mantissa +
   biased exponent) : neeeeeee eeemmmmm mmmmmmmm mmmmmmmm 
   Why not IEEE?  It's just not that important here. */

static ogg_int32_t _float32_unpack(long val,int *point){
  long   mant=val&0x1fffff;
  int    sign=val&0x80000000;
  
  *point=((val&0x7fe00000L)>>21)-788;

  if(mant){
    while(!(mant&0x40000000)){
      mant<<=1;
      *point-=1;
    }
    if(sign)mant= -mant;
  }else{
    *point=-9999;
  }
  return mant;
}

/* choose the smallest supported node size that fits our decode table.
   Legal bytewidths are 1/1 1/2 2/2 2/4 4/4 */
static int _determine_node_bytes(long used, int leafwidth){

  /* special case small books to size 4 to avoid multiple special
     cases in repack */
  if(used<2)
    return 4;

  if(leafwidth==3)leafwidth=4;
  if(_ilog(3*used-6)+1 <= leafwidth*4) 
    return leafwidth/2?leafwidth/2:1;
  return leafwidth;
}

/* convenience/clarity; leaves are specified as multiple of node word
   size (1 or 2) */
static int _determine_leaf_words(int nodeb, int leafwidth){
  if(leafwidth>nodeb)return 2;
  return 1;
}

/* given a list of word lengths, number of used entries, and byte
   width of a leaf, generate the decode table */
static int _make_words(char *l,long n,ogg_uint32_t *r,long quantvals,
		       codebook *b, oggpack_buffer *opb,int maptype){
  long i,j,count=0;
  long top=0;

  if(n<2){
    r[0]=0x80000000;
  }else{
	ogg_uint32_t marker[33] = { 0 };

    for(i=0;i<n;i++){
      long length=l[i];
      if(length){
	ogg_uint32_t entry=marker[length];
	long chase=0;
	if(count && !entry)return -1; /* overpopulated tree! */
	
	/* chase the tree as far as it's already populated, fill in past */
	for(j=0;j<length-1;j++){
	  int bit=(entry>>(length-j-1))&1;
	  if(chase>=top){ 
	    top++;
	    r[chase*2]=top;
	    r[chase*2+1]=0;
	  }else
	    if(!r[chase*2+bit])
	      r[chase*2+bit]=top;
	  chase=r[chase*2+bit];
	}
	{	
	  int bit=(entry>>(length-j-1))&1;
	  if(chase>=top){ 
	    top++;
	    r[chase*2+1]=0;
	  }
	  r[chase*2+bit]= decpack(i,count++,quantvals,b,opb,maptype) | 
	    0x80000000;
	}

	/* Look to see if the next shorter marker points to the node
	   above. if so, update it and repeat.  */
	for(j=length;j>0;j--){          
	  if(marker[j]&1){
	    marker[j]=marker[j-1]<<1;
	    break;
	  }
	  marker[j]++;
	}
	
	/* prune the tree; the implicit invariant says all the longer
	   markers were dangling from our just-taken node.  Dangle them
	   from our *new* node. */
/*
	for(j=length+1;j<33;j++)
	  if((marker[j]>>1) == entry){
	    entry=marker[j];
	    marker[j]=marker[j-1]<<1;
	  }else
	    break;
*/
	{	// Reformulation of the above code to eliminate a crippling LHS.
		ogg_uint32_t entryLast = marker[length];
		for(j=length+1;j<33;j++)
		{
			if((marker[j]>>1) == entry)
			{
				entry=marker[j];
				entryLast<<=1;
				marker[j]=entryLast;
			}
			else
				break;
		}
	}

      }
    }
  }
  
  return 0;
}

static void _make_decode_table(	codebook*			s,
								char*				lengthlist,
								long				quantvals,
								oggpack_buffer*		opb,
								int					maptype,
								CAkVorbisAllocator&	VorbisAllocator)
{
	int i;
	ogg_uint32_t *work;

	if(s->dec_nodeb==4)
	{
		int AllocSize = (s->used_entries*2+1)*sizeof(*work);
		s->dec_table = VorbisAllocator.Alloc(AllocSize);
		/* +1 (rather than -2) is to accommodate 0 and 1 sized books,
		which are specialcased to nodeb==4 */
		_make_words(lengthlist,s->entries,(ogg_uint32_t*)s->dec_table,quantvals,s,opb,maptype);
	}

	work=(ogg_uint32_t*)alloca((s->used_entries*2-2)*sizeof(*work));

	_make_words(lengthlist,s->entries,work,quantvals,s,opb,maptype);

	int AllocSize = (s->used_entries*(s->dec_leafw+1)-2)*s->dec_nodeb;
	s->dec_table = VorbisAllocator.Alloc(AllocSize);
	if(s->dec_leafw==1)
	{
		switch(s->dec_nodeb)
		{
		case 1:
			for(i=0;i<s->used_entries*2-2;i++)
			{
				((unsigned char *)s->dec_table)[i]=((work[i] & 0x80000000UL) >> 24) | work[i];
			}
			break;
		case 2:
			for(i=0;i<s->used_entries*2-2;i++)
			{
				((ogg_uint16_t *)s->dec_table)[i]=((work[i] & 0x80000000UL) >> 16) | work[i];
			}
			break;
		}
	}
	else
	{
		/* more complex; we have to do a two-pass repack that updates the
		node indexing. */
		long top = s->used_entries*3-2;
		if(s->dec_nodeb==1)
		{
			unsigned char *out=(unsigned char *)s->dec_table;

			for(i=s->used_entries*2-4;i>=0;i-=2)
			{
				if(work[i]&0x80000000UL)
				{
					if(work[i+1]&0x80000000UL)
					{
						top-=4;
						out[top]=(work[i]>>8 & 0x7f)|0x80;
						out[top+1]=(work[i+1]>>8 & 0x7f)|0x80;
						out[top+2]=work[i] & 0xff;
						out[top+3]=work[i+1] & 0xff;
					}
					else
					{
						top-=3;
						out[top]=(work[i]>>8 & 0x7f)|0x80;
						out[top+1]=work[work[i+1]*2];
						out[top+2]=work[i] & 0xff;
					}
				}
				else
				{
					if(work[i+1]&0x80000000UL)
					{
						top-=3;
						out[top]=work[work[i]*2];
						out[top+1]=(work[i+1]>>8 & 0x7f)|0x80;
						out[top+2]=work[i+1] & 0xff;
					}
					else
					{
						top-=2;
						out[top]=work[work[i]*2];
						out[top+1]=work[work[i+1]*2];
					}
				}
				work[i]=top;
			}
		}
		else
		{
			ogg_uint16_t *out=(ogg_uint16_t *)s->dec_table;
			for(i=s->used_entries*2-4;i>=0;i-=2)
			{
				if(work[i]&0x80000000UL)
				{
					if(work[i+1]&0x80000000UL)
					{
						top-=4;
						out[top]=(work[i]>>16 & 0x7fff)|0x8000;
						out[top+1]=(work[i+1]>>16 & 0x7fff)|0x8000;
						out[top+2]=work[i] & 0xffff;
						out[top+3]=work[i+1] & 0xffff;
					}
					else
					{
						top-=3;
						out[top]=(work[i]>>16 & 0x7fff)|0x8000;
						out[top+1]=work[work[i+1]*2];
						out[top+2]=work[i] & 0xffff;
					}
				}
				else
				{
					if(work[i+1]&0x80000000UL)
					{
						top-=3;
						out[top]=work[work[i]*2];
						out[top+1]=(work[i+1]>>16 & 0x7fff)|0x8000;
						out[top+2]=work[i+1] & 0xffff;
					}
					else
					{
						top-=2;
						out[top]=work[work[i]*2];
						out[top+1]=work[work[i+1]*2];
					}
				}
				work[i]=top;
			}
		}
	}
}

/* most of the time, entries%dimensions == 0, but we need to be
   well defined.  We define that the possible vales at each
   scalar is values == entries/dim.  If entries%dim != 0, we'll
   have 'too few' values (values*dim<entries), which means that
   we'll have 'left over' entries; left over entries use zeroed
   values (and are wasted).  So don't generate codebooks like
   that */
/* there might be a straightforward one-line way to do the below
   that's portable and totally safe against roundoff, but I haven't
   thought of it.  Therefore, we opt on the side of caution */
static long _book_maptype1_quantvals(codebook *b){
  /* get us a starting hint, we'll polish it below */
  int bits=_ilog(b->entries);
  int vals=b->entries>>((bits-1)*(b->dim-1)/b->dim);

  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

void vorbis_book_unpack(oggpack_buffer *opb,codebook *s, CAkVorbisAllocator& VorbisAllocator)
{
	int		quantvals=0;
	long	i,j;
	int     maptype;
	int		ak_lenbits;
	memset(s,0,sizeof(*s));

	/* first the basic parameters */
	s->dim = oggpack_read(opb,4);
	s->entries = oggpack_read(opb,14);

	char * lengthlist = (char *)alloca(sizeof(*lengthlist)*s->entries);

	/* codeword ordering.... length ordered or unordered? */
	if ( oggpack_read(opb,1) == 0 )
	{
		/* unordered */

		ak_lenbits = oggpack_read(opb,3);

		/* allocated but unused entries? */
		if(oggpack_read(opb,1))
		{
			/* yes, unused entries */
			for(i=0;i<s->entries;i++)
			{
				if(oggpack_read(opb,1))
				{
					long num=oggpack_read(opb,ak_lenbits);
					lengthlist[i]=num+1;
					s->used_entries++;
					if(num+1>s->dec_maxlength)
					{
						s->dec_maxlength=num+1;
					}
				}
				else
				{
					lengthlist[i]=0;
				}
			}
		}
		else
		{
			/* all entries used; no tagging */
			s->used_entries=s->entries;
			for(i=0;i<s->entries;i++)
			{
				long num=oggpack_read(opb,ak_lenbits);
				lengthlist[i]=num+1;
				if(num+1>s->dec_maxlength)
				{
					s->dec_maxlength=num+1;
				}
			}
		}
	}
	else // 1
	{
		/* ordered */
		long length=oggpack_read(opb,5)+1;

		s->used_entries=s->entries;

		for(i=0;i<s->entries;)
		{
			long num = oggpack_read(opb,_ilog(s->entries-i));
			for(j=0;j<num && i<s->entries;j++,i++)
			{
				lengthlist[i]=length;
			}
			s->dec_maxlength=length;
			length++;
		}
	}

	/* Do we have a mapping to unpack? */
	maptype = oggpack_read(opb,1);
	if ( maptype == 0)
	{
		/* no mapping; decode type 0 */

		/* how many bytes for the indexing? */
		/* this is the correct boundary here; we lose one bit to
		node/leaf mark */
		s->dec_nodeb	= _determine_node_bytes(s->used_entries,_ilog(s->entries)/8+1); 
		s->dec_leafw	= _determine_leaf_words(s->dec_nodeb,_ilog(s->entries)/8+1); 
		s->dec_type		= 0;

		_make_decode_table(s,lengthlist,quantvals,opb,maptype,VorbisAllocator);
	}
	else // maptype == 1
	{
		s->q_min	= _float32_unpack(oggpack_read(opb,32),&s->q_minp);
		s->q_del	= _float32_unpack(oggpack_read(opb,32),&s->q_delp);
		s->q_bits	= oggpack_read(opb,4)+1;
		/*s->q_seq	= */oggpack_read(opb,1);

		s->q_del >>= s->q_bits;
		s->q_delp += s->q_bits;

		/* mapping type 1; implicit values by lattice  position */
		quantvals=_book_maptype1_quantvals(s);

		/* dec_type choices here are 1,2; 3 doesn't make sense */
		{
			/* packed values */
			long total1=(s->q_bits*s->dim+8)/8; /* remember flag bit */
			/* vector of column offsets; remember flag bit */
			long total2=(_ilog(quantvals-1)*s->dim+8)/8+(s->q_bits+7)/8;

			if(total1<=4 && total1<=total2)
			{
				/* use dec_type 1: vector of packed values */

				/* need quantized values before  */
				s->q_val=alloca(sizeof(ogg_uint16_t)*quantvals);

				for(i=0;i<quantvals;i++)
					((ogg_uint16_t *)s->q_val)[i]=oggpack_read(opb,s->q_bits);

				s->dec_type		= 1;
				s->dec_nodeb	= _determine_node_bytes(s->used_entries,(s->q_bits*s->dim+8)/8); 
				s->dec_leafw	= _determine_leaf_words(s->dec_nodeb,(s->q_bits*s->dim+8)/8); 

				_make_decode_table(s,lengthlist,quantvals,opb,maptype,VorbisAllocator);
				s->q_val=0; /* about to go out of scope; _make_decode_table
							was using it */

			}
			else
			{
				/* use dec_type 2: packed vector of column offsets */

				/* need quantized values before */
				if(s->q_bits<=8)
				{
					s->q_val = VorbisAllocator.Alloc(quantvals);

					for(i = 0 ; i < quantvals ; i++)
					{
						((unsigned char *)s->q_val)[i]=oggpack_read(opb,s->q_bits);
					}
				}
				else
				{
					int AllocSize = quantvals*2;
					s->q_val = VorbisAllocator.Alloc(AllocSize);

					for(i = 0 ; i < quantvals ; i++)
					{
						((ogg_uint16_t *)s->q_val)[i]=oggpack_read(opb,s->q_bits);
					}
				}

				s->q_pack		= _ilog(quantvals-1); 
				s->dec_type		= 2;
				s->dec_nodeb	= _determine_node_bytes(s->used_entries,(_ilog(quantvals-1)*s->dim+8)/8); 
				s->dec_leafw	= _determine_leaf_words(s->dec_nodeb,(_ilog(quantvals-1)*s->dim+8)/8); 
				_make_decode_table(s,lengthlist,quantvals,opb,maptype,VorbisAllocator);
			}
		}
	}
}

static inline ogg_uint32_t decode_packed_entry_number(codebook *book, oggpack_buffer *b)
{
	ogg_uint32_t	chase	= 0;
	int				read	= book->dec_maxlength;
	long			lok		= oggpack_look(b,read),i;

	int bit = lok & 1;

	/* chase the tree with the bits we got */
	if(book->dec_nodeb==1)
	{
		if(book->dec_leafw==1)
		{
			/* 8/8 */
			unsigned char *t=(unsigned char *)book->dec_table;
			for(i=0;i<read;i++)
			{
				chase = t[chase * 2 + bit];

				if(!(chase&0x80UL))
				{
					lok >>= 1;
					bit = lok & 1;
				}
				else
				{
					break;
				}
			}
			chase&=0x7fUL;
		}
		else
		{
			/* 8/16 */
			unsigned char *t=(unsigned char *)book->dec_table;
			for(i=0;i<read;i++)
			{
				int next = t[chase+bit];
				if(!(next&0x80))
				{
					lok >>= 1;
					bit = lok & 1;
					chase=next;
				}
				else
				{
					chase= (next<<8) | t[chase+bit+1+(!bit || t[chase]&0x80)];
					break;
				}
			}
			chase&=0x7fffUL;
		}
	}
	else
	{
		if(book->dec_nodeb==2)
		{
			if(book->dec_leafw==1)
			{
				ogg_uint16_t *t = (ogg_uint16_t*)book->dec_table;
				/* 16/16 */
				for(i=0;i<read;i++)
				{
					chase = t[chase * 2 + bit];
					if(!(chase&0x8000UL))
					{
						lok >>= 1;
						bit = lok & 1;
					}
					else
					{
						break;
					}
				}
				chase&=0x7fffUL;
			}
			else
			{
				/* 16/32 */
				ogg_uint16_t *t=(ogg_uint16_t *)book->dec_table;
				for(i=0;i<read;i++)
				{
					int next=t[chase+bit];
					if(!(next&0x8000))
					{
						lok >>= 1;
						bit = lok & 1;
						chase=next;
					}
					else
					{
						chase= (next<<16) | t[chase+bit+1+(!bit || t[chase]&0x8000)];
						break;
					}
				}
				chase&=0x7fffffffUL;
			}
		}
		else
		{
			ogg_uint32_t *t = (ogg_uint32_t *)book->dec_table;
			for(i=0;i<read;i++)
			{
				chase = t[chase * 2 + bit];
				if(!(chase&0x80000000UL))
				{
					lok >>= 1;
					bit = lok & 1;
				}
				else
				{
					break;
				}
			}
			chase&=0x7fffffffUL;
		}
	}

	oggpack_adv(b,i+1);
	return chase;
}

/* returns the [original, not compacted] entry number or -1 on eof *********/
long ak_vorbis_book_decode(codebook *book, oggpack_buffer *b)
{
	if(book->dec_type)
	{
		return -1;
	}
	return decode_packed_entry_number(book,b);
}

void decode_map(codebook *s, oggpack_buffer *b, ogg_int32_t *v, int point)
{
	ogg_uint32_t entry = decode_packed_entry_number(s,b);

	int dim = s->dim;

	/* according to decode type */
	if ( s->dec_type == 1 )
	{
		/* packed vector of values */
		int mask = (1 << s->q_bits) - 1;

		int i = 0, q_bits = s->q_bits;
		do
		{
			v[i++] = entry & mask;
			entry >>= q_bits;
		}
		while ( i < dim );
	}
	else
	{
		AKASSERT( s->dec_type == 2 );

		/* packed vector of column offsets */
		int mask=(1<<s->q_pack)-1;

		int i = 0, q_pack = s->q_pack;
		void * q_val = s->q_val;

		if(s->q_bits<=8)
		{
			do
			{
				v[i++]=((unsigned char *)(q_val))[entry&mask];
				entry>>=q_pack;
			}
			while ( i < dim );
		}
		else
		{
			do
			{
				v[i++]=((ogg_uint16_t *)(q_val))[entry&mask];
				entry>>=q_pack;
			}
			while ( i < dim );
		}
	}

	/* we have the unpacked multiplicands; compute final vals */
	{
		int			shiftM	= point-s->q_delp;
		ogg_int32_t	add		= point-s->q_minp;
		if(add>0)
		{
			add = s->q_min >> add;
		}
		else
		{
			add = s->q_min << -add;
		}

		int i = 0, q_del = s->q_del;

		if(shiftM>0)
		{
			do
			{
				v[i]= add + ((v[i] * q_del) >> shiftM);
			}
			while ( ++i < dim );
		}
		else
		{
			do
			{
				v[i]= add + ((v[i] * q_del) << -shiftM);
			}
			while ( ++i < dim );
		}
	}
}

void vorbis_book_decodev_add(	codebook*		book,
								ogg_int32_t*	a,
								oggpack_buffer*	b,
								int				n,
								int				point)
{
	ogg_int32_t v[ CODEBOOK_DIM_MAX ];

	int i;
  
	for(i=0;i<n;)
	{
		decode_map(book,b,v,point);

		int j = 0, dim = book->dim;
		do
		{
			a[i++]+=v[j++];
		}
		while(j<dim);
	}
}

void vorbis_book_decodevv_add_2ch(	codebook*		book,
								ogg_int32_t**	a,
								long			offset,
								oggpack_buffer*	b,
								int				n,
								int				point)
{
	ogg_int32_t v[ CODEBOOK_DIM_MAX ];

	int chptr=0;

	for(int i=offset;i<offset+n;)
	{
		decode_map(book,b,v,point);
		int j = 0, dim = book->dim;
		do
		{
			a[chptr][i]+=v[j++];
			i += chptr & 1;
			chptr = ( chptr + 1 ) & 1;
		}
		while(j<dim);
	}
}
/* Enable this for > 2ch coupling
void vorbis_book_decodevv_add(
	codebook *book,
	ogg_int32_t **a,
	long offset,
	int ch,
	oggpack_buffer *b,
	int n,
	int point )
{
	ogg_int32_t v[ CODEBOOK_DIM_MAX ];
	int chptr=0;

	for(int i=offset;i<offset+n;)
	{
		decode_map(book,b,v,point);
		int j = 0, dim = book->dim;
		do
		{
			a[chptr++][i]+=v[j++];
			if(chptr==ch)
			{
				chptr=0;
				i++;
			}
		}
		while(j<dim);
	}
}
*/