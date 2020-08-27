/****************************************************************
* 文件名  ：adaptor_port.c
* 负责人  ：陈孟超
* 创建日期：20200818
* 版本号  ： v1.0
* 文件描述：对内对外提供两套pjmedia_port
* 版权说明：
* 其    它：无
* 修改日志：20200818 by陈孟超 chenmc, 首次提交 
******************************************************************************/

#include <pjmedia/adapter_port.h>
#include <pjmedia/errno.h>
#include <pj/assert.h>
#include <pj/log.h>

#define THIS_FILE "adaptor_port.c"



/*************************************************************************
* 函数名  external_get_frame
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：实现pjmedia_port get_frame函数，外部调用获取数据帧
* 输入参数：data - pjemdia_port 
* 输出参数：frame - 存放数据帧的结构
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：一般被conf_port调用
* 其    它： 无
*************************************************************************/
pj_status_t external_get_frame(pjmedia_port *data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame && frame->buf, -1);

    pj_status_t status;

    adaptor_port *port = (adaptor_port *)data;
    PJ_ASSERT_RETURN(port->in_buf != NULL, -1);
    status = pjmedia_delay_buf_get(port->in_buf, (pj_int16_t *)frame->buf);
    
    return status;
}


/*************************************************************************
* 函数名  external_put_frame
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：实现pjmedia_port put_frame函数，外部关联的结构传递数据帧给adaptor_port
* 输入参数：data - pjemdia_port 
            frame - 存放数据帧的结构
* 输出参数：无
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：一般被conf_port调用
* 其    它： 无
*************************************************************************/
pj_status_t external_put_frame(pjmedia_port* data, pjmedia_frame *frame)
{

    PJ_ASSERT_RETURN(data && frame, -1);

    // 判断port->in_buf是否处于重置状态, 
    static int hasRest = 0;

    pj_status_t status;

    adaptor_port *port = (adaptor_port *)data;
    PJ_ASSERT_RETURN(port->out_buf != NULL, -1);

    /*无数据put，重置out_buf*/
    if (frame->buf == NULL || 0 == frame->size )
    {
        
        // out_buf 不处于重置状态，重置他；不然，get时会获取到遗留下的数据
        if (hasRest == 0)
        {
            PJ_LOG(3, (THIS_FILE, "there is no data to external_put_frame  reset out_buf"));
            status = pjmedia_delay_buf_reset(port->out_buf);
            if (status != PJ_SUCCESS)
            {
                PJ_LOG(3, (THIS_FILE, "failed to pjmedia_delay_buf_reset"));
                return -1;
            }
            hasRest = 1;
        }

        return PJ_SUCCESS;
    }
    
    status = pjmedia_delay_buf_put(port->out_buf, (pj_int16_t*)frame->buf);
    
    // 填充过数据，不再是重置状态了
    hasRest = 0;

    return status;
}

/*************************************************************************
* 函数名  internal_get_frame
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：实现pjmedia_port get_frame函数，内部关联的结构，从adaptor_port获取数据帧
* 输入参数：data - pjemdia_port 
* 输出参数：frame - 存放数据帧的结构
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：一般被类audiadev结构调用
* 其    它： 无
*************************************************************************/
pj_status_t internal_get_frame(pjmedia_port* data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame && frame->buf, -1);
    
    pj_status_t status;

    adaptor_port *port = (adaptor_port *)(data->port_data.pdata);

    status = pjmedia_delay_buf_get(port->out_buf, (pj_int16_t*)frame->buf);
    return status;
}


/*************************************************************************
* 函数名  internal_put_frame
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：实现pjmedia_port put_frame函数，内部关联的结构，传递数据帧给adaptor_port
* 输入参数：data - pjemdia_port 
            frame - 存放数据帧的结构
* 输出参数：无
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：一般被类audiadev结构调用
* 其    它： 无
*************************************************************************/
pj_status_t internal_put_frame(pjmedia_port* data, pjmedia_frame *frame)
{
    PJ_ASSERT_RETURN(data && frame, -1);

    pj_status_t status;

    // 判断port->in_buf是否处于重置状态
    static int hasRestInBuf = 0;

    adaptor_port *port = (adaptor_port *)(data->port_data.pdata);

    /*无数据put，重置in_buf*/
    if (frame->buf == NULL || 0 == frame->size )
    {
        
        // in_buf 不处于重置状态，重置他；不然，get时会获取到遗留下的数据
        if (hasRestInBuf == 0)
        {
            PJ_LOG(3, (THIS_FILE, "there is no data to internal_put_frame  reset in_buf"));
            status = pjmedia_delay_buf_reset(port->in_buf);
            if (status != PJ_SUCCESS)
            {
                PJ_LOG(3, (THIS_FILE, "failed to pjmedia_delay_buf_reset"));
                return -1;
            }
            hasRestInBuf = 1;
        }

        return PJ_SUCCESS;
    }

    status = pjmedia_delay_buf_put(port->in_buf, (pj_int16_t*)frame->buf);

    // 填充过数据，不再是重置状态了
    hasRestInBuf = 0; 
    return status;
}

/*************************************************************************
* 函数名  adaptor_port_destory
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：实现pjmedia_port on_destroy函数，销毁adaptor_port
* 输入参数：data - pjemdia_port 
            frame - 存放数据帧的结构
* 输出参数：无
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：一般被类audiadev结构，conf_port调用
* 其    它： 无
*************************************************************************/
pj_status_t adaptor_port_destory(pjmedia_port* data)
{

    if (NULL == data)
    {
        PJ_LOG(5, (THIS_FILE, "invalid param"));
        return PJ_SUCCESS;
    }
    
    pj_status_t status = PJ_SUCCESS;
    adaptor_port *port = (adaptor_port *)(data->port_data.pdata);
    if (port->in_buf != NULL)
    {
        status = pjmedia_delay_buf_destroy(port->in_buf);
        if (status != PJ_SUCCESS)
        {
            return -1;
        }
    }

    if (port->out_buf != NULL)
    {
        status = pjmedia_delay_buf_destroy(port->out_buf);
        if (status != PJ_SUCCESS)
        {
            return -1;
        }
    }

    return status;
}

/*************************************************************************
* 函数名  create_adaptor_port
* 负责人  ：陈孟超
* 创建日期：20200818
* 函数功能：创建一个adaptor_port
* 输入参数：pool - 内存池
            clock_rate - 采样率
            channel_count - 通道数
            bits_per_sample - 每样本比特数
            samples_per_frame - 每帧样本数
* 输出参数：port - adaptor_port
* 返回值：	PJ_SUCCESS: 成功
*           其他: 失败          
* 调用关系：内部成员成为创建类audiadev结构和conf_port的参数
* 其    它： 无
*************************************************************************/
pj_status_t create_adaptor_port(pj_pool_t *pool, 
    int clock_rate,
    int channel_count,
    int bits_per_sample,
    int samples_per_frame,
    adaptor_port **port)
{
    const pj_str_t name = {"ADAPTOR_PORT", 12};
    pj_status_t status;
    adaptor_port *mport;
    unsigned int ptime;

    ptime = samples_per_frame * 1000 / clock_rate / 
	    channel_count;

    mport = PJ_POOL_ZALLOC_T(pool, adaptor_port);

    pjmedia_port_info_init(&mport->external_port.info, &name, 0, 
        clock_rate, channel_count, bits_per_sample, samples_per_frame);
    pjmedia_port_info_init(&mport->internal_port.info, &name, 0, 
        clock_rate, channel_count, bits_per_sample, samples_per_frame);
    
    status = pjmedia_delay_buf_create(pool, name.ptr, clock_rate,
        samples_per_frame, channel_count, ptime * 2, 0, &mport->in_buf);
    if(status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "faied to pjmedia_delay_buf_create"));
        return status;
    }

    /// 保存所属port的指针
    mport->internal_port.port_data.pdata = mport;

    status = pjmedia_delay_buf_create(pool, name.ptr, clock_rate,
        samples_per_frame, channel_count, ptime * 2, 0, &mport->out_buf);
    if(status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE, "faied to pjmedia_delay_buf_create"));
        return status;
    }

    /// 保存所属port的指针
    mport->external_port.port_data.pdata = mport;

    // 赋值对应的pjmedia_port操作
    mport->external_port.get_frame = &external_get_frame;
    mport->external_port.put_frame = &external_put_frame;
    mport->external_port.on_destroy = &adaptor_port_destory;

    mport->internal_port.get_frame = &internal_get_frame;
    mport->internal_port.put_frame = &internal_put_frame;
    mport->internal_port.on_destroy = &adaptor_port_destory;

    *port = mport;

    return PJ_SUCCESS;
}