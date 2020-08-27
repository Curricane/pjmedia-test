/******************************************************************************
 * 文件名  ： rj11_port.h
 * 负责人  ： 陈孟超
 * 创建日期： 20200818 
 * 版本号  ： v1.0
 * 文件描述：rj11_port头文件
 * 版权说明： Copyright (c) 2000-2020	
 * 其    他： 无
 * 修改日志： 无
******************************************************************************/

#ifndef __PJMEDIA_RJ11_PORT_H__
#define __PJMEDIA_RJ11_PORT_H__

#include <pjmedia-audiodev/audiodev.h>
#include <pjmedia/port.h>
#include <pjmedia/clock.h>

PJ_BEGIN_DECL

/**
 * RJ11 port options.
 */
enum pjmedia_rj11_port_option
{
    /** 
     * Don't start the audio device when creating a sound port.
     */
    PJMEDIA_RJ11_PORT_NO_AUTO_START = 1, 
};

/**
 * This structure specifies the parameters to create the sound port.
 * Use pjmedia_rj11_port_param_default() to initialize this structure with
 * default values (mostly zeroes)
 */
typedef struct pjmedia_rj11_port_param
{
    /**
     * Base structure.
    */
    pjmedia_aud_param base;

    /**
    * RJ11 port creation options
    */
    unsigned options;

    /**
     * Arbitrary user data for playback and record preview callbacks below.
     */
    void *user_data;

    /**
     * Optional callback for audio frame preview right before queued to
     * the speaker.
     * Notes:
     * - application MUST NOT block or perform long operation in the callback
     *   as the callback may be executed in sound device thread
     * - when using software echo cancellation, application MUST NOT modify
     *   the audio data from within the callback, otherwise the echo canceller
     *   will not work properly.
     * - the return value of the callback will be ignored
     */
    pjmedia_aud_play_cb on_play_frame;

    /**
     * Optional callback for audio frame preview recorded from the microphone
     * before being processed by any media component such as software echo
     * canceller.
     * Notes:
     * - application MUST NOT block or perform long operation in the callback
     *   as the callback may be executed in sound device thread
     * - when using software echo cancellation, application MUST NOT modify
     *   the audio data from within the callback, otherwise the echo canceller
     *   will not work properly.
     * - the return value of the callback will be ignored
     */
    pjmedia_aud_rec_cb on_rec_frame;


}pjmedia_rj11_port_param;

/**
 * rj11_port 结构
*/
struct pjmedia_rj11_port
{
    int                     rec_id;         /*录音设备id*/
    int			            play_id;        /*播放设备id*/
    pj_uint32_t		        aud_caps;       /*用位表示设备具备的能力*/
    pjmedia_aud_param	    aud_param; 
    pjmedia_aud_stream	    *aud_stream; 
    pjmedia_dir		        dir;
    pjmedia_port	        *port;          /*关联一个port*/

    pjmedia_clock_src       cap_clocksrc,   /*更新时钟、时间戳*/
                            play_clocksrc;

    unsigned		        clock_rate;     /*采样率*/
    unsigned		        channel_count;  /*通道数*/
    unsigned		        samples_per_frame; /*一帧采样数*/
    unsigned		        bits_per_sample; /*一个采样的比特*/
    unsigned		        options; 

    /* audio frame preview callbacks */
    void		            *user_data;
    pjmedia_aud_play_cb     on_play_frame;
    pjmedia_aud_rec_cb      on_rec_frame;
};

typedef struct pjmedia_rj11_port pjmedia_rj11_port;

/**
 * 
 *
 * @param prm		    The parameter.
 */
/*************************************************************************
* 函数名  pjmedia_rj11_port_parm_default
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：Initialize pjmedia_rj11_port_param with default values.
* 输入参数：prm - 为初始化的pjmedia_rj11_port_param
* 输出参数：prm - 默认初始化的为初始化的pjmedia_rj11_port_param
* 返回值：无       
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(void) pjmedia_rj11_port_parm_default(pjmedia_rj11_port_param *prm);

/*别名 struct pjmedia_rj11_port*/
typedef struct pjmedia_rj11_port pjmedia_rj11_port;

/*************************************************************************
* 函数名  pjmedia_rj11_port_create
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：创建rj11_port
* 输入参数：pool - 内存池 
            rec_id - 录音设备id
            play_id - 播放设备id
            clock_rate - 采样率
            channel_count - 通道数
            samples_per_frame - 一帧样本数
            bits_per_sample - 一样本比特数
            options - 选项
            p_port - 未创建和初始化的rj11_port
* 输出参数：p_port - 创建和初始化的rj11_port
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pj_status_t) pjmedia_rj11_port_create(pj_pool_t *pool,
                int rec_id,
                int play_id,
                unsigned clock_rate,
                unsigned channel_count,
                unsigned samples_per_frame,
                unsigned bits_per_sample,
                unsigned options,
                pjmedia_rj11_port **p_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_create_rec
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：创建专门录音的rj11_port
* 输入参数：pool - 内存池 
            dev_id - 录音设备id
            clock_rate - 采样率
            channel_count - 通道数
            samples_per_frame - 一帧样本数
            bits_per_sample - 一样本比特数
            options - 选项
            p_port - 未创建和初始化的rj11_port
* 输出参数：p_port - 创建和初始化的rj11_port
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DEF(pj_status_t) pjmedia_rj11_port_create_rec( pj_pool_t *pool,
						int dev_id,
						unsigned clock_rate,
						unsigned channel_count,
						unsigned samples_per_frame,
						unsigned bits_per_sample,
						unsigned options,
						pjmedia_rj11_port **p_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_create_rec
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：创建专门播放的rj11_port
* 输入参数：pool - 内存池 
            dev_id - 播放设备id
            clock_rate - 采样率
            channel_count - 通道数
            samples_per_frame - 一帧样本数
            bits_per_sample - 一样本比特数
            options - 选项
            p_port - 未创建和初始化的rj11_port
* 输出参数：p_port - 创建和初始化的rj11_port
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DEF(pj_status_t) pjmedia_rj11_port_create_player( pj_pool_t *pool,
						    int dev_id,
						    unsigned clock_rate,
						    unsigned channel_count,
						    unsigned samples_per_frame,
						    unsigned bits_per_sample,
						    unsigned options,
						    pjmedia_rj11_port **p_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_create2
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：用pjmedia_rj11_port_param创建和初始化rj11_port
* 输入参数：pool - 内存池 
            prm - 初始化配置
            p_port - 未创建和初始化的rj11_port
* 输出参数：p_port - 创建和初始化的rj11_port
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pj_status_t) pjmedia_rj11_port_create2(pj_pool_t *pool,
                const pjmedia_rj11_port_param *prm,
                pjmedia_rj11_port **p_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_destroy
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：销毁rj11_port
* 输入参数：rj11_port
* 输出参数：无
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pj_status_t) pjmedia_rj11_port_destroy(pjmedia_rj11_port *rj11_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_get_rj11_stream
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：获取rj11流
* 输入参数：rj11_port
* 输出参数：pjmedia_aud_stream 
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pjmedia_aud_stream*) pjmedia_rj11_port_get_rj11_stream(
                pjmedia_rj11_port *rj11_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_get_clock_src
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：获取时钟源
* 输入参数：rj11_port
        dir - Media direction
* 输出参数：无
* 返回值：	pjmedia_clock_src 函数指针 用于时钟操作 
*                  
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pjmedia_clock_src *)
pjmedia_rj11_port_get_clock_src( pjmedia_rj11_port *rj11_port,
                                pjmedia_dir dir );

/*************************************************************************
* 函数名  pjmedia_rj11_port_connect
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：rj11关联一个port
* 输入参数：rj11_port
            port - pjmedia_port
* 输出参数：PJ_SUCCESS：成功
            其他：失败
* 返回值：	pjmedia_clock_src 函数指针 用于时钟操作 
*                  
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pj_status_t) pjmedia_rj11_port_connect(pjmedia_rj11_port *rj11_port,
					      pjmedia_port *port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_get_port
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：获取rj11关联的port
* 输入参数：rj11_port
* 输出参数：无
* 返回值：	pjmedia_port 
*                  
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pjmedia_port*) pjmedia_rj11_port_get_port(pjmedia_rj11_port *rj11_port);

/*************************************************************************
* 函数名  pjmedia_rj11_port_disconnect
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：断开rj11与之关联的port
* 输入参数：rj11_port
* 输出参数：无
* 返回值：  PJ_SUCCESS：成功
            其他：失败
*                  
* 调用关系：无
* 其    它： 无
*************************************************************************/
PJ_DECL(pj_status_t) pjmedia_rj11_port_disconnect(pjmedia_rj11_port *rj11_port);

PJ_END_DECL

#endif /* __PJMEDIA_RJ11_PORT_H__ */