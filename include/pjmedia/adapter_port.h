/******************************************************************************
 * 文件名  ： adapter_port.h
 * 负责人  ： 陈孟超
 * 创建日期： 20200818 
 * 版本号  ： v1.0
 * 文件描述：adapter_port头文件
 * 版权说明： Copyright (c) 2000-2020	
 * 其    他： 无
 * 修改日志： 无
******************************************************************************/

#include <pjmedia/delaybuf.h>
#include <pjmedia/port.h>
#include <pj/pool.h>
#include <pj/string.h>

/**
 * 需要两套pjmedia_port操作的port，用于适配两种反向的port
*/
struct adaptor_port
{
    /// 对外暴露的port
    pjmedia_port external_port; 

    /// 对内供audiodev驱动使用 
    pjmedia_port internal_port; 

    /// 向conference bridge内流动数据的buffer 
    pjmedia_delay_buf *in_buf; 

    /// 向conference bridge外流动数据的buffer
    pjmedia_delay_buf *out_buf;  
};

typedef struct adaptor_port adaptor_port;

pj_status_t external_get_frame(pjmedia_port *data, pjmedia_frame *frame);

pj_status_t external_put_frame(pjmedia_port* data, pjmedia_frame *frame);

pj_status_t internal_get_frame(pjmedia_port* data, pjmedia_frame *frame);

pj_status_t internal_put_frame(pjmedia_port* data, pjmedia_frame *frame);

pj_status_t adaptor_port_destory(pjmedia_port* data);

pj_status_t create_adaptor_port(pj_pool_t *pool, 
    int clock_rate,
    int channel_count,
    int bits_per_sample,
    int samples_per_frame,
    adaptor_port **port);