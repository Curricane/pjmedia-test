#include <pjmedia-audiodev/audiodev.h>
#include <pjmedia-audiodev/audiotest.h>
#include <pjmedia.h>
#include <pjlib.h>
#include <pjlib-util.h>

#define THIS_FILE "auddemo_w.c"
#define MAX_DEVICES 64
#define WAV_FILE "auddemo_w.wav"

#define PJMEDIA_SIG_PORT_MY		PJMEDIA_SIG_CLASS_PORT_AUD('M','Y')
#define SIGNATURE   PJMEDIA_SIG_PORT_MY

static unsigned dev_count;
static unsigned playback_lat = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
static unsigned capture_lat = PJMEDIA_SND_DEFAULT_REC_LATENCY;

static unsigned int clock_rate = 16000;
static unsigned int samples_per_frame = 16000 / 1000 * 10;
static unsigned int channel_count = 1;
static unsigned int bits_per_sample = 16;

struct myport 
{
    pjmedia_port base;
    pjmedia_delay_buf *delay_buf;
};

typedef struct myport myport;

pj_status_t get_frame(void* data, pjmedia_frame *frame)
{
    if (NULL == data || NULL == frame)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return -1;
    }

    pj_status_t status;

    myport *port = (myport *)data;
    PJ_ASSERT_RETURN(port->delay_buf != NULL, -1);


    status = pjmedia_delay_buf_get(port->delay_buf,
				  (pj_int16_t*)frame->buf);
    PJ_LOG(3, (THIS_FILE, " get frame frame size %d", frame->size));
    
    return status;
}

pj_status_t put_frame(void* data, pjmedia_frame *frame)
{
    if (NULL == data || NULL == frame)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return -1;
    }

    PJ_LOG(3, (THIS_FILE, " get frame frame size %d", frame->size));

    pj_status_t status;
    myport *port = (myport *)data;
    PJ_ASSERT_RETURN(port->delay_buf != NULL, -1);

    status = pjmedia_delay_buf_put(port->delay_buf, (pj_int16_t*)frame->buf);

    return status;

}

pj_status_t my_on_destory(void* data)
{
    if (NULL == data)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return PJ_SUCCESS;
    }
    
    pj_status_t status;
    myport *port = (myport *)data;

    status = pjmedia_delay_buf_destroy(port->delay_buf);
    
    return status;
}

pj_status_t create_myport(pj_pool_t *pool, myport **port)
{
    const pj_str_t MYPORT = { "MYPORT", 6 };
    pj_status_t status;
    myport *mport;
    unsigned int ptime;

    // create the port and the AEC itself 
    mport = PJ_POOL_ZALLOC_T(pool, myport);

    pjmedia_port_info_init(&mport->base.info, &MYPORT, SIGNATURE, 
        clock_rate, channel_count, bits_per_sample, samples_per_frame);

    /* Passive port has delay buf. */
    ptime = samples_per_frame * 1000 / clock_rate / 
	    channel_count;

    status = pjmedia_delay_buf_create(pool, MYPORT.ptr, clock_rate, samples_per_frame, 
        channel_count, ptime * 2, 0, &mport->delay_buf);

    if(status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "faied to pjmedia_delay_buf_create"));
        return status;
    }

    mport->base.get_frame = &get_frame;
    mport->base.put_frame = &put_frame;
    mport->base.on_destroy = &my_on_destory;

    *port = mport;
    return PJ_SUCCESS;
}

static void app_perror(const char *title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    printf("%s: %s (err=%d)\n", title, errmsg, status);
}

static void list_devices(void)
{
    unsigned i;
    pj_status_t status;

    dev_count = pjmedia_aud_dev_count();
    if (dev_count == 0) 
    {
        PJ_LOG(3,(THIS_FILE, "No devices found"));
        return;
    }
    PJ_LOG(3,(THIS_FILE, "Found %d devices:", dev_count));

    for (i=0; i < dev_count; i++)
    {
        pjmedia_aud_dev_info info;
        status = pjmedia_aud_dev_get_info(i, &info);
        if (status != PJ_SUCCESS)
        {
            continue;
        }

        PJ_LOG(3, (THIS_FILE, " %2d: %s [%s] (%d/%d)", 
            i, info.driver, info.name, info.input_count, info.output_count));
    }
}

static const char *decode_caps(unsigned caps)
{
    static char text[200];
    unsigned i;

    text[0] = '\0';

    for (i=0; i<31; ++i)
    {
        if((1 << i) && caps)
        {
            const char *capname;
            capname = pjmedia_aud_dev_cap_name((pjmedia_aud_dev_cap)(1 << i), NULL);
            strcat(text, capname);
            strcat(text, " ");
        }
    }
    return text;
}


static void show_dev_info(unsigned index)
{
#define H "%-20s"
    pjmedia_aud_dev_info info;
    char formats[200];
    pj_status_t status;

    if (index >= dev_count)
    {
        PJ_LOG(1,(THIS_FILE, "Error: invalid index %u", index));
        return;
    }

    status = pjmedia_aud_dev_get_info(index, &info);
    if (status != PJ_SUCCESS) {
        app_perror("pjmedia_aud_dev_get_info() error", status);
        return;
    }

    PJ_LOG(3, (THIS_FILE, "Device at index %u:", index));
    PJ_LOG(3, (THIS_FILE, "-------------------------"));

    PJ_LOG(3, (THIS_FILE, H": %u (0x%x)", "ID", index, index));
    PJ_LOG(3, (THIS_FILE, H": %s", "Name", info.name));
    PJ_LOG(3, (THIS_FILE, H": %s", "Driver", info.driver));
    PJ_LOG(3, (THIS_FILE, H": %u", "Input channels", info.input_count));
    PJ_LOG(3, (THIS_FILE, H": %u", "Output channels", info.output_count));
    PJ_LOG(3, (THIS_FILE, H": %s", "Capabilities", decode_caps(info.caps)));

    formats[0] = '\0';
    if (info.caps & PJMEDIA_AUD_DEV_CAP_EXT_FORMAT)
    {
        unsigned i;
        for (i=0; i <info.ext_fmt_cnt; ++i)
        {
            char bitrate[32];

            switch (info.ext_fmt[i].id)
            {
            case PJMEDIA_FORMAT_L16:
                strcat(formats, "L16/");
                break;
            case PJMEDIA_FORMAT_PCMA:
                strcat(formats, "PCMA/");
                break;
            case PJMEDIA_FORMAT_PCMU:
                strcat(formats, "PCMU/");
                break;
            case PJMEDIA_FORMAT_AMR:
                strcat(formats, "AMR/");
                break;
            case PJMEDIA_FORMAT_G729:
                strcat(formats, "G729/");
                break;
            case PJMEDIA_FORMAT_ILBC:
                strcat(formats, "ILBC/");
                break;
            
            default:
                break;
            }
            sprintf(bitrate, "%u", info.ext_fmt[i].det.aud.avg_bps);
            strcat(formats, bitrate);
            strcat(formats, " ");
        }
    }
    PJ_LOG(3, (THIS_FILE, H": %s", "Extended formats", formats));

#undef H
}

static void test_device(pjmedia_dir dir, unsigned rec_id, unsigned play_id, 
        unsigned clock_rate, unsigned ptime, unsigned chnum)
{
    pjmedia_aud_param param;
}

static pj_status_t rec_cb(void *user_data, pjmedia_frame *frame)
{
    pjmedia_port *port = (pjmedia_port *)user_data;
    return pjmedia_port_put_frame(port, frame);
}

static pj_status_t play_cb(void *user_data, pjmedia_frame *frame)
{
    pjmedia_port *port = (pjmedia_port *)user_data;
    return pjmedia_port_get_frame(port, frame);
}

int main()
{
    pj_caching_pool cp;

    pj_pool_t *pool = NULL;
    
    pj_status_t status;

    pjmedia_aud_param param;
    pjmedia_aud_stream *strm = NULL;

    myport *port = NULL;

    char line[10], *dummy; // get stdin for stop

    // Init pjlib
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    // Must create a pool factory before we can allocate any memory
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    status = pjmedia_aud_subsys_init(&cp.factory);
    if (status != PJ_SUCCESS) 
    {
        app_perror("pjmedia_aud_subsys_init()", status);
        pj_caching_pool_destroy(&cp);
        pj_shutdown();
        return 1;
    }

    list_devices();

    PJ_LOG(3, (THIS_FILE, "start audio test"));
    pool = pj_pool_create(pjmedia_aud_subsys_get_pool_factory(), "wav",
        1000, 1000, NULL);

    status = pjmedia_aud_dev_default_param(0, &param);
    if (status != PJ_SUCCESS)
    {
        app_perror("pjmedia_aud_dev_default_param()", status);
        goto on_return;
    }
    param.rec_id = 0;
    param.play_id = 1;
    param.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param.clock_rate = 16000;
    param.samples_per_frame = 16000 / 1000 * 10;
    param.channel_count = 1;
    param.bits_per_sample = 16;

    // status = pjmedia_null_port_create(pool, param.clock_rate, param.channel_count, 
    //     param.samples_per_frame, param.bits_per_sample, &port);

    status = create_myport(pool, &port);
    if (status != PJ_SUCCESS)
    {
        app_perror("create_myport()", status);
        goto on_return;
    }

    PJ_LOG(3, (THIS_FILE, "pjmedia_aud_stream_create"));
    status = pjmedia_aud_stream_create(&param, &rec_cb, &play_cb, port, &strm);
    if (status != PJ_SUCCESS) 
    {
        app_perror("Error create aud stream", status);
        goto on_return;
    }

    status = pjmedia_aud_stream_start(strm);
    if (status != PJ_SUCCESS)
    {
        app_perror("Error starting the sound device", status);
        goto on_return;
    }

    PJ_LOG(3, (THIS_FILE, "stream started, press ENTER to stop"));
    dummy = fgets(line, sizeof(line), stdin);
    PJ_UNUSED_ARG(dummy);
    

on_return:
    if (strm)
    {
        pjmedia_aud_stream_stop(strm);
        pjmedia_aud_stream_destroy(strm);
    }

    if (pool)
    {
        pj_pool_release(pool);
    }
    pj_caching_pool_destroy(&cp);
    pj_shutdown();
    return 0;

}



/* 
// 录音
static pj_status_t wav_rec_cb(void *user_data, pjmedia_frame *frame)
{
    return pjmedia_port_put_frame((pjmedia_port*)user_data, frame);
}
static void record(unsigned rec_index, const char *filename)
{
    pjmedia_port *wav = NULL;
    pjmedia_aud_param param;
    pjmedia_aud_stream *strm = NULL;

    pj_status_t status;
    status = pjmedia_aud_stream_create(&param, &wav_rec_cb, NULL, wav,
				       &strm);
    status = pjmedia_aud_stream_start(strm);
}

// 播放
static pj_status_t wav_play_cb(void *user_data, pjmedia_frame *frame)
{
    return pjmedia_port_get_frame((pjmedia_port*)user_data, frame);
}
static void play_file(unsigned play_index, const char *filename)
{
    pjmedia_port *wav = NULL;
    pjmedia_aud_param param;
    pjmedia_aud_stream *strm = NULL;
    pj_status_t status;

    status = pjmedia_aud_stream_create(&param, NULL, &wav_play_cb, wav,
				       &strm);
    status = pjmedia_aud_stream_start(strm);
}
*/