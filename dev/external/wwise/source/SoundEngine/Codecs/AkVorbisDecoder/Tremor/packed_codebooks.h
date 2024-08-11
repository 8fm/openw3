/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#ifndef PACKED_CODEBOOKS_H
#define PACKED_CODEBOOKS_H

#define MAX_PACKED_CODEBOOK_SIZE 874
#define NUM_PACKED_CODEBOOKS 598

#ifndef __SPU__
extern const unsigned char * g_packedCodebooks[ NUM_PACKED_CODEBOOKS + 1 ];
#endif

#endif
