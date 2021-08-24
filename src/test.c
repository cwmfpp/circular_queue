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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "circular_queue.h"


#if 1
#define CQLog_Debug(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Warn(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Error(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#else

#define CQLog_Debug(fmt, ...) /**/
#define CQLog_Warn(fmt, ...) /**/
#define CQLog_Error(fmt, ...) /**/

#endif

int CQDemo(void);

static void *EnqueueThread(void *_pArg)
{
    Element stElement;
    int iElementCount = 0;
    
    if(NULL == _pArg)
    {
        CQLog_Error("invalid _pArg(%p)", _pArg);
        goto end;

    }

    iElementCount = iElementCount;
    
    while(1)
    {
        if(CQEnqueue(_pArg, &stElement) < 0)
        {
            CQLog_Error("call CQEnqueue fail");
        }
        
        iElementCount = CQGetElementCount(_pArg);        
        CQLog_Error("enqueue iElementCount(%d)", iElementCount);
        usleep(10 * 1000);
    }

    
end:
    return _pArg;
}

static void *DequeueThread(void *_pArg)
{
    Element stElement;
    int iElementCount = 0;

    if(NULL == _pArg)
    {
        CQLog_Error("invalid _pArg(%p)", _pArg);
        goto end;

    }

    iElementCount = iElementCount;

    for(;;)
    {
        if(CQDequeue(_pArg, &stElement) < 0)
        {
            CQLog_Error("call CQDequeue fail");
        }
        
        iElementCount = CQGetElementCount(_pArg);        
        CQLog_Error("dequeue iElementCount(%d)", iElementCount);
	if (CQ_NONBLOCK == CQGetMode(_pArg)) {
       	    usleep(10 * 1000);
	}
    }

end:
    return _pArg;
}

static int CQDemo1(void)
{
    int iRet = 0;
    void *pvHandle = NULL;
    pthread_t pTidEn = 0;
    pthread_t pTidDe = 0;
    CQAttr stCQAttr;

    stCQAttr.m_iElementCount = 5;
    stCQAttr.m_iMode = CQ_BLOCK;//CQ_NONBLOCK  CQ_BLOCK
    pvHandle = CQInit(&stCQAttr);
    
    if(0 != pthread_create(&pTidEn, NULL, EnqueueThread, pvHandle))
    {
        CQLog_Error("pthread_create EnqueueThread fail");
        iRet = -1;
        goto end;
    }

    if(0 != pthread_create(&pTidDe, NULL, DequeueThread, pvHandle))
    {
        CQLog_Error("pthread_create DequeueThread fail");
        iRet = -1;
        goto end;
    }


    (void)pthread_detach(pTidEn);
    (void)pthread_detach(pTidDe);

    for(;;)
    {
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
    int iElementCount = 0;
    
    CQAttr stCQAttr;

    stCQAttr.m_iElementCount = 5;
    stCQAttr.m_iMode = CQ_NONBLOCK;
    
    iElementCount = iElementCount;
    
    pvHandle = CQInit(&stCQAttr);
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
    
    iElementCount = CQGetElementCount(pvHandle);        
    CQLog_Error("enqueue iElementCount(%d)", iElementCount);

    for(;;)
    {
        if(CQDequeue(pvHandle, &stElement) < 0)
        {
            CQLog_Error("call CQDequeue fail");
            break;
        }
        iElementCount = CQGetElementCount(pvHandle);        
        CQLog_Error("dequeue iElementCount(%d)", iElementCount);
    }
    
    if(CQUninit(pvHandle) < 0)
    {
        CQLog_Error("call CQUninit fail");
    }
    pvHandle = NULL;

    return iRet;
}
#endif

int CQDemo(void)
{
    int iRet = 0;
    
    CQDemo1();
    //CQDemo2();
    
    return iRet;
}


int main(void)
{
	CQDemo();
	return 0;
}
