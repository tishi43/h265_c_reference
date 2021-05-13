
дverilogʵ��H265������ǰΪ�����Э��ʵ�ֵ�һ���򻯰汾��H265������C���룬��VS2013�Ĺ��̣������ļ���test.265,�����ﺬ��һЩ�����ļ����������test.265���ɡ����룬��F5�ܼ��ɣ����out.yuv,�Լ�һЩlog�ļ���

��Ҫ�ο�H265��׼��ffmpeg��HM���ο�����JCTVC-L1003_v34��
���뾡���ͱ�׼д�ı���һ�£��������ֺͱ�׼Ҳ����һ�£�������⣬������HM��ffmpegʱ��Ϊ����������������ʺϳ�ѧh265��׼��ͬѧ�ǡ�
��һ��cabac decode bin�����ӣ�
HM12.0�Ĵ������£�
Void
TDecBinCABAC::decodeBin( UInt& ruiBin, ContextModel &rcCtxModel )
{
  UInt uiLPS = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) - 4 ];
  m_uiRange -= uiLPS;
  UInt scaledRange = m_uiRange << 7;
  
  if( m_uiValue < scaledRange )
  {
    // MPS path
    ruiBin = rcCtxModel.getMps();
    rcCtxModel.updateMPS();
    
    if ( scaledRange >= ( 256 << 7 ) )
    {
      return;
    }
    
    m_uiRange = scaledRange >> 6;
    m_uiValue += m_uiValue;
    
    if ( ++m_bitsNeeded == 0 )
    {
      m_bitsNeeded = -8;
      m_uiValue += m_pcTComBitstream->readByte();      
    }
  }
  else
  {
    // LPS path
    Int numBits = TComCABACTables::sm_aucRenormTable[ uiLPS >> 3 ];
    m_uiValue   = ( m_uiValue - scaledRange ) << numBits;
    m_uiRange   = uiLPS << numBits;
    ruiBin      = 1 - rcCtxModel.getMps();
    rcCtxModel.updateLPS();
    
    m_bitsNeeded += numBits;
    
    if ( m_bitsNeeded >= 0 )
    {
      m_uiValue += m_pcTComBitstream->readByte() << m_bitsNeeded;
      m_bitsNeeded -= 8;
    }
  }
}

FFMPEG�Ĵ��룬
static av_always_inline int get_cabac_inline(CABACContext *c, uint8_t * const state){
    int s = *state;
    int RangeLPS= ff_h264_lps_range[2*(c->range&0xC0) + s];
    int bit, lps_mask;


    c->range -= RangeLPS;
    lps_mask= ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

    s^=lps_mask;
    *state= (ff_h264_mlps_state+128)[s];
    bit= s&1;


    lps_mask= ff_h264_norm_shift[c->range];
    c->range<<= lps_mask;
    c->low  <<= lps_mask;

    if(!(c->low & CABAC_MASK)) {
        refill2(c);

    }
    return bit;
}
����������ȫȫ����spec����ͼ��ʵ�ֵĴ��룬ʵ��֤��������ȫ����spec��д�����Ҳ�ǶԵģ���HM��ffpeg���в�ͬ�̶ȵ����䣬�������������ие����ڡ�





void decode_bin(unsigned char *rbsp, int *binVal, struct context_model *cm)
{
	//9.3.4.3.2  Arithmetic decoding process for a binary decision
	int ivlLpsRange, qRangeIdx;
	qRangeIdx = (ivlCurrRange >> 6) & 3;
	ivlLpsRange = rangeTabLps[cm->pStateIdx][qRangeIdx];

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


