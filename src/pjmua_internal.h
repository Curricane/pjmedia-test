// 待删除，由另一个头文件去包含这些依赖
/* Include all PJLIB-UTIL headers. */
#include <pjlib-util.h>

/* Include all PJLIB headers. */
#include <pjlib.h>

/* Include all PJMEDIA headers. */
#include <pjmedia.h>

/**
 * File player/recorder data.
 */
typedef struct pjmua_file_data
{
    pj_bool_t	     type;  /* 0=player, 1=playlist */
    pjmedia_port    *port;
    pj_pool_t	    *pool;
    unsigned	     slot;
} pjmua_file_data;

/**
 * Additional parameters for conference bridge.
 */
typedef struct pjmua_conf_setting
{
    unsigned	channel_count;
    unsigned	samples_per_frame;
    unsigned	bits_per_sample;
} pjmua_conf_setting;


/**
 * Global pjmua application data
*/
struct pjmua_data
{
    /* Control: */
    pj_caching_pool     cp;     // Global pool factory.
    pj_pool_t           *pool;  // pjmua's private pool.
    pj_mutex_t          *mutex; // mutex protection for this data
    unsigned            mutex_nesting_level;// mutex nesting level
    pj_thread_t         *mutex_owner;// Mutex owner
    pjmua_state         state;  // library state

    /* Media: */
    pjmua_media_config  media_cfg; // Media config.
    pjmedia_endpt       *med_endpt;// Media endpoint.
    pjmua_conf_setting	 mconf_cfg; /**< Additionan conf. bridge. param */
    pjmedia_conf        *mconf; // Conference bridge
    int                 idx_rj11;// index of rj11 conf_port in Conference bridge

    /* Sound device */
    pjmedia_aud_dev_index cap_dev;
    pjmedia_aud_dev_index play_dev;
    pjmedia_aud_dev_index cap_rj11_dev;
    pjmedia_aud_dev_index play_rj11_dev;
    pj_uint32_t		 aud_svmask;/**< Which settings to save		*/
    pjmedia_aud_param	 aud_param; /**< User settings to sound dev	*/
    pj_bool_t		 aud_open_cnt;/**< How many # device is opened	*/
    pj_bool_t		 no_snd;    /**< No sound (app will manage it)	*/
    pj_pool_t		*snd_pool;  /**< Sound's private pool.		*/
    pjmedia_snd_port	*snd_port;  /**< Sound port. */
    pjmedia_master_port	*null_snd;  /**< Master port for null sound.	*/
    pjmedia_port	*null_port; /**< Null port.			*/
    pj_bool_t		 snd_is_on; /**< Media flow is currently active */
    unsigned		 snd_mode;  /**< Sound device mode.		*/ 

    /* File player: */
    unsigned            player_cnt;// Number of file players.
    pjmua_file_data     player[PJMUA_MAX_PLAYERS];// Array of players.

    /* File recorders: */
    unsigned            rec_cnt; // Number of file recorders
    pjmua_file_data     recorder[PJMUA_MAX_RECORDERS];
    /* Threading: */
    pj_bool_t           thread_quit_flag;// thread quit flag
    pj_thread_t         *thread[4];// Array of threads
};

extern struct pjmua_data pjmua_var;

PJ_INLINE(void) PJMUA_LOCK()
{
    pj_mutex_lock(pjmua_var.mutex);
    pjmua_var.mutex_owner = pj_thread_this();
    ++pjmua_var.mutex_nesting_level;
}

PJ_INLINE(void) PJMUA_UNLOCK()
{
    if (--pjmua_var.mutex_nesting_level == 0)
	pjmua_var.mutex_owner = NULL;
    pj_mutex_unlock(pjmua_var.mutex);
}

PJ_INLINE(pj_status_t) PJMUA_TRY_LOCK()
{
    pj_status_t status;
    status = pj_mutex_trylock(pjmua_var.mutex);
    if (status == PJ_SUCCESS) {
	pjmua_var.mutex_owner = pj_thread_this();
	++pjmua_var.mutex_nesting_level;
    }
    return status;
}

PJ_INLINE(pj_bool_t) PJMUA_LOCK_IS_LOCKED()
{
    return pjmua_var.mutex_owner == pj_thread_this();
}

/* Core */
void pjmua_set_state(pjmua_state new_state);

/**
 * Get the instance of pjmua
 */
PJ_DECL(struct pjmua_data*) pjmua_get_var(void);


/**
 * Init media subsystems.
 */
pj_status_t pjmua_media_subsys_init(const pjmua_media_config *cfg);

/**
 * Destroy pjmua media subsystem.
 */
pj_status_t pjmua_media_subsys_destroy(unsigned flags);

/**
 * Start pmsua media subsystem.
 */
pj_status_t pjmua_media_subsys_start(void);

/*
 * Audio
 */
//pj_status_t pjmua_aud_subsys_init(void);
pj_status_t pjmua_aud_subsys_start(void);
//pj_status_t pjmua_aud_subsys_destroy(void);