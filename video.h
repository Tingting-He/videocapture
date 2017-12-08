#ifndef __VIDEO_H__
#define __VIDEO_H__
#include "sample_comm.h"


typedef struct video_resource{
	
	VENC_CHN h264_chn1;
	VENC_GRP h264_grp1;
	HI_S32   h264_chn1_fd;
	
	VENC_CHN h264_chn2;
	VENC_GRP h264_grp2;
	HI_S32   h264_chn2_fd;
	
	VENC_CHN h264_chn3;
	VENC_GRP h264_grp3;
	HI_S32   h264_chn3_fd;
	
	VPSS_GRP vpss_grp1;
	VPSS_CHN vpss_chn1;

	VPSS_GRP vpss_grp2;
	VPSS_CHN vpss_chn2;

	VPSS_GRP vpss_grp3;
	VPSS_CHN vpss_chn3;
	
	VPSS_CHN_MODE_S  vpss_mode;
	SAMPLE_VI_MODE_E enViMode;
	   VIDEO_NORM_E  gs_enNorm;
	   SAMPLE_RC_E   enRcMode;
	   
	HI_U32 u32ViChnCnt;
	HI_S32 s32VpssGrpCnt;
	SIZE_S stSize;
	HI_S32 s32Ret;
	fd_set read_fds;
	HI_S32 maxfd;
	
}VIDEO_RESOURCE;

HI_S32 init_video_resource(VIDEO_RESOURCE *vi);
HI_S32 free_video_resource(VIDEO_RESOURCE *vi);
HI_S32 init_variable(VIDEO_RESOURCE *vi);
HI_S32 SAMPLE_COMM_VI_MemConfig_My(VI_DEV ViDev,HI_U32 u32ViChnCnt);
HI_S32 start_vpss_bind_viChn(VIDEO_RESOURCE *vi);
HI_S32 start_venc_bind_vpssGrp(VIDEO_RESOURCE *vi);

#endif

