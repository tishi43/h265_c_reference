// bitstream.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include "bitstream_header.h"
#include "cavlc.h"

#include <stdarg.h>
#include <string.h>


unsigned int bytes_offset = 0;
unsigned int bits_offset = 0;

struct video_parameter_set Vps;
struct pic_parameter_set Pps;
struct seq_parameter_set Sps;
struct sao_param *sao_params;

int PicWidthInSamplesY, PicHeightInSamplesY;
int MinCbLog2SizeY;
int MinCbSizeY;
int Log2MaxTrafoSize;
int Log2MinTrafoSize;
int MaxTrafoDepth;
int Log2MinCuQpDeltaSize;

int CtbLog2SizeY;
int CtbSizeY;
int PicWidthInCtbsY, PicHeightInCtbsY; //Y是YUV的Y，luma
int PicSizeInCtbsY;
int SliceQpY;
int **CtDepth;
int **cu_skip_flag;
int **qp_y_tab;
int **verBs, **horBs;
int **CbfLuma;
int **no_filter;
unsigned char **SaoBufL, **SaoBufCb, **SaoBufCr;

int QpY_PREV;
int QpY;

int **intra_luma_pred_mode;


int IsCuQpDeltaCoded;
int CuQpDeltaVal;

int PrevPOC = 0;
struct reference_picture_set SliceLocalRPS;

struct HEVCFrame *CurrPic;
struct HEVCFrame *DPB[32];

int **TransIntermediate;
int slice_num = 0;

int **predSamplesL;
int **predSamplesCb;
int **predSamplesCr;
int **resSamplesL;
int **resSamplesCb;
int **resSamplesCr;

struct CodingUnit CU;
FILE* log_info;

unsigned char Z2R[256] = {
	  0,  1, 16, 17,  2,  3, 18, 19, 32, 33, 48, 49, 34, 35, 50, 51,
	  4,  5, 20, 21,  6,  7, 22, 23, 36, 37, 52, 53, 38, 39, 54, 55,
	 64, 65, 80, 81, 66, 67, 82, 83, 96, 97,112,113, 98, 99,114,115,
	 68, 69, 84, 85, 70, 71, 86, 87,100,101,116,117,102,103,118,119,
	  8,  9, 24, 25, 10, 11, 26, 27, 40, 41, 56, 57, 42, 43, 58, 59,
	 12, 13, 28, 29, 14, 15, 30, 31, 44, 45, 60, 61, 46, 47, 62, 63,
	 72, 73, 88, 89, 74, 75, 90, 91,104,105,120,121,106,107,122,123,
	 76, 77, 92, 93, 78, 79, 94, 95,108,109,124,125,110,111,126,127,
	128,129,144,145,130,131,146,147,160,161,176,177,162,163,178,179,
	132,133,148,149,134,135,150,151,164,165,180,181,166,167,182,183,
	192,193,208,209,194,195,210,211,224,225,240,241,226,227,242,243,
	196,197,212,213,198,199,214,215,228,229,244,245,230,231,246,247,
	136,137,152,153,138,139,154,155,168,169,184,185,170,171,186,187,
	140,141,156,157,142,143,158,159,172,173,188,189,174,175,190,191,
	200,201,216,217,202,203,218,219,232,233,248,249,234,235,250,251,
	204,205,220,221,206,207,222,223,236,237,252,253,238,239,254,255,
};

unsigned char R2Z[256] = {
	  0,  1,  4,  5, 16, 17, 20, 21, 64, 65, 68, 69, 80, 81, 84, 85,
	  2,  3,  6,  7, 18, 19, 22, 23, 66, 67, 70, 71, 82, 83, 86, 87,
	  8,  9, 12, 13, 24, 25, 28, 29, 72, 73, 76, 77, 88, 89, 92, 93,
	 10, 11, 14, 15, 26, 27, 30, 31, 74, 75, 78, 79, 90, 91, 94, 95,
	 32, 33, 36, 37, 48, 49, 52, 53, 96, 97,100,101,112,113,116,117,
	 34, 35, 38, 39, 50, 51, 54, 55, 98, 99,102,103,114,115,118,119,
	 40, 41, 44, 45, 56, 57, 60, 61,104,105,108,109,120,121,124,125,
	 42, 43, 46, 47, 58, 59, 62, 63,106,107,110,111,122,123,126,127,
	128,129,132,133,144,145,148,149,192,193,196,197,208,209,212,213,
	130,131,134,135,146,147,150,151,194,195,198,199,210,211,214,215,
	136,137,140,141,152,153,156,157,200,201,204,205,216,217,220,221,
	138,139,142,143,154,155,158,159,202,203,206,207,218,219,222,223,
	160,161,164,165,176,177,180,181,224,225,228,229,240,241,244,245,
	162,163,166,167,178,179,182,183,226,227,230,231,242,243,246,247,
	168,169,172,173,184,185,188,189,232,233,236,237,248,249,252,253,
	170,171,174,175,186,187,190,191,234,235,238,239,250,251,254,255,
};
// 定义全局变量FILE* log 出现编译错误,log和<math.h>中的log函数重名了

int ScanOrder_8x8_diag_x[64] = {
	0, 0, 1, 0,
	1, 2, 0, 1,
	2, 3, 0, 1,
	2, 3, 4, 0,
	1, 2, 3, 4,
	5, 0, 1, 2,
	3, 4, 5, 6,
	0, 1, 2, 3,
	4, 5, 6, 7,
	1, 2, 3, 4,
	5, 6, 7, 2,
	3, 4, 5, 6,
	7, 3, 4, 5,
	6, 7, 4, 5,
	6, 7, 5, 6,
	7, 6, 7, 7,
};
int ScanOrder_8x8_diag_y[64] = {
	0, 1, 0, 2,
	1, 0, 3, 2,
	1, 0, 4, 3,
	2, 1, 0, 5,
	4, 3, 2, 1,
	0, 6, 5, 4,
	3, 2, 1, 0,
	7, 6, 5, 4,
	3, 2, 1, 0,
	7, 6, 5, 4,
	3, 2, 1, 7,
	6, 5, 4, 3,
	2, 7, 6, 5,
	4, 3, 7, 6,
	5, 4, 7, 6,
	5, 7, 6, 7,
};

int ScanOrder_4x4_diag_x[16] = {
	0, 0, 1, 0,
	1, 2, 0, 1,
	2, 3, 1, 2,
	3, 2, 3, 3,
};
int ScanOrder_4x4_diag_y[16] = {
	0, 1, 0, 2,
	1, 0, 3, 2,
	1, 0, 3, 2,
	1, 3, 2, 3,
};
int ScanOrder_2x2_diag_x[4] = {
	0, 0, 1, 1,
};
int ScanOrder_2x2_diag_y[4] = {
	0, 1, 0, 1,
};

int ScanOrder_4x4_horiz_x[16] = {
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
};
int ScanOrder_4x4_horiz_y[16] = {
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
};
int ScanOrder_2x2_horiz_x[4] = {
	0, 1, 0, 1,
};
int ScanOrder_2x2_horiz_y[4] = {
	0, 0, 1, 1
};

int ScanOrder_4x4_vert_x[16] = {
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
};
int ScanOrder_4x4_vert_y[16] = {
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
};
int ScanOrder_2x2_vert_x[4] = {
	0, 0, 1, 1
};
int ScanOrder_2x2_vert_y[4] = {
	0, 1, 0, 1,
};


//return 0 if last nalu in bitstream,else 1
//rbsp is nalu exclude startcode and nalu_head
//return 0 if last nalu in bitstream,else 1
//rbsp is nalu exclude startcode and nalu_head
unsigned int read_one_nalu(FILE* fin,
				int offset,
				unsigned char* stream_buffer,
				struct nal_unit_header* nalu_header,
				unsigned char* rbsp,
				unsigned int* nalu_len)
{
	int result = fseek(fin, offset, SEEK_SET);
	int i = 0;
	assert(result == 0);
	int read_len = fread((void*)stream_buffer, sizeof(char), MAX_NALU_LEN, fin);
	int start_bytes;
	while (!(stream_buffer[i + 0] == 0x0
		&& stream_buffer[i + 1] == 0x0
		&& stream_buffer[i + 2] == 0x0
		&& stream_buffer[i + 3] == 0x1 ||
		stream_buffer[i + 0] == 0x0
		&& stream_buffer[i + 1] == 0x0
		&& stream_buffer[i + 2] == 0x1)) {
		i++;
	}

	if (stream_buffer[i + 0] == 0x0
		&& stream_buffer[i + 1] == 0x0
		&& stream_buffer[i + 2] == 0x0
		&& stream_buffer[i + 3] == 0x1)
	{
		start_bytes = i + 4;
	}
	else
	{
		start_bytes = i + 3;
	}
	bytes_offset = start_bytes;
	bits_offset = 0;
	nalu_header->forbidden_zero_bit = read_one_bit(stream_buffer);
	assert(nalu_header->forbidden_zero_bit == 0);
	nalu_header->nal_unit_type = read_bits(stream_buffer, 6);
	nalu_header->nuh_layer_id = read_bits(stream_buffer, 6);
	assert(nalu_header->nuh_layer_id == 0);
	nalu_header->nuh_temporal_id_plus1 = read_bits(stream_buffer, 3) - 1;

	for (i = 2; i < read_len - 3; i++)
	{
		if (stream_buffer[i] == 0x0 
			&& stream_buffer[i+1] == 0x0
			&& stream_buffer[i+2] == 0x0
			&& stream_buffer[i+3] == 0x1 ||
			stream_buffer[i] == 0x0
			&& stream_buffer[i+1] == 0x0
			&& stream_buffer[i+2] == 0x1)
		{
			*nalu_len = i;
			EBSP2RBSP(stream_buffer+start_bytes-4, rbsp, *nalu_len+4-start_bytes);
			return 1;
		}

	}
	//otherwise there is no any more 0x00000001 in the readed bits, last nalu 
	*nalu_len = read_len;
	EBSP2RBSP(stream_buffer+start_bytes-4, rbsp, *nalu_len+4-start_bytes);
	return 0;
	
}


void EBSP2RBSP(unsigned char* nalu, unsigned char* rbsp, unsigned int nalu_len)
{
	// startcode 4bytes, nalu head 1byte
	rbsp[0] = nalu[6];
	rbsp[1] = nalu[7];
	int rbsp_len = 2;
	int i;
	
	for (i = 8; i < nalu_len; i++)
	{
		if (nalu[i-2] == 0x0
			&& nalu[i-1] == 0x0
			&& nalu[i] == 0x3)
		{
			continue; // bug fix break->continue
		}
		rbsp_len++;
		rbsp[rbsp_len-1] = nalu[i];
	}
}



unsigned int read_ue_v(unsigned char rbsp[])
{
	unsigned short leadingZerobits = 0;

	while (!try_read_one_bit(rbsp))
	{
		GotoNextBit
		leadingZerobits++;
	}
	if (leadingZerobits >=8)
		printf("wow\n");

	if (leadingZerobits)
	{
		//unsigned short val = (1 << leadingZerobits) - 1 + read_bits(rbsp, bytes_offset, bits_offset+1, leadingZerobits);
		unsigned short val = read_bits(rbsp, leadingZerobits+1)-1;
		return val;
	}
	else
	{
		GotoNextBit
		return 0;
	}
}

int read_se_v(unsigned char rbsp[])
{
	unsigned int val = read_ue_v(rbsp);
	unsigned int tmp = (val + 1) % 2;
	int neg_v;
	val = ((val + 1) >> 1);
	neg_v = val;
	neg_v = -neg_v;
	if (tmp)
	{
		return neg_v;
	}
	else
	{
		return val;
	}
}

unsigned int read_te_v(unsigned char rbsp[], unsigned int Max)
{
	unsigned int val;
	if ( Max > 1 )
	{
		val = read_ue_v(rbsp);
	}
	else
	{
		val = 1 - read_one_bit(rbsp);
	}

	return val;
}


// intra_4x4 1 inter 0
unsigned int read_me_v(unsigned char rbsp[], unsigned short intra_4x4)
{
	static unsigned int me_conversion[2][48] = {
		{0,16,1,2,4,8,32,3,5,10,12,15,47,7,11,13,14,6,9,31,35,37,42,44,
			33,34,36,40,39,43,45,46,17,18,20,24,19,21,26,28,23,27,29,30,22,25,38,41},
		{47,31,15,0,23,27,29,30,7,11,13,14,39,43,45,46,16,3,5,10,12,19,21,26,
		28,35,37,42,44,1,2,4,8,17,18,20,24,6,9,22,25,32,33,34,36,40,38,41}
	};

	unsigned int val = read_ue_v(rbsp);
	assert( val >= 0 && val <= 47 );
	val = me_conversion[intra_4x4][val];
	return val;
}

// read from bytes_offset+1 bytes, read from high bit in a byte
    // 比如0x67,(memory high bit)0110 0111(memory low bit) 最高位bit index为0 最低位为7,也就是读的顺序是从高位向低位读,
unsigned int read_one_bit(unsigned char rbsp[])
{
	unsigned int val;
	assert(bits_offset >= 0 && bits_offset <= 7);
	val = rbsp[bytes_offset] >> (7 - bits_offset) & 0x1;
	GotoNextBit
	return val;
}

unsigned int try_read_one_bit(unsigned char rbsp[])
{
	unsigned int val;
	assert(bits_offset >= 0 && bits_offset <= 7);
	val = rbsp[bytes_offset] >> (7 - bits_offset) & 0x1;
	return val;
}

unsigned int read_bits(unsigned char rbsp[], unsigned int len)
{
	assert(bits_offset >= 0 && bits_offset <= 7);
	unsigned int val = 0; // set all the bit to 0
	int i;
	for (i = 0; i < len; i++)
	{		
		if (read_one_bit(rbsp))
		{
			val = val | (1 << (len - 1 - i));
		}
	}

	return val;
}

unsigned int read_byte(unsigned char rbsp[], unsigned int bytes_offset)
{
	unsigned int val = rbsp[bytes_offset];
	bytes_offset++;
	return val;
}

void byte_alignment(unsigned char rbsp[])
{
	unsigned int alignment_bit_equal_to_one = read_one_bit(rbsp);
	unsigned int alignment_bit_equal_to_zero;
	assert(alignment_bit_equal_to_one == 1);
	while (bits_offset) { // !byte_aligned
		alignment_bit_equal_to_zero = read_one_bit(rbsp);
		assert(alignment_bit_equal_to_zero == 0);
	}
}

void parse_slice_header(unsigned char *rbsp,
	struct nal_unit_header *nalu_head,
	struct video_parameter_set *vps,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head)
{
	int pic_order_cnt_lsb;
	int is_sao_enabled;
	int is_dbf_enabled;
	int i, j;
	int dpb = 0;
	if (slice_num == 129)
		int b = 1;
	slice_head->first_slice_segment_in_pic_flag = read_one_bit(rbsp);
	if (nalu_head->nal_unit_type == 19 ||      //NAL_UNIT_CODED_SLICE_IDR_W_RADL
		nalu_head->nal_unit_type == 20 ||  //NAL_UNIT_CODED_SLICE_IDR_N_LP
		nalu_head->nal_unit_type == 18 ||  //NAL_UNIT_CODED_SLICE_BLA_N_LP
		nalu_head->nal_unit_type == 17 ||  //NAL_UNIT_CODED_SLICE_BLA_W_RADL
		nalu_head->nal_unit_type == 16 ||  //NAL_UNIT_CODED_SLICE_BLA_W_LP
		nalu_head->nal_unit_type == 21) {  //NAL_UNIT_CODED_SLICE_CRA
		slice_head->no_output_of_prior_pics_flag = read_one_bit(rbsp);
	}
	slice_head->slice_pic_parameter_set_id = read_ue_v(rbsp);
	slice_head->slice_type = read_ue_v(rbsp);
	assert(slice_head->slice_type != B_SLICE);
	if (nalu_head->nal_unit_type == 19 /*NAL_UNIT_CODED_SLICE_IDR_W_RADL*/ ||
		nalu_head->nal_unit_type == 20 /*NAL_UNIT_CODED_SLICE_IDR_N_LP*/) {
		slice_head->POC = 0;
		PrevPOC = 0;
		for (j = 0; j < 32; j++) {
			free_dpb(DPB[j]);
			DPB[j] = NULL;
		}
	} else {
		int max_poc_lsb = (1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4));
		int prev_poc_lsb = PrevPOC & (max_poc_lsb - 1);
		int prev_poc_msb = PrevPOC - prev_poc_lsb;
		pic_order_cnt_lsb = read_bits(rbsp, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
		if (pic_order_cnt_lsb < prev_poc_lsb) {//todo >>1?
			slice_head->POC =  prev_poc_msb + max_poc_lsb + pic_order_cnt_lsb;
			//原来255，现在0，说明turnaround了，补上256
		} else {
			slice_head->POC =  prev_poc_msb + pic_order_cnt_lsb;
		}
		PrevPOC = slice_head->POC;

		slice_head->short_term_ref_pic_set_sps_flag = read_one_bit(rbsp);
		if (slice_head->short_term_ref_pic_set_sps_flag == 0) {
			parse_short_term_ref_pic_set(rbsp, &SliceLocalRPS, 
					sps->num_short_term_ref_pic_sets, sps);
		} else {
			int num_bits = 0;
			while ((1 << num_bits) < sps->num_short_term_ref_pic_sets)
				num_bits++;
			//num_short_term_ref_pic_sets=13,bits即为4，4bit可以表示0~15
			if (num_bits > 0)
				slice_head->short_term_ref_pic_set_idx = read_bits(rbsp, num_bits);
			else
				slice_head->short_term_ref_pic_set_idx = 0;	//如果只有1个rps，short_term_ref_pic_set_idx不用读了，肯定为0

			SliceLocalRPS = sps->rps[slice_head->short_term_ref_pic_set_idx];
		}
		Log_ref("current POC %d\n", slice_head->POC);
		for (j = 0; j < 32; j++) {
			if (DPB[j]) {
				DPB[j]->used_for_ref = 0; //used_for_ref先全清0，再根据当前rps, 用到的置1，没用到置0
				Log_ref("dpb[%d] poc %d\n", j, DPB[j]->slice_head.POC);
				dpb++;
			}
		}
		Log_ref("dpbs %d\n", dpb);
		CurrPic->rpl->num_refs = SliceLocalRPS.numberOfPictures;
		for (i = 0; i < SliceLocalRPS.numberOfPictures; i++) {
			int ref_poc = slice_head->POC + SliceLocalRPS.deltaPoc[i];
			for (j = 0; j < 32; j++) {
				if (DPB[j] && DPB[j]->slice_head.POC == ref_poc) {
					CurrPic->rpl->ref[i] = DPB[j];
					DPB[j]->used_for_ref = 1;
					Log_ref("rpl[%d] poc %d dpb[%d]\n", i, ref_poc, j);
					break;
				}
			}
		}
		if (dpb > vps->vps_max_dec_pic_buffering_minus1_i + 1) {
			//删除unused for reference中poc最小的一个，暂时这么做
			int min_poc = 0x7FFFFFFF;
			int min_poc_idx = -1;
			for (j = 0; j < 32; j++) {
				if (DPB[j] && !DPB[j]->used_for_ref) {
					if (DPB[j]->slice_head.POC < min_poc) {
						min_poc = DPB[j]->slice_head.POC;
						min_poc_idx = j;
					}
				}
			}
			if (min_poc_idx != -1) {
				free_dpb(DPB[min_poc_idx]);
				DPB[min_poc_idx] = NULL;
				Log_ref("free dpb[%d]\n", min_poc_idx);
			}
		}
		if (sps->sps_temporal_mvp_enabled_flag) {
			slice_head->slice_temporal_mvp_enabled_flag = read_one_bit(rbsp);
		} else {
			slice_head->slice_temporal_mvp_enabled_flag = 0;
		}
	}
	if (sps->sample_adaptive_offset_enabled_flag) {
		slice_head->slice_sao_luma_flag = read_one_bit(rbsp);
		slice_head->slice_sao_chroma_flag = read_one_bit(rbsp);
	}
	else {
		slice_head->slice_sao_luma_flag = 0;
		slice_head->slice_sao_chroma_flag = 0;
	}
	if (nalu_head->nal_unit_type == 19 /*NAL_UNIT_CODED_SLICE_IDR_W_RADL*/ ||
		nalu_head->nal_unit_type == 20 /*NAL_UNIT_CODED_SLICE_IDR_N_LP*/) {
		slice_head->slice_temporal_mvp_enabled_flag = 0;
	}
	if (slice_head->slice_type != I_SLICE) {
		slice_head->num_ref_idx_active_override_flag = read_one_bit(rbsp);
		if (slice_head->num_ref_idx_active_override_flag) {
			slice_head->numRefIdx = read_ue_v(rbsp) + 1; //num_ref_idx_l0_active_minus1
		} else {
			slice_head->numRefIdx = pps->num_ref_idx_l0_default_active_minus1 + 1;
		}
	}
	if (pps->cabac_init_present_flag && slice_head->slice_type != I_SLICE) {
		slice_head->cabac_init_flag = read_one_bit(rbsp);
	} else {
		slice_head->cabac_init_flag = 0;
	}
	
	if (slice_head->slice_type != I_SLICE) {
		if (slice_head->slice_temporal_mvp_enabled_flag) {
			//When collocated_from_l0_flag is not present, it is inferred to be equal to 1
			if (slice_head->numRefIdx > 1)
				slice_head->collocated_ref_idx = read_ue_v(rbsp);
			else
				slice_head->collocated_ref_idx = 0;
		}
		slice_head->five_minus_max_num_merge_cand = read_ue_v(rbsp);
	}
	slice_head->slice_qp_delta = read_se_v(rbsp);

	if (pps->pps_cb_qp_offset) {
		slice_head->slice_qp_delta_cb = read_se_v(rbsp);
		assert(slice_head->slice_qp_delta_cb >= -12);
		assert(slice_head->slice_qp_delta_cb <= 12);
	} else 
		slice_head->slice_qp_delta_cb = 0;

	if (pps->pps_cr_qp_offset) {
		slice_head->slice_qp_delta_cr = read_se_v(rbsp);
		assert(slice_head->slice_qp_delta_cr >= -12);
		assert(slice_head->slice_qp_delta_cr <= 12);
	} else
		slice_head->slice_qp_delta_cr = 0;
	//if (pps->deblocking_filter_control_present_flag) todo
	if (sps->sample_adaptive_offset_enabled_flag) {
		if (slice_head->slice_sao_luma_flag || slice_head->slice_sao_chroma_flag)
			is_sao_enabled = 1;
		else
			is_sao_enabled = 0;
	} else {
		is_sao_enabled = 0;
	}
	slice_head->slice_beta_offset_div2 = pps->pps_beta_offset_div2;
	slice_head->slice_tc_offset_div2 = pps->pps_tc_offset_div2;
	slice_head->deblocking_filter_override_flag = 0;
	slice_head->slice_deblocking_filter_disabled_flag = pps->pps_deblocking_filter_disabled_flag;
	if (pps->deblocking_filter_override_enabled_flag) {
		slice_head->deblocking_filter_override_flag = read_one_bit(rbsp);
		if (slice_head->deblocking_filter_override_flag) {
			slice_head->slice_deblocking_filter_disabled_flag = read_one_bit(rbsp);
			if (!slice_head->slice_deblocking_filter_disabled_flag) {
				slice_head->slice_beta_offset_div2 = read_se_v(rbsp);
				slice_head->slice_tc_offset_div2 = read_se_v(rbsp);
				assert(slice_head->slice_beta_offset_div2 >= -6 &&
					slice_head->slice_beta_offset_div2 <= 6);
				assert(slice_head->slice_tc_offset_div2 >= -6 &&
					slice_head->slice_tc_offset_div2 <= 6);
			}
		}
	}
	is_dbf_enabled = !pps->pps_deblocking_filter_disabled_flag;
	if (pps->loop_filter_across_slices_enabled_flag && (is_dbf_enabled || is_sao_enabled))
		slice_head->slice_loop_filter_across_slices_enabled_flag = read_one_bit(rbsp);
	else
		slice_head->slice_loop_filter_across_slices_enabled_flag =
			pps->loop_filter_across_slices_enabled_flag;
	byte_alignment(rbsp);
}


void parse_coding_quadtree(unsigned char *rbsp, int x0, int y0, int log2CbSize, int cqtDepth,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head)
{
	int split_cu_flag = 0;
	int x1, y1;
	int x, y;

	if ((x0 + (1 << log2CbSize)) <= PicWidthInSamplesY &&
		(y0 + (1 << log2CbSize)) <= PicHeightInSamplesY &&
		log2CbSize > MinCbLog2SizeY) {
		parse_split_cu_flag(rbsp, &split_cu_flag, x0, y0, log2CbSize, cqtDepth);
	}
	else {
	//fix I.7.4.9.4	Coding quadtree semantics
		split_cu_flag = log2CbSize > MinCbLog2SizeY;
	}
	if (split_cu_flag == 1) {
		for (x = x0;
			(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
			x += (1 << MinCbLog2SizeY)) {
			for (y = y0;
				(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
				y += (1 << MinCbLog2SizeY)) {
				CtDepth[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = cqtDepth + 1;
			}
		}
	}
	 //每个quantization group才会编码一次cu_qp_delta_abs
	 //todo qp_y_tab以quantization group为单位就可以了，
	if (pps->cu_qp_delta_enabled_flag &&
		log2CbSize >= Log2MinCuQpDeltaSize) {
		IsCuQpDeltaCoded = 0;
		CuQpDeltaVal = 0;
	}
	if (split_cu_flag) {
		x1 = x0 + (1 << (log2CbSize - 1));
		y1 = y0 + (1 << (log2CbSize - 1));
		parse_coding_quadtree(rbsp, x0, y0, log2CbSize-1, cqtDepth+1, pps, sps, slice_head);
		if( x1 < PicWidthInSamplesY)
			parse_coding_quadtree(rbsp, x1, y0, log2CbSize-1, cqtDepth+1, pps, sps, slice_head);
		if( y1 < PicHeightInSamplesY)   
			parse_coding_quadtree(rbsp, x0, y1, log2CbSize-1, cqtDepth+1, pps, sps, slice_head);
		if( x1 < PicWidthInSamplesY && y1 < PicHeightInSamplesY)
			parse_coding_quadtree(rbsp, x1, y1, log2CbSize-1, cqtDepth+1, pps, sps, slice_head);
	} else {

		CU.x0 = x0;
		CU.y0 = y0;
		CU.width = 1 << log2CbSize;
		CU.height = 1 << log2CbSize;
		parse_coding_unit(rbsp, x0, y0, log2CbSize, pps, sps, slice_head, &CU);

	}
}

void parse_sao_ctb(unsigned char *rbsp, int rx, int ry, struct sao_param *sao,
	struct slice_segment_header *slice_head, int CtbAddr) //rx,ry以ctb 64为单位
{
	int cIdx;
	int binVal;
	int i;

	if (rx > 0) {
		parse_sao_merge(rbsp, &sao->sao_merge_left_flag);
	} else 
		sao->sao_merge_left_flag = 0;
	if (ry > 0 && sao->sao_merge_left_flag == 0) {
		parse_sao_merge(rbsp, &sao->sao_merge_up_flag);
	} else
		sao->sao_merge_up_flag = 0;

	if (sao->sao_merge_left_flag == 0) {
		if (sao->sao_merge_up_flag == 0) {
			for (cIdx = 0; cIdx < 3; cIdx++) {
				if (cIdx == 0 && slice_head->slice_sao_luma_flag ||
					cIdx != 0 && slice_head->slice_sao_chroma_flag) {
					if (cIdx == 2) {
						//sao_type_idx_luma, sao_type_idx_chroma, 2个chroma一个sao_type_idx_chroma
						sao->sao_type_idx[2] = sao->sao_type_idx[1];
					}
					else {
						parse_sao_type_idx(rbsp, &sao->sao_type_idx[cIdx]);
					}

					if (sao->sao_type_idx[cIdx] != 0) {
						sao->sao_offset[cIdx][0] = 0;
						for (i = 0; i < 4; i++) {
							//sao_offset_abs spec cMax = (1 << (Min(bitDepth, 10) - 5)) - 1, cRiceParam = 0
							parse_sao_tr(rbsp, &sao->sao_offset[cIdx][i+1], (1 << 3) - 1);
						}
						if (sao->sao_type_idx[cIdx] == 1) {
							for (i = 0; i < 4; i++) {
								if (sao->sao_offset[cIdx][i+1] != 0) {
									bypass_decode_bin(rbsp, &binVal); //sao_offset_sign
									Log("sao_offset_sign %d\n", binVal);
									if (binVal == 1)
										sao->sao_offset[cIdx][i+1] =
										-sao->sao_offset[cIdx][i+1];
								}
							}
							bypass_decode_bins(rbsp, &sao->sao_band_position[cIdx], 5);  //cMax=31,fixedLength=5
							Log("sao_band_position %d\n", sao->sao_band_position[cIdx]);

						}
						else if (sao->sao_type_idx[cIdx] == 2) {
							//fix If SaoTypeIdx[cIdx][rx][ry] is equal to 2 and i is equal to 2 or 3, offsetSign is set equal to −1.
							//Otherwise, if SaoTypeIdx[cIdx][rx][ry] is equal to 2 and i is equal to 0 or 1, offsetSign is set equal to 1.
							sao->sao_offset[cIdx][3] = -sao->sao_offset[cIdx][3];
							sao->sao_offset[cIdx][4] = -sao->sao_offset[cIdx][4];
							if (cIdx == 2) {
								sao->sao_eo_class[2] = sao->sao_eo_class[1];
							}
							else {
								bypass_decode_bins(rbsp, &sao->sao_eo_class[cIdx], 2); //cMax=3,fixedLength=2
								Log("sao_eo_class %d\n", sao->sao_eo_class[cIdx]);
							}

						}
					}

				}
			}
		}
		else {
			memcpy(sao, &sao_params[CtbAddr-PicWidthInCtbsY], sizeof(struct sao_param));
		}
	}
	else {
		memcpy(sao, &sao_params[CtbAddr-1], sizeof(struct sao_param));
	}

}
//todo
//HM12 edge offset, 前2个正，后2个负
//      parseSaoMaxUvlc(uiSymbol, offsetTh -1 ); psSaoLcuParam->offset[0] = uiSymbol;
//      parseSaoMaxUvlc(uiSymbol, offsetTh -1 ); psSaoLcuParam->offset[1] = uiSymbol;
//      parseSaoMaxUvlc(uiSymbol, offsetTh -1 ); psSaoLcuParam->offset[2] = -(Int)uiSymbol;
//      parseSaoMaxUvlc(uiSymbol, offsetTh -1 ); psSaoLcuParam->offset[3] = -(Int)uiSymbol;

int parse_coding_tree_unit(unsigned char *rbsp, int CtbAddr,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head)
{
	int end_of_slice_segment_flag;
	int xCtb = (CtbAddr % PicWidthInCtbsY);
	int yCtb = (CtbAddr / PicWidthInCtbsY);
	int x0 = xCtb << CtbLog2SizeY;
	int y0 = yCtb << CtbLog2SizeY;
	if (sps->sample_adaptive_offset_enabled_flag && 
		(slice_head->slice_sao_luma_flag || slice_head->slice_sao_chroma_flag)) {
		parse_sao_ctb(rbsp, xCtb, yCtb, &sao_params[CtbAddr], slice_head, CtbAddr);
	}

	parse_coding_quadtree(rbsp, x0, y0, CtbLog2SizeY, 0, pps, sps, slice_head);
	terminate_decode_bin(rbsp, &end_of_slice_segment_flag);
	return end_of_slice_segment_flag;
}

void parse_slice(unsigned char *rbsp,
	struct nal_unit_header *nalu_head,
	struct video_parameter_set *vps,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head)
{
	int CtbAddrInRs;
	int ret;

	ResetDecoder();
	parse_slice_header(rbsp, nalu_head, vps, pps, sps, slice_head); //RBSP为nal去掉start bytes和nalu header，从slice_header开始
	SliceQpY = 26 + pps->init_qp_minus26 + slice_head->slice_qp_delta;
	QpY_PREV = SliceQpY;
	if (!pps->cu_qp_delta_enabled_flag)
		QpY = SliceQpY;
	init_cabac(rbsp, SliceQpY, pps, slice_head);

	for (CtbAddrInRs = 0; CtbAddrInRs < PicSizeInCtbsY; CtbAddrInRs++) {
		ret = parse_coding_tree_unit(rbsp, CtbAddrInRs, pps, sps, slice_head); //ctb64x64在slice是raster存放的
		if (ret)
			break;
	}
}

int test_block_available(int x, int y, int xNbL, int yNbL)
{
	int CtbAddrInRs = (y >> CtbLog2SizeY) * PicWidthInCtbsY + (x >> CtbLog2SizeY);
	int absPartIdx = R2Z[(((y % CtbSizeY) >> 2) << (CtbLog2SizeY - 2)) + //4x4块为单位
		((x % CtbSizeY) >> 2)];
	int CtbAddrNbL = (yNbL >> CtbLog2SizeY) * PicWidthInCtbsY + (xNbL >> CtbLog2SizeY);
	int absPartIdxNbL = R2Z[(((yNbL % CtbSizeY) >> 2) << (CtbLog2SizeY - 2)) +
		((xNbL % CtbSizeY) >> 2)];

	if (xNbL < 0 || yNbL < 0 || xNbL >= PicWidthInSamplesY || yNbL >= PicHeightInSamplesY)
	{
		return 0;
	}

	if (CtbAddrInRs < CtbAddrNbL)
	{
		return 0;
	}

	if (CtbAddrInRs == CtbAddrNbL && absPartIdx < absPartIdxNbL)
	{
		return 0;
	}

	return 1;
	
}

void set_qPY(int xCu, int yCu)
{
	int qPY_A, qPY_B, qPY_PRED;
	int MinCuQpDeltaSizeMask = (1 << Log2MinCuQpDeltaSize) - 1;
	int ctb_size_mask = (1 << CtbLog2SizeY) - 1;
	int xQg = xCu - (xCu & MinCuQpDeltaSizeMask); //所在Qg的x,y坐标
	int yQg = yCu - (yCu & MinCuQpDeltaSizeMask);


	if (((xCu & ctb_size_mask) && (xQg & ctb_size_mask)) == 0) {
		qPY_A = QpY_PREV;
	}
	else {
		qPY_A = qp_y_tab[yQg >> MinCbLog2SizeY][(xQg - 1) >> MinCbLog2SizeY];
		//fix, the neighbouring location (xNbY, yNbY) set equal to (xQg − 1, yQg)
	}
	if (((yCu & ctb_size_mask) && (yQg & ctb_size_mask)) == 0) {
		qPY_B = QpY_PREV;
	}
	else {
		qPY_B = qp_y_tab[(yQg - 1) >> MinCbLog2SizeY][xQg >> MinCbLog2SizeY];
	}
	qPY_PRED = (qPY_A + qPY_B + 1) >> 1;

	//QpY = ((qPY_PRED + CuQpDeltaVal + 52 + 2 * QpBdOffsetY) % (52 + QpBdOffsetY)) - QpBdOffsetY;
	//QpBdOffsetY = 6 * (8 + sps->bit_depth_luma_minus8);
	QpY = (qPY_PRED + CuQpDeltaVal + 52) % 52;
	Log_data("xBase %d yBase %d qPy_pred %d qPy %d\n",
		xCu, yCu, qPY_PRED, QpY);
}

//HM12 xDecodeCU ffmpeg hls_coding_unit
void parse_coding_unit(unsigned char *rbsp, int x0, int y0, int log2CbSize,
	struct pic_parameter_set *pps, struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head,
	struct CodingUnit *cu)
{
	int rqt_root_cbf = 1;
	int i, j;
	int x, y;
	int offsetX, offsetY;
	int IntraPredModeY;
	int IntraPredModeC;
	int nCbS = 1 << log2CbSize;
	int merge_flag = 0;
	int Qg_mask = (1 << Log2MinCuQpDeltaSize) - 1; //quantization group

	Log("##parse_coding_unit x0 %d y0 %d log2CbSize %d slice_num %d\n", x0, y0, log2CbSize, slice_num);
	Log_data("##parse_coding_unit x0 %d y0 %d log2CbSize %d slice_num %d\n", x0, y0, log2CbSize, slice_num);

	cu->cu_transquant_bypass_flag = 0;
	if (pps->transquant_bypass_enabled_flag) {
		parse_cu_transquant_bypass_flag(rbsp, &cu->cu_transquant_bypass_flag);
		if (cu->cu_transquant_bypass_flag) {
			for (x = x0;
				(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
				x += (1 << MinCbLog2SizeY)) {
				for (y = y0;
					(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
					y += (1 << MinCbLog2SizeY)) {
					no_filter[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = 1;
				}
			}
		}
	}

	if (slice_head->slice_type != I_SLICE)
		parse_cu_skip_flag(rbsp, x0, y0, log2CbSize);
	//fix When part_mode is not present, the variables PartMode is set equal to PART_2Nx2N.
	cu->part_mode = PART_2Nx2N;
	if (cu_skip_flag[y0 >> MinCbLog2SizeY][x0 >> MinCbLog2SizeY]) {
		parse_prediction_unit(rbsp, x0, y0, nCbS, nCbS, 0, cu, pps, slice_head, &merge_flag);
		cu->pred_mode = MODE_SKIP;
		//fix
		for (x = x0;
			(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
			x += (1 << MinCbLog2SizeY)) {
			for (y = y0;
				(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
				y += (1 << MinCbLog2SizeY)) {
				CurrPic->cu_predmode[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = MODE_SKIP;
			}
		}

	} else {
		if (slice_head->slice_type != I_SLICE)
			parse_pred_mode_flag(rbsp, &cu->pred_mode, x0, y0, log2CbSize);
		else
			cu->pred_mode = MODE_INTRA;
		if (cu->pred_mode != MODE_INTRA || log2CbSize == MinCbLog2SizeY) //MODE_INTRA下，只有8x8才有4等分的PART_NxN，默认是PART_2Nx2N=0，不分割
			parse_part_mode(rbsp, (int *)&cu->part_mode, x0, y0, log2CbSize, sps);
		if (cu->pred_mode == MODE_INTRA) {
			//HM 12 parseIntraDirLumaAng
			int prev_intra_luma_pred_flag[4], mpm_idx[4], rem_intra_luma_pred_mode[4], part_num;

			//assert pcm_enabled_flag = 0
			if (cu->part_mode == PART_NxN)
				part_num = 4;
			else
				part_num = 1;
			for (i = 0; i < part_num; i++)
				parse_prev_intra_luma_pred_flag(rbsp, &prev_intra_luma_pred_flag[i]);
			for (i = 0; i < part_num; i++) {
				int candModeList[3] = {-1, -1, -1};
				int xPb = x0 + (i % 2) * (nCbS / 2);
				int yPb = y0 + (i / 2) * (nCbS / 2);
				get_cand_mode_list(xPb, yPb, candModeList);
				if (prev_intra_luma_pred_flag[i]) {
					parse_mpm_idx(rbsp, &mpm_idx[i]);
					IntraPredModeY = candModeList[mpm_idx[i]];
				} else {
					bypass_decode_bins(rbsp, &rem_intra_luma_pred_mode[i], 5); //cMax=31,fixedLength=5
					IntraPredModeY = rem_intra_luma_pred_mode[i];
					if (candModeList[0] > candModeList[1])
						swap(candModeList[0], candModeList[1]);
					if (candModeList[0] > candModeList[2])
						swap(candModeList[0], candModeList[2]);
					if (candModeList[1] > candModeList[2])
						swap(candModeList[1], candModeList[2]);
					for (j = 0; j < 3; j++)
						IntraPredModeY += (IntraPredModeY >= candModeList[j]);

				}
				Log("IntraPredModeY %d\n", IntraPredModeY);
				if (part_num == 1) {
					for (offsetX = 0; offsetX < nCbS && (xPb + offsetX) < PicWidthInSamplesY; offsetX += 4) {
						for (offsetY = 0; offsetY < nCbS && (yPb + offsetY) < PicHeightInSamplesY; offsetY += 4) {
							intra_luma_pred_mode[(yPb+offsetY)>>2][(xPb+offsetX)>>2] = IntraPredModeY;
						}
					}
				} else {
					for (offsetX = 0; offsetX < (nCbS / 2) && (xPb + offsetX) < PicWidthInSamplesY; offsetX += 4) {
						for (offsetY = 0; offsetY < (nCbS / 2) && (yPb + offsetY) < PicHeightInSamplesY; offsetY += 4) {
							intra_luma_pred_mode[(yPb+offsetY)>>2][(xPb+offsetX)>>2] = IntraPredModeY;
						}
					}
				}
				cu->intra_pred_mode_luma[i] = IntraPredModeY;
			}

			parse_intra_chroma_pred_mode(rbsp, x0, y0, &IntraPredModeC, cu->intra_pred_mode_luma[0]);
			cu->intra_pred_mode_chroma = IntraPredModeC;

		} else { //inter
			if (x0 == 388 && y0 == 0 && slice_num == 18)
				x0 = x0;
			if (cu->part_mode == PART_2Nx2N) {
				parse_prediction_unit(rbsp, x0, y0, nCbS, nCbS, 0, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_2NxN) {
				parse_prediction_unit(rbsp, x0, y0, nCbS, nCbS/2, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0, y0+(nCbS/2), nCbS, nCbS/2, 1, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_Nx2N) {
				parse_prediction_unit(rbsp, x0, y0, nCbS/2, nCbS, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0+(nCbS/2), y0, nCbS/2, nCbS, 1, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_2NxnU) {
				parse_prediction_unit(rbsp, x0, y0, nCbS, nCbS/4, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0, y0+(nCbS/4), nCbS, nCbS*3/4, 1, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_2NxnD) {
				parse_prediction_unit(rbsp, x0, y0, nCbS, nCbS*3/4, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0, y0+(nCbS*3/4), nCbS, nCbS/4, 1, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_nLx2N) {
				parse_prediction_unit(rbsp, x0, y0, nCbS/4, nCbS, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0+(nCbS/4), y0, nCbS*3/4, nCbS, 1, cu, pps, slice_head, &merge_flag);
			} else if (cu->part_mode == PART_nRx2N) {
				parse_prediction_unit(rbsp, x0, y0, nCbS*3/4, nCbS, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0+(nCbS*3/4), y0, nCbS / 4, nCbS, 1, cu, pps, slice_head, &merge_flag);
			} else { //PART_NxN
				parse_prediction_unit(rbsp, x0, y0, nCbS/2, nCbS/2, 0, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0+(nCbS/2), y0, nCbS/2, nCbS/2, 1, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0, y0+(nCbS/2), nCbS/2, nCbS/2, 2, cu, pps, slice_head, &merge_flag);
				parse_prediction_unit(rbsp, x0+(nCbS/2), y0+(nCbS/2), nCbS/2, nCbS/2, 3, cu, pps, slice_head, &merge_flag);
			}
		}
	}

	if (cu->pred_mode != MODE_INTRA &&
		!(cu->part_mode == PART_2Nx2N && merge_flag))
		 parse_rqt_root_cbf(rbsp, &rqt_root_cbf);

	if (rqt_root_cbf &&
		cu_skip_flag[y0>>MinCbLog2SizeY][x0>>MinCbLog2SizeY] == 0) { //fix
		int IntraSplitFlag = (cu->pred_mode == MODE_INTRA &&
			cu->part_mode == PART_NxN) ? 1 : 0;
		MaxTrafoDepth = (cu->pred_mode ==  MODE_INTRA ?
			(sps->max_transform_hierarchy_depth_intra + IntraSplitFlag) : sps->max_transform_hierarchy_depth_inter);
		parse_transform_tree(rbsp, x0, y0, x0, y0, log2CbSize, 0, 0, 0, 0, pps, sps, cu, slice_head);
	}
	else {
		for (y = 0; y < (1 << log2CbSize); y++) {
			for (x = 0; x < (1 << log2CbSize); x++) {
				CurrPic->SL[y + y0][x + x0] = Clip3(0, 255, predSamplesL[y][x]);
			}
		}

		for (y = 0; y < (1 << (log2CbSize - 1)); y++) {
			for (x = 0; x < (1 << (log2CbSize - 1)); x++) {
					CurrPic->SCb[y + y0 / 2][x + x0 / 2] = Clip3(0, 255, predSamplesCb[y][x]);
					CurrPic->SCr[y + y0 / 2][x + x0 / 2] = Clip3(0, 255, predSamplesCr[y][x]);

			}
		}

		if (!slice_head->slice_deblocking_filter_disabled_flag)
			derive_bs(x0, y0, log2CbSize);
	}
	//一个quantization group到目前为止所有tu还没编码过cu_qp_delta_abs,cu_qp_delta_sign_flag
	//也就是所有tu的cbf_luma,cbf_cb,cbf_cr都为0
	//另外MODE_SKIP的cu也不会进入tu编码，所以不会编码cu_qp_delta_abs,cu_qp_delta_sign_flag
	if (pps->cu_qp_delta_enabled_flag && IsCuQpDeltaCoded == 0)
		set_qPY(x0, y0);

	for (x = x0;
		(x < (x0 + (1 << log2CbSize)) && x < PicWidthInSamplesY);
		x += (1 << MinCbLog2SizeY)) {
		for (y = y0;
			(y < (y0 + (1 << log2CbSize)) && y < PicHeightInSamplesY);
			y += (1 << MinCbLog2SizeY)) {
			qp_y_tab[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY] = QpY;
		}
	}

	//8.6.1 Otherwise, qPY_PREV is set equal to the luma quantization parameter QpY of the last coding unit
	//in the previous quantization group in decoding order.
	if ((((x0 + (1 << log2CbSize)) & Qg_mask) == 0 ||
		x0 + (1 << log2CbSize) == PicWidthInSamplesY) &&
		(((y0 + (1 << log2CbSize)) & Qg_mask) == 0 ||
		y0 + (1 << log2CbSize) == PicHeightInSamplesY))
		QpY_PREV = QpY;

}

//xCu, yCu transform tree所在CU的xy坐标
void parse_transform_tree(unsigned char *rbsp, int x0, int y0, int xCu, int yCu,
	int log2TrafoSize, int trafoDepth, int blkIdx,
	int base_cbf_cb, int base_cbf_cr, struct pic_parameter_set *pps,
	struct seq_parameter_set *sps, struct CodingUnit *cu,
	struct slice_segment_header *slice_head)
{
	int split_transform_flag = 0;
	int IntraSplitFlag = (cu->pred_mode == MODE_INTRA &&
		cu->part_mode == PART_NxN) ? 1 : 0;
	int cbf_cb = 0, cbf_cr = 0, cbf_luma = 1; //7.4.9.8 When cbf_luma[x0][y0][trafoDepth] is not present, it is inferred to be equal to 1.

	if(log2TrafoSize <= Log2MaxTrafoSize && log2TrafoSize > Log2MinTrafoSize &&
		trafoDepth < MaxTrafoDepth && !(IntraSplitFlag && (trafoDepth == 0))) {
		parse_split_transform_flag(rbsp, &split_transform_flag, log2TrafoSize);
	} else {
		int interSplitFlag = 0;
		if (sps->max_transform_hierarchy_depth_inter == 0 &&
			cu->pred_mode == MODE_INTER &&
			cu->part_mode != PART_2Nx2N &&
			trafoDepth == 0)
			interSplitFlag = 1;
		// 7.4.9.8 When split_transform_flag[x0][y0][trafoDepth] is not present, it is inferred as follows:
		if (log2TrafoSize > Log2MaxTrafoSize ||
			(IntraSplitFlag && trafoDepth == 0) ||
			interSplitFlag)
			split_transform_flag = 1;
	}
	if (log2TrafoSize > 2) {
		if (trafoDepth == 0 || base_cbf_cb) //cbf_cb[xBase][yBase][trafoDepth - 1]上一层的cbf_cb
			parse_cbf_cb_cr(rbsp, &cbf_cb, trafoDepth);
	}
	else {
		cbf_cb = base_cbf_cb;
	}
	if (log2TrafoSize > 2) {
		if (trafoDepth == 0 || base_cbf_cr)
			parse_cbf_cb_cr(rbsp, &cbf_cr, trafoDepth);
	}
	else {
		cbf_cr = base_cbf_cr;
	}
	if (split_transform_flag) {
		int x1 = x0 + (1 << (log2TrafoSize - 1));
		int y1 = y0 + (1 << (log2TrafoSize - 1));
		parse_transform_tree(rbsp, x0, y0, xCu, yCu, log2TrafoSize-1, trafoDepth+1, 0, cbf_cb, cbf_cr, pps, sps, cu, slice_head);
		parse_transform_tree(rbsp, x1, y0, xCu, yCu, log2TrafoSize - 1, trafoDepth + 1, 1, cbf_cb, cbf_cr, pps, sps, cu, slice_head);
		parse_transform_tree(rbsp, x0, y1, xCu, yCu, log2TrafoSize - 1, trafoDepth + 1, 2, cbf_cb, cbf_cr, pps, sps, cu, slice_head);
		parse_transform_tree(rbsp, x1, y1, xCu, yCu, log2TrafoSize - 1, trafoDepth + 1, 3, cbf_cb, cbf_cr, pps, sps, cu, slice_head);
	} else {
		int x, y;
		if (cu->pred_mode == MODE_INTRA || trafoDepth != 0 || cbf_cb || cbf_cr)
			parse_cbf_luma(rbsp, &cbf_luma, trafoDepth);
		parse_transform_unit(rbsp, log2TrafoSize, cbf_cb, cbf_cr, cbf_luma, pps, sps, cu, slice_head, blkIdx, x0, y0, xCu, yCu);
		if (cbf_luma) {
			for (x = x0;
				(x < (x0 + (1 << log2TrafoSize)) && x < PicWidthInSamplesY);
				x += 4) {
				for (y = y0;
					(y < (y0 + (1 << log2TrafoSize)) && y < PicHeightInSamplesY);
					y += 4) {
					CbfLuma[y>>2][x>>2] = 1;
				}
			}
		}


		if (!slice_head->slice_deblocking_filter_disabled_flag)
			derive_bs(x0, y0, log2TrafoSize);
	}
}



void parse_transform_unit(unsigned char *rbsp, int log2TrafoSize, int cbf_cb, int cbf_cr, int cbf_luma,
	struct pic_parameter_set *pps, struct seq_parameter_set *sps, struct CodingUnit *cu, struct slice_segment_header *slice_head,
	int blkIdx, int x0, int y0, int xCu, int yCu)
{
	int cu_qp_delta_abs;
	int cu_qp_delta_sign_flag = 0;
	int x, y;
	int xoffL = x0 - cu->x0, yoffL = y0 - cu->y0;////xoff, yoff PU相对CU偏移
	int xoffC = xoffL/2, yoffC = yoffL/2;
	//todo intra_pred放到这里，因为intra不可能无残差，肯定会调用parse_transform_unit？
	if (cu->pred_mode == MODE_INTRA) {
		intra_pred(x0, y0, 0, log2TrafoSize, intra_luma_pred_mode[y0>>2][x0>>2], pps, sps); //luma

	}
	memset(resSamplesL[0], 0, (1 << log2TrafoSize) * 64 * sizeof(int));
	memset(resSamplesCb[0], 0, max(1 << (log2TrafoSize - 1), 4) * 32 * sizeof(int));
	memset(resSamplesCr[0], 0, max(1 << (log2TrafoSize - 1), 4) * 32 * sizeof(int));
	if (cbf_luma || cbf_cb || cbf_cr) {
		int scanIdx = SCAN_DIAG;
		int scanIdxC = SCAN_DIAG;

		if (cu->pred_mode == MODE_INTRA && log2TrafoSize < 4) { //7.4.9.11
			if (intra_luma_pred_mode[y0>>2][x0>>2] >= 6 && intra_luma_pred_mode[y0>>2][x0>>2] <= 14) {
				scanIdx = SCAN_VERT;
			}
			else if (intra_luma_pred_mode[y0>>2][x0>>2] >= 22 && intra_luma_pred_mode[y0>>2][x0>>2] <= 30) {
				scanIdx = SCAN_HORIZ;
			}

			if (cu->intra_pred_mode_chroma >= 6 && cu->intra_pred_mode_chroma <= 14) {
				scanIdxC = SCAN_VERT;
			} else if (cu->intra_pred_mode_chroma >= 22 && cu->intra_pred_mode_chroma <= 30) {
				scanIdxC = SCAN_HORIZ;
			}
		}
		if (pps->cu_qp_delta_enabled_flag && !IsCuQpDeltaCoded) {
			parse_cu_qp_delta_abs(rbsp, &cu_qp_delta_abs);
			if (cu_qp_delta_abs)
				bypass_decode_bin(rbsp, &cu_qp_delta_sign_flag);
			IsCuQpDeltaCoded = 1;
			CuQpDeltaVal = cu_qp_delta_abs * (1 - 2 * cu_qp_delta_sign_flag); // 7-88
			set_qPY(xCu, yCu); // fix,一个CU有可能第二个TU才encode cu_qp_delta_abs，因为第一个TU cbf为0
		}
		if (cbf_luma)
			parse_residual_coding(rbsp, log2TrafoSize, 0, scanIdx, pps, cu, slice_head, x0, y0);

		//如果luma是8x8及以上的话，每个luma 残差后面跟cb和cr的残差，如果luma是4x4的话，只有最后一个luma idx=3之后跟cb，cr残差

			if (log2TrafoSize > 2) {
				if (cu->pred_mode == MODE_INTRA)
					intra_pred(x0, y0, 1, log2TrafoSize - 1, cu->intra_pred_mode_chroma, pps, sps);//Cb
				if (cbf_cb)
					parse_residual_coding(rbsp, log2TrafoSize - 1, 1, scanIdxC, pps, cu, slice_head, x0, y0);
				if (cu->pred_mode == MODE_INTRA)
					intra_pred(x0, y0, 2, log2TrafoSize - 1, cu->intra_pred_mode_chroma, pps, sps);//Cr
				if (cbf_cr)
					parse_residual_coding(rbsp, log2TrafoSize - 1, 2, scanIdxC, pps, cu, slice_head, x0, y0);
			}
			else if (blkIdx == 3) {
				if (cu->pred_mode == MODE_INTRA)
					intra_pred(x0 - 4, y0 - 4, 1, log2TrafoSize, cu->intra_pred_mode_chroma, pps, sps);//Cb
				if (cbf_cb)
					parse_residual_coding(rbsp, log2TrafoSize, 1, scanIdxC, pps, cu, slice_head, x0 - 4, y0 - 4);
				if (cu->pred_mode == MODE_INTRA)
					intra_pred(x0 - 4, y0 - 4, 2, log2TrafoSize, cu->intra_pred_mode_chroma, pps, sps);//Cr

				if (cbf_cr)
					parse_residual_coding(rbsp, log2TrafoSize, 2, scanIdxC, pps, cu, slice_head, x0 - 4, y0 - 4);
			}



	}
	else if (cu->pred_mode == MODE_INTRA) {
		if (log2TrafoSize > 2) {
			intra_pred(x0, y0, 1, log2TrafoSize - 1, cu->intra_pred_mode_chroma, pps, sps);//Cb
			intra_pred(x0, y0, 2, log2TrafoSize - 1, cu->intra_pred_mode_chroma, pps, sps);//Cr
		}
		else if (blkIdx == 3) {
			intra_pred(x0 - 4, y0 - 4, 1, log2TrafoSize, cu->intra_pred_mode_chroma, pps, sps);//Cb
			intra_pred(x0 - 4, y0 - 4, 2, log2TrafoSize, cu->intra_pred_mode_chroma, pps, sps);//Cr
		}
	}
	if (cu->pred_mode == MODE_INTRA)
		xoffL = yoffL = xoffC = yoffC = 0;
	int xoffa = 0, yoffa = 0;
	if (log2TrafoSize == 2 && blkIdx == 3) {
		if (!cu->pred_mode == MODE_INTRA) {
			xoffC -= 2;
			yoffC -= 2;
		}
		xoffa = -2;
		yoffa = -2;
	}

	for (y = 0; y < 1 << log2TrafoSize; y++) {
		for (x = 0; x < 1 << log2TrafoSize; x++) {
			CurrPic->SL[y + y0][x + x0] = Clip3(0, 255, predSamplesL[y+yoffL][x+xoffL] + resSamplesL[y][x]);
		}
	}
	if (!(log2TrafoSize == 2 && blkIdx != 3)) {
		for (y = 0; y < max(1 << (log2TrafoSize - 1), 4); y++) {
			for (x = 0; x < max(1 << (log2TrafoSize - 1), 4); x++) {
				CurrPic->SCb[y + y0 / 2 + yoffa][x + x0 / 2 + xoffa] = Clip3(0, 255, predSamplesCb[y + yoffC][x + xoffC] + resSamplesCb[y][x]);
			}
		}
		for (y = 0; y < max(1 << (log2TrafoSize - 1), 4); y++) {
			for (x = 0; x < max(1 << (log2TrafoSize - 1), 4); x++) {
				CurrPic->SCr[y + y0 / 2 + yoffa][x + x0 / 2 + xoffa] = Clip3(0, 255, predSamplesCr[y + yoffC][x + xoffC] + resSamplesCr[y][x]);
			}
		}
	}

}

//cIdx 0=luma, 1=cb, 2=cr
//x0,y0是亮度TU在图像坐标
void parse_residual_coding(unsigned char *rbsp, int log2TrafoSize, int cIdx, int scanIdx,
	struct pic_parameter_set *pps, struct CodingUnit *cu, struct slice_segment_header *slice_head,
	int x0, int y0)
{
	int transform_skip_flag = 0;
	int last_sig_coeff_x_prefix, last_sig_coeff_y_prefix;
	int last_sig_coeff_x_suffix, last_sig_coeff_y_suffix;
	int coded_sub_block_flag[8][8] = {{0}};
	int sig_coeff_flag[16] = {0,};
	int coeff_abs_level_greater1_flag[16] = {0,};
	int coeff_abs_level_greater2_flag[16] = {0,};
	int coeff_abs_level_remaining[16] = {0,};
	int coeff_sign_flag[16] = {0,};
	int xS, yS, xC, yC;
	int lastSubBlock, lastScanPos;
	int inferSbDcSigCoeffFlag;
	int *ScanOrderSubBlockX, *ScanOrderSubBlockY, *ScanOrderX, *ScanOrderY;
	int LastSignificantCoeffX, LastSignificantCoeffY;
	int csbfCtx, prevCsbf;
	int i, n;
	int firstSigScanPos, lastSigScanPos, numGreater1Flag, lastGreater1ScanPos;
	int greater1Ctx = 1, ctxSet;
	int signHidden;
	int numSigCoeff = 0;
	int sumAbsLevel = 0;
	int baseLevel;
	int cAbsLevel, cRiceParam;
	int qP, qPi;
	int **d, **e, **g, **r, **TransCoeffLevel;
	int xoff = x0 - cu->x0, yoff = y0 - cu->y0; //TU相对CU偏移

	int x, y;

	int trafoSize = 1 << log2TrafoSize;
	if (cIdx != 0) {
		xoff /= 2;
		yoff /= 2;
	}
	if (cIdx == 0)
		TransCoeffLevel = resSamplesL;
	else if (cIdx == 1)
		TransCoeffLevel = resSamplesCb;
	else
		TransCoeffLevel = resSamplesCr;

	d = e = g = r = TransCoeffLevel;
	
	Log("+parse_residual_coding cIdx %d log2TrafoSize %d x0 %d y0 %d scanIdx %d\n",
		cIdx, log2TrafoSize, x0, y0, scanIdx);
	Log_data("+parse_residual_coding cIdx %d log2TrafoSize %d x0 %d y0 %d scanIdx %d\n",
		cIdx, log2TrafoSize, x0, y0, scanIdx);

	if (!cu->cu_transquant_bypass_flag) {
		if (cIdx == 0)
			qP = QpY; //QpY + QpBdOffsetY QpBdOffsetY = 6 * (8 + sps->bit_depth_luma_minus8)
		else {
			static const int qPc_tab[] = { 29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37 };
			if (cIdx == 1)
				qPi = Clip3(0, 57, QpY+pps->pps_cb_qp_offset+slice_head->slice_qp_delta_cb);
				//Clip3(-QpBdOffsetC, 57, QpY + pps->pps_cb_qp_offset + slice_head->slice_qp_delta_cb);
			else
				qPi = Clip3(0, 57, QpY+pps->pps_cr_qp_offset+slice_head->slice_qp_delta_cr);
			if (qPi < 30)
				qP = qPi;
			else if (qPi > 43)
				qP = qPi - 6;
			else
				qP = qPc_tab[qPi-30];
			//qP += QpBdOffsetC;
		}
	}

	if (pps->transform_skip_enabled_flag &&
		!cu->cu_transquant_bypass_flag &&
		log2TrafoSize == 2) {
		parse_transform_skip_flag(rbsp, &transform_skip_flag, cIdx);
	}
	parse_last_sig_coeff_x_prefix(rbsp, &last_sig_coeff_x_prefix, log2TrafoSize, cIdx);
	parse_last_sig_coeff_y_prefix(rbsp, &last_sig_coeff_y_prefix, log2TrafoSize, cIdx);

	//LastSignificantCoeffX,LastSignificantCoeffY最后一个非0系数在TU的x,y坐标值,不是系数的值
	LastSignificantCoeffX = last_sig_coeff_x_prefix;
	LastSignificantCoeffY = last_sig_coeff_y_prefix;
	if (last_sig_coeff_x_prefix > 3) {
		parse_last_sig_coeff_suffix(rbsp, &last_sig_coeff_x_suffix, last_sig_coeff_x_prefix);
		LastSignificantCoeffX = (1 << ((last_sig_coeff_x_prefix >> 1) - 1)) *
			(2 + (last_sig_coeff_x_prefix & 1)) + last_sig_coeff_x_suffix;

	}
	if (last_sig_coeff_y_prefix > 3) {
		parse_last_sig_coeff_suffix(rbsp, &last_sig_coeff_y_suffix, last_sig_coeff_y_prefix);
		LastSignificantCoeffY = (1 << ((last_sig_coeff_y_prefix >> 1) - 1)) *
			(2 + (last_sig_coeff_y_prefix & 1)) + last_sig_coeff_y_suffix;
	}
	if (scanIdx == SCAN_VERT)
		swap(LastSignificantCoeffX, LastSignificantCoeffY); //fix JCTVC-L1003_v34 7-66
//subblock 4x4块
//8x8的块diag
//	首先分成4个4x4 subblock，按
//		0 2
//		1 3 排,每个subblock按diag排

//		00 02 05 09       32 34 37 41
//		01 04 08 12       33 36 40 44
//		03 07 11 14       35 39 43 46
//		06 10 13 15       38 42 45 47

//		16 18 21 25       48 50 53 57
//		17 20 24 28       49 52 56 60
//		19 23 27 30       51 55 59 62
//		22 26 29 31       54 58 61 63
//	一般情况下残差经过DCT变化之后，非0值都集中在左上角，越靠近左上角越有可能非0，
//	8x8分成4个4x4的话，4个4x4也按diag排序，越靠近左上角的4x4非0可能性越大，
//	但是也有可能有极少数情况，非0值越靠近左边越多，或者越靠近上边越多,这就是SCAN_HORIZ,SCAN_VERT的情况
	lastScanPos = 16;
	lastSubBlock = (1 << (log2TrafoSize - 2)) * (1 << (log2TrafoSize - 2)) - 1;
	if (scanIdx == SCAN_DIAG) {
		ScanOrderX = ScanOrder_4x4_diag_x;
		ScanOrderY = ScanOrder_4x4_diag_y;
		if (log2TrafoSize <= 3) {
			ScanOrderSubBlockX = ScanOrder_2x2_diag_x;
			ScanOrderSubBlockY = ScanOrder_2x2_diag_y;
		} else if (log2TrafoSize == 4) {
			ScanOrderSubBlockX = ScanOrder_4x4_diag_x;
			ScanOrderSubBlockY = ScanOrder_4x4_diag_y;
		} else if (log2TrafoSize == 5) {
			ScanOrderSubBlockX = ScanOrder_8x8_diag_x;
			ScanOrderSubBlockY = ScanOrder_8x8_diag_y;
		}
	} else if (scanIdx == SCAN_HORIZ) {
		ScanOrderX = ScanOrder_4x4_horiz_x;
		ScanOrderY = ScanOrder_4x4_horiz_y;
		ScanOrderSubBlockX = ScanOrder_2x2_horiz_x;
		ScanOrderSubBlockY = ScanOrder_2x2_horiz_y;
	} else {
		ScanOrderX = ScanOrder_4x4_vert_x;
		ScanOrderY = ScanOrder_4x4_vert_y;
		ScanOrderSubBlockX = ScanOrder_2x2_vert_x;
		ScanOrderSubBlockY = ScanOrder_2x2_vert_y;
	}
	//这个循环根据LastSignificantCoeffX,LastSignificantCoeffY得到子块号lastSubBlock
	//和lastScanPos,比如SCAN_DIAG,就是上面描述的4x4子块像素的排序序号
	do {
		if(lastScanPos == 0) {
			lastScanPos = 16;
			lastSubBlock--; 
		}
		lastScanPos--;
		xS = ScanOrderSubBlockX[lastSubBlock];
		yS = ScanOrderSubBlockY[lastSubBlock];
		xC = (xS << 2) + ScanOrderX[lastScanPos];
		yC = (yS << 2) + ScanOrderY[lastScanPos];
	} while((xC != LastSignificantCoeffX ) || (yC != LastSignificantCoeffY));

	Log("LastSignificantCoeffX %d LastSignificantCoeffY %d\n",
		LastSignificantCoeffX, LastSignificantCoeffY);

	for (i = lastSubBlock; i >= 0; i--) {

		memset(sig_coeff_flag, 0, 16 * sizeof(int));
		memset(coeff_abs_level_greater1_flag, 0, 16 * sizeof(int));
		memset(coeff_abs_level_greater2_flag, 0, 16 * sizeof(int));
		memset(coeff_abs_level_remaining, 0, 16 * sizeof(int));
		memset(coeff_sign_flag, 0, 16 * sizeof(int));

		xS = ScanOrderSubBlockX[i];
		yS = ScanOrderSubBlockY[i];
		csbfCtx = 0;
		if (xS < (1 << (log2TrafoSize - 2)) - 1)
			csbfCtx += coded_sub_block_flag[yS][xS + 1];
		if (yS < (1 << (log2TrafoSize - 2)) - 1)
			csbfCtx += coded_sub_block_flag[yS + 1][xS];
		prevCsbf = 0;
		if (xS < (1 << (log2TrafoSize - 2)) - 1)
			prevCsbf += coded_sub_block_flag[yS][xS + 1];
		if (yS < (1 << (log2TrafoSize - 2)) - 1)
			prevCsbf += coded_sub_block_flag[yS + 1][xS] << 1;
		//9.3.4.2.4 csbfCtx和9.3.4.2.5 prevCsbf两个定义不一样，仅这个<<1
		inferSbDcSigCoeffFlag = 0;


		//当sig_coeff_flag没有表示, If(xC, yC) is the last significant location in scan order or all of the following conditions are true
		//这里是or，last scan pos的sig_coeff_flag是1
		if (i == lastSubBlock) {
			sig_coeff_flag[lastScanPos] = 1;
		}

		if ((i < lastSubBlock) && (i > 0))
		{
			parse_coded_sub_block_flag(rbsp, &coded_sub_block_flag[yS][xS], cIdx, csbfCtx);
			inferSbDcSigCoeffFlag = 1;
		}
		else {
			//7.4.9.11 Residual coding semantics
			if ((xS == 0 && yS == 0) ||
				((xS == (LastSignificantCoeffX >> 2)) && (yS == (LastSignificantCoeffY >> 2))))
				coded_sub_block_flag[yS][xS] = 1;
			else{
				//上面xS==0&&yS==0就是i==0的情况，
				//xS == LastSignificantCoeffX >> 2 && yS == LastSignificantCoeffY >> 2就是i==LastSubBlock的情况
				//所以这里不会进来
				coded_sub_block_flag[yS][xS] = 0;
			}
				
		}


		for (n = (i == lastSubBlock) ? lastScanPos - 1 : 15; n >= 0; n--)
		{
			//xC,yC current location, xS,yS subblock loaction, xS=xC>>2,都是CU内部偏移坐标
			// inner sub-block location ( xP, yP ) 
			xC = (xS << 2) + ScanOrderX[n];
			yC = (yS << 2) + ScanOrderY[n];
			//Log("xC %d yC %d xS %d yS %d prevCsbf %d\n", xC, yC, xS, yS, prevCsbf);
			if (xC == 1 && yC == 3 && xS == 0 && yS == 0)
				int d = 1;
			//只有1种情况不需要解析sig_coeff_flag, 是DC（也就是n=0，即xC&3=0,yC&3=0），并且inferSbDcSigCoeffFlag=1
			//inferSbDcSigCoeffFlag infer subblock dc sig coeff,顾名思义就是一个4x4 subblock的dc推导得到,不需要解码
			if (coded_sub_block_flag[yS][xS] && (n > 0 || !inferSbDcSigCoeffFlag)) {
				parse_sig_coeff_flag(rbsp, &sig_coeff_flag[n], cIdx, prevCsbf, xC, yC, xS, yS, scanIdx, log2TrafoSize);
			}
			else {
				if (coded_sub_block_flag[yS][xS] && n == 0 && inferSbDcSigCoeffFlag) //fix
					sig_coeff_flag[n] = 1;
				else
					sig_coeff_flag[n] = 0;
			}

			if (sig_coeff_flag[n]) {
				inferSbDcSigCoeffFlag = 0;
			}

		}
		firstSigScanPos = 16;
		lastSigScanPos = -1;
		numGreater1Flag = 0;
		lastGreater1ScanPos = -1;
		if (x0 == 40 && y0 == 0 && i == 0)
			x0 = x0;
		if (coded_sub_block_flag[yS][xS]) // fix
		{
			ctxSet = (i > 0 && cIdx == 0) ? 2 : 0;
			if (!(i == lastSubBlock) && greater1Ctx == 0) {
				ctxSet++;
			}
			greater1Ctx = 1;
		}

		for (n = 15; n >= 0; n--)
		{
			if(sig_coeff_flag[n])
			{
				if(numGreater1Flag < 8)
				{
					int ctxInc = (ctxSet << 2) + min(3, greater1Ctx);
					parse_coeff_abs_level_greater1_flag(rbsp, &coeff_abs_level_greater1_flag[n],
						cIdx, ctxInc);
					if (coeff_abs_level_greater1_flag[n]) {
						greater1Ctx = 0;
					} else if (greater1Ctx > 0 && greater1Ctx < 3) { //与spec对不起来
						greater1Ctx++;
					}
					numGreater1Flag++;
					if( coeff_abs_level_greater1_flag[n] && lastGreater1ScanPos == -1)
						lastGreater1ScanPos = n; //顾名思义，最后1个大于1的系数所在的位置，也就是从后开始扫，最先扫到的第一个大于1的系数
				}
				if(lastSigScanPos == -1)
					lastSigScanPos = n; //第一次扫到的非0系数所在的位置,系数是倒着放的,第一次扫到的就是最后一个系数
				firstSigScanPos = n; //也就是最后一次扫到的非0系数所在的位置
			}
		}
		signHidden = (lastSigScanPos - firstSigScanPos > 3 && !cu->cu_transquant_bypass_flag);
		if (lastGreater1ScanPos != -1)
			parse_coeff_abs_level_greater2_flag(rbsp,
				&coeff_abs_level_greater2_flag[lastGreater1ScanPos], cIdx,
				ctxSet);
		for (n = 15; n >= 0; n--)
		{
			if(sig_coeff_flag[n] &&
				(!pps->sign_data_hiding_enabled_flag || !signHidden || (n != firstSigScanPos)))
				bypass_decode_bin(rbsp, &coeff_sign_flag[n]);
		}
		numSigCoeff = 0;
		sumAbsLevel = 0;
		cAbsLevel = 0;
		cRiceParam = 0;
		for (n = 15; n >= 0; n--) {
			if (sig_coeff_flag[n])
			{
				xC = (xS << 2) + ScanOrderX[n];
				yC = (yS << 2) + ScanOrderY[n];
				baseLevel = 1 + coeff_abs_level_greater1_flag[n] + coeff_abs_level_greater2_flag[n];

				if (baseLevel == ((numSigCoeff < 8) ? ((n == lastGreater1ScanPos) ? 3 : 2) : 1))
					parse_coeff_abs_level_remaining(rbsp, &coeff_abs_level_remaining[n], cRiceParam);
				cAbsLevel = baseLevel + coeff_abs_level_remaining[n];
				cRiceParam = min(cRiceParam + (cAbsLevel > (3 * (1 << cRiceParam)) ? 1 : 0), 4);
				TransCoeffLevel[yC][xC] = (coeff_abs_level_remaining[n] + baseLevel) * (1 - 2 * coeff_sign_flag[n]);
 
				if (pps->sign_data_hiding_enabled_flag && signHidden)
				{
					sumAbsLevel += (coeff_abs_level_remaining[n] + baseLevel);
                    //只有1个系数适用signHidden
					if((n == firstSigScanPos) && ((sumAbsLevel % 2 ) == 1))
						TransCoeffLevel[yC][xC] = -TransCoeffLevel[yC][xC];

				}
				Log_data("TransCoeffLevel[%d][%d] = %d\n", yC, xC, TransCoeffLevel[yC][xC]);
				numSigCoeff++;
			}
		}
	}

	if (cu->cu_transquant_bypass_flag) {
	
	} else {
		//8.6.3 Scaling process for transform coefficients
		int bdShift = 3 + log2TrafoSize; //8-17 BitDepthY/BitDepthC + Log2(nTbS) − 5
		int levelScale[] = {40, 45, 51, 57, 64, 72};
		int m = 16;

		//8.6.3 Scaling process for transform coefficients
		for (y = 0; y < trafoSize; y++) {
			for (x = 0; x < trafoSize; x++) {
				d[y][x] = Clip3(-32768, 32767, ((TransCoeffLevel[y][x] * m *
					levelScale[qP%6]<<(qP/6))+(1<<(bdShift-1)))>>bdShift); //8-21

				if (d[y][x])
					Log_data("Scale [%d] %d\n", y*trafoSize+x, d[y][x]);
			}
		}

		if (transform_skip_flag) {
			//8.6.2 Scaling and transformation process
			int bdShift = 12; //bdShift = (cIdx == 0) ? 20 − BitDepthY : 20 − BitDepthC
			for (y = 0; y < trafoSize; y++) {
				for (x = 0; x < trafoSize; x++) {
					r[y][x] = d[y][x] << 7; //8-14
					r[y][x] = (r[y][x] + (1 << (bdShift - 1))) >> bdShift; //8-16
					if (r[y][x])
						Log_data("dequant [%d] %d\n", y*trafoSize+x, r[y][x]);
				}
			}
		} else {
			transform(d, r, trafoSize, cIdx, x0, y0);
		}
		
	}


}

//        |                  |
//     ___|_B________________|___
//      A |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |

//8.4.2	Derivation process for luma intra prediction mode
void get_cand_mode_list(int x0, int y0, int candModeList[])
{
	int candIntraPredModeA, candIntraPredModeB;

	if (x0 > 0) {
		if (CurrPic->cu_predmode[y0>>MinCbLog2SizeY][(x0-1)>>MinCbLog2SizeY] == MODE_INTRA)
			candIntraPredModeA = intra_luma_pred_mode[y0>>2][(x0-1) >> 2];
		else
			candIntraPredModeA = INTRA_DC;
	} else 
		candIntraPredModeA = INTRA_DC;
	if (y0 > 0) {
		if (CurrPic->cu_predmode[(y0-1)>>MinCbLog2SizeY][x0>>MinCbLog2SizeY] == MODE_INTRA)
			candIntraPredModeB = intra_luma_pred_mode[(y0-1)>>2][x0>>2];
		else
			candIntraPredModeB = INTRA_DC;
	} else 
		candIntraPredModeB = INTRA_DC;
	// 8.4.2 Otherwise, if X is equal to B and yPb - 1 is less than ( ( yPb  >>  CtbLog2SizeY )  <<  CtbLog2SizeY ), candIntraPredModeB is set equal to INTRA_DC. 
	// intra_pred_mode prediction does not cross vertical CTB boundaries,y0-1即B位于上一个CTB，就不行，只能位于本CTB
	if ((y0 - 1) < ((y0 >> CtbLog2SizeY) << CtbLog2SizeY))
		candIntraPredModeB = INTRA_DC;

	if (candIntraPredModeB == candIntraPredModeA) {
		if (candIntraPredModeA < 2) {
			candModeList[0] = INTRA_PLANAR;
			candModeList[1] = INTRA_DC;
			candModeList[2] = INTRA_ANGULAR26;
		} else {
			candModeList[0] = candIntraPredModeA;
			candModeList[1] = ((candIntraPredModeA + 29) % 32) + 2;
			candModeList[2] = ((candIntraPredModeA - 1 ) % 32) + 2;
		}
	} else {
		candModeList[0] = candIntraPredModeA;
		candModeList[1] = candIntraPredModeB;
		if (candIntraPredModeA != INTRA_PLANAR &&
			candIntraPredModeB != INTRA_PLANAR) {
			candModeList[2] = INTRA_PLANAR;
		}
		else if (candIntraPredModeA != INTRA_DC &&
			candIntraPredModeB != INTRA_DC) {
			candModeList[2] = INTRA_DC;
		}
		else {
			candModeList[2] = INTRA_ANGULAR26;
		}

	}

}

//a variable partIdx specifying the index of the current prediction unit within the current coding unit
//xoff, yoff PU相对CU偏移
void parse_prediction_unit(unsigned char *rbsp, int x0, int y0, int nPbW, int nPbH, int partIdx, struct CodingUnit *cu,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head, int *merge_flag)
{
	int merge_idx = 0;
	struct Mv mvd, mvp;
	int mvp_l0_flag;
	int ref_idx_l0;
	int x, y;
	int xoff = x0 - cu->x0, yoff = y0 - cu->y0;
	struct MvField mergeCandList[5];
	int MaxNumMergeCand = 5 - slice_head->five_minus_max_num_merge_cand;

	if (cu_skip_flag[y0 >> MinCbLog2SizeY][x0 >> MinCbLog2SizeY]) {
		if (MaxNumMergeCand > 1)
			parse_merge_idx(rbsp, &merge_idx, MaxNumMergeCand);
		derive_merging_candidates(x0, y0, nPbW, nPbH, partIdx, pps, slice_head, cu, mergeCandList, merge_idx);
		for (x = x0;
			(x < (x0 + nPbW) && x < PicWidthInSamplesY);
			x += 4) {
			for (y = y0;
				(y < (y0 + nPbH) && y < PicWidthInSamplesY);
				y += 4) {
				CurrPic->mvf[y>>2][x>>2] = mergeCandList[merge_idx];
				CurrPic->ref_poc[y >> 2][x >> 2] = CurrPic->rpl->ref[mergeCandList[merge_idx].refIdx]->slice_head.POC;
			}
		}
		Log_data("cu_skip x0 %d y0 %d nPbW %d nPbH %d mv %d %d refIdx %d\n",
			x0, y0, nPbW, nPbH, mergeCandList[merge_idx].mv.mv[0], mergeCandList[merge_idx].mv.mv[1], mergeCandList[merge_idx].refIdx);
		*merge_flag = 1; //–If CuPredMode[x0][y0] is equal to MODE_SKIP, merge_flag[x0][y0] is inferred to be equal to 1.
		ref_idx_l0 = mergeCandList[merge_idx].refIdx;
		mvp = mergeCandList[merge_idx].mv;

	} else { // mode inter

		parse_merge_flag(rbsp, merge_flag);
		if (*merge_flag) {
			if (MaxNumMergeCand > 1)
				parse_merge_idx(rbsp, &merge_idx, MaxNumMergeCand);

			derive_merging_candidates(x0, y0, nPbW, nPbH, partIdx, pps, slice_head, cu, mergeCandList, merge_idx);
			for (x = x0;
				(x < (x0 + nPbW) && x < PicWidthInSamplesY);
				x += 4) {
				for (y = y0;
					(y < (y0 + nPbH) && y < PicWidthInSamplesY);
					y += 4) {
					CurrPic->mvf[y>>2][x>>2] = mergeCandList[merge_idx];
					CurrPic->ref_poc[y >> 2][x >> 2] = CurrPic->rpl->ref[mergeCandList[merge_idx].refIdx]->slice_head.POC;
				}
			}
			ref_idx_l0 = mergeCandList[merge_idx].refIdx;
			mvp = mergeCandList[merge_idx].mv;
			Log_data("x0 %d y0 %d nPbW %d nPbH %d mv %d %d refIdx %d\n",
				x0, y0, nPbW, nPbH, mergeCandList[merge_idx].mv.mv[0], mergeCandList[merge_idx].mv.mv[1], mergeCandList[merge_idx].refIdx);
		} else {
			if (slice_head->numRefIdx > 0)
				parse_ref_idx_l0(rbsp, &ref_idx_l0, slice_head->numRefIdx - 1);
			parse_mvd_coding(rbsp, &mvd);
			parse_mvp_l0_flag(rbsp, &mvp_l0_flag);
			derive_motion_vector_prediction(x0, y0, nPbW, nPbH, partIdx, pps, slice_head, mvp_l0_flag, ref_idx_l0, &mvp);
			mvp.mv[0] += mvd.mv[0];
			mvp.mv[1] += mvd.mv[1];
			Log_data("mvp mode x0 %d y0 %d nPbW %d nPbH %d mv %d %d refIdx %d\n",
				x0, y0, nPbW, nPbH, mvp.mv[0], mvp.mv[1], ref_idx_l0);
			for (x = x0;
				(x < (x0 + nPbW) && x < PicWidthInSamplesY);
				x += 4) {
				for (y = y0;
					(y < (y0 + nPbH) && y < PicHeightInSamplesY);
					y += 4) {
					CurrPic->mvf[y>>2][x>>2].refIdx = ref_idx_l0;
					CurrPic->mvf[y>>2][x>>2].mv = mvp;
					CurrPic->ref_poc[y >> 2][x >> 2] = CurrPic->rpl->ref[ref_idx_l0]->slice_head.POC;
				}
			}
		}
	}
	inter_pred(cu->x0, cu->y0, xoff, yoff, nPbW, nPbH, mvp.mv, ref_idx_l0);
}

void scale(int pocDiff1, int pocDiff2, struct Mv *mv)
{
	int distScaleFactor, td, tb, tx;
	td = Clip3(-128, 127, pocDiff1);
	tb = Clip3(-128, 127, pocDiff2);
	tx = (16384 + (abs(td) >> 1)) / td;
	distScaleFactor = Clip3(-4096, 4095, (tb * tx + 32) >> 6);

	mv->mv[0] = Clip3(-32768, 32767, Sign(distScaleFactor * mv->mv[0]) * ((abs(distScaleFactor * mv->mv[0]) + 127) >> 8));
	mv->mv[1] = Clip3(-32768, 32767, Sign(distScaleFactor * mv->mv[1]) * ((abs(distScaleFactor * mv->mv[1]) + 127) >> 8));
}

//8.5.3.2.5 derive mvp 非merge
//mvp_l0_flag和merge_idx一样的功能
//同样是A0, A1, B0, B1, B2, merge和非merge还是有点区别，一个是A0，A1一组得到其一，B0，B1，B2一组得到其二，
//一个是5个里面选4个, merge的叫spatial，非merge的叫neighbor
void derive_motion_vector_prediction(int xPb, int yPb, int nPbW, int nPbH, int partIdx,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head,
	int mvp_l0_flag, int ref_idx_l0, struct Mv *mvp)
{
	struct Mv mvpList[3];
	int numMvpCand = 0;
	struct Mv mvA, mvB;
	struct MvField mvfCol;
	int xNbA0 = xPb - 1, yNbA0 = yPb + nPbH;
	int xNbA1 = xPb - 1, yNbA1 = yPb + nPbH - 1;
	int xNbB1 = xPb + nPbW - 1, yNbB1 = yPb - 1;
	int xNbB0 = xPb + nPbW, yNbB0 = yPb - 1;
	int xNbB2 = xPb - 1, yNbB2 = yPb - 1;
	int availableA0 = 1, availableA1 = 1, availableB1 = 1, availableB0 = 1, availableB2 = 1;
	int availableFlagA = 0, availableFlagB = 0;
	int isScaledFlag = 0;
	int pocDiff1, pocDiff2;
	// A0
	if (test_block_available(xPb, yPb + nPbH - 1, xNbA0, yNbA0) == 0) {
		availableA0 = 0;
	}
	else if (CurrPic->cu_predmode[yNbA0 >> MinCbLog2SizeY][xNbA0 >> MinCbLog2SizeY] == MODE_INTRA) { //这条spec没有，但是应该要有啊
		availableA0 = 0;
	}

	// A1
	// 8.5.3.2.6  Derivation process for motion vector predictor candidates 
	if (xPb == 0) {
		availableA1 = 0;
	} else if (CurrPic->cu_predmode[yNbA1>>MinCbLog2SizeY][xNbA1>>MinCbLog2SizeY] == MODE_INTRA) {
		availableA1 = 0;
	}
	if (availableA1 || availableA0)
		isScaledFlag = 1;
	if (availableA0) {

		//邻近块A0/A1的参考帧和当前块参考帧指向同一帧
		if (DiffPicOrderCnt(CurrPic->rpl->ref[ref_idx_l0],CurrPic->rpl->ref[CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].refIdx]) == 0) {
			availableFlagA = 1;
			mvA = CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].mv;
			goto b_candidates;
		}


	}
	if (availableA1) {

		if (DiffPicOrderCnt(CurrPic->rpl->ref[ref_idx_l0], CurrPic->rpl->ref[CurrPic->mvf[yNbA1 >> 2][xNbA1 >> 2].refIdx]) == 0) {
			availableFlagA = 1;
			mvA = CurrPic->mvf[yNbA1 >> 2][xNbA1 >> 2].mv;
			goto b_candidates;
		}
	}

	if (availableA0) { //LongTermRefPic相同的情况，同为long term或者同为非long term
		availableFlagA = 1;
		mvA = CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].mv;
		pocDiff1 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].refIdx]);
		pocDiff2 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[ref_idx_l0]);
		scale(pocDiff1, pocDiff2, &mvA);
		goto b_candidates;
	}
	if (availableA1) {
		availableFlagA = 1;
		mvA = CurrPic->mvf[yNbA1 >> 2][xNbA1 >> 2].mv;
		pocDiff1 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[CurrPic->mvf[yNbA1 >> 2][xNbA1 >> 2].refIdx]);
		pocDiff2 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[ref_idx_l0]);
		scale(pocDiff1, pocDiff2, &mvA);
	}

b_candidates:
	//todo A长期参考帧，并且scale
	if (test_block_available(xPb + nPbW - 1, yPb, xNbB0, yNbB0) == 0) {
		availableB0 = 0;
	} else if (CurrPic->cu_predmode[yNbB0>>MinCbLog2SizeY][xNbB0>>MinCbLog2SizeY] == MODE_INTRA) {
		availableB0 = 0;
	}

	if (yPb == 0) {
		availableB1 = 0;
	}
	else if (CurrPic->cu_predmode[yNbB1 >> MinCbLog2SizeY][xNbB1 >> MinCbLog2SizeY] == MODE_INTRA) {
		availableB1 = 0;
	}

	if (xPb == 0 || yPb == 0) {
		availableB2 = 0;
	}
	else if (CurrPic->cu_predmode[yNbB2 >> MinCbLog2SizeY][xNbB2 >> MinCbLog2SizeY] == MODE_INTRA) {
		availableB2 = 0;
	}

	if (availableB0) {

		if (DiffPicOrderCnt(CurrPic->rpl->ref[ref_idx_l0], CurrPic->rpl->ref[CurrPic->mvf[yNbB0 >> 2][xNbB0 >> 2].refIdx]) == 0) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB0 >> 2][xNbB0 >> 2].mv;
			goto scalef;
		}
		
	}


	if (availableB1) {

		if (DiffPicOrderCnt(CurrPic->rpl->ref[ref_idx_l0], CurrPic->rpl->ref[CurrPic->mvf[yNbB1 >> 2][xNbB1 >> 2].refIdx]) == 0) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB1 >> 2][xNbB1 >> 2].mv;
			goto scalef;
		}
		
	}


	if (availableB2) {

		if (DiffPicOrderCnt(CurrPic->rpl->ref[ref_idx_l0], CurrPic->rpl->ref[CurrPic->mvf[yNbB2 >> 2][xNbB2 >> 2].refIdx]) == 0) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB2 >> 2][xNbB2 >> 2].mv;
		}

	}


scalef:

	if (isScaledFlag == 0) {
		if (availableFlagB == 1) {
			availableFlagA = 1;
			mvA = mvB; // (8-161)
		}
		if (availableB0) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB0 >> 2][xNbB0 >> 2].mv;

			pocDiff1 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[CurrPic->mvf[yNbB0 >> 2][xNbB0 >> 2].refIdx]);
			pocDiff2 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[ref_idx_l0]);
			scale(pocDiff1, pocDiff2, &mvB);
			goto done;

		}
		if (availableB1) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB1 >> 2][xNbB1 >> 2].mv;

			pocDiff1 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[CurrPic->mvf[yNbB1 >> 2][xNbB1 >> 2].refIdx]);
			pocDiff2 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[ref_idx_l0]);
			scale(pocDiff1, pocDiff2, &mvB);
			goto done;
		}
		if (availableB2) {
			availableFlagB = 1;
			mvB = CurrPic->mvf[yNbB2 >> 2][xNbB2 >> 2].mv;

			pocDiff1 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[CurrPic->mvf[yNbB2 >> 2][xNbB2 >> 2].refIdx]);
			pocDiff2 = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[ref_idx_l0]);
			scale(pocDiff1, pocDiff2, &mvB);

		}
	}

done:
	if (availableFlagA)
		mvpList[numMvpCand++] = mvA;

	if (availableFlagB && (!availableFlagA || mvA.mv[0] != mvB.mv[0] || mvA.mv[1] != mvB.mv[1]))
		mvpList[numMvpCand++] = mvB;
	//temporal motion vector prediction candidate
	if (numMvpCand < 2 && slice_head->slice_temporal_mvp_enabled_flag &&
		mvp_l0_flag == numMvpCand) {
		mvfCol.refIdx = ref_idx_l0;
		if (derive_temporal_motion_vector(xPb, yPb, nPbW, nPbH, slice_head, &mvfCol))
			mvpList[numMvpCand++] = mvfCol.mv;
	}
	//fix –  When numMvpCandLX is less than 2, the following applies repeatedly until numMvpCandLX is equal to 2:
	//mvpListLX[numMvpCandLX][0] = 0  (8 - 141)
	//	mvpListLX[numMvpCandLX][1] = 0  (8 - 142)
	//	numMvpCandLX = numMvpCandLX + 1  (8 - 143)
	while (numMvpCand < 2) {
		mvpList[numMvpCand].mv[0] = 0;
		mvpList[numMvpCand].mv[1] = 0;
		numMvpCand++;
	}
	*mvp = mvpList[mvp_l0_flag];
}

int derive_temporal_motion_vector(int xPb, int yPb, int nPbW, int nPbH, struct slice_segment_header *slice_head,
	struct MvField *mvfCol)
{
	struct HEVCFrame *colPic = CurrPic->rpl->ref[slice_head->collocated_ref_idx];
	int availableFlagCol = 0;
	int xColBr = xPb + nPbW; //bottom right
	int yColBr = yPb + nPbH;
	int xColPb, yColPb;


	int colPocDiff, currPocDiff;


	if ((yColBr >> CtbLog2SizeY) == (yPb >> CtbLog2SizeY) &&//lies outside the CTB row
		xColBr < PicWidthInSamplesY &&
		yColBr < PicHeightInSamplesY) {
		xColPb = xColBr & ~15;
		yColPb = yColBr & ~15;
		if (colPic->cu_predmode[yColPb >> MinCbLog2SizeY][xColPb >> MinCbLog2SizeY] != MODE_INTRA) {
			//colPocDiff = DiffPicOrderCnt(colPic, refPicListCol[refIdxCol])  (8-177)
			//currPocDiff = DiffPicOrderCnt(currPic, RefPicListX[refIdxLX])  (8-178)
			//当前帧poc和当前块参考的参考帧poc
			//Collaborate picture和bottom right这个块参考的参考帧
			currPocDiff = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[mvfCol->refIdx]);
			colPocDiff = colPic->slice_head.POC - colPic->ref_poc[yColPb >> 2][xColPb >> 2];
			mvfCol->mv = colPic->mvf[yColPb >> 2][xColPb >> 2].mv; //fix temporal只得到mv，不得ref_idx
			if (currPocDiff != colPocDiff) {
				scale(colPocDiff, currPocDiff, &mvfCol->mv);
			}

			availableFlagCol = 1;
		}

	}
	if (availableFlagCol == 0) {
		//derive center collocated motion vector
		int xColCenter = xPb + (nPbW >> 1);
		int yColCenter = yPb + (nPbH >> 1);
		xColPb = xColCenter & ~15;
		yColPb = yColCenter & ~15;
		if (colPic->cu_predmode[yColPb >> MinCbLog2SizeY][xColPb >> MinCbLog2SizeY] != MODE_INTRA) {
			currPocDiff = DiffPicOrderCnt(CurrPic, CurrPic->rpl->ref[mvfCol->refIdx]);
			colPocDiff = colPic->slice_head.POC - colPic->ref_poc[yColPb >> 2][xColPb >> 2];
			mvfCol->mv = colPic->mvf[yColPb >> 2][xColPb >> 2].mv;
			if (currPocDiff != colPocDiff) {
				scale(colPocDiff, currPocDiff, &mvfCol->mv);
			}

			availableFlagCol = 1;
		}
	}
	return availableFlagCol;
}
//spatial candidate
//     B2 |               B1 |B0
//     ___|__________________|___
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//        |                  |
//     A1 |                  |
//    ---- ------------------
//     A0 |
//        
//	merge mode  4种 spatial, 8.5.3.2.2,A0,A1,B0,B1,B2是spatial
//		temporal 8.5.3.2.7
//		combined bi - predictive 8.5.3.2.3
//		zero 8.5.3.2.4
void derive_merging_candidates(int xPb, int yPb, int nPbW, int nPbH, int partIdx,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head, struct CodingUnit *cu,
	struct MvField *mergeCandList, int merge_idx)
{
	//8.5.3.2.2
	int xNbA0 = xPb - 1, yNbA0 = yPb + nPbH;
	int xNbA1 = xPb - 1, yNbA1 = yPb + nPbH - 1;
	int xNbB1 = xPb + nPbW - 1, yNbB1 = yPb - 1;
	int xNbB0 = xPb + nPbW, yNbB0 = yPb - 1;
	int xNbB2 = xPb - 1, yNbB2 = yPb - 1;
	int availableA1 = 1, availableB1 = 1, availableB0 = 1, availableB2 = 1, availableA0 = 1;
	int availableFlagA1 = 1, availableFlagB1 = 1, availableFlagB0 = 1, availableFlagB2 = 1, availableFlagA0 = 1;
	struct Mv mvA0, mvA1, mvB0, mvB1, mvB2;
	int ref_idxA0, ref_idxA1, ref_idxB0, ref_idxB1, ref_idxB2;
	int Log2ParMrgLevel = pps->log2_parallel_merge_level_minus2 + 2;
	int i = 0;
	int MaxNumMergeCand = merge_idx + 1;// MaxNumMergeCand = 5 - slice_head->five_minus_max_num_merge_cand,求到merge_idx就可以了，没必要MaxNumMergeCand个全求出来
	int zeroIdx = 0;

	if (xPb == 0) {
		availableA1 = 0;
	} else if ((xPb  >>  Log2ParMrgLevel) == (xNbA1 >>  Log2ParMrgLevel) &&
		(yPb  >>  Log2ParMrgLevel) == (yNbA1 >>  Log2ParMrgLevel)) {
		availableA1 = 0;
	} else if ((cu->part_mode == PART_Nx2N ||
		cu->part_mode == PART_nLx2N ||
		cu->part_mode == PART_nRx2N) &&
		partIdx == 1) {
		availableA1 = 0;
	} else if (CurrPic->cu_predmode[yNbA1>>MinCbLog2SizeY][xNbA1>>MinCbLog2SizeY] == MODE_INTRA) { //这条201612spec没有，但是应该要有啊
		availableA1 = 0;
	}
	//HEVC IEEE
	//After validating the spatial candidates, two kinds of redun-dancy are removed. If the candidate position for the current
	//PU would refer to the first PU within the same CU, the  
	//position is excluded, as the same merge could be achieved by
	//a CU without splitting into prediction partitions. Furthermore,
	//any redundant entries where candidates have exactly the same  
	//motion information are also excluded.

	availableFlagA1 = availableA1;
	if (availableFlagA1) {
		mvA1 = CurrPic->mvf[yNbA1>>2][xNbA1>>2].mv;
		ref_idxA1 = CurrPic->mvf[yNbA1>>2][xNbA1>>2].refIdx;
		mergeCandList[i] = CurrPic->mvf[yNbA1>>2][xNbA1>>2];
		i++;
		if (i == MaxNumMergeCand)
			return;
	}
	if (yPb == 0) {
		availableB1 = 0;
	} else if ((xPb >> Log2ParMrgLevel) == (xNbB1 >> Log2ParMrgLevel) &&
		(yPb >> Log2ParMrgLevel) == (yNbB1 >> Log2ParMrgLevel)) {
		availableB1 = 0;
	}
	else if ((cu->part_mode == PART_2NxN ||
		cu->part_mode == PART_2NxnU ||
		cu->part_mode == PART_2NxnD) &&
		partIdx == 1) {
		availableB1 = 0;
	} else if (CurrPic->cu_predmode[yNbB1>>MinCbLog2SizeY][xNbB1>>MinCbLog2SizeY] == MODE_INTRA) {
		availableB1 = 0;
	}
	if (availableB1) {
		mvB1 = CurrPic->mvf[yNbB1>>2][xNbB1>>2].mv;
		ref_idxB1 = CurrPic->mvf[yNbB1>>2][xNbB1>>2].refIdx;
	}
	if (availableB1 == 0 || (availableA1 == 1 && mvB1.mv[0] == mvA1.mv[0] &&
		mvB1.mv[1] == mvA1.mv[1] && ref_idxB1 == ref_idxA1))
		availableFlagB1 = 0;

	if (availableFlagB1) {
		mergeCandList[i] = CurrPic->mvf[yNbB1>>2][xNbB1>>2];
		i++;
		if (i == MaxNumMergeCand)
			return;
	}

	if (test_block_available(xPb + nPbW - 1, yPb, xNbB0, yNbB0) == 0) {
		availableB0 = 0;
	} else if ((xPb  >>  Log2ParMrgLevel) == (xNbB0 >>  Log2ParMrgLevel) &&
		(yPb  >>  Log2ParMrgLevel) == (yNbB0 >>  Log2ParMrgLevel)) {
		availableB0 = 0;
	} else if (CurrPic->cu_predmode[yNbB0>>MinCbLog2SizeY][xNbB0>>MinCbLog2SizeY] == MODE_INTRA) {
		availableB0 = 0;
	}
	if (availableB0) {
		mvB0 = CurrPic->mvf[yNbB0>>2][xNbB0>>2].mv;
		ref_idxB0 = CurrPic->mvf[yNbB0>>2][xNbB0>>2].refIdx;
	}
	if (availableB0 == 0 || (availableB1 == 1 && mvB1.mv[0] == mvB0.mv[0] &&
		mvB1.mv[1] == mvB0.mv[1] && ref_idxB1 == ref_idxB0))
		availableFlagB0 = 0;

	if (availableFlagB0) {
		mergeCandList[i]= CurrPic->mvf[yNbB0>>2][xNbB0>>2];
		i++;
		if (i == MaxNumMergeCand)
			return;
	}

	//A0
	if (test_block_available(xPb, yPb + nPbH - 1, xNbA0, yNbA0) == 0) {
		availableA0 = 0;
	}
	else if ((xPb >> Log2ParMrgLevel) == (xNbA0 >> Log2ParMrgLevel) &&
		(yPb >> Log2ParMrgLevel) == (yNbA0 >> Log2ParMrgLevel)) {
		availableA0 = 0;
	}
	else if (CurrPic->cu_predmode[yNbA0 >> MinCbLog2SizeY][xNbA0 >> MinCbLog2SizeY] == MODE_INTRA) {
		availableA0 = 0;
	}
	if (availableA0) {
		mvA0 = CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].mv;
		ref_idxA0 = CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2].refIdx;
	}
	if (availableA0 == 0 || (availableA1 == 1 && mvA1.mv[0] == mvA0.mv[0] &&
		mvA1.mv[1] == mvA0.mv[1] && ref_idxA1 == ref_idxA0))
		availableFlagA0 = 0;

	if (availableFlagA0) {
		mergeCandList[i] = CurrPic->mvf[yNbA0 >> 2][xNbA0 >> 2];
		i++;
		if (i == MaxNumMergeCand)
			return;
	}
	//B2
	if (xPb == 0 || yPb == 0) {
		availableB2 = 0;
	} else if ((xPb  >>  Log2ParMrgLevel) == (xNbB2 >>  Log2ParMrgLevel) &&
		(yPb  >>  Log2ParMrgLevel) == (yNbB2 >>  Log2ParMrgLevel)) {
		availableB2 = 0;
	} else if (CurrPic->cu_predmode[yNbB2>>MinCbLog2SizeY][xNbB2>>MinCbLog2SizeY] == MODE_INTRA) {
		availableB2 = 0;
	}
	if (availableB2) {
		mvB2 = CurrPic->mvf[yNbB2>>2][xNbB2>>2].mv;
		ref_idxB2 = CurrPic->mvf[yNbB2>>2][xNbB2>>2].refIdx;
	}
	if (availableB2 == 0 || (availableA1 == 1 && mvA1.mv[0] == mvB2.mv[0] && mvA1.mv[1] == mvB2.mv[1] && ref_idxA1 == ref_idxB2) ||
		(availableB1 == 1 && mvB1.mv[0] == mvB2.mv[0] && mvB1.mv[1] == mvB2.mv[1] && ref_idxB1 == ref_idxB2) ||
		(availableFlagA0 + availableFlagA1 + availableFlagB0 + availableFlagB1 == 4))
		availableFlagB2 = 0;

	if (availableFlagB2) {
		mergeCandList[i] = CurrPic->mvf[yNbB2>>2][xNbB2>>2];
		i++;
		if (i == MaxNumMergeCand)
			return;
	}
	//8.5.3.2.7 temporal
	if (slice_head->slice_temporal_mvp_enabled_flag) {
		mergeCandList[i].refIdx = 0;
		if (derive_temporal_motion_vector(xPb, yPb, nPbW, nPbH, slice_head, &mergeCandList[i])) {
			i++;
			if (i == MaxNumMergeCand)
				return;
		}
	}
	// append zero motion vector candidates
	//8.5.3.2.4	Derivation process for zero motion vector merging candidates
	while (i < MaxNumMergeCand) {
		mergeCandList[i].refIdx = zeroIdx < slice_head->numRefIdx ? zeroIdx : 0;
		mergeCandList[i].mv.mv[0] = 0;
		mergeCandList[i].mv.mv[1] = 0;
		i++;
		zeroIdx++;
	}
}

unsigned char stream_buffer[MAX_NALU_LEN]; // bug fix 
unsigned char rbsp[MAX_NALU_LEN];

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned int aa = 0x67c33 << 1;

	FILE* input = NULL;
	FILE* output = NULL;
	fopen_s(&input, "test.265", "rb");
	assert(input != NULL);
	fopen_s(&output, "out.yuv", "wb");
	assert(output != NULL);
	fopen_s(&log_info, "log_trace2.txt", "wb");

	struct nal_unit_header nalu_head = { 0 };
	int offset = 0;
	unsigned int nalu_len = 0;
	int ret = 0;

	slice_num = 0;

	do
	{
		ret = read_one_nalu(input, offset, stream_buffer, &nalu_head, rbsp, &nalu_len);
		offset += nalu_len;
		bytes_offset = 0;
		bits_offset = 0;
		if (nalu_head.nal_unit_type == 32) //vps
		{
			parse_vps(rbsp, &Vps);

		}
		else if (nalu_head.nal_unit_type == 33) //sps
		{
			parse_sps(rbsp, &Sps, &Vps);

		}
		else if (nalu_head.nal_unit_type == 34) //pps
		{
			parse_pps(rbsp, &Pps);
			InitDecoder(&Pps, &Sps);
		}
		else if (nalu_head.nal_unit_type == 19 ||
			nalu_head.nal_unit_type == 1) //todo
		{
			parse_slice(rbsp, &nalu_head, &Vps, &Pps, &Sps, &CurrPic->slice_head);
			deblocking_filter(&Pps, &CurrPic->slice_head);
			output_to_file(output);
			InsertFrame();
			slice_num++;
		}
		else
		{
			bytes_offset = 0;
			bits_offset = 0;

		}
	}
	while (ret);
	
	FreeDecoder();

	fclose(input);
	fclose(output);
	fclose(log_info);
	return 0;
}




void InitDecoder(struct pic_parameter_set *pps, struct seq_parameter_set *sps)
{
	int i;
	PicWidthInSamplesY = sps->pic_width_in_luma_samples;
	PicHeightInSamplesY = sps->pic_height_in_luma_samples;
	MinCbLog2SizeY = sps->log2_min_coding_block_size_minus3 + 3; //0+3=3 2^3 = 8
	MinCbSizeY = 1 << MinCbLog2SizeY;
	Log2MinTrafoSize = sps->log2_min_transform_block_size_minus2 + 2;
	Log2MaxTrafoSize = Log2MinTrafoSize + sps->log2_diff_max_min_coding_block_size;


	CtbLog2SizeY = sps->log2_min_coding_block_size_minus3 + 3 + 
		sps->log2_diff_max_min_coding_block_size;
	CtbSizeY = 1 << CtbLog2SizeY;
	Log2MinCuQpDeltaSize = CtbLog2SizeY - pps->diff_cu_qp_delta_depth;

	PicWidthInCtbsY = (PicWidthInSamplesY % CtbSizeY) ?
		PicWidthInSamplesY / CtbSizeY  + 1 : PicWidthInSamplesY / CtbSizeY;
	PicHeightInCtbsY = (PicHeightInSamplesY % CtbSizeY) ?
		PicHeightInSamplesY / CtbSizeY + 1 : PicHeightInSamplesY / CtbSizeY;
	PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;

	sao_params = (struct sao_param *)malloc(sizeof(struct sao_param) * PicSizeInCtbsY);
	get_mem2Dint(&CtDepth, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 0);
	get_mem2Dint(&cu_skip_flag, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 0);

	get_mem2Dint(&qp_y_tab, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 0);
	get_mem2Dint(&verBs, PicHeightInSamplesY >> 2, (PicWidthInSamplesY >> 3) - 1, 0);
	//垂直方向上的分割线，每条相隔8，共有PicWidthInSamplesY >> 3 - 1条，每条又以4像素为单位分成PicHeightInSamplesY >> 2截
	get_mem2Dint(&horBs, (PicHeightInSamplesY >> 3) - 1, PicWidthInSamplesY >> 2, 0);
	get_mem2Dint(&CbfLuma, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, 0);
	get_mem2Dint(&no_filter, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 0);
	get_mem2Dpixel(&SaoBufL, PicHeightInSamplesY, PicWidthInSamplesY, 0);
	get_mem2Dpixel(&SaoBufCb, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);
	get_mem2Dpixel(&SaoBufCr, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);

	get_mem2Dint(&predSamplesL, 64, 64, 0);//CU最大64x64
	get_mem2Dint(&predSamplesCb, 32, 32, 0);
	get_mem2Dint(&predSamplesCr, 32, 32, 0);
	get_mem2Dint(&resSamplesL, 64, 64, 0);
	get_mem2Dint(&resSamplesCb, 32, 32, 0);
	get_mem2Dint(&resSamplesCr, 32, 32, 0);
	//4x4为单位
	get_mem2Dint(&intra_luma_pred_mode, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, 0);


	CurrPic = (struct HEVCFrame *)malloc(sizeof(struct HEVCFrame));
	memset(CurrPic, 0, sizeof(struct HEVCFrame));
	get_mem2Dpixel(&CurrPic->SL, PicHeightInSamplesY, PicWidthInSamplesY, 0);
	get_mem2Dpixel(&CurrPic->SCb, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);
	get_mem2Dpixel(&CurrPic->SCr, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);
	get_mem2D((void ***)&CurrPic->mvf, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, sizeof(struct MvField));
	for (i = 0; i < (PicHeightInSamplesY >> 2) * (PicWidthInSamplesY >> 2); i++)
	{
		CurrPic->mvf[0][i].refIdx = -1;
	}

	get_mem2Dint(&CurrPic->cu_predmode, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 1); //默认memset成MODE_INTRA
	get_mem2Dint(&CurrPic->ref_poc, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, 0);

	CurrPic->rpl = (struct RefPicList *)malloc(sizeof(struct RefPicList));
	memset(CurrPic->rpl, 0, sizeof(struct RefPicList));

	get_mem2Dint(&TransIntermediate, 32, 32, 0);


}

void free_dpb(struct HEVCFrame *dpb)
{
	if (dpb){
		free_mem2Dpixel(dpb->SL);
		free_mem2Dpixel(dpb->SCb);
		free_mem2Dpixel(dpb->SCr);
		free_mem2D((void **)dpb->mvf);
		free_mem2Dint(dpb->cu_predmode);
		free_mem2Dint(dpb->ref_poc);
		free(dpb->rpl);
		free(dpb);
	}

}

void FreeDecoder()
{
	int i;
	free(sao_params);
	free_mem2Dint(CtDepth);
	free_mem2Dint(cu_skip_flag);

	free_mem2Dint(qp_y_tab);
	free_mem2Dint(verBs);
	free_mem2Dint(horBs);
	free_mem2Dint(CbfLuma);
	free_mem2Dint(no_filter);
	free_mem2Dpixel(SaoBufL);
	free_mem2Dpixel(SaoBufCb);
	free_mem2Dpixel(SaoBufCr);
	free_mem2Dint(predSamplesL);
	free_mem2Dint(predSamplesCb);
	free_mem2Dint(predSamplesCr);
	free_mem2Dint(resSamplesL);
	free_mem2Dint(resSamplesCb);
	free_mem2Dint(resSamplesCr);

	free_mem2Dint(intra_luma_pred_mode);


	free_mem2Dpixel(CurrPic->SL);
	free_mem2Dpixel(CurrPic->SCb);
	free_mem2Dpixel(CurrPic->SCr);
	free_mem2D((void **)CurrPic->mvf);
	free_mem2Dint(CurrPic->cu_predmode);
	free(CurrPic->rpl);
	free(CurrPic);
	for (i = 0; i < 32; i++) {
		free_dpb(DPB[i]);
	}

	free_mem2Dint(TransIntermediate);

}

void ResetDecoder()
{
	memset(sao_params, 0, sizeof(struct sao_param) * PicSizeInCtbsY);
	memset(CtDepth[0], 0, (PicHeightInSamplesY >> MinCbLog2SizeY) * (PicWidthInSamplesY >> MinCbLog2SizeY) * sizeof(int));
	memset(cu_skip_flag[0], 0, (PicHeightInSamplesY >> MinCbLog2SizeY) * (PicWidthInSamplesY >> MinCbLog2SizeY) * sizeof(int));
	memset(qp_y_tab[0], 0, (PicHeightInSamplesY >> MinCbLog2SizeY) * (PicWidthInSamplesY >> MinCbLog2SizeY) * sizeof(int));
	memset(verBs[0], 0, (PicHeightInSamplesY >> 2) * ((PicWidthInSamplesY >> 3) - 1) * sizeof(int));
	memset(horBs[0], 0, ((PicHeightInSamplesY >> 3) - 1) * (PicWidthInSamplesY >> 2) * sizeof(int));
	memset(CbfLuma[0], 0, (PicHeightInSamplesY >> 2) * (PicWidthInSamplesY >> 2) * sizeof(int));
	memset(no_filter[0], 0, (PicHeightInSamplesY >> MinCbLog2SizeY) * (PicWidthInSamplesY >> MinCbLog2SizeY) * sizeof(int));

	memset(intra_luma_pred_mode[0], 0, (PicHeightInSamplesY >> 2) * (PicWidthInSamplesY >> 2) * sizeof(int));

}

void InsertFrame()
{
	int i;

	for (i = 0; i < 32; i++) {
		// find a empty slot
		if (DPB[i] == NULL) {
			DPB[i] = CurrPic;
			break;
		}
	}

	CurrPic = (struct HEVCFrame *)malloc(sizeof(struct HEVCFrame));
	memset(CurrPic, 0, sizeof(struct HEVCFrame));
	get_mem2Dpixel(&CurrPic->SL, PicHeightInSamplesY, PicWidthInSamplesY, 0);
	get_mem2Dpixel(&CurrPic->SCb, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);
	get_mem2Dpixel(&CurrPic->SCr, PicHeightInSamplesY >> 1, PicWidthInSamplesY >> 1, 0);
	get_mem2D((void ***)&CurrPic->mvf, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, sizeof(struct MvField));
	for (i = 0; i < (PicHeightInSamplesY >> 2) * (PicWidthInSamplesY >> 2); i++)
	{
		CurrPic->mvf[0][i].refIdx = -1;
	}

	get_mem2Dint(&CurrPic->cu_predmode, PicHeightInSamplesY >> MinCbLog2SizeY, PicWidthInSamplesY >> MinCbLog2SizeY, 1); //默认memset成MODE_INTRA
	get_mem2Dint(&CurrPic->ref_poc, PicHeightInSamplesY >> 2, PicWidthInSamplesY >> 2, 0);
	CurrPic->rpl = (struct RefPicList *)malloc(sizeof(struct RefPicList));
	memset(CurrPic->rpl, 0, sizeof(struct RefPicList));
}



void output_to_file(FILE* out)
{
	fwrite(CurrPic->SL[0], sizeof(unsigned char), PicHeightInSamplesY * PicWidthInSamplesY, out);
	fwrite(CurrPic->SCb[0], sizeof(unsigned char), PicHeightInSamplesY * PicWidthInSamplesY/4, out);
	fwrite(CurrPic->SCr[0], sizeof(unsigned char), PicHeightInSamplesY * PicWidthInSamplesY/4, out);
	fflush(out);

}



void Log(const char* format...)
{

	return;
	//if ( slice_num == 1 )
	{

		char tp[1024] = { 0 };
		va_list param;
		va_start(param, format);
		vsprintf_s(tp, 1024, format, param);
		va_end(param);
		fwrite(tp, strlen(tp), sizeof(char), log_info);
		fflush(log_info);
	}
}

void Log_data(const char* format...)
{

	return;
	//if ( slice_num >= 0 && slice_num <=1200 )
	{

		char tp[1024] = { 0 };
		va_list param;
		va_start(param, format);
		vsprintf_s(tp, 1024, format, param);
		va_end(param);
		fwrite(tp, strlen(tp), sizeof(char), log_info);
		fflush(log_info);
	}
}

void Log_filter(const char* format...)
{

	return;
	//if (slice_num ==0)
	{

		char tp[1024] = { 0 };
		va_list param;
		va_start(param, format);
		vsprintf_s(tp, 1024, format, param);
		va_end(param);
		fwrite(tp, strlen(tp), sizeof(char), log_info);
		fflush(log_info);
	}
}

void Log_deblock(const char* format...)
{

	return;
	//if (slice_num ==0)
	{

		char tp[1024] = { 0 };
		va_list param;
		va_start(param, format);
		vsprintf_s(tp, 1024, format, param);
		va_end(param);
		fwrite(tp, strlen(tp), sizeof(char), log_info);
		fflush(log_info);
	}
}

void Log_ref(const char* format...)
{

	if (1)
	{

		char tp[1024] = { 0 };
		va_list param;
		va_start(param, format);
		vsprintf_s(tp, 1024, format, param);
		va_end(param);
		fwrite(tp, strlen(tp), sizeof(char), log_info);
		fflush(log_info);
	}
}






