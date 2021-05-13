#include "stdafx.h"
#include "bitstream_header.h"
#include "cavlc.h"
#include <assert.h>

//sao_merge_left_flag and sao_merge_up_flag共用context model
struct context_model sao_merge_cm;
struct context_model cu_transquant_bypass_flag_cm;
struct context_model split_cu_flag_cm[3];
struct context_model cu_skip_flag_cm[3];
struct context_model merge_flag_cm;
struct context_model merge_idx_cm;
struct context_model part_mode_cm[4];
struct context_model pred_mode_cm;
struct context_model prev_intra_luma_pred_flag_cm;
struct context_model intra_chroma_pred_mode_cm;
struct context_model inter_dir_cm[5];
struct context_model mvd_cm[2]; //abs_mvd_greater0_flag & abs_mvd_greater1_flag
struct context_model ref_pic_cm[2];
struct context_model dqp_cm[2];
struct context_model qt_cbf_cb_cr_cm[4]; //cbf_cb, cbf_cr
struct context_model qt_cbf_luma_cm[2];
struct context_model qt_root_cbf_cm; //rqt_root_cbf
struct context_model last_sig_coeff_x_prefix_cm[18];
struct context_model last_sig_coeff_y_prefix_cm[18];
struct context_model coded_sub_block_flag_cm[4];
struct context_model sig_flag_cm[42]; //sig_coeff_flag
struct context_model coeff_abs_level_greater1_flag_cm[24];
struct context_model coeff_abs_level_greater2_flag_cm[6];
struct context_model mvp_idx_cm; //mvp_l0_flag & mvp_l1_flag
struct context_model sao_type_idx_cm;
struct context_model trans_subdiv_flag_cm[3]; // split_transform_flag
struct context_model transform_skip_flag_cm[2];

extern int MinCbLog2SizeY;
extern int PicWidthInSamplesY;
extern int PicHeightInSamplesY;
extern int **CtDepth;
extern int **cu_skip_flag;
extern int **intra_luma_pred_mode;

extern struct HEVCFrame *CurrPic;
unsigned int ivlCurrRange, ivlOffset;

#define CABAC_MAX_BIN 31

//9.3.2.2 table 9-5~9-x ffmpeg hevc_cabac.c init_values[3][HEVC_CONTEXTS]
unsigned char initValues_sao_merge_flag[3] =
{
	153, 153, 153,
};

unsigned char initValues_sao_type_idx[3] =
{
#if FIX827
	200, 185, 160,
#else
	160, 185, 200,
#endif
};


unsigned char initValues_split_cu_flag[3][3] =
{
	{ 139,  141,  157, }, //I_SLICE initType = 0
	{ 107,  139,  126, }, //P_SLICE initType = 1
	{ 107,  139,  126, }, //P_SLICE && cabac_init_flag initType = 2
};


unsigned char initValues_cu_transquant_bypass_flag[3] =
{
	154, 154, 154,
};

unsigned char initValues_cu_skip_flag[3][3] =
{
	{ CNU,  CNU,  CNU, },
	{ 197,  185,  201, },
	{ 197,  185,  201, },
};


unsigned char initValues_dqp[3][2] =
{
	{ 154,  154, },
	{ 154,  154, },
	{ 154,  154, },
};

unsigned char initValues_pred_mode[3] =
{
	CNU, 149, 134,
};

//spec initValue	184	154	139	154	154	154	139	154	154
//table 9-4 part_mode	Table 9-11 0	1..4	5..8
//HM12 这个表分成2个表,另一个cu_amp_pos
unsigned char initValues_part_mode[3][4] =
{
	{ 184,  CNU,  CNU,  CNU, },
	{ 154,  139,  154,  CNU, },
	{ 154,  139,  154,  CNU, },
};


unsigned char initValues_prev_intra_luma_pred_flag[3] =
{
	184, 154, 183,
};



unsigned char initValues_intra_chroma_pred_mode[3] =
{
	63, 152, 152,
};


unsigned char initValues_merge_flag[3] =
{
	CNU, 110, 154,
};

unsigned char initValues_merge_idx_ext[3] =
{
	CNU, 122, 137,
};


//todo 与spec是不是对不起来,HM12 binIdx只用到2
//								binIdx
//						0	1	2	3	4	>=  5
//part_mode log2CbSize = = MinCbLog2SizeY	0	1	2	bypass	na	na
//part_mode log2CbSize > MinCbLog2SizeY		0	1	3	bypass	na	na



unsigned char initValues_inter_dir[3][5] =
{
	{ CNU,  CNU,  CNU,  CNU, CNU, },
	{  95,   79,   63,   31,  31, },
	{  95,   79,   63,   31,  31, },
};


unsigned char initValues_ref_pic[3][2] =
{
	{ CNU,  CNU },
	{ 153,  153 },
	{ 153,  153 },
};

//abs_mvd_greater0_flag & abs_mvd_greater1_flag
unsigned char initValues_mvd[3][2] =
{
	{ CNU,  CNU, },
	{ 140,  198, },
	{ 169,  198, },
};

//mvp_lx_flag
unsigned char initValues_mvp_idx[3] =
{
	CNU, 168,  168,
};

//94	138	182	154	149	107	167	154	149	92	167	154	154	154	154
//rqt_root_cbf ffmpeg no_residual_data_flag
unsigned char initValues_qt_root_cbf[3] = 
{
	CNU, 79, 79,
};


//split_transform_flag
unsigned char initValues_trans_subdiv_flag[3][3] =
{
#if FIX712
	{ 153,  138,  138, },
	{ 124,  138,   94, },
	{ 224,  167,  122, },
#else
	{ 224,  167,  122, },
	{ 124,  138,   94, },
	{ 153,  138,  138, },
#endif
};

unsigned char initValues_qt_cbf_luma[3][2] =
{
	{ 111,   141, },
	{ 153,  111, },
	{ 153,  111, },
};

unsigned char initValues_qt_cbf_cb_cr[3][4] =
{
	{ 94,  138,  182,  154, }, //0~3, 12
	{ 149,  107,  167,  154, }, //4~7, 13
	{ 149,   92,  167,  154, }, //8~11,14 12,13,14 spec定义了，但没用到？
};


unsigned char initValues_transform_skip_flag[3][2 * 1] =
{
	{ 139,  139},
	{ 139,  139},
	{ 139,  139},
};

//last_significant_coeff_x_prefix,last_significant_coeff_y_prefix使用相同初始值
unsigned char initValues_last[3][18] =
{
	{ 110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
		108,  123,   63,
	},
	{ 125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
		108,  123,  108,
	},
	{ 125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
		108,  123,   93,
	},
};

unsigned char initValues_coded_sub_block_flag[3][2 * 2] =
{
	{ 91,  171, 134,  141, },
	{ 121,  140, 61,  154, },
	{ 121,  140, 61,  154, },
};

//significant_coeff_flag
unsigned char initValues_sig_flag[3][42] =
{
	{ 111,  111,  125,  110,  110,   94,  124,  108,  124,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  140,  139,  182,  182,  152,  136,  152,  136,  153,  136,  139,  111,  136,  139,  111,  },
	{ 155,  154,  139,  153,  139,  123,  123,   63,  153,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  123,  123,  107,  121,  107,  121,  167,  151,  183,  140,  151,  183,  140,  },
	{ 170,  154,  139,  153,  139,  123,  123,   63,  124,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  138,  138,  122,  121,  122,  121,  167,  151,  183,  140,  151,  183,  140,  },
};


//table 9-30
unsigned char initValues_coeff_abs_level_greater1_flag[3][24] =
{
	{ 140,   92,  137,  138,  140,  152,  138,  139,  153,   74,  149,   92,  139,  107,  122,  152,  140,  179,  166,  182,  140,  227,  122,  197, },
	{ 154,  196,  196,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  137,  169,  194,  166,  167,  154,  167,  137,  182, },
	{ 154,  196,  167,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  122,  169,  208,  166,  167,  154,  152,  167,  182, },
};

unsigned char initValues_coeff_abs_level_greater2_flag[3][6] =
{
	{ 138,  153,  136,  167,  152,  152, },
	{ 107,  167,   91,  122,  107,  167, },
	{ 107,  167,   91,  107,  107,  167, },
};








//table 9-40
unsigned char rangeTabLps[64][4] =
{
  { 128, 176, 208, 240},
  { 128, 167, 197, 227},
  { 128, 158, 187, 216},
  { 123, 150, 178, 205},
  { 116, 142, 169, 195},
  { 111, 135, 160, 185},
  { 105, 128, 152, 175},
  { 100, 122, 144, 166},
  {  95, 116, 137, 158},
  {  90, 110, 130, 150},
  {  85, 104, 123, 142},
  {  81,  99, 117, 135},
  {  77,  94, 111, 128},
  {  73,  89, 105, 122},
  {  69,  85, 100, 116},
  {  66,  80,  95, 110},
  {  62,  76,  90, 104},
  {  59,  72,  86,  99},
  {  56,  69,  81,  94},
  {  53,  65,  77,  89},
  {  51,  62,  73,  85},
  {  48,  59,  69,  80},
  {  46,  56,  66,  76},
  {  43,  53,  63,  72},
  {  41,  50,  59,  69},
  {  39,  48,  56,  65},
  {  37,  45,  54,  62},
  {  35,  43,  51,  59},
  {  33,  41,  48,  56},
  {  32,  39,  46,  53},
  {  30,  37,  43,  50},
  {  29,  35,  41,  48},
  {  27,  33,  39,  45},
  {  26,  31,  37,  43},
  {  24,  30,  35,  41},
  {  23,  28,  33,  39},
  {  22,  27,  32,  37},
  {  21,  26,  30,  35},
  {  20,  24,  29,  33},
  {  19,  23,  27,  31},
  {  18,  22,  26,  30},
  {  17,  21,  25,  28},
  {  16,  20,  23,  27},
  {  15,  19,  22,  25},
  {  14,  18,  21,  24},
  {  14,  17,  20,  23},
  {  13,  16,  19,  22},
  {  12,  15,  18,  21},
  {  12,  14,  17,  20},
  {  11,  14,  16,  19},
  {  11,  13,  15,  18},
  {  10,  12,  15,  17},
  {  10,  12,  14,  16},
  {   9,  11,  13,  15},
  {   9,  11,  12,  14},
  {   8,  10,  12,  14},
  {   8,   9,  11,  13},
  {   7,   9,  11,  12},
  {   7,   9,  10,  12},
  {   7,   8,  10,  11},
  {   6,   8,   9,  11},
  {   6,   7,   9,  10},
  {   6,   7,   8,   9},
  {   2,   2,   2,   2}
};


//table 9-41
unsigned char transIdxMps[64] =
{
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
  33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63,
};

unsigned char transIdxLps[64] =
{
  0, 0, 1, 2, 2, 4, 4, 5, 6, 7,  8,  9,  9,  11, 11, 12,
  13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
  24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
  33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63,
};

//9.3.2.2 Initialization process for context variables
void init_context_variables(struct context_model *cm, int SliceQpY, int initValue)
{
	int slopeIdx = initValue >> 4;
	int offsetIdx = initValue & 15;
	int m = slopeIdx * 5 - 45;
	int n = (offsetIdx << 3) - 16;
	int preCtxState = Clip3(1, 126, ((m * Clip3(0, 51, SliceQpY)) >> 4) + n);
	cm->valMps =  (preCtxState <= 63) ? 0 : 1;
	cm->pStateIdx = cm->valMps ? (preCtxState - 64) : (63 - preCtxState);
	
}
int cm_index_count = 0;
void init_context_variables2(struct context_model *cm, int SliceQpY, int initValue, int initType,char *desc)
{
	int slopeIdx = initValue >> 4;
	int offsetIdx = initValue & 15;
	int m = slopeIdx * 5 - 45;
	int n = (offsetIdx << 3) - 16;
	printf("                    {2'd%d, 8'd%d} : {m,n} = {8'd%u, 8'd%u}; //%s\n", initType, cm_index_count/3,(unsigned char)m,(unsigned char)n,desc);
	int preCtxState = Clip3(1, 126, ((m * Clip3(0, 51, SliceQpY)) >> 4) + n);
	cm->valMps = (preCtxState <= 63) ? 0 : 1;
	cm->pStateIdx = cm->valMps ? (preCtxState - 64) : (63 - preCtxState);
	cm_index_count++;
}

void parse_sao_merge(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &sao_merge_cm);
	Log("parse_sao_merge return %d\n",*synVal);
}

//sao_type_idx_luma	0	bypass	na	na	na	na
//sao_type_idx_chroma	0	bypass	na	na	na	na
void parse_sao_type_idx(unsigned char *rbsp, int *synVal)
{
	int binVal;
	decode_bin(rbsp, &binVal, &sao_type_idx_cm);
	//5是编码成111110
	if (binVal == 0) {
		*synVal = 0;
	} else {
		bypass_decode_bin(rbsp, &binVal);
		if (binVal == 0)
			*synVal = 1;
		else
			*synVal = 2;
	}
	Log("parse_sao_type_idx return %d\n",*synVal);
}

//sao_offset_abs[ ][ ][ ][ ]	TR	cMax = (1 << (Min(bitDepth, 10) - 5)) - 1, cRiceParam = 0
void parse_sao_tr(unsigned char *rbsp, int *synVal, int cMax)
{
	int binVal;
	int i;
	bypass_decode_bin(rbsp, &binVal);
	if (binVal == 0)
	{
		*synVal = 0;
		Log("parse_sao_tr return 0\n");
		return;
	}
	i = 1;
	while (1)
	{
		bypass_decode_bin(rbsp, &binVal);
		if (binVal == 0)
		{
			break;
		}
		i++;
		if (i == cMax) 
		{
			break;
		}
	}

	*synVal = i;
	Log("parse_sao_tr return %d\n",*synVal);
}

void parse_mpm_idx(unsigned char *rbsp, int *synVal)
{
	int binVal;
	bypass_decode_bin(rbsp, &binVal);
	if (binVal == 0)
	{
		*synVal = 0;
		Log("parse_mpm_idx return 0\n");
		return;
	}
	bypass_decode_bin(rbsp, &binVal);
	if (binVal == 0) {
		*synVal = 1;
	} else {
		*synVal = 2;
	}
	Log("parse_mpm_idx return %d\n", *synVal);
}



//9.3.2.5 Initialization process for the arithmetic decoding engine 
//ffmpeg cabac_init_state
void init_cabac(unsigned char *rbsp, int SliceQpY,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head)
{
	int j;
	int initType;
	ivlCurrRange = 510;
	ivlOffset = read_bits(rbsp, 9);
	if (slice_head->slice_type == I_SLICE)
		initType = 0;
	else if (slice_head->slice_type == P_SLICE) {
		if (pps->cabac_init_present_flag && slice_head->cabac_init_flag) {
			initType = 2;
		} else {
			initType = 1;
		}
	} else {
		assert(0);
	}

	init_context_variables(&sao_merge_cm, SliceQpY, initValues_sao_merge_flag[initType]);

	init_context_variables(&cu_transquant_bypass_flag_cm, SliceQpY, initValues_cu_transquant_bypass_flag[initType]);

	for (j = 0; j < 3; j++)
		init_context_variables(&split_cu_flag_cm[j], SliceQpY, initValues_split_cu_flag[initType][j]);

	for (j = 0; j < 3; j++)
		init_context_variables(&cu_skip_flag_cm[j], SliceQpY, initValues_cu_skip_flag[initType][j]);

	init_context_variables(&merge_flag_cm, SliceQpY, initValues_merge_flag[initType]);

	init_context_variables(&merge_idx_cm, SliceQpY, initValues_merge_idx_ext[initType]);

	for (j = 0; j < 4; j++)
		init_context_variables(&part_mode_cm[j], SliceQpY, initValues_part_mode[initType][j]);

	init_context_variables(&pred_mode_cm, SliceQpY, initValues_pred_mode[initType]);
	init_context_variables(&prev_intra_luma_pred_flag_cm, SliceQpY, initValues_prev_intra_luma_pred_flag[initType]);

	init_context_variables(&intra_chroma_pred_mode_cm, SliceQpY, initValues_intra_chroma_pred_mode[initType]);

	for (j = 0; j < 5; j++)
		init_context_variables(&inter_dir_cm[j], SliceQpY, initValues_inter_dir[initType][j]);

	for (j = 0; j < 2; j++)
		init_context_variables(&mvd_cm[j], SliceQpY, initValues_mvd[initType][j]);

	for (j = 0; j < 2; j++)
		init_context_variables(&ref_pic_cm[j], SliceQpY, initValues_ref_pic[initType][j]);


	for (j = 0; j < 2; j++)
		init_context_variables(&dqp_cm[j], SliceQpY, initValues_dqp[initType][j]);
	for (j = 0; j < 4; j++)
		init_context_variables(&qt_cbf_cb_cr_cm[j], SliceQpY, initValues_qt_cbf_cb_cr[initType][j]);
	for (j = 0; j < 2; j++)
		init_context_variables(&qt_cbf_luma_cm[j], SliceQpY, initValues_qt_cbf_luma[initType][j]);

	init_context_variables(&qt_root_cbf_cm, SliceQpY, initValues_qt_root_cbf[initType]);
	for (j = 0; j < 18; j++)
		init_context_variables(&last_sig_coeff_x_prefix_cm[j], SliceQpY, initValues_last[initType][j]);
	for (j = 0; j < 18; j++)
		init_context_variables(&last_sig_coeff_y_prefix_cm[j], SliceQpY, initValues_last[initType][j]);

	for (j = 0; j < 4; j++)
		init_context_variables(&coded_sub_block_flag_cm[j], SliceQpY, initValues_coded_sub_block_flag[initType][j]);
	for (j = 0; j < 42; j++)
		init_context_variables(&sig_flag_cm[j], SliceQpY, initValues_sig_flag[initType][j]);
	for (j = 0; j < 24; j++)
		init_context_variables(&coeff_abs_level_greater1_flag_cm[j], SliceQpY, initValues_coeff_abs_level_greater1_flag[initType][j]);
	for (j = 0; j < 6; j++)
		init_context_variables(&coeff_abs_level_greater2_flag_cm[j], SliceQpY, initValues_coeff_abs_level_greater2_flag[initType][j]);

	init_context_variables(&mvp_idx_cm, SliceQpY, initValues_mvp_idx[initType]);

	init_context_variables(&sao_type_idx_cm, SliceQpY, initValues_sao_type_idx[initType]);
	for (j = 0; j < 3; j++)
		init_context_variables(&trans_subdiv_flag_cm[j], SliceQpY, initValues_trans_subdiv_flag[initType][j]);
	for (j = 0; j < 2; j++)
		init_context_variables(&transform_skip_flag_cm[j], SliceQpY, initValues_transform_skip_flag[initType][j]);

}




void decode_bin(unsigned char *rbsp, int *binVal, struct context_model *cm)
{
	//9.3.4.3.2  Arithmetic decoding process for a binary decision
	int ivlLpsRange, qRangeIdx;
	qRangeIdx = (ivlCurrRange >> 6) & 3;
	ivlLpsRange = rangeTabLps[cm->pStateIdx][qRangeIdx];

	if (ivlLpsRange == 166 && cm->pStateIdx == 7 && ivlCurrRange == 510 && ivlOffset == 502)
		int a = 1;
	ivlCurrRange = ivlCurrRange - ivlLpsRange;
	if (ivlOffset >= ivlCurrRange) {
		//LPS path
		*binVal = 1 - cm->valMps;
		ivlOffset -= ivlCurrRange;
		ivlCurrRange = ivlLpsRange;

	} else {
		//MPS path
		*binVal = cm->valMps;

	}
	//9.3.4.3.2.2  State transition process 
	if(*binVal == cm->valMps){ 
		cm->pStateIdx = transIdxMps[cm->pStateIdx];
	} else {
		if (cm->pStateIdx == 0) 
			cm->valMps = 1 - cm->valMps;

		cm->pStateIdx = transIdxLps[cm->pStateIdx];
	}
	//renorm
	while (ivlCurrRange < 256) {
		ivlCurrRange = ivlCurrRange << 1;
		ivlOffset = ivlOffset << 1;
		ivlOffset = ivlOffset | read_one_bit(rbsp);
	}

}

//9.3.4.3.4	Bypass decoding process for binary decisions
void bypass_decode_bin(unsigned char *rbsp, int *binVal)
{

	ivlOffset = (ivlOffset << 1);
	ivlOffset = ivlOffset | read_one_bit(rbsp);
	if (ivlOffset >= ivlCurrRange) {


		*binVal = 1;
		ivlOffset = ivlOffset - ivlCurrRange;

	} else {
		*binVal = 0;
	}
	
}

//fixedLength = Ceil( Log2( cMax + 1 ) ). cMax=31, 2^5=32,fixedLength=5
//这个函数不是按照11110,1110来解，2bit，00=0，01=1，10=2，11=3
void bypass_decode_bins(unsigned char *rbsp, int *synVal, int fixedLength)
{
	int i;
	int binVal;
	*synVal = 0;
	for (i = 0; i < fixedLength; i++) {
		*synVal = (*synVal) << 1;
		bypass_decode_bin(rbsp, &binVal);
		*synVal += binVal;
	}
}

void terminate_decode_bin(unsigned char *rbsp, int *synVal)
{
	ivlCurrRange -= 2;
	if (ivlOffset >= ivlCurrRange)
		*synVal = 1;
	else {
		*synVal = 0;
		//renorm
		while (ivlCurrRange < 256) {
			ivlCurrRange = ivlCurrRange << 1;
			ivlOffset = ivlOffset << 1;
			ivlOffset = ivlOffset | read_one_bit(rbsp);
		}
	}
}

//TABLE 9-38
//Syntax element		condL					condA					ctxInc
//split_cu_flag[ x0 ][ y0 ]	CtDepth[ xNbL ][ yNbL ] > cqtDepth	CtDepth[ xNbA ][ yNbA ] > cqtDepth	( condL  &&  availableL ) + ( condA  &&  availableA )
//根据左边和上面的PU是否分割，来推断当前cu
void parse_split_cu_flag(unsigned char *rbsp, int *split_cu_flag, int x0, int y0, int log2CbSize, int cqtDepth)
{
	int ctxInc;
	int condL = 0, condA = 0, availableL = 0, availableA = 0;
	int synVal;
	if (x0 == 0)
		availableL = 0;
	else {
		availableL = 1;
		condL = CtDepth[y0>>MinCbLog2SizeY][(x0-1)>>MinCbLog2SizeY] > cqtDepth ? 1 : 0;
	}

	if (y0 == 0)
		availableA = 0;
	else {
		availableA = 1;
		condA = CtDepth[(y0-1)>>MinCbLog2SizeY][x0>>MinCbLog2SizeY] > cqtDepth ? 1 : 0;
	}
	ctxInc = (condL && availableL) + (condA && availableA);
	decode_bin(rbsp, &synVal, &split_cu_flag_cm[ctxInc]);
	*split_cu_flag = synVal;
	Log("parse_split_cu_flag return %d ctxInc %d\n", synVal, ctxInc);

}

void parse_cu_transquant_bypass_flag(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &cu_transquant_bypass_flag_cm);
	Log("parse_cu_transquant_bypass_flag ret %d\n",*synVal);
}


//TABLE 9-38
//Syntax element		condL				condA				ctxInc
//cu_skip_flag[x0][y0]	cu_skip_flag[xNbL][yNbL]	cu_skip_flag[xNbA][yNbA]	(condL && availableL) + (condA && availableA)
//根据左边和上面的PU是否分割，来推断当前cu
void parse_cu_skip_flag(unsigned char *rbsp, int x0, int y0, int log2CbSize)
{
	int ctxInc;
	int condL = 0, condA = 0, availableL = 0, availableA = 0;
	int synVal;
	int x, y;
	if (x0 == 0)
		availableL = 0;
	else {
		availableL = 1;
		condL = cu_skip_flag[y0>>MinCbLog2SizeY][(x0-1)>>MinCbLog2SizeY];
	}

	if (y0 == 0)
		availableA = 0;
	else {
		availableA = 1;
		condA = cu_skip_flag[(y0-1) >> MinCbLog2SizeY][x0>>MinCbLog2SizeY];
	}
	ctxInc = (condL && availableL) + (condA && availableA);
	decode_bin(rbsp, &synVal, &cu_skip_flag_cm[ctxInc]);
	Log("parse_cu_skip_flag ret %d ctxInc %d\n",synVal, ctxInc);
	for (x = x0;
		(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
		x += (1 << MinCbLog2SizeY)) {
		for (y = y0;
			(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
			y += (1 << MinCbLog2SizeY)) {
			cu_skip_flag[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = synVal;
		}
	}
}

void parse_pred_mode_flag(unsigned char *rbsp, int *synVal, int x0, int y0, int log2CbSize)
{
	int x, y;
	decode_bin(rbsp, synVal, &pred_mode_cm);
	for (x = x0;
		(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
		x += (1 << MinCbLog2SizeY)) {
		for (y = y0;
			(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
			y += (1 << MinCbLog2SizeY)) {
			CurrPic->cu_predmode[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = *synVal;
		}
	}
	Log("parse_pred_mode_flag ret %d\n", *synVal);
}


void parse_part_mode(unsigned char *rbsp, int *part_mode, int x0, int y0, int log2CbSize,
	struct seq_parameter_set *sps)
{
	int synVal;
	*part_mode = 0;
	//table 9-34 log2CbSize > MinCbLog2SizeY 必定是NxN
	if (CurrPic->cu_predmode[y0 >> MinCbLog2SizeY][x0 >> MinCbLog2SizeY] == MODE_INTRA) {
		synVal = 1;
		if (log2CbSize == MinCbLog2SizeY) {
			decode_bin(rbsp, &synVal, &part_mode_cm[0]);
		}
		if (synVal == 1)
			*part_mode = PART_2Nx2N;
		else
			*part_mode = PART_NxN; //MODE_INTRA下part_mode cabac值=1，这里返回PART_NxN=3
	} else {
		//table 9-34 最后一列log2CbSize > 3，max_bits=3,
		//第二列，amp_enabled_flag=1的情况，先解析2bit，如果是1，结束只可能是2Nx2N
		//如果01，00，继续解析2bit
		int max_bits = 2;
		int i;
		if ((log2CbSize == MinCbLog2SizeY) && (log2CbSize > 3))
			max_bits++; //第二列amp_enabled_flag=1的情况，也先解析2bit，不解3bit
		for (i = 0; i < max_bits; i++) {
			decode_bin(rbsp, &synVal, &part_mode_cm[i]);
			if (synVal == 1)
				break;
			(*part_mode)++; //fix *part_mode++
		}
		if (sps->amp_enabled_flag && log2CbSize > MinCbLog2SizeY) {
			if (*part_mode == 1) {//01还有011，0101，0100这几种情况
				decode_bin(rbsp, &synVal, &part_mode_cm[3]); //fix part_mode_cm[2] ->part_mode_cm[3]
				if (synVal == 0) {
					bypass_decode_bin(rbsp, &synVal);
					if (synVal == 0)
						*part_mode = PART_2NxnU;//0100
					else
						*part_mode = PART_2NxnD;//0101
				} else {
					*part_mode = PART_2NxN;//011
				}
			} else if (*part_mode == 2) {//00还有001，0000，0001这几种情况
				decode_bin(rbsp, &synVal, &part_mode_cm[3]);//fix part_mode_cm[2] ->part_mode_cm[3]
				if (synVal == 0) {
					bypass_decode_bin(rbsp, &synVal);
					if (synVal == 0)
						*part_mode = PART_nLx2N;//0000
					else
						*part_mode = PART_nRx2N; //0001
				} else
					*part_mode = PART_Nx2N;//001
			}
		}
	}
	Log("parse_part_mode ret %d\n", *part_mode);

}

//Table 9 35 – Binarization for intra_chroma_pred_mode
//Value of intra_chroma_pred_mode		Bin string
//4					0
//0					100  value即1后面的2bit的值
//1					101
//2					110
//3					111

void parse_intra_chroma_pred_mode(unsigned char *rbsp, int x0, int y0, int *IntraPredModeC, int IntraPredModeY)
{
	int binVal;
	int symbol = 0;
	int mode_list[4] = {INTRA_PLANAR, INTRA_VER, INTRA_HOR, INTRA_DC};

	decode_bin(rbsp, &binVal, &intra_chroma_pred_mode_cm);

	if (binVal == 0) {
		symbol = 4; //todo
	} else {
		bypass_decode_bins(rbsp, &symbol, 2);
	}
	//8.4.3	Derivation process for chroma intra prediction mode
	//4:2:0 chroma_format_idc=1
	if (symbol != 4) {
		if (IntraPredModeY == mode_list[symbol])
			*IntraPredModeC = 34;
		else
			*IntraPredModeC = mode_list[symbol];
	}
	else {
		*IntraPredModeC = IntraPredModeY;
	}
	Log("parse_intra_chroma_pred_mode symbol %d value %d\n", symbol, *IntraPredModeC);
}

//intra_chroma_pred_mode[ xCb ][ yCb ]	IntraPredModeY[ xCb ][ yCb ]
//					0	26	10	1	X ( 0  <=  X  <=  34 )
//0					34	0	0	0	0
//1					26	34	26	26	26
//2					10	10	34	10	10
//3					1	1	1	34	1
//4					0	26	10	1	X
//cabac的值是4，Y是啥，chroma就是啥,其余，0=0，1=26，2=10，3=1，但是Y是0，26，10，1时例外，需要把其中一值改成34
//TR	cMax = MaxNumMergeCand - 1, cRiceParam = 0
void parse_merge_idx(unsigned char *rbsp, int *synVal, int MaxNumMergeCand)
{
	int binVal = 0;
	int i = 0;
	for (i = 0; i < MaxNumMergeCand - 1; i++) {
		if (i == 0)
			decode_bin(rbsp, &binVal, &merge_idx_cm);
		else
			bypass_decode_bin(rbsp, &binVal);
		if (binVal == 0) //1111 break执行不到，出for循环i=4
			break;
	}
	*synVal = i;
	Log("parse_merge_idx ret %d\n",i);
}

void parse_merge_flag(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &merge_flag_cm);
	Log("parse_merge_flag ret %d\n",*synVal);
}

void parse_rqt_root_cbf(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &qt_root_cbf_cm);
	Log("parse_rqt_root_cbf ret %d\n",*synVal);
}

//TR cMax = num_ref_idx_l0_active_minus1, cRiceParam = 0
void parse_ref_idx_l0(unsigned char *rbsp, int *synVal, int num_ref_idx_l0_active_minus1)
{
	int i = 0;
	int binVal;
	for (i = 0; i < num_ref_idx_l0_active_minus1; i++) {
		if (i < 2) {
			decode_bin(rbsp, &binVal, &ref_pic_cm[i]);
		} else {
			bypass_decode_bin(rbsp, &binVal);
		}
		if (binVal == 0)
			break;
	}
	*synVal = i;
	Log("parse_ref_idx_l0 ret %d\n",i);
}

void parse_mvp_l0_flag(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &mvp_idx_cm);
	Log("parse_mvp_l0_flag ret %d\n",*synVal);
}

void parse_mvd_coding(unsigned char *rbsp, struct Mv *mvd)
{
	int abs_mvd_greater0_flag[2]; //x,y轴
	int abs_mvd_greater1_flag[2];
	int abs_mvd_minus2[2];
	int mvd_sign_flag[2];
	decode_bin(rbsp, &abs_mvd_greater0_flag[0], &mvd_cm[0]);
	Log("abs_mvd_greater0_flag %d\n", abs_mvd_greater0_flag[0]);
	decode_bin(rbsp, &abs_mvd_greater0_flag[1], &mvd_cm[0]);
	Log("abs_mvd_greater0_flag %d\n", abs_mvd_greater0_flag[1]);
	if (abs_mvd_greater0_flag[0]) {
		decode_bin(rbsp, &abs_mvd_greater1_flag[0], &mvd_cm[1]);
		Log("abs_mvd_greater1_flag %d\n", abs_mvd_greater1_flag[0]);
	}
	if (abs_mvd_greater0_flag[1]) {
		decode_bin(rbsp, &abs_mvd_greater1_flag[1], &mvd_cm[1]);
		Log("abs_mvd_greater1_flag %d\n", abs_mvd_greater1_flag[1]);
	}
	if (abs_mvd_greater0_flag[0]) {
		if (abs_mvd_greater1_flag[0]) {
			parse_abs_mvd_minus2(rbsp, &abs_mvd_minus2[0]);
			//Log("abs_mvd_minus2 %d\n",abs_mvd_minus2[0]);
		}
		bypass_decode_bin(rbsp, &mvd_sign_flag[0]);
		//Log("mvd_sign_flag %d\n", mvd_sign_flag[0]);
		if (abs_mvd_greater1_flag[0])
			mvd->mv[0] = (abs_mvd_minus2[0] + 2 ) * (1 - 2 * mvd_sign_flag[0]);
		else
			mvd->mv[0] = 1 * (1 - 2 * mvd_sign_flag[0]);
	} else {
		mvd->mv[0] = 0;
	}
	if (abs_mvd_greater0_flag[1]) {
		if (abs_mvd_greater1_flag[1]) {
			parse_abs_mvd_minus2(rbsp, &abs_mvd_minus2[1]);
			//Log("abs_mvd_minus2 %d\n",abs_mvd_minus2[1]);
		}
		bypass_decode_bin(rbsp, &mvd_sign_flag[1]);
		//Log("mvd_sign_flag %d\n", mvd_sign_flag[1]);
		if (abs_mvd_greater1_flag[1])
			mvd->mv[1] = (abs_mvd_minus2[1] + 2 ) * (1 - 2 * mvd_sign_flag[1]);
		else
			mvd->mv[1] = 1 * (1 - 2 * mvd_sign_flag[1]);
	} else {
		mvd->mv[1] = 0;
	}
	Log("mvx %d mvy %d\n", mvd->mv[0], mvd->mv[1]);
}

void parse_abs_mvd_minus2(unsigned char *rbsp, int *synVal)
{
	//EG1
	int ret = 0;
	int k = 1;
	int binVal = 0;

	while (k < CABAC_MAX_BIN) {
		bypass_decode_bin(rbsp, &binVal);
		if (binVal) {
			ret += 1U << k;
			k++;
		} else
			break;
	}

	if (k == CABAC_MAX_BIN) {
		printf("parse_abs_mvd_minus2 CABAC_MAX_BIN : %d\n", k);
	}
	while (k--) {
		bypass_decode_bin(rbsp, &binVal);
		ret += binVal << k;
	}
	*synVal = ret;
	Log("parse_abs_mvd_minus2 ret %d\n", ret);
}

void parse_split_transform_flag(unsigned char *rbsp, int *synVal, int log2TrafoSize)
{
	decode_bin(rbsp, synVal, &trans_subdiv_flag_cm[5 - log2TrafoSize]);
	Log("parse_split_transform_flag ret %d\n", *synVal);
}

void parse_cbf_cb_cr(unsigned char *rbsp, int *synVal, int trafoDepth)
{
	decode_bin(rbsp, synVal, &qt_cbf_cb_cr_cm[trafoDepth]);
	Log("parse_cbf_cb_cr ret %d\n", *synVal);
}

void parse_cbf_luma(unsigned char *rbsp, int *synVal, int trafoDepth)
{
	decode_bin(rbsp, synVal, &qt_cbf_luma_cm[trafoDepth == 0 ? 1 : 0]);
	Log("parse_cbf_luma ret %d\n", *synVal);
}

void parse_cu_qp_delta_abs(unsigned char *rbsp, int *synVal)
{
	int prefixVal = 0;
	int suffixVal = 0;
	int ctxInc = 0;
	int binVal;

	while (prefixVal < 5) {
		decode_bin(rbsp, &binVal, &dqp_cm[ctxInc]);
		if (binVal) {
			prefixVal++;
			ctxInc = 1;
		} else
			break;
	}
	if (prefixVal >= 5) { //5以后用bypass
		int k = 0;
		while (k < CABAC_MAX_BIN) {
			bypass_decode_bin(rbsp, &binVal);
			if (binVal) {
				suffixVal += 1 << k;
				k++;
			} else
				break;
		}
		if (k == CABAC_MAX_BIN)
			printf("parse_cu_qp_delta_abs CABAC_MAX_BIN : %d\n", k);
		while (k--) {
			bypass_decode_bin(rbsp, &binVal);
			suffixVal += binVal << k;
		}
	}
	*synVal = prefixVal + suffixVal;
	Log("parse_cu_qp_delta_abs prefix %d suffix %d\n", prefixVal, suffixVal);
}

//transform_skip_flag[x0][y0][cIdx] specifies whether a transform is applied to the associated transform block or not: 
//The array index cIdx specifies an indicator for the colour component; it is equal to 0 for luma, equal to 1 for Cb and equal to 2 for Cr.
void parse_transform_skip_flag(unsigned char *rbsp, int *synVal, int cIdx)
{
	decode_bin(rbsp, synVal, &transform_skip_flag_cm[!!cIdx]);
	Log("parse_transform_skip_flag ret %d cIdx %d\n", *synVal, cIdx);
}

//TR	cMax = (log2TrafoSize << 1) - 1, cRiceParam = 0
void parse_last_sig_coeff_x_prefix(unsigned char *rbsp, int *synVal, int log2TrafoSize, int cIdx)
{
	int i = 0;
	int cMax = (log2TrafoSize << 1) - 1;
	int ctxOffset, ctxShift;
	int ctxIdx;
	int binVal;

	if (cIdx == 0) { //9.3.4.2.3
		ctxOffset = 3 * (log2TrafoSize - 2)  + ((log2TrafoSize - 1) >> 2);
		ctxShift = (log2TrafoSize + 1) >> 2;
	} else {
		ctxOffset = 15;
		ctxShift = log2TrafoSize - 2;
	}
	while (i < cMax) {
		ctxIdx = (i >> ctxShift) + ctxOffset;
		decode_bin(rbsp, &binVal, &last_sig_coeff_x_prefix_cm[ctxIdx]);
		if (binVal)
			i++;
		else
			break;
	}

	*synVal = i;
	Log("parse_last_sig_coeff_x_prefix %d\n", i);
}

void parse_last_sig_coeff_y_prefix(unsigned char *rbsp, int *synVal, int log2TrafoSize, int cIdx)
{
	int i = 0;
	int cMax = (log2TrafoSize << 1) - 1;
	int ctxOffset, ctxShift;
	int ctxIdx;
	int binVal;

	if (cIdx == 0) { //9.3.4.2.3
		ctxOffset = 3 * (log2TrafoSize - 2)  + ((log2TrafoSize - 1) >> 2);
		ctxShift = (log2TrafoSize + 1) >> 2;
	} else {
		ctxOffset = 15;
		ctxShift = log2TrafoSize - 2;
	}
	while (i < cMax) {
		ctxIdx = (i >> ctxShift) + ctxOffset;
		decode_bin(rbsp, &binVal, &last_sig_coeff_y_prefix_cm[ctxIdx]);
		if (binVal)
			i++;
		else
			break;
	}

	*synVal = i;
	Log("parse_last_sig_coeff_y_prefix %d\n", i);
}

//FL	cMax = (1 << ((last_sig_coeff_x_prefix >> 1 ) - 1) - 1)
void parse_last_sig_coeff_suffix(unsigned char *rbsp, int *synVal, int last_sig_coeff_prefix)
{
	int i;
	int binVal;
	int length = (last_sig_coeff_prefix >> 1) - 1;
	bypass_decode_bin(rbsp, synVal);

	for (i = 1; i < length; i++) {
		bypass_decode_bin(rbsp, &binVal);
		*synVal = (*synVal << 1) | binVal;
	}
	Log("parse_last_sig_coeff_suffix %d\n", *synVal);
}

//colour component index cIdx
void parse_coded_sub_block_flag(unsigned char *rbsp, int *synVal, int cIdx, int csbfCtx)
{
	int ctxIdx;
	if (cIdx == 0)
		ctxIdx = min(csbfCtx, 1);
	else
		ctxIdx = 2 + min(csbfCtx, 1);

	decode_bin(rbsp, synVal, &coded_sub_block_flag_cm[ctxIdx]);
	Log("parse_coded_sub_block_flag %d cIdx %d ctxIdx %d\n", *synVal, cIdx, ctxIdx);
}

void parse_sig_coeff_flag(unsigned char *rbsp, int *synVal, int cIdx,
	int prevCsbf, int xC, int yC, int xS, int yS, int scanIdx, int log2TrafoSize)
{
	int sigCtx, ctxInc;
	int xP, yP;
	static int ctxIdxMap[] = {
		0, 1, 4, 5, 2, 3, 4, 5, 6, 6, 8, 8, 7, 7, 8,
	};
	if (xC == 3 && yC == 7)
		int a = 1;
	//transform_skip_context_enabled_flag在pps range extension
	if (log2TrafoSize == 2)
		sigCtx = ctxIdxMap[(yC << 2) + xC];
	else if (xC + yC == 0)
		sigCtx = 0;
	else {
		xP = xC & 0x3;
		yP = yC & 0x3;
		if (prevCsbf == 0)
			sigCtx = (xP + yP == 0) ? 2 : (xP + yP < 3) ? 1 : 0;
		else if (prevCsbf == 1)
			sigCtx = (yP == 0) ? 2 : (yP == 1) ? 1 : 0;
		else if (prevCsbf == 2)
			sigCtx = (xP == 0) ? 2 : (xP == 1) ? 1 : 0;
		else
			sigCtx = 2;
		if (cIdx == 0) {
			if (xS + yS > 0)
				sigCtx += 3;
			if (log2TrafoSize == 3)
				sigCtx += (scanIdx == SCAN_DIAG) ? 9 : 15;
			else
				sigCtx += 21;
		} else {
			if (log2TrafoSize == 3)
				sigCtx += 9;
			else
				sigCtx += 12;
		}
	}
	if (cIdx == 0)
		ctxInc = sigCtx;
	else
		ctxInc = 27 + sigCtx;
	decode_bin(rbsp, synVal, &sig_flag_cm[ctxInc]);
	Log("parse_sig_coeff_flag ret %d ctxInc %d\n", *synVal, ctxInc);
}

//9.3.4.2.6 有疑问，直接照搬ffmpeg的
void parse_coeff_abs_level_greater1_flag(unsigned char *rbsp, int *synVal, int cIdx, int ctxInc)
{
	if (cIdx > 0)
		ctxInc = ctxInc + 16;
	decode_bin(rbsp, synVal, &coeff_abs_level_greater1_flag_cm[ctxInc]);
	Log("parse_coeff_abs_level_greater1_flag ret %d ctxInc %d cIdx %d\n", *synVal, ctxInc, cIdx);
}

void parse_coeff_abs_level_greater2_flag(unsigned char *rbsp, int *synVal, int cIdx, int ctxInc)
{
	if (cIdx > 0)
		ctxInc = ctxInc + 4;
	decode_bin(rbsp, synVal, &coeff_abs_level_greater2_flag_cm[ctxInc]);
	Log("parse_coeff_abs_level_greater2_flag ret %d ctxInc %d cIdx %d\n", *synVal, ctxInc, cIdx);
}

// ffmpeg与spec对不起来，先全部照抄ffmpeg的
void parse_coeff_abs_level_remaining(unsigned char *rbsp, int *synVal, int cRiceParam)
{
	int prefixVal = 0;
	int suffixVal = 0;
	int i;
	int binVal;

	while (prefixVal < CABAC_MAX_BIN) {
		bypass_decode_bin(rbsp, &binVal);
		if (binVal)
			prefixVal++;
		else
			break;
	}

	if (prefixVal == CABAC_MAX_BIN) {
		printf("coeff_abs_level_remaining_decode CABAC_MAX_BIN : %d\n", prefixVal);
	}
	if (prefixVal < 3) {
		for (i = 0; i < cRiceParam; i++) {
			bypass_decode_bin(rbsp, &binVal);
			suffixVal = (suffixVal << 1) | binVal;
		}
		*synVal = (prefixVal << cRiceParam) + suffixVal;
	} else {
		int prefix_minus3 = prefixVal - 3;
		for (i = 0; i < prefix_minus3 + cRiceParam; i++) {
			bypass_decode_bin(rbsp, &binVal);
			suffixVal = (suffixVal << 1) | binVal;
		}

		*synVal = (((1 << prefix_minus3) + 3 - 1) << cRiceParam) + suffixVal;
	}
	Log("parse_coeff_abs_level_remaining ret %d prefixVal %d suffixVal %d\n", *synVal, prefixVal, suffixVal);
}

void parse_prev_intra_luma_pred_flag(unsigned char *rbsp, int *synVal)
{
	decode_bin(rbsp, synVal, &prev_intra_luma_pred_flag_cm);
	Log("parse_prev_intra_luma_pred_flag ret %d\n", *synVal);
}
