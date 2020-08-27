#ifndef __PJMUA_H__
#define __PJMUA_H__

/**
 * @file pjmua.h
 * @brief PJMUA API.
 */


/* Include all PJSIP core headers. */
#include <pjsip.h>

/* Include all PJMEDIA headers. */
#include <pjmedia.h>

/* Include all PJMEDIA-CODEC headers. */
#include <pjmedia-codec.h>

/* Videodev too */
#include <pjmedia_videodev.h>

/* Include all PJSIP-UA headers */
#include <pjsip_ua.h>

/* Include all PJSIP-SIMPLE headers */
#include <pjsip_simple.h>

/* Include all PJNATH headers */
#include <pjnath.h>

/* Include all PJLIB-UTIL headers. */
#include <pjlib-util.h>

/* Include all PJLIB headers. */
#include <pjlib.h>


PJ_BEGIN_DECL

/**
 * The default clock rate to be used by the conference bridge. This setting
 * is the default value for pjmua_media_config.clock_rate.
 */
#ifndef PJMUA_DEFAULT_CLOCK_RATE
#   define PJMUA_DEFAULT_CLOCK_RATE	16000
#endif

/**
 * Default frame length in the conference bridge. This setting
 * is the default value for pjmua_media_config.audio_frame_ptime.
 */
#ifndef PJMUA_DEFAULT_AUDIO_FRAME_PTIME
#   define PJMUA_DEFAULT_AUDIO_FRAME_PTIME  20
#endif

/**
 * Max ports in the conference bridge. This setting is the default value
 * for pjmua_media_config.max_media_ports.
 */
#ifndef PJMUA_MAX_CONF_PORTS
#   define PJMUA_MAX_CONF_PORTS		254
#endif

/**
 * Default codec quality settings. This setting is the default value
 * for pjmua_media_config.quality.
 */
#ifndef PJMUA_DEFAULT_CODEC_QUALITY
#   define PJMUA_DEFAULT_CODEC_QUALITY	8
#endif

/**
 * The default echo canceller tail length. This setting
 * is the default value for pjmua_media_config.ec_tail_len.
 */
#ifndef PJMUA_DEFAULT_EC_TAIL_LEN
#   define PJMUA_DEFAULT_EC_TAIL_LEN	200
#endif

/**
 * The maximum file player.
 */
#ifndef PJMUA_MAX_PLAYERS
#   define PJMUA_MAX_PLAYERS		32
#endif


/**
 * The maximum file player.
 */
#ifndef PJMUA_MAX_RECORDERS
#   define PJMUA_MAX_RECORDERS		32
#endif

/**
 * This enumeration represents pjmua state.
 */
typedef enum pjmua_state
{
    /**
     * The library has not been initialized.
     */
    PJMUA_STATE_NULL,

    /**
     * After pjmua_create() is called but before pjmua_init() is called.
     */
    PJMUA_STATE_CREATED,

    /**
     * After pjmua_init() is called but before pjmua_start() is called.
     */
    PJMUA_STATE_INIT,

    /**
     * After pjmua_start() is called but before everything is running.
     */
    PJMUA_STATE_STARTING,

    /**
     * After pjmua_start() is called and before pjmua_destroy() is called.
     */
    PJMUA_STATE_RUNNING,

    /**
     * After pjmua_destroy() is called but before the function returns.
     */
    PJMUA_STATE_CLOSING

} pjmua_state;

/** Forward declaration */
typedef struct pjmua_media_config pjmua_media_config;


/**
 * This structure describes media configuration, which will be specified
 * when calling #pjmua_init(). Application MUST initialize this structure
 * by calling #pjmua_media_config_default().
 */
struct pjmua_media_config
{
    /**
     * Clock rate to be applied to the conference bridge.
     * If value is zero, default clock rate will be used 
     * (PJMUA_DEFAULT_CLOCK_RATE, which by default is 16KHz).
     */
    unsigned		clock_rate;

    /**
     * Clock rate to be applied when opening the sound device.
     * If value is zero, conference bridge clock rate will be used.
     */
    unsigned		snd_clock_rate;

    /**
     * Channel count be applied when opening the sound device and
     * conference bridge.
     */
    unsigned		channel_count;

    /**
     * Specify audio frame ptime. The value here will affect the 
     * samples per frame of both the sound device and the conference
     * bridge. Specifying lower ptime will normally reduce the
     * latency.
     *
     * Default value: PJMUA_DEFAULT_AUDIO_FRAME_PTIME
     */
    unsigned		audio_frame_ptime;

    /**
     * Specify maximum number of media ports to be created in the
     * conference bridge. Since all media terminate in the bridge
     * (calls, file player, file recorder, etc), the value must be
     * large enough to support all of them. However, the larger
     * the value, the more computations are performed.
     *
     * Default value: PJMUA_MAX_CONF_PORTS
     */
    unsigned		max_media_ports;

    /**
     * Specify the number of worker threads to handle incoming RTP
     * packets. A value of one is recommended for most applications.
     */
    unsigned		thread_cnt;

    /**
     * Media quality, 0-10, according to this table:
     *   5-10: resampling use large filter,
     *   3-4:  resampling use small filter,
     *   1-2:  resampling use linear.
     * The media quality also sets speex codec quality/complexity to the
     * number.
     *
     * Default: 5 (PJMUA_DEFAULT_CODEC_QUALITY).
     */
    unsigned		quality;

    /**
     * Specify default codec ptime.
     *
     * Default: 0 (codec specific)
     */
    unsigned		ptime;

    /**
     * Disable VAD?
     *
     * Default: 0 (no (meaning VAD is enabled))
     */
    pj_bool_t		no_vad;


    /**
     * Echo canceller options (see #pjmedia_echo_create())
     *
     * Default: 0.
     */
    unsigned		ec_options;

    /**
     * Echo canceller tail length, in miliseconds.
     *
     * Default: PJMUA_DEFAULT_EC_TAIL_LEN
     */
    unsigned		ec_tail_len;

    /**
     * Audio capture buffer length, in milliseconds.
     *
     * Default: PJMEDIA_SND_DEFAULT_REC_LATENCY
     */
    unsigned		snd_rec_latency;

    /**
     * Audio playback buffer length, in milliseconds.
     *
     * Default: PJMEDIA_SND_DEFAULT_PLAY_LATENCY
     */
    unsigned		snd_play_latency;

    /** 
     * Jitter buffer initial prefetch delay in msec. The value must be
     * between jb_min_pre and jb_max_pre below. If the value is 0,
     * prefetching will be disabled.
     *
     * Default: -1 (to use default stream settings, currently 0)
     */
    int			jb_init;

    /**
     * Jitter buffer minimum prefetch delay in msec.
     *
     * Default: -1 (to use default stream settings, currently 60 msec)
     */
    int			jb_min_pre;
    
    /**
     * Jitter buffer maximum prefetch delay in msec.
     *
     * Default: -1 (to use default stream settings, currently 240 msec)
     */
    int			jb_max_pre;

    /**
     * Set maximum delay that can be accomodated by the jitter buffer msec.
     *
     * Default: -1 (to use default stream settings, currently 360 msec)
     */
    int			jb_max;

    /**
     * Specify idle time of sound device before it is automatically closed,
     * in seconds. Use value -1 to disable the auto-close feature of sound
     * device
     *
     * Default : 1
     */
    int			snd_auto_close_time;

    /**
     * Optional callback for audio frame preview right before queued to
     * the speaker.
     * Notes:
     * - application MUST NOT block or perform long operation in the callback
     *   as the callback may be executed in sound device thread
     * - when using software echo cancellation, application MUST NOT modify
     *   the audio data from within the callback, otherwise the echo canceller
     *   will not work properly.
     */
    void (*on_aud_prev_play_frame)(pjmedia_frame *frame);

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
     */
    void (*on_aud_prev_rec_frame)(pjmedia_frame *frame);
};


/**
 * Use this function to initialize media config.
 *
 * @param cfg	The media config to be initialized.
 */
PJ_DECL(void) pjmua_media_config_default(pjmua_media_config *cfg);

/**
 * Signal all worker threads to quit. This will only wait until internal
 * threads are done.
 */
PJ_DECL(void) pjmua_stop_worker_threads(void);

/**
 * Instantiate pjmua application. Application must call this function before
 * calling any other functions, to make sure that the underlying libraries
 * are properly initialized. Once this function has returned success,
 * application must call pjmua_destroy() before quitting.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_create(void);

/**
 * Create memory pool to be used by the application. Once application
 * finished using the pool, it must be released with pj_pool_release().
 *
 * @param name		Optional pool name.
 * @param init_size	Initial size of the pool.
 * @param increment	Increment size.
 *
 * @return		The pool, or NULL when there's no memory.
 */
PJ_DECL(pj_pool_t*) pjmua_pool_create(const char *name, pj_size_t init_size,
				      pj_size_t increment);

/**
 * Initialize pjmua with the specified settings. All the settings are 
 * optional, and the default values will be used when the config is not
 * specified.
 *
 * Note that #pjmua_create() MUST be called before calling this function.
 * @param media_cfg	Optional media configuration.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_init(const pjmua_media_config *media_cfg);


/**
 * Application is recommended to call this function after all initialization
 * is done, so that the library can do additional checking set up
 * additional 
 *
 * Application may call this function anytime after #pjmua_init().
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_start(void);


/**
 * Destroy pjmua. Application is recommended to perform graceful shutdown
 * before calling this function , however,
 * this function will do all of these if it finds there are active sessions
 * that need to be terminated. This function will approximately block for
 * one second to wait for replies from remote.
 *
 * Application.may safely call this function more than once if it doesn't
 * keep track of it's state.
 *
 * @see pjmua_destroy2()
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_destroy(void);

/**
 * Variant of destroy with additional flags.
 *
 * @param flags		Combination of pjmua_destroy_flag enumeration.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_destroy2(unsigned flags);

/**
 * Retrieve pjmua state.
 *
 * @return 	pjmua state.
 */
PJ_DECL(pjmua_state) pjmua_get_state(void);

/**
 * Internal function to get media endpoint instance.
 * Only valid after #pjmua_init() is called.
 *
 * @return		Media endpoint instance.
 */
PJ_DECL(pjmedia_endpt*) pjmua_get_pjmedia_endpt(void);

/**
 * Internal function to get PJMUA pool factory.
 * Only valid after #pjmua_create() is called.
 *
 * @return		Pool factory currently used by PJMUA.
 */
PJ_DECL(pj_pool_factory*) pjmua_get_pool_factory(void);


/**
 * This structure describes information about a particular media port that
 * has been registered into the conference bridge. Application can query
 * this info by calling #pjmua_conf_get_port_info().
 */
typedef struct pjmua_conf_port_info
{
    /** Conference port number. */
    int	slot_id;

    /** Port name. */
    pj_str_t		name;

    /** Format. */
    pjmedia_format	format;

    /** Clock rate. */
    unsigned		clock_rate;

    /** Number of channels. */
    unsigned		channel_count;

    /** Samples per frame */
    unsigned		samples_per_frame;

    /** Bits per sample */
    unsigned		bits_per_sample;

    /** Tx level adjustment. */
    float		tx_level_adj;

    /** Rx level adjustment. */
    float		rx_level_adj;

    /** Number of listeners in the array. */
    unsigned		listener_cnt;

    /** Array of listeners (in other words, ports where this port is 
     *  transmitting to).
     */
    int	listeners[PJMUA_MAX_CONF_PORTS];

} pjmua_conf_port_info;

/**
 * This structure specifies the parameters for conference ports connection.
 * Use pjmua_conf_connect_param_default() to initialize this structure with
 * default values.
 */
typedef struct pjmua_conf_connect_param
{
    /*
     * Signal level adjustment from the source to the sink to make it
     * louder or quieter. Value 1.0 means no level adjustment,
     * while value 0 means to mute the port.
     *
     * Default: 1.0
     */
    float		level;

} pjmua_conf_connect_param;

/**
 * Initialize pjmua_conf_connect_param with default values.
 *
 * @param prm		The parameter.
 */
PJ_DECL(void) pjmua_conf_connect_param_default(pjmua_conf_connect_param *prm);


/**
 * Get maxinum number of conference ports.
 *
 * @return		Maximum number of ports in the conference bridge.
 */
PJ_DECL(unsigned) pjmua_conf_get_max_ports(void);


/**
 * Get current number of active ports in the bridge.
 *
 * @return		The number.
 */
PJ_DECL(unsigned) pjmua_conf_get_active_ports(void);


/**
 * Enumerate all conference ports.
 *
 * @param id		Array of conference port ID to be initialized.
 * @param count		On input, specifies max elements in the array.
 *			On return, it contains actual number of elements
 *			that have been initialized.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_enum_conf_ports(int id[],
					   unsigned *count);


/**
 * Get information about the specified conference port
 *
 * @param port_id	Port identification.
 * @param info		Pointer to store the port info.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_conf_get_port_info( int port_id,
					       pjmua_conf_port_info *info);


/**
 * Add arbitrary media port to PJMUA's conference bridge. Application
 * can use this function to add the media port that it creates. For
 * media ports that are created by PJMUA-LIB (such as calls, file player,
 * or file recorder), PJMUA-LIB will automatically add the port to
 * the bridge.
 *
 * @param pool		Pool to use.
 * @param port		Media port to be added to the bridge.
 * @param p_id		Optional pointer to receive the conference 
 *			slot id.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_conf_add_port(pj_pool_t *pool,
					 pjmedia_port *port,
					 int *p_id);


/**
 * Remove arbitrary slot from the conference bridge. Application should only
 * call this function if it registered the port manually with previous call
 * to #pjmua_conf_add_port().
 *
 * @param port_id	The slot id of the port to be removed.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pjmua_conf_remove_port(int port_id);



PJ_END_DECL

#endif // __PJMUA_H__