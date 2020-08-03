#include <pjmedia.h>
#include <pjlib-util.h>	/* pj_getopt */
#include <pjlib.h>

#include <stdlib.h>	/* atoi() */
#include <stdio.h>

#include "util.h"

/* For logging purpose. */
#define THIS_FILE   "confsample_w.c"

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
    
    pjmedia_port *rec_port;

    int i, port_count;

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

    /* Create the conference bridge. 
     * With default options (zero), the bridge will create an instance of
     * sound capture and playback device and connect them to slot zero.
     */
    status = pjmedia_conf_create( pool, 
        2,
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

    status = pjmedia_wav_writer_port_create(pool, "confwrite.wav", 
                    clock_rate, channel_count,
                    samples_per_frame,
                    bits_per_sample, 0, 0,
                    &rec_port);
    pjmedia_conf_add_port(conf, pool, rec_port, NULL, NULL);

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