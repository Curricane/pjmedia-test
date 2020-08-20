/****************************************************************
* 文件名  ：rj11_port.c
* 负责人  ：陈孟超
* 创建日期：20200818
* 版本号  ： v1.0
* 文件描述：通过pjmedia_audiadev，以类似sound_port方式，使用rj11
* 版权说明：Copyright (c) 2000-2020   烽火通信科技股份有限公司
* 其    它：无
* 修改日志：20200818 by陈孟超 chenmc, 首次提交 
******************************************************************************/


#include <pjmedia/rj11_port.h>
#include <pjmedia/alaw_ulaw.h>
#include <pjmedia/delaybuf.h>
#include <pjmedia/errno.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/rand.h>
#include <pj/string.h>	    /* pj_memset() */


#define THIS_FILE	    "rj11_port.c"


/*
 * rj11 output回调函数，当需要更多samples，调用该回调函数
 */
static pj_status_t play_cb(void *user_data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(user_data && frame && frame->buf, -1);

    pjmedia_rj11_port *rj11_port = (pjmedia_rj11_port*) user_data;
    pjmedia_port *port;
    const unsigned required_size = (unsigned) frame->size;
    pj_status_t status;

    /// 更新时间戳
    pjmedia_clock_src_update(&rj11_port->play_clocksrc, &frame->timestamp);

    port = rj11_port->port;
    if (port == NULL)
        goto no_frame;
    
    /// 从rj11_port关联的port中获取数据帧
    status = pjmedia_port_get_frame(port, frame);
    if (status != PJ_SUCCESS)
        goto no_frame;
    
    if (frame->type != PJMEDIA_FRAME_TYPE_AUDIO)
        goto no_frame;

    /// Must supply the required samples 
    pj_assert(frame->size == required_size);

    /// Invoke preview callback 
    if (rj11_port->on_play_frame)
        (*rj11_port->on_play_frame)(rj11_port->user_data, frame);

    return PJ_SUCCESS;


/// 返回信号为0的帧
no_frame:
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame->size = required_size;
    pj_bzero(frame->buf, frame->size);

    /* Invoke preview callback */
    if (rj11_port->on_play_frame)
        (*rj11_port->on_play_frame)(rj11_port->user_data, frame);
    
    return PJ_SUCCESS;
    
}

/*
 * 当完成从rj11获取一帧数据时，调用该回调函数，把数据放入指定的port中
 */
static pj_status_t rec_cb(void *user_data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(user_data && frame && frame->buf, -1);

    pjmedia_rj11_port *rj11_port = (pjmedia_rj11_port*) user_data;
    pjmedia_port *port;

    /// 更新时间戳
    pjmedia_clock_src_update(&rj11_port->cap_clocksrc, &frame->timestamp);
    
    /// Invoke preview callback
    if (rj11_port->on_rec_frame)
        (*rj11_port->on_rec_frame)(rj11_port->user_data, frame);

    port = rj11_port->port;

    if (port == NULL)
        return PJ_SUCCESS;

    /// 把获取到的数据帧，通过关联的port放到指定的地方
    pjmedia_port_put_frame(port, frame); 

    return PJ_SUCCESS;

}

/* Initialize with default values (zero) */
PJ_DEF(void) pjmedia_rj11_port_param_default(pjmedia_rj11_port_param *prm)
{
    pj_bzero(prm, sizeof(*prm));
}

/*
 * Start the sound stream.
 * This may be called even when the sound stream has already been started.
 */
static pj_status_t start_sound_device(pj_pool_t *pool, pjmedia_rj11_port *rj11_port)
{
    pjmedia_aud_rec_cb rj11_rec_cb;
    pjmedia_aud_play_cb rj11_play_cb;
    pjmedia_aud_param param_copy;
    pj_status_t status;

    /* Check if sound has been started */
    if (rj11_port->aud_stream != NULL)
        return PJ_SUCCESS;

    PJ_ASSERT_RETURN(rj11_port->dir == PJMEDIA_DIR_CAPTURE || 
                    rj11_port->dir == PJMEDIA_DIR_PLAYBACK || 
                    rj11_port->dir == PJMEDIA_DIR_CAPTURE_PLAYBACK,
                    PJ_EBUG);

    /* 获取设备的能力 */
    if (rj11_port->aud_param.dir & PJMEDIA_DIR_CAPTURE)
    {
        pjmedia_aud_dev_info dev_info;

        status = pjmedia_aud_dev_get_info(rj11_port->aud_param.rec_id,
                    &dev_info);
        if (status != PJ_SUCCESS)
            return status;

        rj11_port->aud_caps = dev_info.caps;
    }
    else 
    {
        rj11_port->aud_caps = 0;
    }

    /* Process EC settings */
    pj_memcpy(&param_copy, &rj11_port->aud_param, sizeof(param_copy));

    rj11_rec_cb = &rec_cb;
	rj11_play_cb = &play_cb;

    /* Open the device */
    status = pjmedia_aud_stream_create(&param_copy,
                    rj11_rec_cb,
                    rj11_play_cb,
                    rj11_port,
                    &rj11_port->aud_stream);
    if (status != PJ_SUCCESS)
        return status;

    /* Start sound stream. */
    if ( !(rj11_port->options & PJMEDIA_RJ11_PORT_NO_AUTO_START))
    {
        status = pjmedia_aud_stream_start(rj11_port->aud_stream);
    }
    if (status != PJ_SUCCESS)
    {
        pjmedia_aud_stream_destroy(rj11_port->aud_stream);
        rj11_port->aud_stream = NULL;
        return status;
    }
    return PJ_SUCCESS;
}

/* 停止rj11设备，停止获取音频数据 */
static pj_status_t stop_sound_device( pjmedia_rj11_port *rj11_port)
{
    /* Check if we have sound stream device. */
    if (rj11_port->aud_stream)
    {
        pjmedia_aud_stream_stop(rj11_port->aud_stream);
        pjmedia_aud_stream_destroy(rj11_port->aud_stream);
        rj11_port->aud_stream = NULL;
    } 

    return PJ_SUCCESS;
}

/*
 * 创建双向的port.
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_create( pj_pool_t *pool,
                    int rec_id,
                    int play_id,
                    unsigned clock_rate,
                    unsigned channel_count,
                    unsigned samples_per_frame,
                    unsigned bits_per_sample,
                    unsigned options,
                    pjmedia_rj11_port **p_port)
{
    pjmedia_rj11_port_param param;
    pj_status_t status;

    pjmedia_rj11_port_param_default(&param);

    /* 没有指定设备id，使用默认的设备id */
    if (rec_id < 0)
	rec_id = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    if (play_id < 0)
	play_id = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;

    status = pjmedia_aud_dev_default_param(rec_id, &param.base);
    if (status != PJ_SUCCESS)
	    return status;
    
    param.base.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK; /// 双向
    param.base.rec_id = rec_id;
    param.base.play_id = play_id;
    param.base.clock_rate = clock_rate;
    param.base.channel_count = channel_count;
    param.base.samples_per_frame = samples_per_frame;
    param.base.bits_per_sample = bits_per_sample;
    param.options = options;

    return pjmedia_rj11_port_create2(pool, &param, p_port);
}

/*
 * Create sound recorder port.
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_create_rec( pj_pool_t *pool,
						int dev_id,
						unsigned clock_rate,
						unsigned channel_count,
						unsigned samples_per_frame,
						unsigned bits_per_sample,
						unsigned options,
						pjmedia_rj11_port **p_port)
{
    pjmedia_rj11_port_param param;
    pj_status_t status;

    pjmedia_rj11_port_param_default(&param);

    /* Normalize dev_id */
    if (dev_id < 0)
	    dev_id = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    
    status = pjmedia_aud_dev_default_param(dev_id, &param.base);
    if (status != PJ_SUCCESS)
	return status;

    /// 配置参数
    param.base.dir = PJMEDIA_DIR_CAPTURE;
    param.base.rec_id = dev_id;
    param.base.clock_rate = clock_rate;
    param.base.channel_count = channel_count;
    param.base.samples_per_frame = samples_per_frame;
    param.base.bits_per_sample = bits_per_sample;
    param.options = options;

    return pjmedia_rj11_port_create2(pool, &param, p_port);
}

/*
 * Create sound player port.
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_create_player( pj_pool_t *pool,
						    int dev_id,
						    unsigned clock_rate,
						    unsigned channel_count,
						    unsigned samples_per_frame,
						    unsigned bits_per_sample,
						    unsigned options,
						    pjmedia_rj11_port **p_port)
{
    pjmedia_rj11_port_param param;
    pj_status_t status;

    pjmedia_rj11_port_param_default(&param);

    /* Normalize dev_id */
    if (dev_id < 0)
	dev_id = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;

    status = pjmedia_aud_dev_default_param(dev_id, &param.base);
    if (status != PJ_SUCCESS)
	return status;

    /// 配置参数
    param.base.dir = PJMEDIA_DIR_PLAYBACK;
    param.base.play_id = dev_id;
    param.base.clock_rate = clock_rate;
    param.base.channel_count = channel_count;
    param.base.samples_per_frame = samples_per_frame;
    param.base.bits_per_sample = bits_per_sample;
    param.options = options;

    return pjmedia_rj11_port_create2(pool, &param, p_port);
}

/*
 * 使用配置参数创建rj11_port.
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_create2(pj_pool_t *pool,
                const pjmedia_rj11_port_param *prm,
                pjmedia_rj11_port **p_port)
{
    pjmedia_rj11_port *rj11_port;
    pj_status_t status;
    unsigned ptime_usec;

    PJ_ASSERT_RETURN(pool && prm && p_port, PJ_EINVAL);

    rj11_port = PJ_POOL_ZALLOC_T(pool, pjmedia_rj11_port);
    PJ_ASSERT_RETURN(rj11_port, PJ_ENOMEM);

    rj11_port->dir = prm->base.dir;
    rj11_port->rec_id = prm->base.rec_id;
    rj11_port->play_id = prm->base.play_id;
    rj11_port->clock_rate = prm->base.clock_rate;
    rj11_port->channel_count = prm->base.channel_count;
    rj11_port->samples_per_frame = prm->base.samples_per_frame;
    rj11_port->bits_per_sample = prm->base.bits_per_sample;
    pj_memcpy(&rj11_port->aud_param, &prm->base, sizeof(rj11_port->aud_param));
    rj11_port->options = prm->options;
    rj11_port->user_data = prm->user_data;
    rj11_port->on_play_frame = prm->on_play_frame;
    rj11_port->on_rec_frame = prm->on_rec_frame;

    /// rj11采样一帧时间间隔
    ptime_usec = prm->base.samples_per_frame * 1000 / prm->base.channel_count /
                 prm->base.clock_rate * 1000;
    pjmedia_clock_src_init(&rj11_port->cap_clocksrc, PJMEDIA_TYPE_AUDIO,
                    rj11_port->clock_rate, ptime_usec);
    pjmedia_clock_src_init(&rj11_port->play_clocksrc, PJMEDIA_TYPE_AUDIO,
                    rj11_port->clock_rate, ptime_usec);
    
    /* Start sound device immediately.
     * If there's no port connected, the sound callback will return
     * empty signal.
     */
    status = start_sound_device(pool, rj11_port);
    if (status != PJ_SUCCESS)
    {
        pjmedia_rj11_port_destroy(rj11_port);
        return status;
    }

    *p_port = rj11_port;
    return PJ_SUCCESS;
}

/*
 * Destroy port (also destroys the sound device).
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_destroy(pjmedia_rj11_port *rj11_port)
{
    PJ_ASSERT_RETURN(rj11_port, PJ_EINVAL);

    return stop_sound_device(rj11_port);
}

/*
 * Retrieve the sound stream associated by this sound device port.
 */
PJ_DEF(pjmedia_aud_stream*) pjmedia_rj11_port_get_snd_stream(
						pjmedia_rj11_port *rj11_port)
{
    PJ_ASSERT_RETURN(rj11_port, NULL);
    return rj11_port->aud_stream;
}

/*
 * Get clock source.
 */
PJ_DEF(pjmedia_clock_src *)
pjmedia_rj11_port_get_clock_src( pjmedia_rj11_port *rj11_port,
                                pjmedia_dir dir )
{
    return (dir == PJMEDIA_DIR_CAPTURE? &rj11_port->cap_clocksrc:
            &rj11_port->play_clocksrc);
}

/*
 * rj11_port关联一个pjmedia_port用于rj11存放获取到的数据，和传递rj11数据
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_connect( pjmedia_rj11_port *rj11_port,
					      pjmedia_port *port)
{
    
    pjmedia_audio_format_detail *afd; 

    PJ_ASSERT_RETURN(rj11_port && port, PJ_EINVAL);

    /// 音频信号的格式信息
    afd = pjmedia_format_get_audio_format_detail(&port->info.fmt, PJ_TRUE);

    /* Check that port has the same configuration as the sound device
     * port.
     */
    if (afd->clock_rate != rj11_port->clock_rate)
	return PJMEDIA_ENCCLOCKRATE;

    if (PJMEDIA_AFD_SPF(afd) != rj11_port->samples_per_frame)
	return PJMEDIA_ENCSAMPLESPFRAME;

    if (afd->channel_count != rj11_port->channel_count)
	return PJMEDIA_ENCCHANNEL;

    if (afd->bits_per_sample != rj11_port->bits_per_sample)
	return PJMEDIA_ENCBITS;

    /* Port is okay. */
    rj11_port->port = port;
    return PJ_SUCCESS;

}

/*
 * Get the connected port.
 */
PJ_DEF(pjmedia_port*) pjmedia_rj11_port_get_port(pjmedia_rj11_port *rj11_port)
{
    PJ_ASSERT_RETURN(rj11_port, NULL);
    return rj11_port->port;
}


/*
 * Disconnect port.
 */
PJ_DEF(pj_status_t) pjmedia_rj11_port_disconnect(pjmedia_rj11_port *rj11_port)
{
    PJ_ASSERT_RETURN(rj11_port, PJ_EINVAL);

    rj11_port->port = NULL;

    return PJ_SUCCESS;
}