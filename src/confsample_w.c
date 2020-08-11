#include <pjmedia.h>
#include <pjlib-util.h>	/* pj_getopt */
#include <pjlib.h>

#include <stdlib.h>	/* atoi() */
#include <stdio.h>

#include "util.h"

/* For logging purpose. */
#define THIS_FILE   "confsample_w.c"
#define RECORDER 1

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
    port_count = file_count + 1 + RECORDER;

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