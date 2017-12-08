#include "video.h"
#include "threadcontrol.h"

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODEING_MODE_PAL;

typedef struct thread_group{
	pthread_t tid_h264;
	pthread_t tid_vpss;
}THREAD_GROUP;

typedef struct enc_param{
	HI_S32       fd;
	long         work_times;
	long         usleep_long;
	VENC_CHN     venc_chn;
	int          work_seconds;
	int          frame_of_seconds;
	int          finish_flag;
	int          stop_flag;
	HI_S32       out_msg_fd;
	int          ret;
}ENC_PARAM;

typedef struct vpss_param{
	long         work_times;
	long         usleep_long;
	VPSS_GRP     vpss_grp;
	VPSS_CHN     vpss_chn;
	int          work_seconds;
	int          frame_of_seconds;
	int          finish_flag;
	int          stop_flag;
	HI_S32       out_msg_fd;
	int          ret;
}VPSS_PARAM;

/************************************************************
*   init the vb and mpp system
*************************************************************/
HI_S32 init_variable(VIDEO_RESOURCE *vi)
{
	
	VB_CONF_S stVbConf;
    HI_U32 u32BlkSize;
    PIC_SIZE_E enSize[2] = {PIC_D1, PIC_CIF};
	PAYLOAD_TYPE_E enPayLoad[2]= {PT_H264, PT_H264};
    HI_S32 s32Ret;

	
    /******************************************
     step  1: init variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = vi->u32ViChnCnt * 5;
    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = vi->u32ViChnCnt * 4;
    memset(stVbConf.astCommPool[1].acMmzName,0,
        sizeof(stVbConf.astCommPool[1].acMmzName));

        /******************************************
        step 2: mpp system init. 
        ******************************************/
        s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("system init failed with %d!\n", s32Ret);
                SAMPLE_COMM_SYS_Exit();
        }

        s32Ret = SAMPLE_COMM_VI_MemConfig(vi->enViMode);
        //s32Ret = SAMPLE_COMM_VI_MemConfig_My(vi->ViDevD1,vi->ViDevHD,vi->u32ViChnCnt);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("SAMPLE_COMM_VI_MemConfig failed with %d!\n", s32Ret);
                SAMPLE_COMM_SYS_Exit();
        }
        s32Ret = SAMPLE_COMM_VPSS_MemConfig();
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
                SAMPLE_COMM_SYS_Exit();
        }

        s32Ret = SAMPLE_COMM_VENC_MemConfig();
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
                SAMPLE_COMM_SYS_Exit();
        }

        return HI_SUCCESS;		
}

/****************************************************
*  start vpss and bind to th viChn
*******************************************************/
HI_S32 start_vpss_bind_viChn(VIDEO_RESOURCE *vi)
{
	HI_S32 s32Ret;
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_D1, &vi->stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		return HI_FAILURE;
	}
	 
	s32Ret = SAMPLE_COMM_VPSS_Start(vi->s32VpssGrpCnt, &vi->stSize, VPSS_MAX_CHN_NUM,NULL);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Vpss failed!\n");
		SAMPLE_COMM_VI_Stop(vi->enViMode);
	}

	s32Ret = SAMPLE_COMM_VI_BindVpss(vi->enViMode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Vi bind Vpss failed!\n");
		SAMPLE_COMM_VPSS_Stop(vi->s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
	}
	 //SAMPLE_PRT("start vpss and vi bind vpss is finished!\n");	 
	return HI_SUCCESS;
}

/***************************************************
* start venc and bind to vpss,and parepare to get the fds
*****************************************************/
HI_S32 start_venc_bind_vpssGrp(VIDEO_RESOURCE * vi)
{
        VPSS_CHN VpssChn;
        //VPSS_GRP VpssGrp;
        PAYLOAD_TYPE_E enPayLoad[2]= {PT_H264, PT_H264};
        PIC_SIZE_E enSize[2] = {PIC_D1, PIC_CIF};

        HI_S32 s32Ret;
        /*** main frame **/

        /**** D1 H264 main frame 1******/

        s32Ret = SAMPLE_COMM_VENC_Start(vi->h264_grp1, vi->h264_chn1, enPayLoad[0], \
                                gs_enNorm, enSize[0], vi->enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                SAMPLE_COMM_VENC_Stop(vi->h264_grp1,vi->h264_chn1);
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(vi->h264_grp1, vi->vpss_grp1, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                
                SAMPLE_COMM_VENC_UnBindVpss(vi->h264_grp1, vi->vpss_grp1, VPSS_BSTR_CHN);
        }

        /**** D1 H264 main frame 2******/

        s32Ret = SAMPLE_COMM_VENC_Start(vi->h264_grp2, vi->h264_chn2, enPayLoad[0], \
                                gs_enNorm, enSize[0], vi->enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                SAMPLE_COMM_VENC_Stop(vi->h264_grp2,vi->h264_chn2);
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(vi->h264_grp2, vi->vpss_grp2, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                
                SAMPLE_COMM_VENC_UnBindVpss(vi->h264_grp2, vi->vpss_grp2, VPSS_BSTR_CHN);
        }

		/**** D1 H264 main frame 3******/

        s32Ret = SAMPLE_COMM_VENC_Start(vi->h264_grp3, vi->h264_chn3, enPayLoad[0], \
                                gs_enNorm, enSize[0], vi->enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                SAMPLE_COMM_VENC_Stop(vi->h264_grp3,vi->h264_chn3);
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(vi->h264_grp3, vi->vpss_grp3, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
                SAMPLE_PRT("Start Venc failed!\n");
                
                SAMPLE_COMM_VENC_UnBindVpss(vi->h264_grp3, vi->vpss_grp3, VPSS_BSTR_CHN);
        }

        return HI_SUCCESS;
}

HI_S32 get_venc_fds(VIDEO_RESOURCE *vi)
{
        HI_S32 s32Ret;
        
        /***get the venc fds**********/
        vi->h264_chn1_fd = HI_MPI_VENC_GetFd(vi->h264_chn1);
        if ( vi->h264_chn1_fd< 0)
        {
                 SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", vi->h264_chn1_fd);
                 return HI_FAILURE;
        }
        vi->h264_chn2_fd = HI_MPI_VENC_GetFd(vi->h264_chn2);
        if(vi->h264_chn2_fd < 0)
        {
                 SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", vi->h264_chn2_fd);
                 return HI_FAILURE;
        }
        vi->h264_chn3_fd = HI_MPI_VENC_GetFd(vi->h264_chn3);
        if ( vi->h264_chn3_fd< 0)
        {
                 SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", vi->h264_chn3_fd);
                 return HI_FAILURE;
        }
        
        return HI_SUCCESS;
	 
}


HI_S32 init_video_resource(VIDEO_RESOURCE *vi)
{
	HI_S32 s32Ret;
	
	vi->u32ViChnCnt   = 3;
	vi->s32VpssGrpCnt = 3;
	vi->enRcMode      = SAMPLE_RC_CBR;
	
	vi->enViMode      = SAMPLE_VI_MODE_4_D1;
	vi->h264_chn1     = 0;
	vi->h264_grp1     = 0;
	vi->vpss_grp1     = 0;

	vi->h264_chn2     = 1;
	vi->h264_grp2     = 1;
	vi->vpss_grp2     = 1;
	
	vi->h264_chn3     = 2;
	vi->h264_grp3     = 2;
	vi->vpss_grp3     = 2;
	/********************************
	    init variable
	********************************/
	s32Ret = init_variable(vi);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("init variable failed!\n");
		return HI_FAILURE;
	}


	/******************************************
		start vi dev & chn to capture
	******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(vi->enViMode, gs_enNorm);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		SAMPLE_COMM_SYS_Exit();
	}	

	/* ***************************************
	*    start the vpss and bind to viChn
	*******************************************/
	s32Ret = start_vpss_bind_viChn(vi);
	if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start the vpss and bind to viChn failed!\n");
        return HI_FAILURE;
    }

	/******************************************
      	step 5: start stream venc (big + snap)
	******************************************/
	s32Ret = start_venc_bind_vpssGrp(vi);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start stream venc failed!\n");
		return HI_FAILURE;
	}
        /******************************************
        *       get the venc fds to the struct video_return
        ******************************************/
        s32Ret = get_venc_fds(vi);
        if(HI_SUCCESS != s32Ret)
        {
	        SAMPLE_PRT("get the venc fds failed!\n");
	        return HI_FAILURE;
        }
        return HI_SUCCESS;	
}
