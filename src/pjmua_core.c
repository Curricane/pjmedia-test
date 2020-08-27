#include "pjmua.h"
#include "pjmua_internal.h"

#define THIS_FILE "pjmua_core.c"

/* PJMUA application instance */
struct  pjmua_data pjmua_var;

/* get pjmua instance */
PJ_DEF(struct pjmua_data *) pjmua_get_var(void)
{
    return &pjmua_var;
}

static void init_data()
{
    // todo
    pj_bzero(&pjmua_var, sizeof(pjmua_var));
};

PJ_DEF(void) pjmua_media_config_default(pjmua_media_config *cfg)
{
    const pj_sys_info *si = pj_get_sys_info();

    pj_str_t dev_model = {"iPhone5", 7};
    
    pj_bzero(cfg, sizeof(*cfg));

    cfg->clock_rate = PJMUA_DEFAULT_CLOCK_RATE;

    /* It is reported that there may be some media server resampling problem
     * with iPhone 5 devices running iOS 7, so we set the sound device's
     * clock rate to 44100 to avoid resampling.
     */
    if (pj_stristr(&si->machine, &dev_model) &&
        ((si->os_ver & 0xFF000000) >> 24) >= 7)
    {
        cfg->snd_clock_rate = 44100;
    } else {
        cfg->snd_clock_rate = 0;
    }

    cfg->channel_count = 1;
    cfg->audio_frame_ptime = PJMUA_DEFAULT_AUDIO_FRAME_PTIME;
    cfg->max_media_ports = PJMUA_MAX_CONF_PORTS;
    cfg->thread_cnt = 1;
    cfg->quality = PJMUA_DEFAULT_CODEC_QUALITY;
    cfg->ec_tail_len = PJMUA_DEFAULT_EC_TAIL_LEN;
    cfg->snd_rec_latency = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    cfg->snd_play_latency = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    cfg->jb_init = cfg->jb_min_pre = cfg->jb_max_pre = cfg->jb_max = -1;
    cfg->snd_auto_close_time = 1;
}

/*****************************************************************************
 * PJMUA Base API.
 */

PJ_DEF(void) pjmua_stop_worker_threads(void)
{
    unsigned i;

    pjmua_var.thread_quit_flag = 1;

    /* wait worker threads to quit: */
    for (i = 0; i < 4; ++i)
    {
        if (pjmua_var.thread[i])
        {
            pj_status_t status;
            status = pj_thread_join(pjmua_var.thread[i]);
            if (status != PJ_SUCCESS)
            {
                PJ_LOG(4, (THIS_FILE, "Error joining worker thread"));
                pj_thread_sleep(1000);
            }
            pj_thread_destroy(pjmua_var.thread[i]);
            pjmua_var.thread[i] = NULL;
        }
    }
}


/*
 * Instantiate pjmua application.
 */
PJ_DEF(pj_status_t) pjmua_create(void)
{
    pj_status_t status;

    /* Init pjmua data */
    init_data();

    /* init PJLIB */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Init PJLIB-UTIL: */
    status = pjlib_util_init();
    if (status != PJ_SUCCESS) 
    {
        PJ_LOG(3, (THIS_FILE, "Failed in initializing pjlib-util"));
        pj_shutdown();
	    return status;
    }

    /* Set default sound device ID */
    pjmua_var.cap_dev = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    pjmua_var.play_dev = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;
    pjmua_var.cap_rj11_dev = -3;
    pjmua_var.play_rj11_dev = -4;

    /* init caching pool */
    pj_caching_pool_init(&pjmua_var.cp, NULL, 0);

    /* Create memory pool for application. */
    pjmua_var.pool = pjmua_pool_create("pjmua", 1000, 1000);
    if (pjmua_var.pool == NULL)
    {
        PJ_LOG(3, (THIS_FILE, "Unable to create pjmua pool"));
        pj_shutdown();
    }

    /* Create mutex */
    status = pj_mutex_create_recursive(pjmua_var.pool, "pjmua", 
        &pjmua_var.mutex);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "Unable to create mutex"));
        pjmua_destroy();
    }

    /* Init timer entry list */
    // todo ?

    /* Create timer mutex */
    // todo ?

    // change state to created
    pjmua_set_state(PJMUA_STATE_CREATED);
    return PJ_SUCCESS;

}

/*
 * Initialize pjmua with the specified settings. All the settings are 
 * optional, and the default values will be used when the config is not
 * specified.
 */
PJ_DEF(pj_status_t) pjmua_init(const pjmua_media_config *media_cfg)
{
    pjmua_media_config default_media_cfg;
    pj_status_t status;

    /* Create default configurations when the config is not supplied */
    if (media_cfg == NULL)
    {
        pjmua_media_config_default(&default_media_cfg);
        media_cfg = &default_media_cfg;
    }

    /* Initialize PJMUA media subsystem */
    status = pjmua_media_subsys_init(media_cfg);
    if (status != PJ_SUCCESS)
    {
        goto on_error;
    }

    pjmua_set_state(PJMUA_STATE_INIT);
    return PJ_SUCCESS;

on_error:
    return status;
}

/**
 * Create memory pool.
*/
PJ_DEF(pj_pool_t *) pjmua_pool_create( const char *name, pj_size_t init_size,
    pj_size_t increment)
{
    /* Pool factory is thread safe, no need to lock */
    return pj_pool_create(&pjmua_var.cp.factory, name, init_size, increment, NULL);
}

PJ_DEF(pj_status_t) pjmua_destroy(void)
{
    return pjmua_destroy2(0);
}

/**
 * Destroy pjmua.
*/
PJ_DEF(pj_status_t) pjmua_destroy2(unsigned flags)
{
    int i; 

    if (pjmua_var.state > PJMUA_STATE_NULL && 
        pjmua_var.state < PJMUA_STATE_CLOSING)
    {
        pjmua_set_state(PJMUA_STATE_CLOSING);
    }

    /* Signal threads to quit: */
    // todo

    /* Destroy media (to shutdown media endpoint, etc) */
	pjmua_media_subsys_destroy(flags);

    /* Destroy mutex */
    if (pjmua_var.mutex)
    {
        pj_mutex_destroy(pjmua_var.mutex);
        pjmua_var.mutex = NULL;
    }

    /* Destroy pool and pool factory. */
    if (pjmua_var.pool)
    {
        pj_pool_release(pjmua_var.pool);
        pjmua_var.pool = NULL;
        pj_caching_pool_destroy(&pjmua_var.cp);

        pjmua_set_state(PJMUA_STATE_NULL);

        PJ_LOG(4,(THIS_FILE, "PJMUA destroyed..."));

        /* Shutdown PJLIB */
	    pj_shutdown();
    }

    /* Clear pjmua_var */
    pj_bzero(&pjmua_var, sizeof(pjmua_var));

    /* Done. */
    return PJ_SUCCESS;
}

/**
 * Set library state
*/
void pjmua_set_state(pjmua_state new_state)
{
    const char *state_name[] = {
        "NULL",
        "CREATED",
        "INIT",
        "STARTING",
        "RUNNING",
        "CLOSING"
    };

    pjmua_state old_state = pjmua_var.state;

    pjmua_var.state = new_state;
    PJ_LOG(4,(THIS_FILE, "PJMUA state changed: %s --> %s",
	      state_name[old_state], state_name[new_state]));
}

/* Get state */
PJ_DEF(pjmua_state) pjmua_get_state(void)
{
    return pjmua_var.state;
}

/**
 * Application is recommended to call this function after all initialization
 * is done, so that the library can do additional checking set up
 * additional 
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DEF(pj_status_t) pjmua_start(void)
{
    pj_status_t status;

    pjmua_set_state(PJMUA_STATE_STARTING);


    status = pjmua_media_subsys_start();
    if (status != PJ_SUCCESS)
	    goto on_return;

   // todo

    pjmua_set_state(PJMUA_STATE_RUNNING);

on_return:
    return status;
}

/*
 * Internal function to get media endpoint instance.
 * Only valid after #pjmua_init() is called.
 */
PJ_DEF(pjmedia_endpt*) pjmua_get_pjmedia_endpt(void)
{
    return pjmua_var.med_endpt;
}

/*
 * Internal function to get PJMUA pool factory.
 */
PJ_DEF(pj_pool_factory *) pjmua_get_pool_factory(void)
{
    return &pjmua_var.cp.factory;
}