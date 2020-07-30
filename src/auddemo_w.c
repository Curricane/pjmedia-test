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
    
    return status;
}

pj_status_t put_frame(void* data, pjmedia_frame *frame)
{
    if (NULL == data || NULL == frame)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return -1;
    }


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
        if((1 << i) & caps)
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

static void test_rec_play(int rec_id, int play_id)
{
    PJ_LOG(3, (THIS_FILE, "start test_rec_play"));
    myport *port = NULL;
    pj_pool_t *pool = NULL;
    pjmedia_aud_stream *strm = NULL;
    pjmedia_aud_param param;
    pj_status_t status;

    char line[10], *dummy; // get stdin for stop

    pool = pj_pool_create(pjmedia_aud_subsys_get_pool_factory(), "wav",
        1000, 1000, NULL);

    status = pjmedia_aud_dev_default_param(0, &param);
    if (status != PJ_SUCCESS)
    {
        app_perror("pjmedia_aud_dev_default_param()", status);
        goto on_return;
    }
    param.rec_id = rec_id;
    param.play_id = play_id;
    param.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param.clock_rate = 16000;
    param.samples_per_frame = 16000 / 1000 * 10;
    param.channel_count = 1;
    param.bits_per_sample = 16;

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

}

static void print_menu(void)
{
    puts("");
    puts("Audio demo menu:");
    puts("-------------------------------");
    puts("  l                        List devices");
    puts("  t mic_id play_id         Perform test on the device: get mic and put to speaker ");
    puts("  i ID                     Show device info for device ID");
    puts("  q                        Quit");
    puts("");
    printf("Enter selection: ");
    fflush(stdout);
}

int main()
{
    pj_caching_pool cp;
    pj_bool_t done = PJ_FALSE;
    pj_pool_t *pool = NULL;
    pj_status_t status;
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

    while (!done)
    {
        char line[80];

        print_menu();

        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

        switch (line[0])
        {
        case 'l':
            list_devices();
            break;
        case 'q': 
            done = PJ_TRUE;
            break;
        case 't':
            {
                int mic_id = -1, play_id = -1;
                int count;
                count = sscanf(line+2, "%d %d", &mic_id, &play_id);
                if (count != 2)
                {
                    PJ_LOG(3, (THIS_FILE, "invalid param, we need 2 int"));
                    PJ_LOG(3, (THIS_FILE, "mic_id: %d, play_id: %d, count: %d", mic_id, play_id, count));
                    break;
                }
                test_rec_play(mic_id, play_id);
            }
            break;
        case 'i':
            {
                unsigned dev_index;
                if(sscanf(line+2, "%u", &dev_index) != 1)
                {
                    PJ_LOG(3, (THIS_FILE, "invalid param, we need 1 int"));
                    break;
                }
                show_dev_info(dev_index);
            }
            break;
        case 'R':
            pjmedia_aud_dev_refresh();
            PJ_LOG(3, (THIS_FILE, "Audio device list refreshed."));
            break;
        default:
            break;
        }
        
    }

    pj_caching_pool_destroy(&cp);
    pj_shutdown();
    return 0;

}
