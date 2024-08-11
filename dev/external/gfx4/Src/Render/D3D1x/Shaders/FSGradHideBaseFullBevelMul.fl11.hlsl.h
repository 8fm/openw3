#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 srctexscale;                // Offset:   32 Size:    16
//   float4 texscale;                   // Offset:   48 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_gradtex                   sampler      NA          NA    0        1
// sampler_srctex                    sampler      NA          NA    1        1
// sampler_tex                       sampler      NA          NA    2        1
// gradtex                           texture  float4          2d    0        1
// srctex                            texture  float4          2d    1        1
// tex                               texture  float4          2d    2        1
// Constants                         cbuffer      NA          NA    0        1
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
// First Precompiled Shader at offset:[310]
// Embedded Data:
//  0x00000136 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000131 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[4], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 3
mov r0.x, l(0)
mov r0.y, -cb0[0].x
loop 
  lt r0.z, cb0[0].x, r0.y
  breakc_nz r0.z
  add r1.x, r0.y, cb0[1].x
  mov r2.x, r0.x
  mov r2.y, -cb0[0].y
  loop 
    lt r0.z, cb0[0].y, r2.y
    breakc_nz r0.z
    add r1.y, r2.y, cb0[1].y
    mad r0.zw, r1.xxxy, cb0[3].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.z, r0.zwzz, t2.xywz, s2, l(0.000000)
    add r2.x, r0.z, r2.x
    add r2.y, r2.y, l(1.000000)
  endloop 
  mov r0.x, r2.x
  add r0.y, r0.y, l(1.000000)
endloop 
mul r0.x, r0.x, cb0[0].w
mul_sat r0.x, r0.x, cb0[0].z
mov r0.y, l(0)
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, r0.xyxx, t0.xyzw, s0
mul r1.xy, v2.xyxx, cb0[2].xyxx
sample_l_indexable(texture2d)(float,float,float,float) r1.x, r1.xyxx, t1.wxyz, s1, l(0.000000)
mul r1.w, r1.x, l(0.000100)
mov_sat r2.x, r0.w
mov r0.w, l(1.000000)
add r2.y, -r2.x, l(1.000000)
mov r1.xyz, l(0,0,0,0)
mul r1.xyzw, r1.xyzw, r2.yyyy
mad r0.xyzw, r0.xyzw, r2.xxxx, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[310], bundle is:[224] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSGradHideBaseFullBevelMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 432;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 7;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 0, offset 24:27 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 1, offset 28:31 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 2, offset 32:35 dwords
;  extUserElements 1[6] = IMM_CONST_BUFFER, 0, offset 36:39 dwords
NumVgprs             = 13;
NumSgprs             = 34;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000007
; constBufUsage           = 0x00000001

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
  s_mov_b64     s[32:33], exec                          // 000000000000: BEA0047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x20                 // 00000000000C: C0C20120
  s_waitcnt     lgkmcnt(0)                              // 000000000010: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 000000000014: C2860900
  v_mov_b32     v2, 0                                   // 000000000018: 7E040280
  s_load_dwordx8  s[16:23], s[0:1], 0x10                // 00000000001C: C0C80110
  s_waitcnt     lgkmcnt(0)                              // 000000000020: BF8C007F
  v_max_f32     v3, -s12, -s12                          // 000000000024: D2200003 6000180C
label_000B:
  v_mov_b32     v4, s12                                 // 00000000002C: 7E08020C
  v_cmp_gt_f32  vcc, v3, v4                             // 000000000030: 7C080903
  s_cbranch_vccnz  label_0029                           // 000000000034: BF87001B
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 000000000038: C2410904
  s_waitcnt     lgkmcnt(0)                              // 00000000003C: BF8C007F
  v_add_f32     v4, s2, v3                              // 000000000040: 06080602
  v_max_f32     v5, -s13, -s13                          // 000000000044: D2200005 60001A0D
label_0013:
  v_mov_b32     v6, s13                                 // 00000000004C: 7E0C020D
  v_cmp_gt_f32  vcc, v5, v6                             // 000000000050: 7C080D05
  s_cbranch_vccnz  label_0027                           // 000000000054: BF870011
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x0c        // 000000000058: C24C090C
  v_interp_p1_f32  v7, v0, attr2.x                      // 00000000005C: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 000000000060: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 000000000064: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 000000000068: C8210901
  v_add_f32     v6, s3, v5                              // 00000000006C: 060C0A03
  s_waitcnt     lgkmcnt(0)                              // 000000000070: BF8C007F
  v_mad_legacy_f32  v9, v4, s24, v7                     // 000000000074: D2800009 041C3104
  v_mad_legacy_f32  v10, v6, s25, v8                    // 00000000007C: D280000A 04203306
  image_sample_lz  v[6:9], v[9:10], s[16:23], s[4:7] dmask:0x8 // 000000000084: F09C0800 00240609
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000008C: BF8C0F70
  v_add_f32     v2, v2, v6                              // 000000000090: 06040D02
  v_add_f32     v5, 1.0, v5                             // 000000000094: 060A0AF2
  s_branch      label_0013                              // 000000000098: BF82FFEC
label_0027:
  v_add_f32     v3, 1.0, v3                             // 00000000009C: 060606F2
  s_branch      label_000B                              // 0000000000A0: BF82FFE2
label_0029:
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x08          // 0000000000A4: C2410908
  v_interp_p1_f32  v3, v0, attr2.x                      // 0000000000A8: C80C0800
  v_interp_p1_f32  v4, v0, attr2.y                      // 0000000000AC: C8100900
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 0000000000B0: C0C20100
  s_load_dwordx8  s[16:23], s[0:1], 0x08                // 0000000000B4: C0C80108
  s_load_dwordx8  s[24:31], s[0:1], 0x18                // 0000000000B8: C0CC0118
  v_mul_legacy_f32  v2, s15, v2                         // 0000000000BC: 0E04040F
  v_interp_p2_f32  v3, v1, attr2.x                      // 0000000000C0: C80D0801
  v_interp_p2_f32  v4, v1, attr2.y                      // 0000000000C4: C8110901
  v_mul_legacy_f32  v5, s14, v2 clamp                   // 0000000000C8: D20E0805 0002040E
  s_waitcnt     lgkmcnt(0)                              // 0000000000D0: BF8C007F
  v_mul_legacy_f32  v2, s2, v3                          // 0000000000D4: 0E040602
  v_mul_legacy_f32  v3, s3, v4                          // 0000000000D8: 0E060803
  v_mov_b32     v6, 0                                   // 0000000000DC: 7E0C0280
  image_sample  v[5:8], v[5:6], s[4:11], s[24:27] dmask:0xf // 0000000000E0: F0800F00 00C10505
  image_sample_lz  v[2:5], v[2:3], s[16:23], s[28:31] dmask:0x8 // 0000000000E8: F09C0800 00E40202
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000F0: BF8C0F71
  v_max_f32     v3, v8, v8 clamp                        // 0000000000F4: D2200803 00021108
  v_interp_p1_f32  v4, v0, attr1.x                      // 0000000000FC: C8100400
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000100: BF8C0F70
  v_mul_legacy_f32  v2, 0x38d1b717, v2                  // 000000000104: 0E0404FF 38D1B717
  v_interp_p1_f32  v8, v0, attr1.y                      // 00000000010C: C8200500
  v_interp_p1_f32  v9, v0, attr1.z                      // 000000000110: C8240600
  v_mad_legacy_f32  v2, -v3, v2, v2                     // 000000000114: D2800002 240A0503
  v_interp_p2_f32  v4, v1, attr1.x                      // 00000000011C: C8110401
  v_mul_legacy_f32  v5, v5, v3                          // 000000000120: 0E0A0705
  v_mul_legacy_f32  v6, v6, v3                          // 000000000124: 0E0C0706
  v_mul_legacy_f32  v7, v7, v3                          // 000000000128: 0E0E0707
  v_interp_p2_f32  v8, v1, attr1.y                      // 00000000012C: C8210501
  v_interp_p2_f32  v9, v1, attr1.z                      // 000000000130: C8250601
  v_add_f32     v2, v3, v2                              // 000000000134: 06040503
  v_mul_legacy_f32  v3, v5, v4                          // 000000000138: 0E060905
  v_interp_p1_f32  v10, v0, attr1.w                     // 00000000013C: C8280700
  v_mul_legacy_f32  v4, v6, v8                          // 000000000140: 0E081106
  v_mul_legacy_f32  v5, v7, v9                          // 000000000144: 0E0A1307
  v_interp_p1_f32  v6, v0, attr0.x                      // 000000000148: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 00000000014C: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 000000000150: C8200200
  v_interp_p2_f32  v10, v1, attr1.w                     // 000000000154: C8290701
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000158: C8000300
  v_interp_p2_f32  v6, v1, attr0.x                      // 00000000015C: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 000000000160: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 000000000164: C8210201
  v_mul_legacy_f32  v3, v3, v10                         // 000000000168: 0E061503
  v_mul_legacy_f32  v4, v4, v10                         // 00000000016C: 0E081504
  v_mul_legacy_f32  v5, v5, v10                         // 000000000170: 0E0A1505
  v_mul_legacy_f32  v2, v2, v10                         // 000000000174: 0E041502
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000178: C8010301
  v_mac_legacy_f32  v3, v6, v2                          // 00000000017C: 0C060506
  v_mac_legacy_f32  v4, v7, v2                          // 000000000180: 0C080507
  v_mac_legacy_f32  v5, v8, v2                          // 000000000184: 0C0A0508
  v_mac_legacy_f32  v2, v0, v2                          // 000000000188: 0C040500
  v_mul_legacy_f32  v0, v3, v2                          // 00000000018C: 0E000503
  v_mul_legacy_f32  v1, v4, v2                          // 000000000190: 0E020504
  v_mul_legacy_f32  v3, v5, v2                          // 000000000194: 0E060505
  s_mov_b64     exec, s[32:33]                          // 000000000198: BEFE0420
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 00000000019C: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v3, v2                       // 0000000001A0: 5E020503
  exp           mrt0, v0, v0, v1, v1 compr vm           // 0000000001A4: F800140F 00000100
  s_endpgm                                              // 0000000001AC: BF810000
end


// Approximately 41 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSGradHideBaseFullBevelMul[] =
{
     68,  88,  66,  67, 186,  85, 
     78, 145,  37, 230,  53, 157, 
     23, 228, 225, 130,  25,  61, 
     80, 170,   1,   0,   0,   0, 
    128,  12,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    228,   2,   0,   0,  72,   3, 
      0,   0, 124,   3,   0,   0, 
    228,  11,   0,   0,  82,  68, 
     69,  70, 168,   2,   0,   0, 
      1,   0,   0,   0, 100,   1, 
      0,   0,   7,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    108,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     28,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  44,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  59,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     71,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,  79,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0,  86,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      2,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
     90,   1,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 103, 114, 
     97, 100, 116, 101, 120,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 115, 114,  99, 116, 
    101, 120,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,   0, 103, 114,  97, 
    100, 116, 101, 120,   0, 115, 
    114,  99, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0,  90,   1,   0,   0, 
      4,   0,   0,   0, 124,   1, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  28,   2,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     80,   2,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  44,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  87,   2, 
      0,   0,  32,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  44,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  99,   2,   0,   0, 
     48,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     34,   2,   0,   0, 111, 102, 
    102, 115, 101, 116,   0, 115, 
    114,  99, 116, 101, 120, 115, 
     99,  97, 108, 101,   0, 116, 
    101, 120, 115,  99,  97, 108, 
    101,   0,  77, 105,  99, 114, 
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
     69,  88,  96,   8,   0,   0, 
     80,   0,   0,   0,  24,   2, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  54,   1, 
      0,   0,  80,   0,   0,   0, 
     49,   1,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   3,   0, 
      0,   0,  54,   0,   0,   5, 
     18,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  34,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   3,   0,   4,   3, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5,  18,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  48,   0, 
      0,   1,  49,   0,   0,   8, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,   3,   0, 
      4,   3,  42,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     50,   0,   0,  10, 194,   0, 
     16,   0,   0,   0,   0,   0, 
      6,   4,  16,   0,   1,   0, 
      0,   0,   6, 132,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0,  66,   0, 
     16,   0,   0,   0,   0,   0, 
    230,  10,  16,   0,   0,   0, 
      0,   0,  70, 123,  16,   0, 
      2,   0,   0,   0,   0,  96, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     26,   0,  16,   0,   2,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  22,   0, 
      0,   1,  54,   0,   0,   5, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  34,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     56,   0,   0,   8,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  56,  32,   0,   8, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  42, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5,  34,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     50,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  70, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   0,  16,   0,   1,   0, 
      0,   0,  54, 121,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,  23, 183, 209,  56, 
     54,  32,   0,   5,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     54,   0,   0,   8, 114,   0, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  86,   5, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,  32, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  53,  16, 
      0,   0, 226,   0,   0,   0, 
     21,   0,   1,   0,  41, 209, 
     11,   0, 120,   1, 237,  86, 
     61, 108,  28,  69,  20, 126, 
     59,  59,  51,  59, 187, 236, 
    205, 217,  74,  44,  46, 193, 
     17, 151, 228, 162, 152, 198, 
    138,  73, 138, 136,  42, 177, 
    162,  64,   1,  13,  22,   5, 
     97,  21, 221,  57,  94, 199, 
     81, 214,  78, 114, 231,  68, 
     22, 138, 142, 131, 130,  52, 
     41,  34, 165,  66,  66, 130, 
    130, 130, 146, 142, 206, 231, 
     88,  20,  83,  80,  16,  23, 
     72, 180, 136, 130,   2,   9, 
     43,  52,  41,  80, 204, 247, 
    246,  39,  57,  80, 232,  44, 
     17,   9, 127, 210,  55, 111, 
    191, 121, 179, 239, 189, 153, 
     89, 237, 204, 195, 151,  40, 
    199,  79, 127, 124, 113, 156, 
    237,   7,  87, 239, 205, 126, 
     39, 137,  20, 158,  61, 144, 
     45,  99, 205, 112,  75, 244, 
    117, 196,  45, 209,  20,  55, 
     60,  64,  16, 213,  96, 142, 
    128, 223,  64, 159, 129, 101, 
     95, 197,   2, 238, 193,   9, 
    248,  78, 225, 169,   1,  30, 
     65, 208,  54, 236, 123,  32, 
    167, 159,   5,  15, 130,  24, 
    146, 195, 231, 166,   4, 199, 
    102, 160, 164, 191, 129, 245, 
     33,  48,   0,  81,  66,  30, 
     55,   4,  25, 236, 227, 190, 
    167,  56, 198, 205, 243, 193, 
    117,  84,  57, 158,   7, 174, 
    173, 170, 135, 235, 227,  58, 
     25,  60, 223, 255,  26, 197, 
    122,  21, 213, 241,  58, 176, 
    174, 246, 137, 193, 251,  49, 
     10,  11,  86,  93,  83, 229, 
     62, 242, 123, 140, 211, 165, 
    230, 181, 224, 185, 109, 128, 
    255, 134,  29, 224, 217, 222, 
     22, 168,  52,  87, 163, 169, 
    142,  22, 214, 123, 149, 154, 
    108, 197, 209, 252, 187, 248, 
    191, 128, 191,  63,   6, 175, 
      5, 127, 135,  99,  96, 177, 
     34,   5, 216, 127,  25,  36, 
     26,  12, 153, 247,  26, 244, 
    152,  85, 165, 189, 127, 104, 
    159, 117, 190, 113,  35, 154, 
      3, 251, 116, 209,  87, 226, 
     98, 125, 178, 244, 251, 148, 
     64,  39,  35, 250,   2, 244, 
    133,  17, 221, 134, 110, 143, 
    232, 121, 232, 249, 167, 186, 
    140,  95, 207, 243, 249, 130, 
    247,  25, 157, 253,  23, 153, 
    123,  53, 238,  14, 247, 106, 
    220,  29, 238, 213, 184, 107, 
    204, 209, 151,  95, 174, 247, 
    163,  39, 235, 194, 255, 115, 
    189, 233, 109, 110, 124,  68, 
    119, 135,  20, 126, 186,  57, 
     16, 178,  63, 230, 185,  92, 
    251, 212, 220, 138,  27, 212, 
    142, 133, 233, 251, 161, 185, 
    253,  10, 221,  25, 202, 240, 
    236,  38, 251, 132,  54,  90, 
    193,  95,  59,  72, 237, 154, 
    136, 251, 170, 102, 110, 143, 
    195,  31, 135, 111, 111, 146, 
    153, 116,  20,  54, 157, 103, 
     14,  57,  47,  60, 236, 252, 
     40, 214, 252,  78,  72, 131, 
     45,  57,  51,  41,  35,  88, 
    125, 178,  41, 201, 124, 190, 
     29, 234,  22,  93, 175,  35, 
     94,  77, 234,  71,  81, 164, 
    127, 219, 249, 100, 248,  72, 
    107, 253,  51, 172,  65,  46, 
     50,  49,  98, 141,  57,  66, 
    141,   6, 117,  53, 188, 239, 
     55, 234,  82,  90, 207, 212, 
     16, 123, 220,  41,  99, 183, 
    172,  20,  84, 212,  36, 173, 
    111, 180,  29, 160,  30, 170, 
     15, 182, 149, 122,  64, 156, 
     67, 136,  95, 232,   6, 114, 
    248, 166, 185, 101, 198, 241, 
    131, 151,  99, 142, 115, 238, 
     32, 206, 203, 223,  62,  60, 
     77, 170, 233,  72, 183, 156, 
     64,  93, 190, 138,  90, 158, 
     68, 220,  32, 178,  58, 136, 
    109,  16,  88, 235, 169, 195, 
    206, 211, 199, 156, 175, 164, 
     86, 161, 182,  20,  76,  57, 
     61, 110, 108, 176,  47, 194, 
    209, 223,  64, 109, 152, 175, 
    192, 124, 131, 215,  28, 254, 
    253, 206, 163,   3, 206, 243, 
     48, 119, 129, 185,  79, 104, 
     43,  39, 140,  85,  19, 145, 
     21,  19, 168, 219, 247, 156, 
     86,  58,  14, 148, 137, 141, 
    138,  98,  82,  50, 246,  21, 
     89, 169, 132,  85,  74, 219, 
    166, 124, 178, 142,  24, 197, 
    249, 179, 191,  58,  63,  62, 
     30,  30,  64, 155,  63,   2, 
    124,  68, 141,   2,  87, 147, 
    220, 197,  13,  95,  83,  42, 
    205, 227, 248, 140, 100, 157, 
    119,   0, 124, 159, 195,  57, 
    153, 107, 230,  36,  88, 105, 
     30, 207, 247,  10, 214, 252, 
    204, 104, 129, 172, 127,  40, 
    239,  49,  27, 165, 229, 184, 
    232, 199, 221, 135, 223, 124, 
    134, 237, 210, 255, 107, 105, 
    145,  59, 191, 139,  38,  63, 
    238, 107, 171, 247, 151,  63, 
    139,  89,   0, 166, 180,  28, 
    131, 135,  54, 238, 255,  94, 
    187,  91, 246, 181,  74, 187, 
      4, 242, 251, 139, 111,  36, 
    243, 157, 172, 243,  97, 111, 
    122, 245,  90, 247, 218, 213, 
    233, 222, 106,  55, 237,  44, 
    191, 158,  44, 164, 183, 146, 
    116, 109,  53, 237, 174, 116, 
    178, 228, 242, 226, 218, 169, 
    100, 174, 123,  41, 121,  55, 
     93,  89,  72, 187, 201, 185, 
    147, 231, 102, 214, 146, 185, 
    165,  14,  68,  47,  57,  63, 
    247, 102, 183, 179, 240, 214, 
    149, 133, 116, 182, 211,  75, 
    207, 223, 204, 178, 217, 244, 
     86, 154, 189, 115,  51, 155, 
     94, 204, 102, 102, 166, 151, 
    178,  94, 134,  76,   5, 190, 
     42, 243, 243,  29, 157, 239, 
    113, 203, 157,  43,  43, 244, 
     23, 242,  14, 108, 239,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  41,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  19,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  12,   0,   0,   0, 
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
