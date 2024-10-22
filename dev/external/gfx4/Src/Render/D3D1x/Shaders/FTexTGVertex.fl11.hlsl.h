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
// COLOR                    0   xyzw        0     NONE   float   xyzw
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
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.x
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v2.xyxx, t0.xyzw, s0
add r0.xyzw, r0.xyzw, -v0.xyzw
mad o0.xyzw, v1.xxxx, r0.xyzw, v0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[58], bundle is:[156] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGVertex.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  1, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 148;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 10;
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
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3] dmask:0xf // 000000000028: F0800F00 00010202
  v_interp_p1_f32  v6, v0, attr0.x                      // 000000000030: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 000000000034: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 000000000038: C8200200
  v_interp_p1_f32  v9, v0, attr0.w                      // 00000000003C: C8240300
  v_interp_p1_f32  v0, v0, attr1.x                      // 000000000040: C8000400
  v_interp_p2_f32  v6, v1, attr0.x                      // 000000000044: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 000000000048: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 00000000004C: C8210201
  v_interp_p2_f32  v9, v1, attr0.w                      // 000000000050: C8250301
  v_interp_p2_f32  v0, v1, attr1.x                      // 000000000054: C8010401
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000058: BF8C0F70
  v_sub_f32     v2, v2, v6                              // 00000000005C: 08040D02
  v_sub_f32     v3, v3, v7                              // 000000000060: 08060F03
  v_sub_f32     v4, v4, v8                              // 000000000064: 08081104
  v_sub_f32     v5, v5, v9                              // 000000000068: 080A1305
  v_mac_legacy_f32  v6, v0, v2                          // 00000000006C: 0C0C0500
  v_mac_legacy_f32  v7, v0, v3                          // 000000000070: 0C0E0700
  v_mac_legacy_f32  v8, v0, v4                          // 000000000074: 0C100900
  v_mac_legacy_f32  v9, v0, v5                          // 000000000078: 0C120B00
  s_mov_b64     exec, s[12:13]                          // 00000000007C: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v6, v7                       // 000000000080: 5E000F06
  v_cvt_pkrtz_f16_f32  v1, v8, v9                       // 000000000084: 5E021308
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000088: F800140F 00000100
  s_endpgm                                              // 000000000090: BF810000
end


// Approximately 4 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGVertex[] =
{
     68,  88,  66,  67, 213,  98, 
    104, 198,  22, 120,  20,  99, 
      0,  72, 209,  63,  13,  81, 
     68, 143,   1,   0,   0,   0, 
    164,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 108,   1, 
      0,   0, 160,   1,   0,   0, 
      8,   5,   0,   0,  82,  68, 
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
      0,   0,  15,  15,   0,   0, 
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
     96,   3,   0,   0,  80,   0, 
      0,   0, 216,   0,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0,  58,   0,   0,   0, 
     80,   0,   0,   0,  53,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
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
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,  32,  16,   0, 
      0,   0,   0,   0,   6,  16, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    158,   0,   0,   0,  21,   0, 
      1,   0,  41,  87,  10,   0, 
    120,   1, 237,  86, 207, 107, 
     19,  65,  20, 126, 179, 217, 
    164, 187, 233, 166, 219,  31, 
     30,  82, 136, 208,  82,   3, 
    226,  33,  16, 245,  36, 130, 
     61,  20, 197, 171,  13,  10, 
     97,  41,  77, 204, 198,  74, 
    147,  84, 146,  80,  22, 145, 
     52,  21, 241, 148, 131,   7, 
    193, 139,   7, 111, 254,  11, 
    158, 146, 224, 105, 254,   0, 
     79,  94, 252,  19, 244,  82, 
    240,  32, 209, 247, 237, 206, 
    232,  18, 244,  22,  80,  48, 
     31, 124, 239, 205, 183,  51, 
    243, 246, 237, 204,  48, 111, 
    239, 165,  41,  68,  54, 255, 
    233,   3, 252, 219, 231, 159, 
    159, 162, 145, 100,  10, 229, 
    129, 192, 130,  37, 186, 110, 
    195,  18,  93, 132, 193,   0, 
    131,   8,  33, 150, 152,  47, 
    153, 219,  76, 244, 105,  70, 
    216,  30,   7, 108, 151, 153, 
     24, 247, 145, 231, 221, 102, 
    191, 201, 228, 233, 148, 101, 
     34,  44, 194,   1, 218,   3, 
      9,  24, 134,   9,  51,   5, 
    149, 122,   8, 196,  69,  44, 
      0,  62,  30, 131, 242,  48, 
    191, 199,  22,  51,  30, 103, 
     26, 200,  77, 199, 130,  87, 
    159,  31, 190, 239, 111,  35, 
    202,  43, 110, 127, 237,  19, 
    128, 253, 136,   3,  57, 235, 
    113, 142, 250,  16, 189, 102, 
     57, 165, 177,  22,  24,  55, 
    102, 254,   9, 223,  25,  14, 
     26,  49, 104, 141, 253,  74, 
    145, 203, 150, 189,  88, 161, 
     13, 120,  99,  43,  60,  23, 
    255,  11, 244,  89, 197,  90, 
     96, 125, 113, 238, 163,  21, 
    137, 128, 126, 172,  11,  81, 
    127,   4, 190, 200, 210,  87, 
     40, 173,   5, 235,  20, 164, 
    210,   9, 232, 133,  41, 205, 
    251, 149, 114, 105, 207,  90, 
     51, 246, 220,  28, 207, 231, 
    141, 101, 237, 177, 246,  98, 
    186, 204, 186,  28, 211, 251, 
    172, 247,  99, 186, 202, 186, 
    170, 181, 142, 239, 134, 239, 
    179, 108, 236,  51,  63, 236, 
    253, 203, 156, 231,  56,  27, 
    206, 115, 156,  13, 231,  57, 
    206, 140,  33, 122, 230,  96, 
    216,  75,  79, 134,  70, 226, 
    219, 144, 196, 251, 177,  37, 
    250,  99, 178,  44,  73, 182, 
     35, 133, 101,  75,  97, 103, 
    228,   9,  13,  70, 228, 246, 
    191,  24,   6,  46, 177, 172, 
     36, 145, 147, 100, 108,  72, 
     74,  92, 144, 124, 217,  74, 
     65, 235,  82, 136, 243,  82, 
     24, 155,  82,  36, 242,  82, 
    152,  66,  62, 114,   7,  35, 
     35,  99,  90,   9,  55, 101, 
    153,  43, 150, 149,  92,  75, 
     91, 148, 116,  28,  90,  88, 
    114, 200,  94, 118, 104, 113, 
    213, 113, 204, 201, 144, 239, 
    200, 232, 142,  61, 167, 239, 
    200, 211, 209,  58, 219, 176, 
    201, 208, 245,  83, 131, 203, 
    175, 208, 125,  40, 197, 124, 
    215, 139, 154, 170, 173,  37, 
    229,  81,  15,  80,   3, 150, 
    126,  70, 137, 240,  76, 245, 
     63,  81,  30, 243,  23, 153, 
    167, 185, 179, 195,  27,  39, 
    103, 205,  55, 234, 249, 107, 
    229,  17,   3, 245, 250,  85, 
    102, 114, 188, 138,   6, 227, 
    157, 234,  43,  51,  49, 191, 
    126, 205, 171,  86,  26, 149, 
    199, 157,  66, 247, 168, 125, 
    116,  88, 232, 116, 219, 126, 
    165, 121, 217, 171, 249, 199, 
    158,  31, 116, 253, 118, 171, 
    210, 240,  30, 212, 131, 171, 
    222, 110, 251, 190, 119, 199, 
    111, 213, 252, 182, 183, 115, 
    101, 167,  24, 120, 187,   7, 
     21,  22,  29, 239, 102, 201, 
     15,  74, 183, 238, 250, 237, 
    174,  31,  20, 234, 141,  98, 
    177, 112, 208, 232,  52,  56, 
    118, 132,  75, 234, 205, 248, 
     83,  68, 145, 106,  86,  30, 
    182, 232,   7, 255, 156,  36, 
     83,   0,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
      4,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
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
      0,   0,   0,   0
};
