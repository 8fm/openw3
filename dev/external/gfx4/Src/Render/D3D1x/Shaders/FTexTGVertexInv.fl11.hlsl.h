#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex                       sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// COLOR                    1   xyzw        1     NONE   float   x   
// TEXCOORD                 0   xy          2     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[58]
// Embedded Data:
//  0x0000003a - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000035 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.w
dcl_input_ps linear v1.x
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v2.xyxx, t0.wxyz, s0
add r0.x, r0.x, -v0.w
mad o0.xyzw, v1.xxxx, r0.xxxx, v0.wwww
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[58], bundle is:[142] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGVertexInv.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  1, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 100;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 6;
NumSgprs             = 14;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000001
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b64     s[12:13], exec                          // 000000000000: BE8C047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx4  s[0:3], s[0:1], 0x08                  // 000000000010: C0800108
  v_interp_p1_f32  v2, v0, attr2.x                      // 000000000014: C8080800
  v_interp_p1_f32  v3, v0, attr2.y                      // 000000000018: C80C0900
  v_interp_p2_f32  v2, v1, attr2.x                      // 00000000001C: C8090801
  v_interp_p2_f32  v3, v1, attr2.y                      // 000000000020: C80D0901
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3] dmask:0x8 // 000000000028: F0800800 00010202
  v_interp_p1_f32  v3, v0, attr0.w                      // 000000000030: C80C0300
  v_interp_p1_f32  v0, v0, attr1.x                      // 000000000034: C8000400
  v_interp_p2_f32  v3, v1, attr0.w                      // 000000000038: C80D0301
  v_interp_p2_f32  v0, v1, attr1.x                      // 00000000003C: C8010401
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000040: BF8C0F70
  v_sub_f32     v2, v2, v3                              // 000000000044: 08040702
  v_mac_legacy_f32  v3, v0, v2                          // 000000000048: 0C060500
  s_mov_b64     exec, s[12:13]                          // 00000000004C: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v3, v3                       // 000000000050: 5E000703
  s_nop         0x0000                                  // 000000000054: BF800000
  exp           mrt0, v0, v0, v0, v0 compr vm           // 000000000058: F800140F 00000000
  s_endpgm                                              // 000000000060: BF810000
end


// Approximately 4 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGVertexInv[] =
{
     68,  88,  66,  67, 165,  22, 
    245, 168,  63, 101,  18,  35, 
     54, 243, 163, 142,  59,  59, 
    179,  40,   1,   0,   0,   0, 
    108,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 108,   1, 
      0,   0, 160,   1,   0,   0, 
    208,   4,   0,   0,  82,  68, 
     69,  70, 200,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    140,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    124,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 136,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,   0, 116, 101, 
    120,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  70, 111, 114,  32,  68, 
    117, 114,  97, 110, 103, 111, 
     32,  57,  46,  51,  48,  46, 
     49,  50,  48,  57,  56,  46, 
     48,   0,  73,  83,  71,  78, 
     96,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   1,   0,   0, 
     86,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171,  83,  72,  69,  88, 
     40,   3,   0,   0,  80,   0, 
      0,   0, 202,   0,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0,  58,   0,   0,   0, 
     80,   0,   0,   0,  53,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3,  18,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  54, 121, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,  32,  16,   0, 
      0,   0,   0,   0,   6,  16, 
     16,   0,   1,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    144,   0,   0,   0,  21,   0, 
      1,   0,  41,  42,  10,   0, 
    120,   1, 237,  86,  49, 111, 
    211,  64,  24, 253, 206, 118, 
     19, 167, 184,  13, 160,  10, 
    129, 196,   0,   2,  68, 213, 
     33,  82,  32,  19,  83, 135, 
     42,  21,  18,  83,  27, 117, 
     64,   7, 194,  37,  23, 138, 
    112, 147, 202, 177,  42, 139, 
     33, 132, 173,  67,   6, 102, 
     36, 254,   4,  63,  32, 137, 
     58, 153, 157,  17, 137, 137, 
    141, 129,  13,   6, 218, 242, 
     61, 251,  14,  76,  68, 183, 
     72,  32, 145,  39, 189, 239, 
    249, 221, 249, 190, 124, 241, 
    157, 238, 110, 101, 158,  82, 
     92,  61, 255, 102,   3, 186, 
     85, 251, 250, 238,  61, 235, 
     28,  83, 104,   5,  98,  23, 
    145, 200,  43,  33,  18,  45, 
     35, 224,   5, 139, 168, 192, 
    178, 200, 108,  50,  87, 153, 
    232,  51, 204, 176,  58, 238, 
    112,  60, 203,  68, 190,  67, 
     30, 183, 206, 186, 196, 228, 
    225, 116, 134, 137, 180,  72, 
      7,  24,   5, 108,   4, 134, 
    131,  48,   1, 211,   7, 160, 
     60, 228,   2, 160, 249,  28, 
    116,   3, 225, 207, 184, 192, 
     68, 253, 167,   1, 181, 153, 
     92,  80, 253, 247, 211, 255, 
    251, 183, 145, 213, 149, 143, 
    191, 230,   9, 192, 124, 228, 
    129, 154, 205, 123,  31, 244, 
    123, 230, 155, 125, 214, 222, 
    204, 229, 152, 121,  26,  78, 
     24,  30,  30, 114,  48,  30, 
    115,  82,  72, 103, 131,  85, 
    156, 163,  43,  80, 235,  90, 
    186,  46, 254,  23, 152, 181, 
    138, 111, 129, 239, 139, 117, 
     95, 102,  26, 160,  31, 107, 
    159, 168,  63,   2,  95,  93, 
    164, 111, 112, 198,  11, 246, 
     24, 107, 188,  13, 159,  54, 
    252, 238, 237,  34,  61, 132, 
     47,  95, 206, 198, 179, 151, 
     19, 254, 254, 132, 127,  52, 
    225, 183, 243,  30, 207, 169, 
     79, 243, 219,  54, 230, 153, 
     27, 123, 255,  50, 103,  53, 
     78, 135, 179,  26, 167, 195, 
     89, 141,  83,  99, 138, 158, 
     51,  24, 246, 230, 143, 135, 
    150, 253, 125,  72, 226, 112, 
    236, 138, 254, 152,  92,  55, 
    161, 146, 151,   8, 183, 148, 
    136, 210,  66, 242, 130,   6, 
     35, 114, 251,  95,  44, 139, 
     79,  55, 219,  75, 120, 131, 
     77, 132, 189, 144,   8,  71, 
     36, 123, 229, 193, 200,  42, 
     58,  46, 205,  21,  60, 207, 
     57,  30, 242, 158, 151, 237, 
    153,  75, 102, 207, 123,  57, 
    186, 196, 209, 156, 139, 230, 
     60,  52, 224, 147,  76, 152, 
     62, 156, 106, 188, 119, 139, 
    154, 190,   4,  92, 215, 138, 
    253,  29, 123, 250, 226, 207, 
     44,  25,  26, 186, 255, 158, 
     86, 140, 199, 125, 226, 109, 
    221, 105, 222,  92, 142,  14, 
    246, 116, 123, 160,  21,  57, 
    112,  29, 251,  88, 127,  29, 
    125, 210, 109,   7,  90,  31, 
     48,  49, 190, 117,  71, 110, 
    251, 129, 255, 188,  91, 137, 
     58,  97, 231,  89, 165,  27, 
    133, 202, 223, 189,  37, 155, 
    106,  95, 170,  56,  82,  97, 
    219,  15, 228, 147,  86,  92, 
    147, 155, 225,  99, 185, 161, 
    218,  77,  21, 202, 181, 219, 
    107, 213,  88, 110, 238, 248, 
    108, 186, 178, 222,  80, 113, 
     99, 125,  75, 133, 145, 138, 
    239, 182, 247,  43, 173, 160, 
     90, 173, 236,   4, 221, 128, 
    211, 103,  56, 210,  63, 138, 
    203,  90, 145, 185, 235,  63, 
    109, 211,  15,  92, 140,  33, 
     89,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   4,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
