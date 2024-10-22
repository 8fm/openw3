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
// TEXCOORD                 0   xyzw        0     NONE   float      w
// TEXCOORD                 1   xyzw        1     NONE   float      w
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
// First Precompiled Shader at offset:[50]
// Embedded Data:
//  0x00000032 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000002d - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.w
dcl_input_ps linear v1.w
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v2.xyxx, t0.wxyz, s0
mad o0.xyzw, r0.xxxx, v1.wwww, v0.wwww
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[50], bundle is:[142] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGCxformInv.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  8, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 96;Bytes

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
  v_interp_p1_f32  v0, v0, attr1.w                      // 000000000034: C8000700
  v_interp_p2_f32  v3, v1, attr0.w                      // 000000000038: C80D0301
  v_interp_p2_f32  v0, v1, attr1.w                      // 00000000003C: C8010701
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000040: BF8C0F70
  v_mac_legacy_f32  v3, v2, v0                          // 000000000044: 0C060102
  s_mov_b64     exec, s[12:13]                          // 000000000048: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v3, v3                       // 00000000004C: 5E000703
  s_nop         0x0000                                  // 000000000050: BF800000
  exp           mrt0, v0, v0, v0, v0 compr vm           // 000000000054: F800140F 00000000
  s_endpgm                                              // 00000000005C: BF810000
end


// Approximately 3 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGCxformInv[] =
{
     68,  88,  66,  67, 127, 163, 
    186,  33, 110, 159, 213,  38, 
    180,  47, 126,  69, 188,  57, 
     27,  91,   1,   0,   0,   0, 
     72,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 104,   1, 
      0,   0, 156,   1,   0,   0, 
    172,   4,   0,   0,  82,  68, 
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
      0,   0,  15,   8,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   8,   0,   0, 
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
     69,  88,   8,   3,   0,   0, 
     80,   0,   0,   0, 194,   0, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  50,   0, 
      0,   0,  80,   0,   0,   0, 
     45,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   2,   0,   0,   0, 
     54, 121,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,  32,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   1,   0, 
      0,   0, 246,  31,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    144,   0,   0,   0,  21,   0, 
      1,   0,  41,  38,  10,   0, 
    120,   1, 237,  86,  65, 107, 
     19,  65,  24, 253, 102,  55, 
    110,  54, 237,  54,  41, 168, 
     80, 193,  67, 161,  40, 158, 
    162, 209, 158, 164, 135, 130, 
    197, 234,  65,  16, 219,  83, 
    153, 138,  91, 187,  49, 226, 
     38, 177, 155,  80, 150,  30, 
    210,   8,  30, 115, 240, 232, 
    205, 139, 191,  64,  60, 121, 
    200,   6,  79, 251,  19, 188, 
     40, 120, 243, 216,  99, 149, 
    146, 250, 189, 221,  25, 221, 
      6, 123,  11,  40, 152,   7, 
    239, 123, 251, 102, 118, 190, 
    124, 153,  25, 102, 246, 242, 
     20,  37, 152, 189, 218, 121, 
     13,  29, 190, 249, 242, 249, 
     61, 235,  25, 166,  80,  10, 
    132,  54,  34, 145,  93,  64, 
     36, 186, 130, 128,  23,  12, 
     34, 139, 165, 200, 124, 196, 
     92, 102, 162,  79,  51, 197, 
    242, 160, 206, 113, 150, 137, 
    124,  31, 120, 220,  42, 235, 
     89,  38,  15,  39, 148, 128, 
    180,  72,   7, 104,   5,  76, 
      4,  70,  14,  97,   4,  24, 
    171, 129, 242, 180, 135, 102, 
    115, 208,  37, 132,  63, 227, 
     60,  19, 245, 159,   6, 212, 
    166, 115,  65, 213, 223,  79, 
    254, 239, 223,  70,  90,  87, 
     54, 254,  94,  39,   0, 235, 
    145,   5, 106, 214, 239, 125, 
     82, 239, 233,  57, 251, 166, 
    188,  94, 203,   1, 243,  52, 
     28,  51,  28,  60, 100, 160, 
     61, 214, 203,  74,  86, 131, 
     85, 204, 209,  60, 212,  88, 
     72, 246, 197, 255,   2, 189, 
     87,  49,  23, 152,  95, 236, 
    251,  18,  83,   3, 253, 216, 
    251,  68, 221,   8, 124,  53, 
     71, 135, 112, 218, 243, 180, 
     29,  98, 172, 246,  38, 124, 
    210, 112, 210, 155, 121, 122, 
      8,  95, 186, 152, 142, 103, 
     47,  71, 252, 198, 136, 231, 
     45, 113, 194, 111, 101,  61, 
    158,  19, 159, 228,  55,  77, 
    172,  51,  55, 118, 254, 101, 
     78, 106,  28,  15,  39,  53, 
    142, 135, 147,  26, 199, 198, 
      4, 157,  92, 175, 223, 153, 
     26, 246,  13, 243, 168,  79, 
    226, 227, 192,  22, 221,   1, 
    217, 118,  76,   5,  39,  22, 
    118,  33,  22, 133, 153, 120, 
    159, 122,  17, 217, 221,   3, 
    195, 224, 219, 205, 116,  98, 
    202,  83,  44, 204, 153,  88, 
    228,  69, 252, 188, 212, 139, 
     12,  97,  57,  78, 110, 216, 
    231, 243,  46,  61,  47, 207, 
    233, 243, 238,  69, 116, 129, 
    163, 190,  19, 245,  93, 168, 
    193, 183, 152, 208, 125, 184, 
    209, 248, 220,  22, 215, 212, 
      7, 192, 188,  82, 156, 237, 
     56, 207, 139, 191, 178, 164, 
    184, 175, 250, 239,  40, 197, 
    248, 105, 230, 187, 123, 209, 
     91, 107, 201, 250, 225, 171, 
    246, 154,  82, 228, 192, 183, 
    198, 244,  82, 113, 231, 171, 
    106, 123, 169, 116, 147, 137, 
    241, 213, 155, 114, 203, 245, 
    221, 189,  86, 185, 221,  12, 
    154, 207, 202, 173, 118, 224, 
    185, 245, 235, 114, 219, 219, 
    149,  94, 216, 246, 130, 134, 
    235, 203,  39, 213, 112,  81, 
    174,   5, 143, 229,   3, 175, 
    177, 237,   5, 114, 229, 198, 
     74,  37, 148, 107,  53, 151, 
     77,  75, 222,  94, 247, 194, 
    245, 213,  91,  97, 181,  25, 
    212, 239,  54, 118, 203,  85, 
    191,  82,  41, 215, 252, 150, 
    207, 233,  83, 124,  87,  63, 
    138,  15, 181,  60, 179, 238, 
     62, 109, 208,  79, 124, 117, 
     33, 161,  83,  84,  65,  84, 
    148,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   1,   0,   0,   0, 
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
