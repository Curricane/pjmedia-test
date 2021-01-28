#include <pjmedia/port.h>

PJ_BEGIN_DECL

typedef pj_status_t (*updata_buffer_data)(char *buf, unsigned int *len);

static struct buffer_port *create_buffer_port(pj_pool_t *pool,
                unsigned int clock_rate,
                unsigned int channel_count,
                unsigned int bits_per_sample,
                unsigned int samples_per_frame,
                unsigned int buffercap,
                updata_buffer_data updata);

PJ_END_DECL