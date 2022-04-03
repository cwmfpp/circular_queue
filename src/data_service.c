
#define _GNU_SOURCE      /* See feature_test_macros(7) */
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <unistd.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "circular_queue.h"
#include "data_service.h"

#if 1
#define cq_log_deb(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define cq_log_warn(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define cq_log_err(fmt, x...)  fprintf(stderr, "%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#else

#define cq_log_deb(fmt, ...) /**/
#define cq_log_warn(fmt, ...) /**/
#define cq_log_err(fmt, ...) /**/

#endif

/* notice service thread exit*/
#define TRANS_DATA_THREAD_EXIT  1000

typedef struct DataServiceThread {
    pthread_t m_service_create_tid;
    void *m_data2service_handle; /* send data to service thread */
    int m_service_tid;
    int m_service_exit;
    int m_used_ref;
    data_service_callback m_call_func;
} DataServiceThread;

typedef struct DataServiceInfo {
    int m_cpu_count;
    DataServiceThread *m_pst_dst;
} DataServiceInfo;

typedef struct SendDataInfo {
    void *m_data2service_handle; /* send data to service thread */
    void *m_service2data_handle; /* recv service data file */
    int m_send_data_seq;
    int m_send_data_succ_num;
    int m_send_data_failed_num;
    int m_recv_service_succ_num;
    int m_recv_service_failed_num;
    int m_recv_pend_num;
    int m_data_tid;
    int m_cmd;
} SendDataInfo;

typedef struct DataTrans {
    void *m_service2data_handle;
    void *m_data_addr;
    int m_data_len;
    void *m_priv;
    int m_service_len;
    int m_seq;
    int m_data_tid;
    int m_cmd;
    char m_pend_data[0];
} DataTrans;

static void *g_data_service = NULL;
static pthread_mutex_t g_data_service_mutex = PTHREAD_MUTEX_INITIALIZER;

long get_tid(void)
{
#ifdef MAC_OS
    uint64_t tid;
    pthread_threadid_np(0, &tid);
    return (long)tid;
#else
    long tid = 0;
    tid = syscall(__NR_gettid);
    return (long)tid;
#endif
}

static void *data_service_thread(void *_pv_arg)
{
    DataServiceThread *pst_dst = NULL;
    Element st_data2service_elem;
    int ideq_count = 0;
    int tid = 0;
    DataTrans *pst_data_trans = NULL;
    int cmd = 0;
    int send_service_failed_num = 0;

    if (NULL == _pv_arg) {
        cq_log_err("invalid _pv_arg(%p)", _pv_arg);
        return NULL;
    }
    pst_dst = (DataServiceThread *)_pv_arg;
    tid = (int)get_tid();
    for (;;) {
        if (0 == pst_dst->m_service_tid) {
            pst_dst->m_service_tid = tid;
        }
        //  recv service data
        if (CQDequeue(pst_dst->m_data2service_handle, &st_data2service_elem) <
                0 &&
            CQ_NONBLOCK ==
                CQGetDeqMode(pst_dst->m_data2service_handle)) {
            cq_log_err("call CQDequeue fail, tid(%d)", (int)tid);
        }

        pst_data_trans = (DataTrans *)st_data2service_elem.m_pvData;
        cmd = pst_data_trans->m_cmd;

        switch (cmd) {
        case TRANS_DATA:
        case TRANS_DATA_END:
            // send data
            ++ideq_count;
            /*cq_log_err("tid(%d) ideq_count=%d m_seq=%d \n", (int)tid,
                       ideq_count, pst_data_trans->m_seq);*/
            if (NULL != pst_dst->m_call_func) {
                pst_dst->m_call_func(pst_data_trans->m_data_addr,
                                            pst_data_trans->m_data_len,
                                            pst_data_trans->m_priv);
            }
            if (CQEnqueue(pst_data_trans->m_service2data_handle,
                             &st_data2service_elem) < 0) {
                send_service_failed_num++;
                cq_log_err(
                    "call CQEnqueue fail, send_service_failed_num=%d tid(%d)",
                    send_service_failed_num, (int)tid);
            }

            if (CQ_NONBLOCK ==
                CQGetEnqMode(pst_dst->m_data2service_handle)) {
            }
            break;

        case TRANS_DATA_THREAD_EXIT:
            /* code */
            if (1 == pst_dst->m_service_exit) {
                //cq_log_err("data_service_tid(%d) exit ", (int)tid);
                free(pst_data_trans);
                pst_data_trans = NULL;
                return NULL;
            }
            break;

        default:
            break;
        }
    }

    cq_log_err("tid(%d) send_service_failed_num=%d ", (int)tid,
                send_service_failed_num);
    return NULL;
}

static int get_cpu_count(void)
{
    int num_cpus = 1;

    num_cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);

    return num_cpus;
}

static void *open_service(data_service_callback call_func)
{
    int i = 0;

    pthread_mutex_lock(&g_data_service_mutex);

    DataServiceInfo *pst_dsi = NULL;
    DataServiceThread *pst_dst = NULL;
    void *d2s_handle = NULL;
    CQAttr st_cq_attr;
    if (NULL != g_data_service) {
        int thread_index = 0;
        pst_dsi = (DataServiceInfo *)g_data_service;
        pst_dst = pst_dsi->m_pst_dst;
        pst_dst[0].m_used_ref++;
        thread_index = (pst_dst[0].m_used_ref - 1) % pst_dsi->m_cpu_count;
        cq_log_err("map service thread_index=%d", thread_index);
        pthread_mutex_unlock(&g_data_service_mutex);
        return pst_dst[thread_index].m_data2service_handle;
    }
    pst_dsi = (DataServiceInfo *)malloc(sizeof(DataServiceInfo)); 
    if (NULL == pst_dsi) {
        pthread_mutex_unlock(&g_data_service_mutex);
        return NULL;
    }
    pst_dsi->m_cpu_count = get_cpu_count();
    cq_log_err("cpu count=%d", pst_dsi->m_cpu_count);

    pst_dst = (DataServiceThread *)calloc((size_t)(pst_dsi->m_cpu_count),
                                          sizeof(DataServiceThread));
    if (NULL == pst_dst) {
        free(pst_dsi);
        pst_dsi = NULL;
        pthread_mutex_unlock(&g_data_service_mutex);
        return NULL;
    }

    for (i = 0; i < pst_dsi->m_cpu_count; i++) {
        st_cq_attr.m_iElementCount = 10;
        st_cq_attr.m_iModeDeq = CQ_BLOCK; // CQ_NONBLOCK  CQ_BLOCK
        st_cq_attr.m_iModeEnq = CQ_BLOCK; // CQ_NONBLOCK  CQ_BLOCK
        d2s_handle = CQInit(&st_cq_attr);
        if (NULL == d2s_handle) {
            pthread_mutex_unlock(&g_data_service_mutex);
            return NULL;
        }
        pst_dst[i].m_data2service_handle = d2s_handle;
        pst_dst[i].m_service_tid = 0;
        pst_dst[i].m_service_exit = 0;
        pst_dst[i].m_used_ref = 1;
        pst_dst[i].m_call_func = call_func;
        if (0 != pthread_create(&pst_dst[i].m_service_create_tid, NULL,
                                data_service_thread, &pst_dst[i])) {
            cq_log_err("pthread_create EnqueueThread fail");
            CQUninit(pst_dst[i].m_data2service_handle);
            pst_dst[i].m_data2service_handle = NULL;
            free(pst_dst);
            pst_dst = NULL;
            pthread_mutex_unlock(&g_data_service_mutex);
            return NULL;
        }
    }

    for (i = 0; i < pst_dsi->m_cpu_count; i++) {
        cq_log_err("i=%d tid=%d\n", i, pst_dst[i].m_service_tid);
        if (0 == pst_dst[i].m_service_tid) {
            cq_log_err("continue i=%d\n", i);
            i = -1;
        }
    }

    pst_dsi->m_pst_dst = pst_dst;
    g_data_service = (void *)pst_dsi;
    pthread_mutex_unlock(&g_data_service_mutex);
    return pst_dst[0].m_data2service_handle;
}

static int close_data2service_handle(void)
{
    int i = 0;
    int s = 0;
    void *res = NULL;
    DataServiceInfo *pst_dsi = NULL;
    DataServiceThread *pst_dst = NULL;
    pthread_mutex_lock(&g_data_service_mutex);
    if (NULL == g_data_service) {
        pthread_mutex_unlock(&g_data_service_mutex);
        return 0;
    }
    pst_dsi = (DataServiceInfo *)g_data_service;
    pst_dst = pst_dsi->m_pst_dst;
    pst_dst[0].m_used_ref--;
    if (0 != pst_dst[0].m_used_ref) {
        pthread_mutex_unlock(&g_data_service_mutex);
        return 0;
    }

    for (i = 0; i < pst_dsi->m_cpu_count; i++) {
        Element st_send_data_elem;
        int trans_size = (int)(sizeof(DataTrans));
        pst_dst[i].m_service_exit = 1;

        DataTrans *pst_data_trans = (DataTrans *)malloc((size_t)trans_size);
        if (NULL == pst_data_trans) {
            return -1;
        }
        pst_data_trans->m_cmd = TRANS_DATA_THREAD_EXIT;
        pst_data_trans->m_service2data_handle = NULL;
       
        pst_data_trans->m_priv = NULL;
        pst_data_trans->m_seq = -1;
        pst_data_trans->m_data_tid = 0;

        st_send_data_elem.m_pvData = (DataTrans *)pst_data_trans;
        st_send_data_elem.m_iDataLen = trans_size;
        // send data
        if (CQEnqueue(pst_dst[i].m_data2service_handle, &st_send_data_elem) <
            0) {
            cq_log_err("call CQEnqueue fail ");
        }
    }

    for (i = 0; i < pst_dsi->m_cpu_count; i++) {
        cq_log_err("service thread[%d] tid(%d) wait join\n", i, pst_dst[i].m_service_tid);
        s = pthread_join(pst_dst[i].m_service_create_tid, &res);
        if (0 != s) {
            printf("pthread_join error\n");
        }
        cq_log_err("service thread[%d] tid(%d) exit\n", i, pst_dst[i].m_service_tid);
        if (NULL != res) {
            cq_log_err("returned value was %s\n", (char *)res);
            free(res); /* Free memory allocated by thread */
        }
        if ((pst_dsi->m_cpu_count - 1) == i &&
            0 != CQGetElementCount(pst_dst[i].m_data2service_handle)) {
            cq_log_err("ele left fifo in data2service, error");
        }
        CQUninit(pst_dst[i].m_data2service_handle);
        pst_dst[i].m_data2service_handle = NULL;
    }

    free(pst_dst);
    pst_dst = NULL;
    free(pst_dsi);
    pst_dsi = NULL;
    g_data_service = NULL;
    pthread_mutex_unlock(&g_data_service_mutex);

    return 0;
}

void *create_data_service(data_service_callback call_func, int recv_block)
{
    void *phdl = NULL;
    SendDataInfo *pst_send_data_info = NULL;
    CQAttr st_cq_attr;
    void *d2s_handle = NULL;
    void *s2d_handle = NULL;

    d2s_handle = open_service(call_func);
    //cq_log_err("d2s_handle=%p", d2s_handle);
    if (NULL == d2s_handle) {
        return phdl;
    }
    st_cq_attr.m_iElementCount = 5;
    st_cq_attr.m_iModeEnq = CQ_BLOCK; // CQ_NONBLOCK  CQ_BLOCK
    st_cq_attr.m_iModeDeq = (recv_block == 1) ? CQ_BLOCK : CQ_NONBLOCK; // CQ_NONBLOCK  CQ_BLOCK
    s2d_handle = CQInit(&st_cq_attr);
    if (NULL == s2d_handle) {
        return NULL;
    }

    pst_send_data_info = (SendDataInfo *)malloc(sizeof(*pst_send_data_info));
    if (NULL == pst_send_data_info) {
        return phdl;
    }
    memset(pst_send_data_info, 0, sizeof(*pst_send_data_info));
    pst_send_data_info->m_data2service_handle = d2s_handle;
    pst_send_data_info->m_service2data_handle = s2d_handle;
    pst_send_data_info->m_send_data_seq = 0;
    pst_send_data_info->m_send_data_succ_num= 0;
    pst_send_data_info->m_send_data_failed_num= 0;
    pst_send_data_info->m_recv_service_succ_num = 0;
    pst_send_data_info->m_recv_service_failed_num = 0;
    pst_send_data_info->m_recv_pend_num = 0;
    pst_send_data_info->m_data_tid= 0;
    pst_send_data_info->m_cmd= 0;

    phdl = pst_send_data_info;

    return phdl; 
}

int send_data_to_service(void *phdl, int data_tid, int data_cmd, void *data_addr, int data_len, void *priv)
{
    SendDataInfo *pst_send_data_info = NULL;
    DataTrans *pst_data_trans = NULL;
    Element st_send_data_elem;
    int trans_size = 0;
    if (NULL == phdl) {
        return -1;
    }
    pst_send_data_info = (SendDataInfo *)phdl;
    trans_size = (int)(sizeof(DataTrans)) + (data_len > 0 ? data_len : 0);
    pst_data_trans = (DataTrans *)malloc((size_t)trans_size);
    if (NULL == pst_data_trans) {
        return -1;
    }
    pst_data_trans->m_cmd = data_cmd;
    pst_data_trans->m_service2data_handle = pst_send_data_info->m_service2data_handle;
    pst_data_trans->m_data_addr = NULL;
    pst_data_trans->m_data_len = 0;
    if (data_addr != NULL && data_len > 0) {
        pst_data_trans->m_data_addr =
            (char *)pst_data_trans + sizeof(DataTrans);
        pst_data_trans->m_data_len = data_len;
        memcpy(pst_data_trans->m_data_addr, data_addr, (size_t)data_len);
    }
    pst_data_trans->m_priv = priv;
    pst_data_trans->m_seq = pst_send_data_info->m_send_data_seq++;
    pst_data_trans->m_data_tid = data_tid;

    pst_send_data_info->m_cmd = data_cmd;
    pst_send_data_info->m_data_tid = data_tid;

    st_send_data_elem.m_pvData = (DataTrans *)pst_data_trans;
    st_send_data_elem.m_iDataLen = trans_size;
    // send data
    if (CQEnqueue(pst_send_data_info->m_data2service_handle,
                  &st_send_data_elem) < 0) {
        pst_send_data_info->m_send_data_failed_num++;
        cq_log_err("call CQEnqueue fail pst_send_data_info->m_send_data_seq=%d",
                   pst_send_data_info->m_send_data_seq);
    }
    /*cq_log_err(
        "data_tid(%d) send data seq=%d, senq=%d, revq=%d", data_tid,
        pst_data_trans->m_seq,
        CQGetElementCount(pst_send_data_info->m_data2service_handle),
        CQGetElementCount(pst_send_data_info->m_service2data_handle));*/
    pst_send_data_info->m_send_data_succ_num++;
    pst_send_data_info->m_recv_pend_num++;

    return 0;
}

int recv_data_from_service(void *phdl, void *data_addr, int data_len, void **ppriv, int flush_data)
{
    SendDataInfo *pst_send_data_info = NULL;
    void *s2d_handle = NULL;
    Element st_send_data_elem;
    DataTrans *pst_data_trans = NULL;
    int recv_data_cmd = 0;
    if (NULL == phdl) {
        return -1;
    }
    pst_send_data_info = (SendDataInfo *)phdl;

    s2d_handle = pst_send_data_info->m_service2data_handle;

    if (pst_send_data_info->m_recv_pend_num > 0/*1*/ || 1 == flush_data) {
        if (CQDequeue(s2d_handle, &st_send_data_elem) < 0 &&
            CQ_NONBLOCK == CQGetDeqMode(s2d_handle)) {
            cq_log_err("call CQDequeue fail, data_tid(%d)",
                       (int)pst_send_data_info->m_data_tid);
            pst_send_data_info->m_recv_service_failed_num++;
            recv_data_cmd = TRANS_DATA_EAGAIN;
        } else {
            pst_send_data_info->m_recv_service_succ_num++;
            pst_send_data_info->m_recv_pend_num--;
            pst_data_trans = (DataTrans *)st_send_data_elem.m_pvData;
            if (NULL == pst_data_trans) {
                return -1;
            }
            recv_data_cmd = pst_data_trans->m_cmd;
            /*cq_log_err("data_tid(%d) recv service seq=%d m_recv_pend_num=%d",
                       (int)pst_send_data_info->m_data_tid,
                       pst_data_trans->m_seq,
                       pst_send_data_info->m_recv_pend_num);*/
            if (pst_send_data_info->m_data_tid !=
                pst_data_trans->m_data_tid) {
                cq_log_err("recv mismatch data data_tid(%d) tid(%d)",
                           pst_send_data_info->m_data_tid,
                           pst_data_trans->m_data_tid);
                recv_data_cmd = TRANS_DATA_END;
            }
            if (NULL != data_addr && NULL != pst_data_trans->m_data_addr &&
                data_len > 0 && data_len == pst_data_trans->m_data_len) {
                memcpy((char *)data_addr, (char *)pst_data_trans->m_data_addr, (size_t)data_len);
        }
        if (NULL != ppriv) {
            *ppriv = pst_data_trans->m_priv;
        }
        free(pst_data_trans);
        pst_data_trans = NULL;
        }
    }

    return recv_data_cmd;
}

int destroy_data_service(void *phdl)
{
    SendDataInfo *pst_send_data_info = NULL;
    if (NULL == phdl) {
        return -1;
    }
    close_data2service_handle();
    pst_send_data_info = (SendDataInfo *)phdl;
    cq_log_err("m_data_tid(%d) m_send_data_seq=%d "
               "m_send_data_succ_num=%d "
               "m_send_data_failed_num=%d, "
               "m_recv_service_succ_num =%d "
               "m_recv_service_failed_num=%d"
               "m_recv_pend_num=%d",
               pst_send_data_info->m_data_tid,
               pst_send_data_info->m_send_data_seq,
               pst_send_data_info->m_send_data_succ_num,
               pst_send_data_info->m_send_data_failed_num,
               pst_send_data_info->m_recv_service_succ_num,
               pst_send_data_info->m_recv_service_failed_num,
               pst_send_data_info->m_recv_pend_num);

    CQUninit(pst_send_data_info->m_service2data_handle);
    pst_send_data_info->m_data2service_handle = NULL;
    pst_send_data_info->m_service2data_handle = NULL;
    pst_send_data_info->m_send_data_seq = 0;
    pst_send_data_info->m_send_data_succ_num= 0;
    pst_send_data_info->m_send_data_failed_num= 0;
    pst_send_data_info->m_recv_service_succ_num = 0;
    pst_send_data_info->m_recv_service_failed_num = 0;
    pst_send_data_info->m_recv_pend_num = 0;
    pst_send_data_info->m_data_tid= 0;
    pst_send_data_info->m_cmd= 0;
    free(phdl);

    return 0; 
}

