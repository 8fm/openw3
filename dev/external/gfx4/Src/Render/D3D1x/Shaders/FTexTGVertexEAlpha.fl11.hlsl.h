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
// COLOR                    1   xyzw        1     NONE   float   x  w
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
// First Precompiled Shader at offset:[70]
// Embedded Data:
//  0x00000046 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000041 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v2.xyxx, t0.xyzw, s0
add r0.xyzw, r0.xyzw, -v0.xyzw
mad r0.xyzw, v1.xxxx, r0.xyzw, v0.xyzw
mul o0.w, r0.w, v1.w
mov o0.xyz, r0.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[70], bundle is:[161] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGVertexEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  9, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 160;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 11;
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
  v_interp_p1_f32  v6, v0, attr0.w                      // 000000000030: C8180300
  v_interp_p1_f32  v7, v0, attr0.x                      // 000000000034: C81C0000
  v_interp_p1_f32  v8, v0, attr0.y                      // 000000000038: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 00000000003C: C8240200
  v_interp_p1_f32  v10, v0, attr1.x                     // 000000000040: C8280400
  v_interp_p2_f32  v6, v1, attr0.w                      // 000000000044: C8190301
  v_interp_p1_f32  v0, v0, attr1.w                      // 000000000048: C8000700
  v_interp_p2_f32  v7, v1, attr0.x                      // 00000000004C: C81D0001
  v_interp_p2_f32  v8, v1, attr0.y                      // 000000000050: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 000000000054: C8250201
  v_interp_p2_f32  v10, v1, attr1.x                     // 000000000058: C8290401
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000005C: BF8C0F70
  v_sub_f32     v5, v5, v6                              // 000000000060: 080A0D05
  v_interp_p2_f32  v0, v1, attr1.w                      // 000000000064: C8010701
  v_sub_f32     v2, v2, v7                              // 000000000068: 08040F02
  v_sub_f32     v3, v3, v8                              // 00000000006C: 08061103
  v_sub_f32     v4, v4, v9                              // 000000000070: 08081304
  v_mac_legacy_f32  v6, v10, v5                         // 000000000074: 0C0C0B0A
  v_mac_legacy_f32  v7, v10, v2                         // 000000000078: 0C0E050A
  v_mac_legacy_f32  v8, v10, v3                         // 00000000007C: 0C10070A
  v_mac_legacy_f32  v9, v10, v4                         // 000000000080: 0C12090A
  v_mul_legacy_f32  v0, v6, v0                          // 000000000084: 0E000106
  s_mov_b64     exec, s[12:13]                          // 000000000088: BEFE040C
  v_cvt_pkrtz_f16_f32  v1, v7, v8                       // 00000000008C: 5E021107
  v_cvt_pkrtz_f16_f32  v0, v9, v0                       // 000000000090: 5E000109
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000094: F800140F 00000001
  s_endpgm                                              // 00000000009C: BF810000
end


// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGVertexEAlpha[] =
{
     68,  88,  66,  67, 164, 112, 
    232, 253, 207,  23,  82,  17, 
     67, 161, 147, 167, 160, 152, 
    136,  80,   1,   0,   0,   0, 
    232,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 108,   1, 
      0,   0, 160,   1,   0,   0, 
     76,   5,   0,   0,  82,  68, 
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
      0,   0,  15,   9,   0,   0, 
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
    164,   3,   0,   0,  80,   0, 
      0,   0, 233,   0,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0,  70,   0,   0,   0, 
     80,   0,   0,   0,  65,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 146,  16,  16,   0, 
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
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,   6,  16, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 130,  32,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     58,  16,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    114,  32,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    163,   0,   0,   0,  21,   0, 
      1,   0,  41, 105,  10,   0, 
    120,   1, 237,  86, 191, 111, 
    211,  64,  20, 126, 103,  59, 
    137, 237, 184, 113,  11,  12, 
    137,  40,  82,  81, 211,  42, 
     44, 145,   2, 136,   1,  49, 
      4, 169, 252,  16,  35, 173, 
     24, 138,  85, 197, 161,  78, 
     83, 225, 166, 193, 137, 144, 
    133,  80,   8,  27,  67, 134, 
    206,  76,  72,  44,  12, 140, 
     48, 176,  36,  17,  98,  56, 
    241,   7, 176, 178,  49,  48, 
     32, 198,  14,  85, 225,  61, 
    251, 142,  90,  21, 108, 145, 
     64,  34, 159, 244, 189, 119, 
    159, 207, 247, 238, 124, 207, 
    122, 119, 219,  38,  68, 120, 
    247, 241, 229,  27, 242, 119, 
    191, 101,  53, 141,   1, 164, 
    176, 141,  46, 242, 132,  80, 
     39,  11, 112, 211,  32,  11, 
     80,  34,  67,  47,  40,   0, 
     89, 116,  57, 228,  11, 100, 
     21,  73, 125, 146,  49, 170, 
    227, 199, 104, 103, 145,  54, 
    242,  11, 142, 187, 133, 190, 
    136, 196, 225, 112,  26,  73, 
     97,  41,  28,  65, 122, 130, 
     74,   6, 161, 145,  57,   6, 
    154,  87, 130,  98,  83,  44, 
      2, 249, 100,  12,  88,  34, 
    243, 123,  44,  35, 147, 113, 
    142, 131, 214,  38,  99, 145, 
     23, 159,  31, 125, 239, 223, 
     70, 188, 174, 164,  61, 202, 
     19, 129, 242, 145,   4, 173, 
     89, 190, 151,  23,  31,  34, 
    247, 172,  36,  52, 237,   5, 
    189,  55,  70, 254,   9,  63, 
     16,  22,  53,  18, 144, 154, 
    242, 149, 142, 178, 140, 158, 
     21,  96, 129, 188, 178,  24, 
    253,  23, 255,  11, 228, 191, 
     74, 123,  65, 251,  43, 255, 
    123,   9, 234,  95,  68,   2, 
    244,  71, 196, 189,  60, 236, 
    147, 146, 154, 161, 206, 144, 
     20,  90,  37,  29, 229,  53, 
    161,  81, 101, 230, 148,  13, 
    131, 193, 134,  61,  15, 251, 
    148,  87, 212,  14, 106,  39, 
    161, 215,  81, 175,  39, 116, 
     13, 117,  45, 161, 235, 168, 
    235,  82, 203, 248, 118,  52, 
    159,  17, 229,  25,  31, 246, 
    254, 101,  78, 215,  56,  25, 
     78, 215,  56,  25,  78, 215, 
     56,  49,  70, 232, 105, 131, 
     97, 207,  60,  28,  42, 234, 
    193,  16, 216, 251, 177, 206, 
    250,  99, 208, 117,  14, 134, 
    197, 153, 110, 112, 102, 204, 
    240,  39,  48,  24, 129, 221, 
    255, 174,  40,  88, 197, 212, 
     60,   7, 152, 231, 192,  22, 
     56,  40,  69,  14,  90, 137, 
     51, 181, 192,  33,   3, 156, 
    193,  25, 206, 216,  89, 206, 
    148,  37, 206, 180, 115, 188, 
    109,  15,  70, 169,  25,  83, 
    103,  25, 198,  21,  91, 211, 
    213, 185, 180, 174, 157, 212, 
    117,  51, 107,  89, 102,  42, 
    103, 153, 153,  89, 203,  52, 
     78,  88, 105,   6,  57,  75, 
     59,  28,  98, 189, 140, 235, 
    237,  41,  89,  47, 159, 142, 
     10, 104, 227, 246, 209,  89, 
     42, 129,  37, 155, 201,  62, 
     42, 223,  88, 247,  89,  91, 
    156, 179,  53, 225, 233, 108, 
    160, 243,  32, 247,  43,  74, 
    140,  61, 209, 255,  76, 120, 
     26,  79, 103, 243, 193, 167, 
    231, 213, 175, 203, 151, 182, 
     94, 139, 231, 175, 132, 167, 
     24, 116, 149, 123, 240, 246, 
    243,  82,  81, 220, 233,  62, 
    136, 190,  26, 146, 198,  55, 
     46,  59, 117, 215, 119,  31, 
    117, 202, 221, 221,  96, 247, 
    126, 185, 211,  13,  60, 119, 
    231, 188, 179, 233,  61, 116, 
    188, 176, 235,   5,  45, 215, 
    119, 182,  26, 225,  69, 103, 
     53, 184, 231, 220, 246,  90, 
    155,  94, 224, 172,  92,  88, 
    169, 132, 206, 106, 211,  69, 
    209, 113, 174, 175, 121, 225, 
    218, 141,  59,  94, 208, 245, 
    194, 107,  87, 253, 118, 211, 
     45,  55, 252,  74, 165, 220, 
    244,  59,  62, 206,  16, 227, 
    138, 152, 159, 238, 142, 116, 
     78, 237, 184, 219,  45, 248, 
      9,  89,  29,  40, 106,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   6,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
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
      0,   0,   0,   0,   0,   0
};
