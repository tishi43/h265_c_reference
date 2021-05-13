#ifndef _bitstream_header_
#define _bitstream_header_
#include <stdio.h>




enum SliceType
{
	B_SLICE,
	P_SLICE,
	I_SLICE
};
#define MODE_INTRA 1
#define MODE_INTER 0
#define MODE_SKIP 2

#define INTRA_PLANAR             0
#define INTRA_VER                26                    // index for intra VERTICAL   mode
#define INTRA_HOR                10                    // index for intra HORIZONTAL mode
#define INTRA_DC                 1                     // index for intra DC mode
#define DM_CHROMA_IDX            36
#define INTRA_ANGULAR26          26

#define SCAN_DIAG 0
#define SCAN_HORIZ 1
#define SCAN_VERT 2

#define HEVC_MAX_REFS 16

#define EDGE_VER 0
#define EDGE_HOR 1

enum PartMode
{
  PART_2Nx2N,           ///< symmetric motion partition,  2Nx2N
  PART_2NxN,            ///< symmetric motion partition,  2Nx N
  PART_Nx2N,            ///< symmetric motion partition,   Nx2N
  PART_NxN,             ///< symmetric motion partition,   Nx N
  PART_2NxnU,           ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
  PART_2NxnD,           ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
  PART_nLx2N,           ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
  PART_nRx2N,           ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
  PART_NONE = 15
};


struct Mv {
	int mv[2];
};

struct MvField {
	struct Mv mv;
	int refIdx;
};

struct CodingUnit {
	int width;
	int height;
	int x0;
	int y0;
	int cu_transquant_bypass_flag;

	enum PartMode part_mode;
	int pred_mode;
	int intra_pred_mode_luma[4]; // PART_NxN 有4个不同的intra_pred_mode
	int intra_pred_mode_chroma;
	struct MvField mvf[4]; //最多4个Prediction Unit，信息全在mvf里了

};

struct slice_segment_header {
	int first_slice_segment_in_pic_flag;
	int no_output_of_prior_pics_flag;
	int slice_pic_parameter_set_id;
	int slice_type;
	int POC;
	int short_term_ref_pic_set_sps_flag;
	int short_term_ref_pic_set_idx;
	int slice_temporal_mvp_enabled_flag;
	int slice_sao_luma_flag;
	int slice_sao_chroma_flag;
	int num_ref_idx_active_override_flag;
	int numRefIdx;
	int cabac_init_flag;
	int collocated_ref_idx;
	int five_minus_max_num_merge_cand;
	int slice_qp_delta;
	int slice_qp_delta_cb;
	int slice_qp_delta_cr;
	int deblocking_filter_override_flag;
	int slice_deblocking_filter_disabled_flag;
	int slice_beta_offset_div2;
	int slice_tc_offset_div2;
	int slice_loop_filter_across_slices_enabled_flag;
};

struct RefPicList;

struct HEVCFrame {
	unsigned char **SL;
	unsigned char **SCb;
	unsigned char **SCr;
	struct MvField **mvf;
	int **ref_poc; //每个块参考的帧的poc，求DiffPicOrderCnt(colPic, refPicListCol[refIdxCol])时用到，因为此时有可能colPic的一个块参考的参考帧已经从DPB里删除了
	int   **cu_predmode;
	struct slice_segment_header slice_head;
	struct RefPicList *rpl;
	int used_for_ref;
};

struct RefPicList {
	struct HEVCFrame *ref[HEVC_MAX_REFS];
	int num_refs;
};


#define MAX_NALU_LEN (1024 * 700) // bug fix 有时一个nalu的长度大于8196

struct nal_unit_header
{
	unsigned char forbidden_zero_bit;   //1
	unsigned char nal_unit_type;        //6
	unsigned char nuh_layer_id;         //6
	unsigned char nuh_temporal_id_plus1;//3

};

struct video_parameter_set
{
	int vps_video_parameter_set_id;
	int vps_max_sub_layers_minus1;
	int vps_temporal_id_nesting_flag;
	int general_profile_space;
	int general_tier_flag;
	int general_profile_idc;
	int general_profile_compatibility_j[32];
	int general_progressive_source_flag;
	int general_interlaced_source_flag;
	int general_non_packed_constraint_flag;
	int general_frame_only_constraint_flag;
	int general_level_idc;
	int vps_sub_layer_ordering_info_present_flag;
	int vps_max_dec_pic_buffering_minus1_i;
	int vps_max_num_reorder_pics_i;
	int vps_max_latency_increase_plus1_i;
	int vps_max_layer_id;
	int vps_num_layer_sets_minus1;
	int vps_timing_info_present_flag;
	int vps_extension_flag;

};

//一个rps不是一个参考帧，而是一堆参考帧，其中包含与当前POC的偏移，deltaPOC
struct reference_picture_set
{
	int inter_ref_pic_set_prediction_flag;
	int delta_idx_minus1;
	int numberOfPictures;
	int deltaPoc[16];
	int usedByCurr[16];
};

struct seq_parameter_set
{
	int sps_video_parameter_set_id;
	int sps_max_sub_layers_minus1;
	int sps_temporal_id_nesting_flag;
	int sps_seq_parameter_set_id;
	int chroma_format_idc;
	int pic_width_in_luma_samples;
	int pic_height_in_luma_samples;
	int conformance_window_flag;
	int conf_win_left_offset;
	int conf_win_right_offset;
	int conf_win_top_offset;
	int conf_win_bottom_offset;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int log2_max_pic_order_cnt_lsb_minus4;
	int sps_sub_layer_ordering_info_present_flag;
	int sps_max_dec_pic_buffering_minus1;
	int sps_num_reorder_pics;
	int sps_max_latency_increase_plus1;
	int log2_min_coding_block_size_minus3;
	int log2_diff_max_min_coding_block_size;
	int log2_min_transform_block_size_minus2;
	int log2_diff_max_min_transform_block_size;
	int max_transform_hierarchy_depth_inter;
	int max_transform_hierarchy_depth_intra;
	int scaling_list_enabled_flag;
	int amp_enabled_flag;
	int sample_adaptive_offset_enabled_flag;
	int pcm_enabled_flag;
	int num_short_term_ref_pic_sets;
	struct reference_picture_set rps[64];
	int long_term_ref_pics_present_flag;
	int sps_temporal_mvp_enabled_flag;
	int strong_intra_smoothing_enabled_flag;
	int vui_parameters_present_flag;
	int sps_extension_flag;

};

struct pic_parameter_set {
	int pps_pic_parameter_set_id;
	int pps_seq_parameter_set_id;
	int dependent_slice_segments_enabled_flag;
	int output_flag_present_flag;
	int num_extra_slice_header_bits;
	int sign_data_hiding_enabled_flag;
	int cabac_init_present_flag;
	int num_ref_idx_l0_default_active_minus1;
	int num_ref_idx_l1_default_active_minus1;
	int init_qp_minus26;
	int constrained_intra_pred_flag;
	int transform_skip_enabled_flag;
	int cu_qp_delta_enabled_flag;
	int diff_cu_qp_delta_depth;

	int pps_cb_qp_offset;

	int pps_cr_qp_offset;
	int pps_slice_chroma_qp_offsets_present_flag;
	int weighted_pred_flag;
	int transquant_bypass_enabled_flag;
	int tiles_enabled_flag;
	int entropy_coding_sync_enabled_flag;
	int loop_filter_across_slices_enabled_flag;
	int deblocking_filter_control_present_flag;
	int deblocking_filter_override_enabled_flag;
	int pps_deblocking_filter_disabled_flag;
	int pps_beta_offset_div2;
	int pps_tc_offset_div2;

	int pps_scaling_list_data_present_flag;
	int lists_modification_present_flag;
	int log2_parallel_merge_level_minus2;
	int slice_segment_header_extension_present_flag;
	int pps_extension_flag;

};

struct sao_param {
	int sao_merge_left_flag;
	int sao_merge_up_flag;
	int sao_type_idx[3];
	int sao_offset[3][5];
	int sao_band_position[3];
	int sao_eo_class[3];
};

#define GotoNextBit {bits_offset++; bytes_offset += (bits_offset / 8); bits_offset = bits_offset % 8;}
#define GotoNextNBits(n) {bits_offset+=n; bytes_offset += (bits_offset / 8); bits_offset = bits_offset % 8;}

#define min(a, b) ((a < b) ? (a) : (b))
#define max(a, b) ((a > b) ? (a) : (b))
#define Clip3(Min,Max,val) ((val) > (Min)? ((val) < (Max)? (val):(Max)):(Min))
#define Median(a,b,c) ((a) > (b)) ? ((a) > (c) ? max(b,c):(a)):((a) > (c) ? (a) : min(b,c))
#define swap(a, b) {int tmp; tmp = a; a = b; b = tmp;}
#define Sign(a) (a == 0 ? 0 : (a > 0 ? 1 : -1))

#define PicOrderCnt(PicX) (PicX->slice_head.POC)
#define DiffPicOrderCnt(picA, picB)  PicOrderCnt(picA) - PicOrderCnt(picB)

unsigned int read_one_nalu(FILE* fin, 
				  int offset, 
				  unsigned char* stream_buffer, 
				  struct nal_unit_header *nalu_header,
				  unsigned char* rbsp,
				  unsigned int* nalu_len);

void EBSP2RBSP(unsigned char* nalu, unsigned char* rbsp, unsigned int nalu_len);

// read from bytes_offset+1 bytes, bits_offset : start from low bit
// high bit ---> low bit
// 0  1  1  0  0  1  1  1
// <-- left shift  right shift -->
// in RAM low bit --- > high bit
// 1  1  1  0  0  1  1  0
// <-- right shift  left shift -->
unsigned int read_one_bit(unsigned char rbsp[]);
unsigned int try_read_one_bit(unsigned char rbsp[]);

unsigned int read_byte(unsigned char rbsp[], unsigned int bytes_offset);

unsigned int read_bits(unsigned char rbsp[], unsigned int len);

int read_se_v(unsigned char rbsp[]);

unsigned int read_ue_v(unsigned char rbsp[]);

unsigned int read_me_v(unsigned char rbsp[], unsigned short intra_4x4);

unsigned int read_te_v(unsigned char rbsp[], unsigned short Max);

void InitDecoder(struct pic_parameter_set *pps, struct seq_parameter_set *sps);
void FreeDecoder();
void ResetDecoder();

void output_to_file(FILE* out);
void Log(const char* format...);
void Log_data(const char* format...);
void Log_filter(const char* format...);
void Log_deblock(const char* format...);
void Log_ref(const char* format...);

void InsertFrame();
void TraceBits(unsigned char rbsp[], 
								 unsigned short bytes_offset, 
								 unsigned short bits_offset, 
								 unsigned short len);
void parse_pps(unsigned char *rbsp, struct pic_parameter_set *pps);
void parse_vps(unsigned char *rbsp, struct video_parameter_set *vps);
void parse_sps(unsigned char *rbsp, struct seq_parameter_set *sps, struct video_parameter_set *vps);



int get_mem2D(void ***array2D, int rows, int columns, int size);
void free_mem2D(void **array2D);
int get_mem2Dpixel(unsigned char ***array2D, unsigned short rows, unsigned short columns, unsigned char init_value);
void free_mem2Dpixel(unsigned char** array2D);
int get_mem2Dint(int ***array2D, int rows, int columns, int init_value);
void free_mem2Dint(int **array2D);


void parse_slice_header(unsigned char *rbsp,
	struct nal_unit_header *nalu_head,
	struct video_parameter_set *vps,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head);
void parse_coding_quadtree(unsigned char *rbsp, int x0, int y0, int log2CbSize, int cqtDepth,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head);
void parse_sao_ctb(unsigned char *rbsp, int rx, int ry, struct sao_param *sao,
	struct slice_segment_header *slice_head, int CtbAddr);
int parse_coding_tree_unit(unsigned char *rbsp, int CtbAddr,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head);
void parse_slice(unsigned char *rbsp,
	struct nal_unit_header *nalu_head,
	struct video_parameter_set *vps,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps,
	struct slice_segment_header_t *slice_head);
int test_block_available(int x, int y, int xNbL, int yNbL);
void parse_coding_unit(unsigned char *rbsp, int x0, int y0, int log2CbSize,
	struct pic_parameter_set *pps, struct seq_parameter_set *sps,
	struct slice_segment_header *slice_head,
	struct CodingUnit *cu);
void parse_transform_tree(unsigned char *rbsp, int x0, int y0, int xCu, int yCu,
	int log2TrafoSize, int trafoDepth, int blkIdx,
	int base_cbf_cb, int base_cbf_cr, struct pic_parameter_set *pps,
	struct seq_parameter_set *sps, struct CodingUnit *cu,
	struct slice_segment_header *slice_head);
void parse_transform_unit(unsigned char *rbsp, int log2TrafoSize, int cbf_cb, int cbf_cr, int cbf_luma,
struct pic_parameter_set *pps, struct seq_parameter_set *sps, struct CodingUnit *cu, struct slice_segment_header *slice_head,
	int blkIdx, int x0, int y0, int xCu, int yCu);
void parse_residual_coding(unsigned char *rbsp, int log2TrafoSize, int cIdx, int scanIdx,
	struct pic_parameter_set *pps, struct CodingUnit *cu, struct slice_segment_header *slice_head,
	int x0, int y0);
void get_cand_mode_list(int x0, int y0, int candModeList[]);
void parse_prediction_unit(unsigned char *rbsp, int xPb, int yPb, int nPbW, int nPbH, int partIdx,
	struct CodingUnit *cu,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head, int *merge_flag);
void derive_motion_vector_prediction(int xPb, int yPb, int nPbW, int nPbH, int partIdx,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head,
	int mvp_l0_flag, int ref_idx_l0, struct Mv *mvp);
int derive_temporal_motion_vector(int xPb, int yPb, int nPbW, int nPbH, struct slice_segment_header *slice_head,
	struct MvField *mvfCol);

void derive_merging_candidates(int xPb, int yPb, int nPbW, int nPbH, int partIdx,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head, struct CodingUnit *cu,
	struct MvField *mergeCandList, int merge_idx);
void parse_profile_tier(unsigned char *rbsp, struct video_parameter_set *vps);
void parse_short_term_ref_pic_set(unsigned char *rbsp, struct reference_picture_set *rps, 
					int stRpsIdx,
					struct seq_parameter_set *sps);
void parse_vps(unsigned char *rbsp, struct video_parameter_set *vps);
void parse_sps(unsigned char *rbsp, struct seq_parameter_set *sps, struct video_parameter_set *vps);
void parse_pps(unsigned char *rbsp, struct pic_parameter_set *pps);
void transform(int **d, int **r, int nTbS, int cIdx, int x0, int y0);

void derive_bs(int x0, int y0, int log2TrafoSize);
void deblocking_filter(struct pic_parameter_set *pps, struct slice_segment_header *slice_head);
void intra_pred(int xTbY, int yTbY, int cIdx, int log2TrafoSize, int predModeIntra,
struct pic_parameter_set *pps,
struct seq_parameter_set *sps);
void inter_pred(int xC, int yC, int xB, int yB, int nPbW, int nPbH, int mv[], int refIdx);
void free_dpb(struct HEVCFrame *dpb);


#endif









