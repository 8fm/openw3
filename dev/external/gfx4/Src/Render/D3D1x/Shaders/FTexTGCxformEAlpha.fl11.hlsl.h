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
// TEXCOORD                 0   xyzw        1     NONE   float   xyzw
// TEXCOORD                 1   xyzw        2     NONE   float   xyzw
// TEXCOORD                 2   xy          3     NONE   float   xy  
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
// First Precompiled Shader at offset:[65]
// Embedded Data:
//  0x00000041 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000003c - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v3.xyxx, t0.xyzw, s0
mad r0.xyzw, r0.xyzw, v2.xyzw, v1.xyzw
mul o0.w, r0.w, v0.w
mov o0.xyz, r0.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[65], bundle is:[163] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGCxformEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask 15, param2, paramSlot 2, DefaultVal={0,0,0,0};   [3] generic, usageIdx  3, channelMask  3, param3, paramSlot 3, DefaultVal={0,0,0,0}

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
NumVgprs             = 14;
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
; SPI_PS_IN_CONTROL       = 0x00000004
SPIC:NUM_INTERP             = 4
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
  v_interp_p1_f32  v2, v0, attr3.x                      // 000000000014: C8080C00
  v_interp_p1_f32  v3, v0, attr3.y                      // 000000000018: C80C0D00
  v_interp_p2_f32  v2, v1, attr3.x                      // 00000000001C: C8090C01
  v_interp_p2_f32  v3, v1, attr3.y                      // 000000000020: C80D0D01
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3] dmask:0xf // 000000000028: F0800F00 00010202
  v_interp_p1_f32  v6, v0, attr1.w                      // 000000000030: C8180700
  v_interp_p1_f32  v7, v0, attr2.w                      // 000000000034: C81C0B00
  v_interp_p1_f32  v8, v0, attr1.x                      // 000000000038: C8200400
  v_interp_p1_f32  v9, v0, attr1.y                      // 00000000003C: C8240500
  v_interp_p1_f32  v10, v0, attr1.z                     // 000000000040: C8280600
  v_interp_p1_f32  v11, v0, attr2.x                     // 000000000044: C82C0800
  v_interp_p1_f32  v12, v0, attr2.y                     // 000000000048: C8300900
  v_interp_p1_f32  v13, v0, attr2.z                     // 00000000004C: C8340A00
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000050: C8000300
  v_interp_p2_f32  v6, v1, attr1.w                      // 000000000054: C8190701
  v_interp_p2_f32  v7, v1, attr2.w                      // 000000000058: C81D0B01
  v_interp_p2_f32  v8, v1, attr1.x                      // 00000000005C: C8210401
  v_interp_p2_f32  v9, v1, attr1.y                      // 000000000060: C8250501
  v_interp_p2_f32  v10, v1, attr1.z                     // 000000000064: C8290601
  v_interp_p2_f32  v11, v1, attr2.x                     // 000000000068: C82D0801
  v_interp_p2_f32  v12, v1, attr2.y                     // 00000000006C: C8310901
  v_interp_p2_f32  v13, v1, attr2.z                     // 000000000070: C8350A01
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000074: C8010301
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000078: BF8C0F70
  v_mac_legacy_f32  v6, v5, v7                          // 00000000007C: 0C0C0F05
  v_mac_legacy_f32  v8, v2, v11                         // 000000000080: 0C101702
  v_mac_legacy_f32  v9, v3, v12                         // 000000000084: 0C121903
  v_mac_legacy_f32  v10, v4, v13                        // 000000000088: 0C141B04
  v_mul_legacy_f32  v0, v6, v0                          // 00000000008C: 0E000106
  s_mov_b64     exec, s[12:13]                          // 000000000090: BEFE040C
  v_cvt_pkrtz_f16_f32  v1, v8, v9                       // 000000000094: 5E021308
  v_cvt_pkrtz_f16_f32  v0, v10, v0                      // 000000000098: 5E00010A
  exp           mrt0, v1, v1, v0, v0 compr vm           // 00000000009C: F800140F 00000001
  s_endpgm                                              // 0000000000A4: BF810000
end


// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGCxformEAlpha[] =
{
     68,  88,  66,  67,  31,  63, 
    118, 135, 251,  22,  90, 186, 
    106,   2,  57, 148, 188, 197, 
     86,  69,   1,   0,   0,   0, 
    244,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 132,   1, 
      0,   0, 184,   1,   0,   0, 
     88,   5,   0,   0,  82,  68, 
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
    120,   0,   0,   0,   4,   0, 
      0,   0,   8,   0,   0,   0, 
    104,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
    110,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
    110,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,  15,   0,   0, 
    110,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
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
    152,   3,   0,   0,  80,   0, 
      0,   0, 230,   0,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0,  65,   0,   0,   0, 
     80,   0,   0,   0,  60,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      2,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      3,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 114,  32, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 165,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 113,  10,   0, 120,   1, 
    237,  86,  49, 111, 211,  64, 
     20, 126, 103,  39,  78, 226, 
     56, 117,  84,  42, 218, 136, 
     86,   4, 137, 162,  34, 149, 
    208, 180, 101,  97,  42,  34, 
    133, 129,   5, 209,  78, 149, 
     85, 197, 161,  14,  65,  56, 
     73, 235,  68, 200,  48, 132, 
    176,  32, 134,  12,  29,  25, 
    248,   1, 140,  12, 204,  73, 
    196, 116,   3, 130,  63, 192, 
    196, 204, 208, 129, 161,   3, 
    106, 121, 207, 190, 163,  22, 
    162,  76,  17,  66,  40, 159, 
    244, 189, 207, 159, 207, 247, 
    252, 236,  59, 221, 221, 158, 
     14,   1, 142,  59, 137,  79, 
    164, 185, 236, 215, 189,   3, 
    212,  56, 146,   9,  37, 248, 
     73, 138,   0, 119,  83,  20, 
      1,  22,  40, 208,   3,  10, 
    192,   4,  10, 241,  13, 114, 
     13,  73, 109, 146,  33, 214, 
    134,  79,  48, 102, 145,  38, 
    242,  35, 246, 187, 131, 122, 
      9, 137, 221,  97,  14,  73, 
    105,  41,  29,  65,  42,  65, 
    165, 128, 136,  81, 248,   5, 
      9,  10,   2, 103, 145, 148, 
    139,  64,  26, 205,   1, 243, 
     20, 126,  15, 170, 145, 106, 
     63,  13,  84, 155, 204,  69, 
     42,  62, 255, 143, 125, 254, 
     22, 194, 186, 162, 241, 100, 
    156,   8,  52,  30,  81,  80, 
    205, 242, 185, 188, 248,  16, 
    249, 207, 150, 132, 167, 103, 
    136,  67, 228, 105,  56,  70, 
     24, 116,  17, 129, 244,  52, 
     78,  26, 132,  69, 104, 236, 
     60, 228,  73, 149, 171, 193, 
    188, 208, 212,  21,  40, 163, 
    254, 239, 144, 115, 149, 148, 
    254, 175, 156, 247,  18, 116, 
     63, 156, 146, 221,   1, 113, 
    127,   6,  14, 201,  73, 207, 
    208, 135, 127,  48, 244,  42, 
    249,  96, 124,  34,  30,  93, 
    242, 140, 178, 173,  51, 216, 
     54, 103, 225, 144, 198,  21, 
    189, 133, 222, 138, 248,  45, 
    244,  91,  17,  95,  70,  95, 
    142, 248,  10, 250, 138, 244, 
     50, 191,  25, 188,  79,  15, 
    198,  25, 111, 118, 254, 101, 
    142, 107,  28,  13, 199,  53, 
    142, 134, 227,  26,  71, 198, 
      0, 157,  88, 175, 223, 209, 
    143, 250, 138, 250, 189,  15, 
    236, 253,  48, 201, 186,  67, 
     48, 146,  28,  50,   6, 103, 
     70, 138, 179,  76, 134,  63, 
    131, 222,   0, 204, 238, 129, 
    162, 224,  42, 150, 152, 225, 
    144, 158, 229,  16, 203, 115, 
    136,  95, 228, 160,  45, 112, 
     72,  46, 114,  72,  45, 113, 
    208,  87,  57,  30,  40,  56, 
     75, 228,  56,  75, 207, 113, 
     22, 187, 192,  89, 124, 158, 
     51, 237,  50, 103, 201,  43, 
    156, 165, 138, 156, 233, 215, 
     56,  83,  25, 223,  53, 123, 
    131, 184, 105,  24, 202, 116, 
    214,  80, 115, 147,  70, 236, 
    220, 148, 161,  49, 152,  48, 
     98,  71, 125,  92,  55, 195, 
    117, 119,  74, 174, 155, 207, 
      7,  57, 140, 225, 245, 201, 
    158,  42, 129, 107,  53, 147, 
    109, 180, 110, 227, 250, 207, 
    124, 177, 223, 214, 132, 210, 
     30,  65, 251, 194, 196, 207, 
     44,  33,  94, 137, 246, 125, 
    161, 212,  63, 141, 252,  50, 
    249, 121, 122, 249, 229,  11, 
    120,  39, 238, 191,  21,  74, 
     57, 232,  72, 167, 189,  94, 
    255, 182,  40, 206, 118,  31, 
     68,  27, 237, 189, 212, 191, 
    122, 221, 170, 216, 174, 253, 
    180,  85, 104,  55, 189, 230, 
    163,  66, 171, 237,  57, 118, 
    125, 217, 218, 113,  30,  91, 
    142, 223, 118, 188, 134, 237, 
     90,  15, 170, 254, 170, 181, 
    225, 221, 183, 238,  57, 141, 
     29, 199, 179,  74,  43, 165, 
    162, 111, 109, 212, 108,  52, 
     45, 235, 214, 166, 227, 111, 
    222, 190, 233,  87, 155,  94, 
    125, 253, 134, 187,  91, 179, 
     11,  85, 183,  88,  44, 212, 
    220, 150, 139, 111,   8,  81, 
     18, 239, 167,  51,  36, 157, 
    213, 234, 246, 195,   6, 252, 
      0, 122, 226,  46, 133,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   5,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
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
