// 待删除，由另一个头文件去包含这些依赖
/* Include all PJLIB-UTIL headers. */
#include <pjlib-util.h>

/* Include all PJLIB headers. */
#include <pjlib.h>

/* Include all PJMEDIA headers. */
#include <pjmedia.h>

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
    pjmedia_conf        *mconf; // Conference bridge

    /* Sound device */
    pjmedia_aud_dev_index cap_dev;
    pjmedia_aud_dev_index play_dev;


    /* Threading: */
    pj_bool_t           thread_quit_flag;// thread quit flag
    pj_thread_t         *thread[4];// Array of threads
};

extern struct pjmua_data pjmua_var;

/* Core */
void pjmua_set_state(pjmua_state new_state);

/**
 * Get the instance of pjsua
 */
PJ_DECL(struct pjsua_data*) pjsua_get_var(void);

