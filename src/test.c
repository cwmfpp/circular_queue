/*
 * Copyright (c) 2021 circular queue.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE      /* See feature_test_macros(7) */
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <unistd.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "circular_queue.h"
#include "data_service.h"


#if 1
#define CQLog_Debug(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Warn(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Error(fmt, x...)  fprintf(stderr, "%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#else

#define CQLog_Debug(fmt, ...) /**/
#define CQLog_Warn(fmt, ...) /**/
#define CQLog_Error(fmt, ...) /**/

#endif

static long get_tid_a(void)
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

int CQDemo(void);

static void *EnqueueThread(void *_pv_arg)
{
    Element stElement;

    if (NULL == _pv_arg) {
        CQLog_Error("invalid _pv_arg(%p)", _pv_arg);
        goto end;
    }

    while (1) {
        if (CQEnqueue(_pv_arg, &stElement) < 0) {
            CQLog_Error("call CQEnqueue fail");
        }

        usleep(10 * 1000);
    }

end:
    return _pv_arg;
}

static void *DequeueThread(void *_pv_arg)
{
    Element stElement;
    int ideq_count = 0;
    int tid;

    if (NULL == _pv_arg) {
        CQLog_Error("invalid _pv_arg(%p)", _pv_arg);
        goto end;
    }

    for (;;) {
        tid = (int)get_tid_a();
        CQLog_Error("============================= %d", (int)tid);
        if (CQDequeue(_pv_arg, &stElement) < 0) {
            CQLog_Error("call CQDequeue fail, tid(%d)", (int)tid);
        }

        ideq_count++;
        CQLog_Error("tid(%d) ideq_count=%d\n", (int)tid, ideq_count);
        usleep(40 * 1000);
        if (CQ_NONBLOCK == CQGetDeqMode(_pv_arg)) {
            usleep(10 * 1000);
        }
    }

end:
    return _pv_arg;
}

static int CQDemo1(void)
{
    int iRet = 0;
    void *pvHandle = NULL;
    pthread_t pTidEn = 0;
    pthread_t pTidDe_1 = 0;
    pthread_t pTidDe_2 = 0;
    pthread_t pTidDe_3 = 0;
    pthread_t pTidDe_4 = 0;
    pthread_t pTidDe_5 = 0;
    CQAttr st_cq_attr;

    st_cq_attr.m_iElementCount = 5;
    st_cq_attr.m_iModeDeq = CQ_BLOCK;//CQ_NONBLOCK  CQ_BLOCK
    st_cq_attr.m_iModeEnq = CQ_BLOCK;//CQ_NONBLOCK  CQ_BLOCK
    pvHandle = CQInit(&st_cq_attr);

    if (0 != pthread_create(&pTidEn, NULL, EnqueueThread, pvHandle)) {
        CQLog_Error("pthread_create EnqueueThread fail");
        iRet = -1;
        goto end;
    }

    if (0 != pthread_create(&pTidDe_1, NULL, DequeueThread, pvHandle)) {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }

    if (0 != pthread_create(&pTidDe_2, NULL, DequeueThread, pvHandle)) {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }

    if (0 != pthread_create(&pTidDe_3, NULL, DequeueThread, pvHandle)) {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }

    if (0 != pthread_create(&pTidDe_4, NULL, DequeueThread, pvHandle)) {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }

    if (0 != pthread_create(&pTidDe_5, NULL, DequeueThread, pvHandle)) {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }

    (void)pthread_detach(pTidEn);
    (void)pthread_detach(pTidDe_1);
    (void)pthread_detach(pTidDe_2);
    (void)pthread_detach(pTidDe_3);
    (void)pthread_detach(pTidDe_4);
    (void)pthread_detach(pTidDe_5);

    for (;;) {
        sleep(5);
    }

end:
    return iRet;
}

#if 0
static int CQDemo2(void)
{
    int iRet = 0;
    void *pvHandle = NULL;
    Element stElement;
    int elem_count = 0;
    
    CQAttr st_cq_attr;

    st_cq_attr.m_iElementCount = 5;
    st_cq_attr.m_iModeDeq = CQ_NONBLOCK;
    st_cq_attr.m_iModeEnq = CQ_NONBLOCK;
    
    elem_count = elem_count;
    
    pvHandle = CQInit(&st_cq_attr);
    if(CQEnqueue(pvHandle, &stElement) < 0)
    {
        CQLog_Error("call CQEnqueue fail");
    }
    if(CQEnqueue(pvHandle, &stElement) < 0)
    {
        CQLog_Error("call CQEnqueue fail");
    }
    if(CQEnqueue(pvHandle, &stElement) < 0)
    {
        CQLog_Error("call CQEnqueue fail");
    }
    if(CQEnqueue(pvHandle, &stElement) < 0)
    {
        CQLog_Error("call CQEnqueue fail");
    }
    if(CQEnqueue(pvHandle, &stElement) < 0)
    {
        CQLog_Error("call CQEnqueue fail");
    }
    
    elem_count = CQGetElementCount(pvHandle);        
    CQLog_Error("enqueue elem_count(%d)", elem_count);

    for(;;)
    {
        if(CQDequeue(pvHandle, &stElement) < 0)
        {
            CQLog_Error("call CQDequeue fail");
            break;
        }
        elem_count = CQGetElementCount(pvHandle);        
        CQLog_Error("dequeue elem_count(%d)", elem_count);
    }
    
    if(CQUninit(pvHandle) < 0)
    {
        CQLog_Error("call CQUninit fail");
    }
    pvHandle = NULL;

    return iRet;
}
#endif

#if 1

typedef struct SendDataThread {
    pthread_t m_send_data_tid;
    void *m_data2service_create; /* send data to service thread */
} SendDataThread;

typedef struct DataPayload{
    void *m_req_data;
    int m_req_len;
    void *m_resq_data;
    int m_resq_len;
    int m_tid;
} DataPayload;

#define SEND_RECV_THREAD_NUM 90

#define fifo_log_deb(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define fifo_log_warn(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define fifo_log_err(fmt, x...)  fprintf(stderr, "%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);

static void *send_data_recv_service_thread(void *_pv_arg)
{
    void *pst_send_data_handle = NULL;
    int data_tid = 0;
    int recv_data_cmd = 0;
    int send_num = 0;

    // service data
    if (NULL == _pv_arg) {
        fifo_log_err("invalid _pv_arg(%p)", _pv_arg);
        return NULL;
    }
    pst_send_data_handle = (void *)_pv_arg;
    data_tid = (int)get_tid();

    while (1) {
#if 1
        if (send_num == 100) {
            send_data_to_service(pst_send_data_handle, data_tid, TRANS_DATA_END,
                                 NULL, 0, NULL);
            while (TRANS_DATA_END !=
                   recv_data_from_service(pst_send_data_handle, NULL, 0, NULL,
                                          1)) {
                usleep(20 * 1000);
                fifo_log_err("data_tid(%d) recv_data_cmd=%d", (int)data_tid,
                             recv_data_cmd);
            }
            return NULL;
        } else {
            DataPayload stDataPayload;
            char *reqdata_arr = (char *)malloc(4096);
            stDataPayload.m_req_data = reqdata_arr;
            stDataPayload.m_req_len = 4096; 
            stDataPayload.m_resq_data = (char *)malloc(1024);
            stDataPayload.m_resq_len = 1024;
            stDataPayload.m_tid = data_tid;
            //fifo_log_err("tid(%d) malloc req data %p", data_tid, stDataPayload.m_req_data);

            send_data_to_service(pst_send_data_handle, data_tid, TRANS_DATA,
                                 &stDataPayload, sizeof(stDataPayload), NULL);
            send_num++;
        }
#else
        if (0 == access("/home/chenwenmin03/hello", F_OK)) {
            send_data_to_service(pst_send_data_handle, data_tid,
                                TRANS_DATA_END, NULL, 0, NULL);
            while (TRANS_DATA_END !=
                   recv_data_from_service(pst_send_data_handle, NULL, 0,
                                         NULL, 1)) {
                usleep(20 * 1000);
                fifo_log_err("data_tid(%d) recv_data_cmd=%d", (int)data_tid, recv_data_cmd);
            }
            return NULL;
        } else {
            send_data_to_service(pst_send_data_handle, data_tid, TRANS_DATA,
                                 NULL, 0, NULL);
        }
#endif

        // recv service data, suggest other thread for nonblock
        DataPayload stDataPayload = {NULL, 0, NULL, 0, 0};
        recv_data_cmd = recv_data_from_service(pst_send_data_handle, &stDataPayload, sizeof(stDataPayload), NULL, 0);
        if (TRANS_DATA_END == recv_data_cmd) {
            fifo_log_err("data_tid(%d) recv_data_cmd=%d data end", (int)data_tid, recv_data_cmd);
            break;
        } else if(TRANS_DATA_EAGAIN == recv_data_cmd) {
            fifo_log_err("data_tid(%d) recv_data_cmd=%d data again", (int)data_tid, recv_data_cmd);
            continue;
        }
        if (NULL != stDataPayload.m_req_data) {
            //fifo_log_err("tid(%d) free req data %p", stDataPayload.m_tid, stDataPayload.m_req_data);
            free(stDataPayload.m_req_data);
            stDataPayload.m_req_data = NULL;
        }
        if (NULL != stDataPayload.m_resq_data) {
            //fifo_log_err("tid(%d) free req data %p", stDataPayload.m_tid, stDataPayload.m_req_data);
            free(stDataPayload.m_resq_data);
            stDataPayload.m_resq_data = NULL;
        }
    }

    return NULL;
}

static int data_service_callback_func(void *data_addr, int data_len, void *priv)
{
    //int tid = (int)get_tid();
#if 0
    DataPayload *pstDataPayload = NULL;

    pstDataPayload = (DataPayload *)data_addr;
#endif
    //fifo_log_err("tid(%d) data_len=%d", (int)tid, data_len);

    return 0;
}

static int m_enqueue_2_n_dequeue_demo(void)
{
    int iRet = 0;
    SendDataThread pst_send_data_thread[SEND_RECV_THREAD_NUM];

    void *res = NULL;
    int i = 0;
    int s = 0;

    for (i = 0; i < SEND_RECV_THREAD_NUM; i++) {
        pst_send_data_thread[i].m_data2service_create = create_data_service(data_service_callback_func, 1);
        if (0 != pthread_create(&pst_send_data_thread[i].m_send_data_tid, NULL,
                                send_data_recv_service_thread,
                                pst_send_data_thread[i].m_data2service_create)) {
            fifo_log_err("pthread_create DequeueThread fail");
            int j = 0;
            for (j = 0; j < i; j++) {
                destroy_data_service((void *)(pst_send_data_thread[j].m_data2service_create));
                pst_send_data_thread[i].m_data2service_create = NULL;
            }
        }
    }

    for (i = 0; i < SEND_RECV_THREAD_NUM; i++) {
        s = pthread_join(pst_send_data_thread[i].m_send_data_tid, &res);
        if (0 != s) {
            printf("pthread_join error\n");
        }
        fifo_log_err("send recv tid[%d] exit\n", i);
        if (NULL != res) {
            fifo_log_err("returned value was %s\n", (char *)res);
            free(res); /* Free memory allocated by thread */
        }
        destroy_data_service(
            (void *)(pst_send_data_thread[i].m_data2service_create));
        pst_send_data_thread[i].m_data2service_create = NULL;
    }

    return iRet;
}
#endif

int CQDemo(void)
{
    int iRet = 0;
    
    m_enqueue_2_n_dequeue_demo();
    return 0;
    CQDemo1();
    // CQDemo2();

    return iRet;
}


int main(void)
{
	CQDemo();
	return 0;
}
