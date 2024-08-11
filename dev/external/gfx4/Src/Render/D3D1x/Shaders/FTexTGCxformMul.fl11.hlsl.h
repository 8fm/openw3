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
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[62]
// Embedded Data:
//  0x0000003e - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000039 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v2.xyxx, t0.xyzw, s0
mad r0.xyzw, r0.xyzw, v1.xyzw, v0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[62], bundle is:[160] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGCxformMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 168;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 13;
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
  v_interp_p1_f32  v10, v0, attr1.x                     // 000000000040: C8280400
  v_interp_p1_f32  v11, v0, attr1.y                     // 000000000044: C82C0500
  v_interp_p1_f32  v12, v0, attr1.z                     // 000000000048: C8300600
  v_interp_p1_f32  v0, v0, attr1.w                      // 00000000004C: C8000700
  v_interp_p2_f32  v6, v1, attr0.x                      // 000000000050: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 000000000054: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 000000000058: C8210201
  v_interp_p2_f32  v9, v1, attr0.w                      // 00000000005C: C8250301
  v_interp_p2_f32  v10, v1, attr1.x                     // 000000000060: C8290401
  v_interp_p2_f32  v11, v1, attr1.y                     // 000000000064: C82D0501
  v_interp_p2_f32  v12, v1, attr1.z                     // 000000000068: C8310601
  v_interp_p2_f32  v0, v1, attr1.w                      // 00000000006C: C8010701
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000070: BF8C0F70
  v_mac_legacy_f32  v6, v2, v10                         // 000000000074: 0C0C1502
  v_mac_legacy_f32  v7, v3, v11                         // 000000000078: 0C0E1703
  v_mac_legacy_f32  v8, v4, v12                         // 00000000007C: 0C101904
  v_mac_legacy_f32  v9, v5, v0                          // 000000000080: 0C120105
  v_mul_legacy_f32  v0, v6, v9                          // 000000000084: 0E001306
  v_mul_legacy_f32  v1, v7, v9                          // 000000000088: 0E021307
  v_mul_legacy_f32  v2, v8, v9                          // 00000000008C: 0E041308
  s_mov_b64     exec, s[12:13]                          // 000000000090: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000094: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v9                       // 000000000098: 5E021302
  exp           mrt0, v0, v0, v1, v1 compr vm           // 00000000009C: F800140F 00000100
  s_endpgm                                              // 0000000000A4: BF810000
end


// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGCxformMul[] =
{
     68,  88,  66,  67, 167, 228, 
     78,  20, 214, 104, 216, 229, 
    231,   0, 239, 105,  96,  41, 
    197, 255,   1,   0,   0,   0, 
    192,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 104,   1, 
      0,   0, 156,   1,   0,   0, 
     36,   5,   0,   0,  82,  68, 
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
     92,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88, 128,   3,   0,   0, 
     80,   0,   0,   0, 224,   0, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  62,   0, 
      0,   0,  80,   0,   0,   0, 
     57,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   2,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    162,   0,   0,   0,  21,   0, 
      1,   0,  41, 110,  10,   0, 
    120,   1, 237,  86,  65, 107, 
     19,  65,  20, 126,  51, 187, 
     73,  54, 117, 155,  20,  83, 
    177, 129,  22,  35, 181, 218, 
    162, 134,  70, 197,  67, 189, 
     84,  44, 122,  16,  65, 108, 
     79, 101,  44, 221, 216, 141, 
     17,  55, 137, 108, 162, 172, 
     34,  49, 222, 115, 232, 193, 
    131,  63, 193, 155,  30, 188, 
    154,   4,  65,  24, 193, 147, 
     55, 239,  94, 188, 121,  84, 
    144, 234, 123, 187,  51, 117, 
     41, 246,  22,  80,  48,  31, 
    124, 239, 237,  55, 179, 243, 
    230, 237, 188,  97, 102, 235, 
     99,  16, 226, 196, 167, 119, 
     73, 242,  95, 102, 237, 243, 
    159, 209,  39, 144,  76, 121, 
     66,  96, 145,   5, 184, 158, 
     38,  11,  48,  79, 134,  94, 
    224,   0, 227, 232,  50, 200, 
     23, 200, 101,  36, 245, 105, 
     70,  88,  30,  60,  68,  59, 
    129, 164, 247,  62, 226, 184, 
    171, 232, 143,  35, 113,  56, 
    204,  32,  41,  44, 133,  35, 
    104,  79,  48, 200,  32,  76, 
     50, 123, 160,  82,   9,  49, 
    137, 164,  88,   4, 242, 241, 
     24,  48,  71, 230, 207, 160, 
     28,  41, 255, 253,  64, 185, 
    233,  88, 228, 245, 156, 244, 
     29, 127,  27,  81,  94, 113, 
    251, 187,  78,   4, 170,  71, 
     28, 148, 179, 126, 175, 160, 
     62,  68, 175, 217, 162, 210, 
    186, 150,   3, 228, 126, 248, 
    137, 176, 233,  33,   6, 173, 
    169,  94,  73, 200, 162,  69, 
    207, 142,  64, 129,  60, 159, 
     13, 247, 197, 255,   2, 189, 
     87, 105,  45, 104, 125, 105, 
    223,  71,  43,  18, 129, 250, 
    163,  45, 217, 233,  19, 183, 
    167, 224,  27,  41, 173, 217, 
     30, 109, 144,  14,  11,  23, 
    211,  84,  47,   3,  54, 120, 
    142, 111, 100, 167,  85, 191, 
      1,   2, 181, 136, 233, 117, 
    212, 235,  49, 189, 137, 122, 
     51, 166, 203, 168, 203, 187, 
     90, 197, 207, 134, 243, 241, 
     52, 213,  25,  27, 219, 255, 
     50,  71,  57,  14, 135, 163, 
     28, 135, 195,  81, 142,  67, 
     99, 136, 182, 217, 237, 181, 
    199, 118, 122, 220, 248, 209, 
      3, 246, 118,  96, 177, 206, 
      0,  44,  75,  66, 218, 150, 
    204,  74,  75, 150,  30, 151, 
     79, 160, 219, 135, 108, 231, 
     43, 231, 116, 136,  77,  73, 
     96, 211,  18, 120,  65, 130, 
    113,  76, 130,  57,  47,  33, 
    113,  74,  66, 114,  81,  66, 
     10,  36, 131, 188, 100, 108, 
     70,  50, 126,  84,  50,  99, 
     78,  50, 115,  65, 178, 196, 
    105, 201, 146,  37, 201,  82, 
     76, 222, 203, 118, 251, 252, 
    144, 109,  27, 135,  51, 182, 
    153, 159, 176,  19, 236, 160, 
    157, 204,  65,  38, 149, 227, 
     25,  43, 103,  98, 227,  78, 
     15, 207, 205, 232, 220, 157, 
    212, 231, 230, 211, 126,  30, 
    109, 248, 136, 208, 119, 170, 
      6,  94, 201,  76, 247, 209, 
    245, 140, 231,  63,  11, 212, 
    125,  91,  85, 158, 238,   8, 
    186,  23,  50, 187,  81,  34, 
     60,  87, 253, 219, 202, 211, 
    248,   3, 200,  11, 222, 146, 
    241, 236, 251, 155, 151, 175, 
     85, 251,  43, 229,  41,   6, 
    253, 210,  13,  30, 159, 124, 
    191, 160, 254, 237,  62, 168, 
    190, 155,  72,  26,  95,  89, 
     18, 101, 199, 115,  30,  53, 
    139, 173, 134, 223, 184,  91, 
    108, 182, 124, 215, 169, 157, 
     17,  91, 238,   3, 225,   6, 
     45, 215, 175,  59, 158, 184, 
     93,   9, 206, 137,  85, 255, 
    150, 184, 225, 214, 183,  92, 
     95, 172, 156,  93,  41,   5, 
     98, 181, 234, 160, 104, 138, 
    203, 107, 110, 176, 118, 229, 
     82,  80, 105, 248, 181, 107, 
    247, 189,  98, 197,  43, 149, 
    138,  85, 175, 233,  97, 248, 
      8,  23, 213, 228, 244,   3, 
    153,  66, 214, 156,  59, 117, 
    248,   5, 117, 217,  42, 157, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   5,   0, 
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
      0,   0
};
