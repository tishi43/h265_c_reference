// bitstream.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include "bitstream_header.h"

extern struct HEVCFrame *CurrPic;
extern int **CbfLuma;
extern int CtbLog2SizeY, CtbSizeY;
extern int **horBs, **verBs;
extern int MinCbLog2SizeY;
extern int PicWidthInSamplesY, PicHeightInSamplesY;
extern int PicWidthInCtbsY, PicHeightInCtbsY;
extern int PicSizeInCtbsY;
extern struct sao_param *sao_params;
extern unsigned char **SaoBufL, **SaoBufCb, **SaoBufCr;
extern int **qp_y_tab;
extern int **no_filter;
extern int slice_num;

static int tc_tab[54] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, // QP  0...18
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, // QP 19...37
	5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24           // QP 38...53
};

static int beta_tab[52] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, // QP 0...18
	9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, // QP 19...37
	38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64                      // QP 38...51
};

//只有TU,PU的边界才滤波
void derive_bs(int x0, int y0, int log2TrafoSize)
{
	int x, y;

	int ref_idx_A, ref_idx_B;

	if (x0 == 112 && y0 == 48 && slice_num == 0)
		x0 = x0;
	//只求TU的上边界和左边界,解完当前TU，当前TU的下面一个TU没有解完，显然不可能求下/右边界bs
	if (y0 > 0 && (y0 & 7) == 0) { //8x8边界
		for (x = x0; x < x0 + (1 << log2TrafoSize); x += 4) {
			if ((x >> 2) == 24 && ((y0 - 8) >> 3) == 4)
				x0 = x0;
			if (CurrPic->cu_predmode[y0 >> MinCbLog2SizeY][x >> MinCbLog2SizeY] == MODE_INTRA ||
				CurrPic->cu_predmode[(y0-1) >> MinCbLog2SizeY][x >> MinCbLog2SizeY] == MODE_INTRA)
				horBs[(y0-8) >> 3][x >> 2] = 2;
			else if (CbfLuma[y0>>2][x>>2] || CbfLuma[(y0-1)>>2][x>>2])
				horBs[(y0-8) >> 3][x >> 2] = 1;
			else {
				ref_idx_A = CurrPic->mvf[y0>>2][x>>2].refIdx;
				ref_idx_B = CurrPic->mvf[(y0-1)>>2][x>>2].refIdx;
				//参考不同的图像或mv相差4
				if (CurrPic->rpl->ref[ref_idx_A] != CurrPic->rpl->ref[ref_idx_B])
					horBs[(y0-8) >> 3][x >> 2] = 1;
				else if (abs(CurrPic->mvf[y0>>2][x>>2].mv.mv[0] - CurrPic->mvf[(y0-1)>>2][x>>2].mv.mv[0]) >= 4 ||
					abs(CurrPic->mvf[y0>>2][x>>2].mv.mv[1] - CurrPic->mvf[(y0-1)>>2][x>>2].mv.mv[1]) >= 4)
					horBs[(y0-8) >> 3][x >> 2] = 1;
			}

			Log_data("horBs[%d][%d]=%d\n", (y0 - 8) >> 3, x >> 2, horBs[(y0 - 8) >> 3][x >> 2]);
		}
	}

	if (x0 > 0 && (x0 & 7) == 0) { //8x8边界
		for (y = y0; y < y0 + (1 << log2TrafoSize); y += 4) {
			if (CurrPic->cu_predmode[y >> MinCbLog2SizeY][x0 >> MinCbLog2SizeY] == MODE_INTRA ||
				CurrPic->cu_predmode[y >> MinCbLog2SizeY][(x0 - 1) >> MinCbLog2SizeY] == MODE_INTRA)
				verBs[y >> 2][(x0 - 8) >> 3] = 2;
			else if (CbfLuma[y >> 2][x0 >> 2] || CbfLuma[y >> 2][(x0 - 1) >> 2])
				verBs[y >> 2][(x0 - 8) >> 3] = 1;
			else {
				ref_idx_A = CurrPic->mvf[y >> 2][x0 >> 2].refIdx;
				ref_idx_B = CurrPic->mvf[y >> 2][(x0 - 1) >> 2].refIdx;
				//参考不同的图像或mv相差4
				if (CurrPic->rpl->ref[ref_idx_A] != CurrPic->rpl->ref[ref_idx_B])
					verBs[y >> 2][(x0 - 8) >> 3] = 1;
				else if (abs(CurrPic->mvf[y >> 2][x0 >> 2].mv.mv[0] - CurrPic->mvf[y >> 2][(x0 - 1) >> 2].mv.mv[0]) >= 4 ||
					abs(CurrPic->mvf[y >> 2][x0 >> 2].mv.mv[1] - CurrPic->mvf[y >> 2][(x0 - 1) >> 2].mv.mv[1]) >= 4)
					verBs[y >> 2][(x0 - 8) >> 3] = 1;
			}
			Log_data("verBs[%d][%d]=%d\n", y >> 2, (x0 - 8) >> 3, verBs[y >> 2][(x0 - 8) >> 3]);
		}
	}

	//TU内部PU
	if (log2TrafoSize > 2 && //Log2MinPuSize
		CurrPic->cu_predmode[y0 >> MinCbLog2SizeY][x0 >> MinCbLog2SizeY] != MODE_INTRA) {
		for (y = y0 + 8; y < y0 + (1 << log2TrafoSize); y += 8) {
			for (x = x0; x < x0 + (1 << log2TrafoSize); x += 4) {
				ref_idx_A = CurrPic->mvf[y >> 2][x >> 2].refIdx;
				ref_idx_B = CurrPic->mvf[(y - 1) >> 2][x >> 2].refIdx;
				if (CurrPic->rpl->ref[ref_idx_A] != CurrPic->rpl->ref[ref_idx_B])
					horBs[(y - 8) >> 3][x >> 2] = 1;
				else if (abs(CurrPic->mvf[y >> 2][x >> 2].mv.mv[0] - CurrPic->mvf[(y - 1) >> 2][x >> 2].mv.mv[0]) >= 4 ||
					abs(CurrPic->mvf[y >> 2][x >> 2].mv.mv[1] - CurrPic->mvf[(y - 1) >> 2][x >> 2].mv.mv[1]) >= 4)
					horBs[(y - 8) >> 3][x >> 2] = 1;
				Log_data("PU inside TU horBs[%d][%d]=%d\n", (y - 8) >> 3, x >> 2, horBs[(y - 8) >> 3][x >> 2]);
			}
		}
		//TU内部相隔8的边界线先假定都为PU边界，如果非真正PU边界，参考不同图像，mv相差4这两个条件必不满足,bs必为0
		for (y = y0; y < y0 + (1 << log2TrafoSize); y += 4) {
			for (x = x0 + 8; x < x0 + (1 << log2TrafoSize); x += 8) {

				ref_idx_A = CurrPic->mvf[y>>2][x>>2].refIdx;
				ref_idx_B = CurrPic->mvf[y>>2][(x-1)>>2].refIdx;
				if (CurrPic->rpl->ref[ref_idx_A] != CurrPic->rpl->ref[ref_idx_B])
					verBs[y >> 2][(x-8) >> 3] = 1;
				else if (abs(CurrPic->mvf[y>>2][x>>2].mv.mv[0] - CurrPic->mvf[y>>2][(x-1)>>2].mv.mv[0]) >= 4 ||
					abs(CurrPic->mvf[y>>2][x>>2].mv.mv[1] - CurrPic->mvf[y>>2][(x-1)>>2].mv.mv[1]) >= 4)
					verBs[y >> 2][(x-8) >> 3] = 1;
				Log_data("PU inside TU verBs[%d][%d]=%d\n", y >> 2, (x - 8) >> 3, verBs[y >> 2][(x - 8) >> 3]);
			}
		}

	}


}

#define pik(i,k) ((edgeType == EDGE_VER) ? recPicture[y+k][x-i-1] : recPicture[y-i-1][x+k])
#define qik(i,k) ((edgeType == EDGE_VER) ? recPicture[y+k][x+i] : recPicture[y+i][x+k])
#define p00 pik(0,0)
#define p10 pik(1,0)
#define p20 pik(2,0)
#define p30 pik(3,0)
#define q00 qik(0,0)
#define q10 qik(1,0)
#define q20 qik(2,0)
#define q30 qik(3,0)


#define p03 pik(0,3)
#define p13 pik(1,3)
#define p23 pik(2,3)
#define p33 pik(3,3)

#define q03 qik(0,3)
#define q13 qik(1,3)
#define q23 qik(2,3)
#define q33 qik(3,3)

void deblocking_filter_luma(int x, int y, int edgeType,
	struct slice_segment_header *slice_head)
{

	int bS = edgeType == EDGE_VER ? verBs[y>>2][(x-8)>>3] : horBs[(y-8)>>3][x>>2];
	int beta, tC, dE, dEp, dEq;
	int QpQ, QpP, qPL, Q;
	int dp0, dp3, dq0, dq3, dpq0, dpq3, dp, dq, d, dpq, dSam0, dSam3;
	int p0, p1, p2, p3, q0, q1, q2, q3;
	int delta, deltap, deltaq;
	int end;
	unsigned char **recPicture = CurrPic->SL;
	int bypass_p, bypass_q;


	if (bS) {
		QpQ = qp_y_tab[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
		QpP = edgeType == EDGE_VER ?
			qp_y_tab[y>>MinCbLog2SizeY][(x-1)>>MinCbLog2SizeY] :
			qp_y_tab[(y-1)>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
		qPL = (QpQ + QpP + 1) >> 1; //8-288
		Q = Clip3(0, 51, qPL + (slice_head->slice_beta_offset_div2 << 1));
		beta = beta_tab[Q];
		//β = β′ * (1 << (BitDepthY - 8)) 忽略这条
		Q = Clip3(0, 53, qPL + 2 * (bS - 1) + (slice_head->slice_tc_offset_div2 << 1));
		tC = tc_tab[Q];
		//tC = tC′ * (1 << (BitDepthY - 8)) 忽略这条

		//垂直
		//p30 p20 p10 p00   q00 q10 q20 q30
		//p31 p21 p11 p01   q01 q11 q21 q31
		//p32 p22 p12 p02   q02 q12 q22 q32
		//p33 p23 p13 p03   q03 q13 q23 q33
		//水平
		//p30 p31 p32 p33
		//p20 p21 p22 p23
		//p10 p11 p12 p13
		//p00 p01 p02 p03
		//
		//q00 q01 q02 q03
		//q10 q11 q12 q13
		//q20 q21 q22 q23
		//q30 q31 q32 q33
		dp0 = abs(p20 - 2 * p10 + p00);
		dp3 = abs(p23 - 2 * p13 + p03);
		dq0 = abs(q20 - 2 * q10 + q00);
		dq3 = abs(q23 - 2 * q13 + q03);
		dpq0 = dp0 + dq0;
		dpq3 = dp3 + dq3;
		dp = dp0 + dp3;
		dq = dq0 + dq3;

		d = dpq0 + dpq3;
		dE = 0;
		dEp = 0;
		dEq = 0;
		if (d < beta) {
			Log_deblock("luma %s bs %d x %d y %d\n", edgeType == EDGE_VER ? "ver" : "hor", bS, x, y);
			dpq = 2 * dpq0;
			if (dpq < (beta >> 2) &&
				abs(p30 - p00) + abs(q00 - q30) < (beta >> 3) &&
				abs(p00 - q00) < ((5 * tC + 1) >> 1))
				dSam0 = 1;
			else
				dSam0 = 0;
			dpq = 2 * dpq3;
			if (dpq < (beta >> 2) &&
				abs(p33 - p03) + abs(q03 - q33) < (beta >> 3) &&
				abs(p03 - q03) < ((5 * tC + 1) >> 1))
				dSam3 = 1;
			else
				dSam3 = 0;
			dE = 1;
			if (dSam0 == 1 && dSam3 == 1)
				dE = 2;
			if (dp < (beta + (beta >> 1)) >> 3)
				dEp = 1;
			if (dq < (beta + (beta >> 1)) >> 3)
				dEq = 1;
			end = edgeType == EDGE_VER ? (y + 4) : (x + 4);
			bypass_p = edgeType == EDGE_VER ?
				no_filter[y>>MinCbLog2SizeY][(x-1)>>MinCbLog2SizeY] :
				no_filter[(y-1)>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
			bypass_q = no_filter[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
			if (dE == 2) { //strong filtering
				for (; edgeType == EDGE_VER ? (y < end) : (x < end);
					edgeType == EDGE_VER ? y++ : x++) {
					p0 = p00;
					p1 = p10;
					p2 = p20;
					p3 = p30;
					q0 = q00;
					q1 = q10;
					q2 = q20;
					q3 = q30;
					if (!bypass_p) {
						p00 = Clip3(p0-2*tC, p0+2*tC, (p2+2*p1+2*p0+2*q0+q1+4)>>3);//p0'
						p10 = Clip3(p1-2*tC, p1+2*tC, (p2+p1+p0+q0+2)>>2);//p1'
						p20 = Clip3(p2-2*tC, p2+2*tC, (2*p3+3*p2+p1+p0+q0+4)>>3);
						Log_deblock("strong p00 %d p10 %d p20 %d\n", p00, p10, p20);
					}
					if (!bypass_q) {
						q00 = Clip3(q0-2*tC, q0+2*tC, (p1+2*p0+2*q0+2*q1+q2+4)>>3);//q0'
						q10 = Clip3(q1-2*tC, q1+2*tC, (p0+q0+q1+q2+2)>>2);//q1'
						q20 = Clip3(q2-2*tC, q2+2*tC, (p0+q0+q1+3*q2+2*q3+4)>>3);//q2'
						Log_deblock("strong q00 %d q10 %d q20 %d\n", q00, q10, q20);
					}
				}
			} else { //normal filter
				for (; edgeType == EDGE_VER ? (y < end) : (x < end);
					edgeType == EDGE_VER ? y++ : x++) {

					p0 = p00;
					p1 = p10;
					p2 = p20;
					q0 = q00;
					q1 = q10;
					q2 = q20;
					delta = (9 * (q0 - p0) - 3 * (q1 - p1) + 8) >> 4;
					if (abs(delta) < tC * 10) {
						delta = Clip3(-tC, tC, delta);
						if (!bypass_p) {
							p00 = Clip3(0, 255, p0 + delta);//p0'
							Log_deblock("normal p00 %d\n", p00);
						}
						if (!bypass_q) {
							q00 = Clip3(0, 255, q0 - delta);//q0'
							Log_deblock("normal q00 %d\n", q00);
						}
						if (dEp == 1) {
							deltap = Clip3(-(tC >> 1), tC >> 1, (((p2 + p0 + 1) >> 1) - p1 + delta) >> 1);
							if (!bypass_p) {
								p10 = Clip3(0, 255, p1 + deltap);//p1'
								Log_deblock("normal p10 %d\n", p10);
							}

						}
						if (dEq == 1) {
							deltaq = Clip3(-(tC >> 1), tC >> 1, (((q2 + q0 + 1) >> 1) - q1 - delta) >> 1);
							if (!bypass_q) {
								q10 = Clip3(0, 255, q1 + deltaq);//q1'
								Log_deblock("normal q10 %d\n", q10);
							}
						}
					}

				}
			}
		}
	}

}

void deblocking_filter_chroma(int x, int y, int chroma, int edgeType,
	struct pic_parameter_set *pps,
	struct slice_segment_header *slice_head)
{
	static int QpC_tab[] = {
		29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37
	};
	int p0, p1, q0, q1;

	int delta;
	int QpQ, QpP, qPi, Q, tC, cQpPicOffset, QpC;
	int xCb, yCb;
	int bS = edgeType == EDGE_VER ? verBs[y>>2][(x-8)>>3] : horBs[(y-8)>>3][x>>2];
	int end = edgeType == EDGE_VER ? (y + 8) : (x + 8);
	int bypass_p, bypass_q;
	unsigned char **recPicture;

	if (chroma == 0) {
		recPicture = CurrPic->SCb;
		cQpPicOffset = pps->pps_cb_qp_offset;
	} else {
		recPicture = CurrPic->SCr;
		cQpPicOffset = pps->pps_cr_qp_offset;
	}

	if (bS == 2) {
		Log_deblock("%s %s bs %d x %d y %d\n", chroma ? "Cr" : "Cb", edgeType == EDGE_VER ? "ver" : "hor", bS, x, y);
		QpQ = qp_y_tab[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
		QpP = edgeType == EDGE_VER ?
			qp_y_tab[y>>MinCbLog2SizeY][(x-1)>>MinCbLog2SizeY] :
			qp_y_tab[(y-1)>>MinCbLog2SizeY][x>>MinCbLog2SizeY];
		qPi = ((QpQ + QpP + 1) >> 1) + cQpPicOffset;
		if (qPi < 30)
			QpC = qPi;
		else if (qPi > 43)
			QpC = qPi - 6;
		else
			QpC = QpC_tab[qPi-30];
		Q = Clip3(0, 53, QpC + 2 + (slice_head->slice_tc_offset_div2 << 1));
		tC = tc_tab[Q];
		//tC = tC′ * (1 << (BitDepthY - 8)) 忽略这条
		xCb = x;
		yCb = y;
		x >>= 1;
		y >>= 1;
		for (; edgeType == EDGE_VER ? (yCb < end) : (xCb < end);
					edgeType == EDGE_VER ? y++, yCb += 2 : x++, xCb += 2) {
			p0 = p00;
			p1 = p10;
			q0 = q00;
			q1 = q10;
			delta = Clip3(-tC, tC, ((((q0 - p0) << 2) + p1 - q1 + 4) >> 3));
			bypass_p = edgeType == EDGE_VER ?
				no_filter[yCb>>MinCbLog2SizeY][(xCb-1)>>MinCbLog2SizeY] :
				no_filter[(yCb-1)>>MinCbLog2SizeY][xCb>>MinCbLog2SizeY];
			bypass_q = no_filter[yCb>>MinCbLog2SizeY][xCb>>MinCbLog2SizeY];
			if (!bypass_p) {
				p00 = Clip3(0, 255, p0 + delta);//p0'
				Log_deblock("p00 %d\n", p00);
			}
			if (!bypass_q) {
				q00 = Clip3(0, 255, q0 - delta);//q0'
				Log_deblock("q00 %d\n", q00);
			}
		}

	}
}

//x0,y0为亮度/色度坐标
void sao_band_filter(int x0, int y0, int width, int height,
	int saoLeftClass, int *SaoOffsetVal,
	unsigned char **recPicture, unsigned char **saoPicture)
{
	int bandShift = 3; //bitDepth-5
	int x, y;
	int bandTable[32] = {0, };
	int k;
	int bandIdx;
	//这个SaoOffsetVal有5个元素，不是4个，第0个的值是0，bandTable 32个数组项只有4个值指向1,2,3,4,其余指向0，
	//意思就是只有特定value的像素才会加offset，其余不加
	for (k = 0; k < 4; k++) {
		bandTable[(k+saoLeftClass)&31] = k + 1;
	}
	//此处不考虑cu_transquant_bypass_flag，无论是否cu_transquant_bypass_flag一律计算，
	//因为cu_transquant_bypass_flag本来就非常少数，多计算几个无所谓，spec里也是计算完替换原像素时才考虑，大概就是这个意思
	for (y = y0; y < y0 + height; y++) {
		for (x = x0; x < x0 + width; x++) {
			bandIdx = bandTable[recPicture[y][x] >> bandShift];
			saoPicture[y][x] = Clip3(0, 255, recPicture[y][x] + SaoOffsetVal[bandIdx]);
			Log_filter("x %d y %d band %d+%d=%d\n",x-x0,y-y0,recPicture[y][x],SaoOffsetVal[bandIdx],saoPicture[y][x]);
		}
	}
}

////x0,y0为亮度/色度坐标
void sao_edge_filter(int x0, int y0, int width, int height, int sao_eo_class,
	int *SaoOffsetVal, int boundary_x, int boundary_y,
	unsigned char **recPicture, unsigned char **saoPicture)
{
	static int hPos[4][2] = {{-1, 1}, {0, 0}, {-1, 1}, {1, -1}};
	static int vPos[4][2] = {{0, 0}, {-1, 1}, {-1, 1}, {-1, 1}};
	int edgeIdx;
	int x, y;

	for (y = y0; y < y0 + height; y++) {
		for (x = x0; x < x0 + width; x++) {
			if ( y ==6 && x==155&& slice_num == 0)
				x0 = x0;
			if (y + vPos[sao_eo_class][0] < 0 || y + vPos[sao_eo_class][0] >= boundary_y ||
				x + hPos[sao_eo_class][0] < 0 || x + hPos[sao_eo_class][0] >= boundary_x ||
				y + vPos[sao_eo_class][1] < 0 || y + vPos[sao_eo_class][1] >= boundary_y ||
				x + hPos[sao_eo_class][1] < 0 || x + hPos[sao_eo_class][1] >= boundary_x)
			{
				saoPicture[y][x] = recPicture[y][x];
			}
			else {
				edgeIdx = 2 + Sign(recPicture[y][x] - recPicture[y + vPos[sao_eo_class][0]][x + hPos[sao_eo_class][0]]) +
					Sign(recPicture[y][x] - recPicture[y + vPos[sao_eo_class][1]][x + hPos[sao_eo_class][1]]);
				if (edgeIdx == 0 || edgeIdx == 1 || edgeIdx == 2)
					edgeIdx = (edgeIdx == 2) ? 0 : (edgeIdx + 1);
				saoPicture[y][x] = Clip3(0, 255, recPicture[y][x] + SaoOffsetVal[edgeIdx]);
				Log_filter("x %d y %d edge %d+%d=%d edgeIdx %d\n",
					x - x0, y - y0, recPicture[y][x], SaoOffsetVal[edgeIdx], saoPicture[y][x], edgeIdx);
			}
			

		}
	}
}

void sao_filter_ctb(int x0, int y0, int CtbAddrInRs, struct slice_segment_header *slice_head)
{
	struct sao_param *sao = &sao_params[CtbAddrInRs];
	unsigned char **recPicture = NULL;
	unsigned char **saoPicture = NULL;
	int CtbWidth = min(CtbSizeY, PicWidthInSamplesY - x0);
	int CtbHeight = min(CtbSizeY, PicHeightInSamplesY - y0);
	int *SaoOffsetVal;
	int CtbWidthC = CtbWidth / 2, CtbHeightC = CtbHeight / 2;
	int x, y;
	int saoLeftClass, sao_eo_class;

	Log_filter("luma\n");
	for (y = y0; y < y0 + CtbSizeY && y < PicHeightInSamplesY; y++) {
		for (x = x0; x < x0 + CtbSizeY && x < PicWidthInSamplesY; x++) {
			if (x % 16 == 0)
				Log_filter("y %d x %d: ", y, x);
			Log_filter("%02x ", CurrPic->SL[y][x]);
			if (x % 16 == 15)
				Log_filter("\n");
		}
	}
	Log_filter("cb\n");
	for (y = y0; y < y0 + CtbSizeY && y < PicHeightInSamplesY; y += 2) {
		for (x = x0; x < x0 + CtbSizeY && x < PicWidthInSamplesY; x += 2) {
			if (y0 == 128 && x0 == 0)
				x = x;
			if (x % 16 == 0)
				Log_filter("y %d x %d: ", y, x);
			Log_filter("%02x ", CurrPic->SCb[y >> 1][x >> 1]);
			if (x % 16 == 14)
				Log_filter("\n");
		}
	}
	Log_filter("cr\n");
	for (y = y0; y < y0 + CtbSizeY && y < PicHeightInSamplesY; y += 2) {
		for (x = x0; x < x0 + CtbSizeY && x < PicWidthInSamplesY; x += 2) {
			if (x % 16 == 0)
				Log_filter("y %d x %d: ", y, x);
			Log_filter("%02x ", CurrPic->SCr[y >> 1][x >> 1]);
			if (x % 16 == 14)
				Log_filter("\n");
		}
	}

	if (slice_head->slice_sao_luma_flag) {
		recPicture = CurrPic->SL;
		saoPicture = SaoBufL;

		SaoOffsetVal = sao->sao_offset[0];
		Log_filter("sao_filter_ctb x %d y %d cIdx 0 type %d\n",
			x0, y0, sao->sao_type_idx[0]);
		if (x0 == 128 && y0 == 0)
			x0 = x0;
		if (sao->sao_type_idx[0] == 1) { //BAND
			saoLeftClass = sao->sao_band_position[0];
			//SAO_EDGE要用到左右，对角，上下2点，此2点是滤波之前的值，非滤之后的值，所以滤波后的数据要放入另一份buf
			sao_band_filter(x0, y0, CtbWidth, CtbHeight, saoLeftClass, SaoOffsetVal,
				recPicture, saoPicture);
		}
		else if (sao->sao_type_idx[0] == 2){//EDGE
			sao_eo_class = sao->sao_eo_class[0];
			sao_edge_filter(x0, y0, CtbWidth, CtbHeight, sao_eo_class, SaoOffsetVal,
				PicWidthInSamplesY, PicHeightInSamplesY,
				recPicture, saoPicture);
		}
	}
	if (slice_head->slice_sao_chroma_flag) {

		recPicture = CurrPic->SCb;
		saoPicture = SaoBufCb;

		SaoOffsetVal = sao->sao_offset[1];
		if (x0 == 256 && y0 == 0)
			x0 = x0;
		Log_filter("sao_filter_ctb x %d y %d cIdx 1 type %d\n",
			x0, y0, sao->sao_type_idx[1]);
		if (sao->sao_type_idx[1] == 1) { //BAND
			saoLeftClass = sao->sao_band_position[1];

			sao_band_filter(x0>>1, y0>>1, CtbWidthC, CtbHeightC, saoLeftClass, SaoOffsetVal,
				recPicture, saoPicture);
		}
		else if (sao->sao_type_idx[1] == 2){//EDGE
			sao_eo_class = sao->sao_eo_class[1];
			sao_edge_filter(x0 >> 1, y0 >> 1, CtbWidthC, CtbHeightC, sao_eo_class, SaoOffsetVal,
				PicWidthInSamplesY >> 1, PicHeightInSamplesY >> 1,
				recPicture, saoPicture);
		}

		recPicture = CurrPic->SCr;
		saoPicture = SaoBufCr;
		SaoOffsetVal = sao->sao_offset[2];
		Log_filter("sao_filter_ctb x %d y %d cIdx 2 type %d\n",
			x0, y0, sao->sao_type_idx[2]);
		if (sao->sao_type_idx[2] == 1) { //BAND
			saoLeftClass = sao->sao_band_position[2];

			sao_band_filter(x0 >> 1, y0 >> 1, CtbWidthC, CtbHeightC, saoLeftClass, SaoOffsetVal,
				recPicture, saoPicture);
		}
		else if (sao->sao_type_idx[2] == 2){//EDGE
			sao_eo_class = sao->sao_eo_class[2];
			sao_edge_filter(x0 >> 1, y0 >> 1, CtbWidthC, CtbHeightC, sao_eo_class, SaoOffsetVal,
				PicWidthInSamplesY >> 1, PicHeightInSamplesY >> 1,
				recPicture, saoPicture);
		}
	}


}

void deblocking_filter(struct pic_parameter_set *pps,
	struct slice_segment_header *slice_head)
{
	int CtbAddrInRs;
	int x0, y0;
	int x, y, i;
	int chroma;

	//垂直滤左边界和水平滤上边界
	for (x = 8; x < PicWidthInSamplesY; x += 8) {
		for (y = 0; y < PicHeightInSamplesY; y += 4) {

			deblocking_filter_luma(x, y, EDGE_VER, slice_head);
		}
	}

	for (y = 8; y < PicHeightInSamplesY; y += 8) {
		for (x = 0; x < PicWidthInSamplesY; x += 4) {
			if (y == 8 && x == 148)
				x = x;
			deblocking_filter_luma(x, y, EDGE_HOR, slice_head);
		}
	}

	for (chroma = 0; chroma < 2; chroma++) {
		//色度垂直,x,y是亮度坐标
		for (x = 16; x < PicWidthInSamplesY; x += 16) {
			for (y = 0; y < PicHeightInSamplesY; y += 8) {
				deblocking_filter_chroma(x, y, chroma, EDGE_VER, pps, slice_head);
			}
		}
		//色度水平
		for (y = 16; y < PicHeightInSamplesY; y += 16) {
			for (x = 0; x < PicWidthInSamplesY; x += 8) {
				deblocking_filter_chroma(x, y, chroma, EDGE_HOR, pps, slice_head);
			}
		}
	}

	for (CtbAddrInRs = 0; CtbAddrInRs < PicSizeInCtbsY; CtbAddrInRs++) {
		x0 = (CtbAddrInRs % PicWidthInCtbsY) << CtbLog2SizeY;
		y0 = (CtbAddrInRs / PicWidthInCtbsY) << CtbLog2SizeY;
		sao_filter_ctb(x0, y0, CtbAddrInRs, slice_head);
	}
	
	for (CtbAddrInRs = 0; CtbAddrInRs < PicSizeInCtbsY; CtbAddrInRs++) {
		x0 = (CtbAddrInRs % PicWidthInCtbsY) << CtbLog2SizeY;
		y0 = (CtbAddrInRs / PicWidthInCtbsY) << CtbLog2SizeY;
		struct sao_param *sao = &sao_params[CtbAddrInRs];
		if (sao->sao_type_idx[0]) {
			for (y = y0; y < y0+CtbSizeY && y < PicHeightInSamplesY; y += (1<<MinCbLog2SizeY)) {
				for (x = x0; x < x0+CtbSizeY && x < PicWidthInSamplesY; x += (1<<MinCbLog2SizeY)) {
					if (!no_filter[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY]) {
						for (i = 0; i < (1<<MinCbLog2SizeY); i++)
							memcpy(&CurrPic->SL[y+i][x], &SaoBufL[y+i][x], (1<<MinCbLog2SizeY)*sizeof(unsigned char));
					}
				}
			}
		}
		if (sao->sao_type_idx[1]) {
			for (y = y0; y < y0+CtbSizeY && y < PicHeightInSamplesY; y += (1<<MinCbLog2SizeY)) {
				for (x = x0; x < x0+CtbSizeY && x < PicWidthInSamplesY; x += (1<<MinCbLog2SizeY)) {

					if (!no_filter[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY]) {
						for (i = 0; i < (1<<MinCbLog2SizeY); i += 2)
							memcpy(&CurrPic->SCb[(y+i)>>1][x>>1], &SaoBufCb[(y+i)>>1][x>>1], 1<<(MinCbLog2SizeY-1));
					}
				}
			}
		}
		if (sao->sao_type_idx[2]) {
			for (y = y0; y < y0+CtbSizeY && y < PicHeightInSamplesY; y += (1<<MinCbLog2SizeY)) {
				for (x = x0; x < x0+CtbSizeY && x < PicWidthInSamplesY; x += (1<<MinCbLog2SizeY)) {
					if (!no_filter[y>>MinCbLog2SizeY][x>>MinCbLog2SizeY]) {
						for (i = 0; i < (1<<MinCbLog2SizeY); i += 2)
							memcpy(&CurrPic->SCr[(y+i)>>1][x>>1], &SaoBufCr[(y+i)>>1][x>>1], 1<<(MinCbLog2SizeY-1));
					}
				}
			}
		}
	}
}


