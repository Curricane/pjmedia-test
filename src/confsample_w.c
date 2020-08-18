#include <pjmedia-audiodev/audiodev.h>
#include "../include/pjmedia/rj11_port.h"
#include <pjmedia.h>
#include <pjlib-util.h>	/* pj_getopt */
#include <pjlib.h>

#include <stdlib.h>	/* atoi() */
#include <stdio.h>

#include "util.h"

/* For logging purpose. */
#define THIS_FILE   "confsample_w.c"
#define RECORDER 1

struct rj_port
{
    pjmedia_port external;
    pjmedia_port internal;
    pjmedia_delay_buf *rj11_in_buf;
    pjmedia_delay_buf *rj11_out_buf;
};

typedef struct rj_port rj_port;

// conf从rj_port获取数据
pj_status_t external_get_frame(pjmedia_port* data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame, -1);
    
    if (frame->buf == NULL)
    {
        PJ_LOG(5, (THIS_FILE, "external_get_frame  frame->buf == NULL"));
        return PJ_SUCCESS;
    }

    pj_status_t status;

    rj_port *port = (rj_port *)data;
    PJ_ASSERT_RETURN(port->rj11_in_buf != NULL, -1);

    status = pjmedia_delay_buf_get(port->rj11_in_buf, (pj_int16_t*)frame->buf);
    return status;
}

// conf提供数据给rj_port
pj_status_t external_put_frame(pjmedia_port* data, pjmedia_frame *frame)
{

    PJ_ASSERT_RETURN(data && frame, -1);
    
    if (frame->buf == NULL)
    {
        PJ_LOG(5, (THIS_FILE, "external_put_frame  frame->buf == NULL"));
        return PJ_SUCCESS;
    }

    pj_status_t status;

    rj_port *port = (rj_port *)data;
    PJ_ASSERT_RETURN(port->rj11_out_buf != NULL, -1);
    
    PJ_LOG(3, (THIS_FILE, "put fame->size is: %d", frame->size));
    status = pjmedia_delay_buf_put(port->rj11_out_buf, (pj_int16_t*)frame->buf);
    return status;
}

// rj11提供数据给rj_port
pj_status_t internal_put_frame(pjmedia_port* data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame, -1);
    
    if (frame->buf == NULL)
    {
        PJ_LOG(5, (THIS_FILE, "internal_put_frame  frame->buf == NULL"));
        return PJ_SUCCESS;
    }

    pj_status_t status;

    rj_port *port = (rj_port*)(data->port_data.pdata);

    status = pjmedia_delay_buf_put(port->rj11_in_buf, (pj_int16_t*)frame->buf);

    return status;
}

// rj11从rj11_port中获取数据
pj_status_t internal_get_frame(pjmedia_port* data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame, -1);
    
    if (frame->buf == NULL)
    {
        PJ_LOG(5, (THIS_FILE, "internal_get_frame  frame->buf == NULL"));
        return PJ_SUCCESS;
    }

    pj_status_t status;

    rj_port *port = (rj_port*)(data->port_data.pdata);

    status = pjmedia_delay_buf_get(port->rj11_out_buf, (pj_int16_t*)frame->buf);
    if(status == PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "internal get frame: %d", frame->size));
    }
    else
    {
        PJ_LOG(3, (THIS_FILE, "internal get frame: failed"));
    }
    

    return status;
}

pj_status_t rj_destory(pjmedia_port* data)
{
    PJ_LOG(3, (THIS_FILE, "start rj_destory"));
    if (NULL == data)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return PJ_SUCCESS;
    }
    
    pj_status_t status;
    rj_port *port = (rj_port *)data;

    status = pjmedia_delay_buf_destroy(port->rj11_in_buf);
    status = pjmedia_delay_buf_destroy(port->rj11_out_buf);
    
    return status;
}

pj_status_t create_rj_port(pj_pool_t *pool, 
    int clock_rate,
    int channel_count,
    int bits_per_sample,
    int samples_per_frame,
    rj_port **port)
{
    const pj_str_t RJ_PORT = {"RJ_PORT", 7};
    pj_status_t status;
    rj_port *mport;
    unsigned int ptime;

    ptime = samples_per_frame * 1000 / clock_rate / 
	    channel_count;

    mport = PJ_POOL_ZALLOC_T(pool, rj_port);

    pjmedia_port_info_init(&mport->external.info, &RJ_PORT, 0, 
        clock_rate, channel_count, bits_per_sample, samples_per_frame);
    pjmedia_port_info_init(&mport->internal.info, &RJ_PORT, 0, 
        clock_rate, channel_count, bits_per_sample, samples_per_frame);
    
    status = pjmedia_delay_buf_create(pool, RJ_PORT.ptr, clock_rate,
        samples_per_frame, channel_count, ptime * 2, 0, &mport->rj11_in_buf);
    if(status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "faied to pjmedia_delay_buf_create"));
        return status;
    }

    // 保存所属port的指针
    mport->internal.port_data.pdata = mport;

    status = pjmedia_delay_buf_create(pool, RJ_PORT.ptr, clock_rate,
        samples_per_frame, channel_count, ptime * 2, 0, &mport->rj11_out_buf);
    if(status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "faied to pjmedia_delay_buf_create"));
        return status;
    }
    mport->external.get_frame = &external_get_frame;
    mport->external.put_frame = &external_put_frame;
    mport->internal.get_frame = &internal_get_frame;
    mport->internal.put_frame = &internal_put_frame;
    mport->external.on_destroy = &rj_destory;

    *port = mport;

    return PJ_SUCCESS;
}
/* 显示的声明pjmedia_conf，才能显示使用pjmeida_conf中的参数
 * Conference bridge.
 */
struct pjmedia_conf
{
    unsigned		  options;	/**< Bitmask options.		    */
    unsigned		  max_ports;	/**< Maximum ports.		    */
    unsigned		  port_cnt;	/**< Current number of ports.	    */
    unsigned		  connect_cnt;	/**< Total number of connections    */
    pjmedia_snd_port	 *snd_dev_port;	/**< Sound device port.		    */
    pjmedia_port	 *master_port;	/**< Port zero's port.		    */
    char		  master_name_buf[80]; /**< Port0 name buffer.	    */
    pj_mutex_t		 *mutex;	/**< Conference mutex.		    */
    struct conf_port	**ports;	/**< Array of ports.		    */
    unsigned		  clock_rate;	/**< Sampling rate.		    */
    unsigned		  channel_count;/**< Number of channels (1=mono).   */
    unsigned		  samples_per_frame;	/**< Samples per frame.	    */
    unsigned		  bits_per_sample;	/**< Bits per sample.	    */
};


static const char *desc = 
 " FILE:								    \n"
 "									    \n"
 "  confsample.c							    \n"
 "									    \n"
 " PURPOSE:								    \n"
 "									    \n"
 "  Demonstrate how to use conference bridge.				    \n"
 "									    \n"
 " USAGE:								    \n"
 "									    \n"
 "  confsample [options] [file1.wav] [file2.wav] ...			    \n"
 "									    \n"
 " options:								    \n"
 SND_USAGE
 "									    \n"
 "  fileN.wav are optional WAV files to be connected to the conference      \n"
 "  bridge. The WAV files MUST have single channel (mono) and 16 bit PCM    \n"
 "  samples. It can have arbitrary sampling rate.			    \n"
 "									    \n"
 " DESCRIPTION:								    \n"
 "									    \n"
 "  Here we create a conference bridge, with at least one port (port zero   \n"
 "  is always created for the sound device).				    \n"
 "									    \n"
 "  If WAV files are specified, the WAV file player ports will be connected \n"
 "  to slot starting from number one in the bridge. The WAV files can have  \n"
 "  arbitrary sampling rate; the bridge will convert it to its clock rate.  \n"
 "  However, the files MUST have a single audio channel only (i.e. mono).  \n";

 /* 
 * Prototypes: 
 */

/* List the ports in the conference bridge */
static void conf_list(pjmedia_conf *conf, pj_bool_t detail);

/* Display VU meter */
static void monitor_level(pjmedia_conf *conf, int slot, int dir, int dur);

// 徐亚给这个port加上存储数据的功能
pj_status_t spk_put_frame(pjmedia_port *this_port, 
			     pjmedia_frame *frame)
{
    pjmedia_rj11_port *rj11_port = (pjmedia_rj11_port*)this_port->port_data.pdata;
    return PJ_SUCCESS;
}

pj_status_t spk_get_frame (pjmedia_port *this_port, 
			     pjmedia_frame *frame)
{
    static int get_count = 0;
    if (get_count > 2147483647)
        get_count = 0;
    ++get_count;
    if (get_count % 1000 == 0)
    {
        PJ_LOG(3, (THIS_FILE, "spk_get_frame: %d", get_count));
    }
    return PJ_SUCCESS;
}

/* Show usage */
static void usage(void)
{
    puts("");
    puts(desc);
}

/* Input simple string */
static pj_bool_t input(const char *title, char *buf, pj_size_t len)
{
    char *p;

    printf("%s (empty to cancel): ", title);
    fflush(stdout);
    if (fgets(buf, (int)len, stdin) == NULL)
        return PJ_FALSE;

    /* Remove trailing newlines. */
    for (p=buf; ; ++p)
    {
        if (*p == '\r' || *p == '\n')
            *p = '\0';
        else if (!*p)
            break;
    }

    if (!*buf)
        return PJ_FALSE;
    
    return PJ_TRUE;
}

/*****************************************************************************
 * main()
 */
int main(int argc, char *argv[])
{
    int dev_id = -1;
    int clock_rate = 16000;
    int channel_count = NCHANNELS;
    int samples_per_frame = clock_rate * 20 / 1000;
    int bits_per_sample = NBITS;

    pj_caching_pool cp;
    pjmedia_endpt *med_endpt;
    pj_pool_t *pool;
    pjmedia_conf *conf;
    
    pjmedia_port **file_port; // array of file ports for wav player
    pjmedia_port *rec_port;

    int i = 0, port_count = 2, file_count = 0;

    char tmp[10];
    pj_status_t status;

    /* Must init PJLIB first: */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Get command line options. */
    if (get_snd_options(THIS_FILE, argc, argv, &dev_id, &clock_rate,
			&channel_count, &samples_per_frame, &bits_per_sample))
    {
        usage();
        return 1;
    }

    /* Must create a pool factory before we can allocate any memory. */
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    /* 
     * Initialize media endpoint.
     * This will implicitly initialize PJMEDIA too.
     */
    status = pjmedia_endpt_create(&cp.factory, NULL, 1, &med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create memory pool to allocate memory */
    pool = pj_pool_create( &cp.factory,	    /* pool factory	    */
			   "wav",	    /* pool name.	    */
			   4000,	    /* init size	    */
			   4000,	    /* increment size	    */
			   NULL		    /* callback on error    */
			   );

    file_count = argc - pj_optind;
    port_count = file_count + 1 + RECORDER + 2;

    /* Create the conference bridge. 
     * With default options (zero), the bridge will create an instance of
     * sound capture and playback device and connect them to slot zero.
     */
    status = pjmedia_conf_create( pool, 
        port_count,
        clock_rate,
        channel_count,
        samples_per_frame,
        bits_per_sample,
        0,
        &conf);
    
    if (status != PJ_SUCCESS) 
    {
        app_perror(THIS_FILE, "Unable to create conference bridge", status);
        return 1;
    }

    //pjmedia_snd_port_destroy(conf->snd_dev_port);

    int log_level = pj_log_get_level();
    printf("the log level is %d \n", log_level);
    pj_log_set_level(3);

#if RECORDER
    status = pjmedia_wav_writer_port_create(pool, "confwrite.wav", 
                    clock_rate, channel_count,
                    samples_per_frame,
                    bits_per_sample, 0, 0,
                    &rec_port);
    if (status != PJ_SUCCESS) 
    {
        app_perror(THIS_FILE, "Unable to create WAV writer", status);
        return 1;
    }
    pjmedia_conf_add_port(conf, pool, rec_port, NULL, NULL);
#endif

    // Create file ports. */
    file_port = pj_pool_alloc(pool, file_count * sizeof(pjmedia_port*));


    // create file ports
    for (i = 0; i < file_count; ++i)
    {
        // Load the wav file to file port
        status = pjmedia_wav_player_port_create(
            pool, argv[i+pj_optind],
            0,
            0,
            0,
            &file_port[i]);
        if (status != PJ_SUCCESS)
        {
            char title[80];
            pj_ansi_sprintf(title, "Unable to use %s", argv[i+pj_optind]);
            app_perror(THIS_FILE, title, status);
            usage();
            return 1;
        }

        // add the file port to conference bridge
        status = pjmedia_conf_add_port( conf, 
            pool,
            file_port[i],
            NULL,
            NULL);

        if (status != PJ_SUCCESS) 
        {
            app_perror(THIS_FILE, "Unable to add conference port", status);
            return 1;
	    }
    }
    /* Dump memory usage */
    dump_pool_usage(THIS_FILE, &cp);

    /* Sleep to allow log messages to flush */
    pj_thread_sleep(100);

    // 创建一个扬声器snd
    // 怎么识别扬声器与mic
    int dev_count = 0;
    dev_count = pjmedia_aud_dev_count();
    if (dev_count == 0)
    {
        PJ_LOG(3,(THIS_FILE, "No devices found"));
    }
    int j = 0;
    int speaker_id = 0;
    int mic_id = 0;
    for (; j < dev_count; ++j)
    {
        pjmedia_aud_dev_info info;
        status = pjmedia_aud_dev_get_info(j, &info);
        if (status != PJ_SUCCESS)
        {
            PJ_LOG(3,(THIS_FILE, "failed to get the %dth dev info", j));
            continue;
        }

        PJ_LOG(3, (THIS_FILE, " %2d: %s [%s] (%d/%d) default_sp %d", 
            j, info.driver, info.name, info.input_count, info.output_count, info.default_samples_per_sec));
        
        // 知道MacBook pro扬声器对应的index
        if (info.input_count == 0 && info.name[0] != 'M')
        {
            speaker_id = j;
            PJ_LOG(3, (THIS_FILE, "find MacBook Pro 扬声器 对应的index： %d", speaker_id));
        }

        if (info.input_count != 0 && info.name[0] == 'M')
        {
            mic_id = j;
            PJ_LOG(3, (THIS_FILE, "find MacBook Pro 麦克风 对应的index： %d", mic_id));
        }
    }

    // 创建一个speaker snd
    struct conf_port *spk_conf_port;
    pjmedia_rj11_port *spk_snd;
    pj_str_t spk_name = {"snd/speaker", 11};

    status = pjmedia_rj11_port_create_player(pool, speaker_id, conf->clock_rate, conf->channel_count,
        conf->samples_per_frame, conf->bits_per_sample, 0, &spk_snd);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "failed to pjmedia_rj11_port_create_player"));
        return status;
    }

    // snd缺少一个port去add
    // spk_snd->port = PJ_POOL_ZALLOC_T(pool, pjmedia_port);
    rj_port* rj;
    status = create_rj_port(pool, conf->clock_rate,
        conf->channel_count, conf->bits_per_sample,
        conf->samples_per_frame, &rj);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "failed to create_rj_port"));
        return -1;
    }
    spk_snd->port = &rj->internal;
    // pj_str_t spk_port_name = {"Snd", 3};
    // pjmedia_port_info_init(&spk_snd->port->info, 
    //     &spk_port_name, 0, conf->clock_rate, 
    //     conf->channel_count, conf->bits_per_sample, 
    //     conf->samples_per_frame);
    // spk_snd->port->port_data.pdata = spk_snd;
    // spk_snd->port->port_data.ldata = 0;
    // // spk_snd->port->get_frame = spk_get_frame;
    // spk_snd->port->put_frame = spk_put_frame;
    // 把snd 变为conf_port
    unsigned int slot_num;
    pjmedia_conf_add_port(conf, pool, &rj->external, &spk_name, &slot_num);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "failed to pjmedia_conf_add_port add snd"));
        return status;
    }
    // status =  pjmedia_conf_connect_port(conf, 0, slot_num, 0);
    // if (status != PJ_SUCCESS)
    // {
    //     PJ_LOG(3, (THIS_FILE, "failed to pjmedia_conf_connect_port "));
    //     return status;
    // }


    /*
     * UI Menu: 
     */
    for (;;)
    {
        char tmp1[10];
        char tmp2[10];
        char *err;
        int src, dst, level, dur;

        puts("");
        conf_list(conf, 0);
        puts("");
        puts("Menu:");
        puts("  s    Show ports details");
        puts("  c    Connect one port to another");
        puts("  d    Disconnect port connection");
        puts("  l    set log level");
        puts("  q    Quit");
	    puts("");

        printf("Enter selection: "); fflush(stdout);
        
        if (fgets(tmp, sizeof(tmp), stdin) == NULL)
            break;
        
        switch (tmp[0])
        {
            case 's': 
                puts("");
                conf_list(conf, 1);
                break;

            case 'c': 
                puts("");
                puts("Connect source port to destination port");
                if (!input("Enter source port number", tmp1, sizeof(tmp1)))
                    continue;
                src = strtol(tmp1, &err, 10);
                if (*err || src < 0 || src >= port_count)
                {
                    puts("Invalid slot number");
                    continue;
                }
                puts("Connect dst port to destination port");
                if (!input("Enter dst port number", tmp2, sizeof(tmp1)))
                    continue;
                dst = strtol(tmp2, &err, 10);
                if (*err || dst < 0 || dst >= port_count) {
                    puts("Invalid slot number");
                    continue;
                }
                status = pjmedia_conf_connect_port(conf, src, dst, 0);
                break;
            case 'd': 
                puts("");
                puts("Disconnect port connection");
                if (!input("Enter source port number", tmp1, sizeof(tmp1)))
                    continue;
                src = strtol(tmp1, &err, 10);
                if (*err || src < 0 || src >= port_count)
                {
                    puts("Invalid slot number");
                    continue;
                }
                puts("Connect dst port to destination port");
                if (!input("Enter dst port number", tmp2, sizeof(tmp1)))
                    continue;
                dst = strtol(tmp2, &err, 10);
                if (*err || dst < 0 || dst >= port_count) {
                    puts("Invalid slot number");
                    continue;
                }
                status = pjmedia_conf_disconnect_port(conf, src, dst);
                if (status != PJ_SUCCESS)
		            app_perror(THIS_FILE, "Error connecting port", status);
                break;
            case 'e':
                puts("");
                puts("reset echo param");
                if (!input("Enter options", tmp1, sizeof(tmp1)))
                    continue;
                src = strtol(tmp1, &err, 10);
                if (*err || src < 0)
                {
                    puts("Invalid ec_options");
                    continue;
                }
                if (!input("Enter tail", tmp2, sizeof(tmp2)))
                    continue;
                dst = strtol(tmp2, &err, 10);
                if (*err || dst < 0)
                {
                    puts("Invalid tail");
                    continue;
                }

                status = pjmedia_snd_port_set_ec(conf->snd_dev_port, pool, dst, src);
                if (status != PJ_SUCCESS)
                {
                    PJ_LOG(3, (THIS_FILE, "failed to set ec tail:%d options: %d", dst, src));
                    continue;
                }

                break;
            
            case 'l':
                puts("");
                puts("set a log level less than 6");
                if (!input("Enter log levle", tmp1, sizeof(tmp1)))
                    continue;
                src = strtol(tmp1, &err, 10);
                if (*err || src < 0 || src >= port_count)
                {
                    puts("Invalid slot number");
                    continue;
                }
                pj_log_set_level(src);
                break;

            case 'q':
	            goto on_quit;
            
            default:
                printf("Invalid input character '%c'\n", tmp[0]);
                break;


        }
    }


on_quit:
    
    /* Start deinitialization: */

    /* Destroy conference bridge */
    status = pjmedia_conf_destroy( conf );
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Release application pool */
    pj_pool_release( pool );

    /* Destroy media endpoint. */
    pjmedia_endpt_destroy( med_endpt );

    /* Destroy pool factory */
    pj_caching_pool_destroy( &cp );

    /* Shutdown PJLIB */
    pj_shutdown();

    /* Done. */
    return 0;
}

/*
 * List the ports in conference bridge
 */
static void conf_list(pjmedia_conf *conf, pj_bool_t detail)
{
    enum { MAX_PORTS = 32};

    unsigned i, count;
    pjmedia_conf_port_info info[MAX_PORTS];

    printf("Conference ports:\n");

    count = PJ_ARRAY_SIZE(info);
    pjmedia_conf_get_ports_info(conf, &count, info);

    for (i = 0; i < count; i++)
    {
        char txlist[4 * MAX_PORTS];
        unsigned j;
        pjmedia_conf_port_info *port_info = &info[i];

        txlist[0] = '\0';
        for (j = 0; j < port_info->listener_cnt; ++j)
        {
            char s[10];
            pj_ansi_sprintf(s, "#%d ", port_info->listener_slots[j]);
            pj_ansi_strcat(txlist, s);
        }

        if (txlist[0] == '\0')
        {
            txlist[0] = '-';
            txlist[1] = '\0';
        }

        if (!detail)
        {
            printf("Port #%02d %-25.*s  transmitting to: %s\n",
                port_info->slot,
                (int)port_info->name.slen,
                port_info->name.ptr,
                txlist);
        }
        else
        {
            unsigned tx_level, rx_level;

            pjmedia_conf_get_signal_level(conf, port_info->slot,
                &tx_level, &rx_level);
            
            printf("Port #%02d: \n"
                "  Name                     : %.*s\n"
                "  Sampling rate            : %d Hz\n"
                "  Samples per frame        : %d\n"
                "  Frame time               : %d ms\n"
                "  Signal level adjustment  : tx=%d, rx=%d\n"
                "  Current signal level     : tx=%u, rx=%u\n"
                "  Transmitting to ports    : %s\n\n",
                port_info->slot, 
                (int)port_info->name.slen, port_info->name.ptr,
                port_info->clock_rate,
                port_info->samples_per_frame,
                port_info->samples_per_frame * 1000 / port_info->clock_rate,
                port_info->tx_adj_level, port_info->rx_adj_level,
                tx_level,
                rx_level,
                txlist);
        }
        
    }
    puts("");
}