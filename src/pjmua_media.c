#include "pjmua.h"
#include "pjmua_internal.h"

#define THIS_FILE		"pjmua_media.c"


static void pjmua_media_config_dup(pj_pool_t *pool, 
    pjmua_media_config *dst,
    const pjmua_media_config *src)
{
    pj_memcpy(dst, src, sizeof(*dst));
}

/**
 * Init media subsystems
*/
pj_status_t pjmua_media_subsys_init(const pjmua_media_config *cfg)
{
    pj_status_t status;

    /* 指定可保存的音频设备设置 */
    pjmua_var.aud_svmask = 0xFFFFFFFF;

    /* 这些是不可设置的 */
    pjmua_var.aud_svmask &= ~(PJMEDIA_AUD_DEV_CAP_EXT_FORMAT |
			      PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER |
			      PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER);
    
    /* 回声消除也要置为不可设置， 它有其他API去设置*/
    pjmua_var.aud_svmask &= ~(PJMEDIA_AUD_DEV_CAP_EC | 
        PJMEDIA_AUD_DEV_CAP_EC_TAIL); 
    
    /* 复制配置 */
    pjmua_media_config_dup(pjmua_var.pool, &pjmua_var.media_cfg, cfg);

    /* 规范配置 */
    if (pjmua_var.media_cfg.snd_clock_rate <= 0)
    {
        pjmua_var.media_cfg.snd_clock_rate = pjmua_var.media_cfg.clock_rate;
    }
    
    /* Create media endpoint */
    status = pjmedia_endpt_create(&pjmua_var.cp.factory,
        NULL,
        0,
        &pjmua_var.med_endpt);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "Media stack initialization has returned error"));
        goto on_error;
    }

    status = pjmua_aud_subsys_init();
    if (status != PJ_SUCCESS)
        goto on_error;

    /* create event manger*/
    // todo

    return PJ_SUCCESS;

on_error:
    return status;
}

/**
 * Destroy pjmua media subsystem
*/
pj_status_t pjmua_media_subsys_destroy(unsigned flags)
{
    PJ_UNUSED_ARG(flags);

    PJ_LOG(4, (THIS_FILE, "Shutting down media..."));

    if (pjmua_var.med_endpt)
    {
        /* Wait for media endpoint's worker threads to quit */
        pjmedia_endpt_stop_threads(pjmua_var.med_endpt);
        pjmua_aud_subsys_destroy();
    }

    return PJ_SUCCESS;
}

/**
 * Start pjmua media subsystem
*/
pj_status_t pjmua_media_subsys_start(void)
{
    pj_status_t status;

    /* Audio */
    status = pjmua_aud_subsys_start();
    if (status != PJ_SUCCESS)
    {
        return status;
    }

    return PJ_SUCCESS;
}