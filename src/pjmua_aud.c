#include "pjmua.h"
#include "pjmua_internal.h"

#define THIS_FILE "pjmua_aud.c"


/* Close existing sound device */
static void close_snd_dev(void);

/*****************************************************************************
 *
 * Audio media with PJMEDIA backend
 */

/* Init pjmedia audio subsystem */
pj_status_t pjmua_aud_subsys_init()
{
    unsigned opt;
    pj_status_t status;

    /* Save additional conference bridge parameters for future
     * reference.
     */
    pjmua_var.mconf_cfg.channel_count = pjmua_var.media_cfg.channel_count;
    pjmua_var.mconf_cfg.bits_per_sample = 16;
    pjmua_var.mconf_cfg.samples_per_frame = pjmua_var.media_cfg.clock_rate * 
        pjmua_var.mconf_cfg.channel_count *
        pjmua_var.media_cfg.audio_frame_ptime / 1000;
    
    /* Init options for conference bridge. */
    opt = PJMEDIA_CONF_NO_DEVICE;
    if (pjmua_var.media_cfg.quality >= 3 &&
        pjmua_var.media_cfg.quality <= 4)
    {
        opt |= PJMEDIA_CONF_SMALL_FILTER;
    }
    else if (pjmua_var.media_cfg.quality < 3)
    {
        opt |= PJMEDIA_CONF_SMALL_FILTER;
    }

    /* Init conference bridge. */
    status = pjmedia_conf_create(pjmua_var.pool,
        pjmua_var.media_cfg.max_media_ports,
        pjmua_var.media_cfg.clock_rate,
        pjmua_var.mconf_cfg.channel_count,
        pjmua_var.mconf_cfg.samples_per_frame,
        pjmua_var.mconf_cfg.bits_per_sample,
        opt, &pjmua_var.mconf);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "failed to create pjmedia_conf"));
        goto on_error;
    }

    // using switchboard 
    // todo

    /* Create null port just in case user wants to user null sound. */
    status = pjmedia_null_port_create(pjmua_var.pool,
        pjmua_var.media_cfg.clock_rate,
        pjmua_var.mconf_cfg.channel_count,
        pjmua_var.mconf_cfg.samples_per_frame,
        pjmua_var.mconf_cfg.bits_per_sample,
        &pjmua_var.null_port);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    return status;

on_error:
    return status;
}
pj_status_t pjmua_aud_subsys_start(void)
{
    pj_status_t status = PJ_SUCCESS;

    // todo 
    return status;
}

pj_status_t pjmua_aud_subsys_destroy()
{
    unsigned i;

    close_snd_dev();
    // 关闭rj11

    if(pjmua_var.mconf)
    {
        pjmedia_conf_destroy(pjmua_var.mconf);
        pjmua_var.mconf = NULL;
    }

    /*Destroy file players*/
    for (i = 0; i < PJ_ARRAY_SIZE(pjmua_var.player); ++i)
    {
        if (pjmua_var.player[i].port)
        {
            pjmedia_port_destroy(pjmua_var.player[i].port);
            pjmua_var.player[i].port = NULL;
        }
    }

    /*Destroy file recorder */
    for (i = 0; i < PJ_ARRAY_SIZE(pjmua_var.recorder); ++i)
    {
        if (pjmua_var.recorder[i].port)
        {
            pjmedia_port_destroy(pjmua_var.recorder[i].port);
            pjmua_var.recorder[i].port = NULL;
        }
    }

    return PJ_SUCCESS;
}

/* Close existing sound device */
static void close_snd_dev(void)
{
    /* Notify app */
    // todo

    /* CLose sound device */
    if (pjmua_var.snd_port)
    {
        pjmedia_snd_port_disconnect(pjmua_var.snd_port);
        pjmedia_snd_port_destroy(pjmua_var.snd_port);
        pjmua_var.snd_port = NULL;
    }

    // close rj11?
    // todo

    /* close null sound device */
    if (pjmua_var.null_snd)
    {
        PJ_LOG(4, (THIS_FILE, "Closing null sound device.."));
        pjmedia_master_port_destroy(pjmua_var.null_snd, PJ_FALSE);
        pjmua_var.null_snd = NULL;
    }

    // 是否需要来个rj11_pool并删除
    // todo

    if (pjmua_var.snd_pool)
    {
        pj_pool_release(pjmua_var.snd_pool);
        pjmua_var.snd_pool = NULL;
    }

    pjmua_var.snd_is_on = PJ_FALSE;
}

/*
 * Get maxinum number of conference ports.
 */
PJ_DEF(unsigned) pjmua_conf_get_max_ports(void)
{
    return pjmua_var.media_cfg.max_media_ports;
}

/*
 * Enumerate all conference ports.
 */
PJ_DEF(pj_status_t)pjmua_enum_conf_ports(int id[], unsigned *count)
{
    return pjmedia_conf_enum_ports(pjmua_var.mconf, (unsigned *)id, count);
}

/**
 * Get information about the specified conference port
*/
PJ_DEF(pj_status_t) pjmua_conf_get_port_info(int id, 
        pjmua_conf_port_info *info)
{
    pjmedia_conf_port_info cinfo;
    unsigned i;
    pj_status_t status;

    status = pjmedia_conf_get_port_info(pjmua_var.mconf, id, &cinfo);
    if (status != PJ_SUCCESS)
	    return status;

    pj_bzero(info, sizeof(*info));
    info->slot_id = id;
    info->name = cinfo.name;
    pjmedia_format_copy(&info->format, &cinfo.format);
    info->clock_rate = cinfo.clock_rate;
    info->channel_count = cinfo.channel_count;
    info->samples_per_frame = cinfo.samples_per_frame;
    info->bits_per_sample = cinfo.bits_per_sample;
    info->tx_level_adj = ((float)cinfo.tx_adj_level) / 128 + 1;
    info->rx_level_adj = ((float)cinfo.rx_adj_level) / 128 + 1;

    /* Build array of listeners */
    info->listener_cnt = cinfo.listener_cnt;
    for (i=0; i<cinfo.listener_cnt; ++i) 
    {
	    info->listeners[i] = cinfo.listener_slots[i];
    }

    return PJ_SUCCESS;
}

/*
 * Add arbitrary media port to PJMUA's conference bridge.
 */
PJ_DEF(pj_status_t) pjmua_conf_add_port(pj_pool_t *pool, 
    pjmedia_port *port,
    int *p_id)
{
    pj_status_t status;

    status = pjmedia_conf_add_port(pjmua_var.mconf, pool,
		port, NULL, (unsigned*)p_id);
    if (status != PJ_SUCCESS) 
    {
        if (p_id)
            *p_id = -1;
    }

    return status;
}

/*
 * Remove arbitrary slot from the conference bridge.
 */
PJ_DEF(pj_status_t) pjsua_conf_remove_port(int id)
{
    pj_status_t status;

    status = pjmedia_conf_remove_port(pjmua_var.mconf, (unsigned)id);
    // pjmua_check_snd_dev_idle();

    return status;
}

/*
 * Establish unidirectional media flow from souce to sink.
 */
PJ_DEF(pj_status_t) pjmua_conf_connect(int source, int sink)
{
    pjmua_conf_connect_param prm;

    pjmua_conf_connect_param_default(&prm);
    return pjmua_conf_connect2(source, sink, &prm);
}

/*
 * Establish unidirectional media flow from souce to sink, with signal
 * level adjustment.
 */
PJ_DEF(pj_status_t) pjmua_conf_connect2(int source, int sink,
    const pjmua_conf_connect_param *prm)
{
    pj_status_t status = PJ_SUCCESS;

    PJMUA_LOCK();

    /* The bridge version */

    /* Create sound port if none is instantiated */
    if (pjmua_var.snd_port == NULL && pjmua_var.null_port == NULL && !pjmua_var.no_snd)
    {
        status = pjmua_set_snd_dev(pjmua_var.cap_dev, pjmua_var.play_dev);
        if (status != PJ_SUCCESS)
        {
            PJ_LOG(3, (THIS_FILE, "Error opening sound device", status));
            goto on_return;
        }
    }
    else if (pjmua_var.no_snd && !pjmua_var.snd_is_on)
    {
        pjmua_var.snd_is_on = PJ_TRUE;

        /* Notith app */
        // todo
    }

on_return: 
    PJMUA_UNLOCK();

    if (status == PJ_SUCCESS)
    {
        pjmua_conf_connect_param cc_param;

        if (!prm)
        {
            pjmua_conf_connect_param_default(&cc_param);
        }
        else
        {
            pj_memcpy(&cc_param, prm, sizeof(cc_param));
        }
        status = pjmedia_conf_connect_port(pjmua_var.mconf, source, sink, 
            (int)((cc_param.level - 1) * 128));
    }

    return status;
}

/**
 * Disconnect media flow from the source to destination port.
*/
PJ_DEF(pj_status_t) pjmua_conf_disconnect( int source,
					   int sink)
{
    pj_status_t status;

    status = pjmedia_conf_disconnect_port(pjmua_var.mconf, source, sink);
    
    // pjmua_check_snd_dev_idle();

    return status;
}

/*
 * Adjust the signal level to be transmitted from the bridge to the
 * specified port by making it louder or quieter.
 */
PJ_DEF(pj_status_t) pjmua_conf_adjust_tx_level(int slot,
					       float level)
{
    return pjmedia_conf_adjust_tx_level(pjmua_var.mconf, slot,
					(int)((level-1) * 128));
}

/*
 * Adjust the signal level to be received from the specified port (to
 * the bridge) by making it louder or quieter.
 */
PJ_DEF(pj_status_t) pjmua_conf_adjust_rx_level(int slot,
					       float level)
{
    return pjmedia_conf_adjust_rx_level(pjmua_var.mconf, slot,
					(int)((level-1) * 128));
}


/*
 * Get last signal level transmitted to or received from the specified port.
 */
PJ_DEF(pj_status_t) pjmua_conf_get_signal_level(int slot,
						unsigned *tx_level,
						unsigned *rx_level)
{
    return pjmedia_conf_get_signal_level(pjmua_var.mconf, slot,
					 tx_level, rx_level);
}

/*****************************************************************************
 * File player.
 */

static char* get_basename(const char *path, unsigned len)
{
    char *p = ((char*)path) + len;

    if (len==0)
	return p;

    for (--p; p!=path && *p!='/' && *p!='\\'; ) --p;

    return (p==path) ? p : p+1;
}

/*
 * Create a file player, and automatically connect this player to
 * the conference bridge.
 */
PJ_DEF(pj_status_t) pjmua_player_create( const pj_str_t *filename,
					 unsigned options,
					 int *p_id)
{
    unsigned slot, file_id;
    char path[PJ_MAXPATH];
    pj_pool_t *pool = NULL;
    pjmedia_port *port;
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(pjmua_var.player_cnt < PJ_ARRAY_SIZE(pjmua_var.player), PJ_ETOOMANY);
    PJ_LOG(4,(THIS_FILE, "Creating file player: %.*s..",
	      (int)filename->slen, filename->ptr));
    
    PJMUA_LOCK();
    
    // 找到首个空位置
    for (file_id = 0; file_id < PJ_ARRAY_SIZE(pjmua_var.player); ++file_id)
    {
        if (pjmua_var.player[file_id].port == NULL)
        {
            break;
        }
    }
    if(file_id == PJ_ARRAY_SIZE(pjmua_var.player))
    {
        /* This is unexpected */
        pj_assert(0);
        status = PJ_EBUG;
        goto on_error;
    }

    pj_memcpy(path, filename->ptr, filename->slen);
    path[filename->slen] = '\0';

    pool = pjmua_pool_create(get_basename(path, (unsigned)filename->slen), 1000, 1000);
    if (!pool)
    {
        status = PJ_ENOMEM; // 内存不足
        goto on_error;
    }

    status = pjmedia_wav_player_port_create(
        pool, path,
        pjmua_var.mconf_cfg.samples_per_frame * 1000 / 
        pjmua_var.media_cfg.channel_count / pjmua_var.media_cfg.clock_rate, 
        options, 0 , &port);

    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "failed to create wav_player"));
        goto on_error;
    }

    status = pjmedia_conf_add_port(pjmua_var.mconf, pool, port, filename, &slot);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "Error failed to add_port"));
        goto on_error;
    }

    // 放入player中
    pjmua_var.player[file_id].type = 0;
    pjmua_var.player[file_id].pool = pool;
    pjmua_var.player[file_id].port = port;
    pjmua_var.player[file_id].slot = slot;

    if (p_id)
    {
        *p_id = file_id;
    }

    ++pjmua_var.player_cnt;

    PJMUA_UNLOCK();

    PJ_LOG(4,(THIS_FILE, "Player created, id=%d, slot=%d", file_id, slot));

    return PJ_SUCCESS;

on_error: 
    PJMUA_UNLOCK();
    if (pool)
    {
        pj_pool_release(pool);
    }
    return status;
}