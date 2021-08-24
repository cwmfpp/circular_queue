  /*
   * Copyright (c) 2021 circular queue.
   * Licensed under the Apache License, Version 2.0 (the "License");
   * you may not use this file except in compliance with the License.
   * You may obtain a copy of the License at
   *
   *	 http://www.apache.org/licenses/LICENSE-2.0
   *
   * Unless required by applicable law or agreed to in writing, software
   * distributed under the License is distributed on an "AS IS" BASIS,
   * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   * See the License for the specific language governing permissions and
   * limitations under the License.
   */



#include "circular_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct _CQueue{
    int m_iBlock;/*  0, 1 */
    pthread_mutex_t m_stMutex;
    pthread_cond_t m_stCond;
    int m_iElementCount;
    int m_iFront;/* read */
    int m_iRear;/* write */
    Element *m_pstElement;
}CQueue;

#if 0
#define CQLog_Debug(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Warn(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define CQLog_Error(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#else

#define CQLog_Debug(fmt, ...)
#define CQLog_Warn(fmt, ...)
#define CQLog_Error(fmt, ...)

#endif

static int _ElementInit(Element *_pstElement)
{
    int iRet = 0;

    if(NULL == _pstElement)
    {
        CQLog_Error("invalid _pstElement(%p)", _pstElement);
        iRet = -1;
        goto end;
    }

    _pstElement->m_pvData = NULL;
    _pstElement->m_iDataLen = 0;
    
end:
    return iRet;
}

static void *_CQInit(CQAttr *_pstCQAttr)
{
    int iRet = 0;
    void *pH = NULL;
    CQueue *pstCQueue = NULL;
    Element *pstElement = NULL;
    Element *pstElementTmp = NULL;
    int i = 0;
    int iElementNum = 0;
    if(NULL == _pstCQAttr)
    {
        CQLog_Error("invalid _pstCQAttr(%p)", _pstCQAttr);
        iRet = -1;
        goto end;
    }
    iElementNum = _pstCQAttr->m_iElementCount;
    
    if(iElementNum < 3)
    {
        CQLog_Error("invalid iElementNum(%d) < 3", iElementNum);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)malloc(sizeof(CQueue));
    if(NULL == pstCQueue)
    {
        CQLog_Error("call malloc failed for CQueue");
        iRet = -1;
        goto end;
    }
    pstElement = (Element *)malloc(sizeof(Element) * (size_t)iElementNum);
    if(NULL == pstElement)
    {
        CQLog_Error("call malloc failed for Element");
        iRet = -1;
        goto end;
    }
    pstElementTmp = pstElement;
    for(i = 0; i < iElementNum; i++)
    {
        if(_ElementInit(pstElementTmp) < 0)
        {
            CQLog_Error("call _ElementInit failed");
            iRet = -1;
            goto end;
        }
        pstElementTmp++;
    }
    pstCQueue->m_iBlock = _pstCQAttr->m_iMode;
    if(pthread_mutex_init(&pstCQueue->m_stMutex, NULL) != 0)
    {
        CQLog_Error("call malloc failed for CQueue");
        iRet = -1;
        goto end;
    }
    if(CQ_BLOCK == pstCQueue->m_iBlock)
    {
        if(pthread_cond_init(&pstCQueue->m_stCond, NULL) != 0)
        {
            CQLog_Error("call malloc failed for CQueue");
            iRet = -1;
            goto end;
        }
    }
    pstCQueue->m_iElementCount = iElementNum;
    pstCQueue->m_iFront = 0;
    pstCQueue->m_iRear = pstCQueue->m_iFront;
    pstCQueue->m_pstElement = pstElement;
    pH = pstCQueue;
    
    CQLog_Debug("successful");
    
end:
    if(iRet < 0)
    {                
        if(NULL != pstElement)
        {
            free(pstElement);
            pstElement = NULL;
        }
        
        if(NULL != pstCQueue)
        {
            free(pstCQueue);
            pstCQueue = NULL;
        }
    }
    
    return pH;
}


void *CQInit(CQAttr *_pstCQAttr)
{
    return _CQInit(_pstCQAttr);
}

static int _CQIsFull(CQueue *_pstCQueue)
{    
    int iRet = 0;
    if(NULL == _pstCQueue)
    {
        CQLog_Error("invalid _pstCQueue(%p)", _pstCQueue);
        iRet = -1;
        goto end;
    }
    iRet = ((_pstCQueue->m_iRear + 1) % _pstCQueue->m_iElementCount) == _pstCQueue->m_iFront;
end:
    return iRet;
}

static int _CQIsEmpty(CQueue *_pstCQueue)
{    
    int iRet = 0;
    if(NULL == _pstCQueue)
    {
        CQLog_Error("invalid _pstCQueue(%p)", _pstCQueue);
        iRet = -1;
        goto end;
    }
    iRet = (_pstCQueue->m_iRear == _pstCQueue->m_iFront);
    
    CQLog_Error("_pstCQueue->m_iRear(%d)", _pstCQueue->m_iRear);
    CQLog_Error("_pstCQueue->m_iFront(%d)", _pstCQueue->m_iFront);
    CQLog_Error("iRet(%d)", iRet);

end:
    return iRet;
}

static int _CQEnqueue(void *_pCQHandle, Element *_pstElement)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    if(NULL == _pCQHandle && NULL == _pstElement)
    {
        CQLog_Error("invalid _pCQHandle(%p) _pstElement(%p)", _pCQHandle, _pstElement);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;

    if(1 == _CQIsFull(pstCQueue))
    {
        CQLog_Error("CQueuq is full");
        iRet = -1;
        goto end;
    }

    pstCQueue->m_pstElement[pstCQueue->m_iRear] = *_pstElement;
    pstCQueue->m_iRear = (pstCQueue->m_iRear + 1) % pstCQueue->m_iElementCount;
    
end:
    return iRet;
}


int CQEnqueue(void *_pCQHandle, Element *_pstElement)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle && NULL == _pstElement)
    {
        CQLog_Error("invalid _pCQHandle(%p) _pstElement(%p)", _pCQHandle, _pstElement);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;
    
    pthread_mutex_lock(&pstCQueue->m_stMutex);
    if(_CQEnqueue(_pCQHandle, _pstElement) < 0)
    {
        CQLog_Error("call _CQEnqueue failed");
        pthread_mutex_unlock(&pstCQueue->m_stMutex);
        iRet = -1;
        goto end;

    }
    
    if(CQ_BLOCK == pstCQueue->m_iBlock)
    {
        pthread_cond_signal(&pstCQueue->m_stCond);
    }
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    
end:
    return iRet;
}


static int _CQDequeue(void *_pCQHandle, Element *_pstElement)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle && NULL == _pstElement)
    {
        CQLog_Error("invalid _pCQHandle(%p) _pstElement(%p)", _pCQHandle, _pstElement);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;

    if(1 == _CQIsEmpty(pstCQueue))
    {
        CQLog_Error("CQueuq is empty");
        iRet = -1;
        goto end;
    }

    *_pstElement = pstCQueue->m_pstElement[pstCQueue->m_iFront];
    pstCQueue->m_iFront = (pstCQueue->m_iFront + 1) % pstCQueue->m_iElementCount;
    
end:
    return iRet;
}

int CQDequeue(void *_pCQHandle, Element *_pstElement)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle && NULL == _pstElement)
    {
        CQLog_Error("invalid _pCQHandle(%p) _pstElement(%p)", _pCQHandle, _pstElement);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;
    
    pthread_mutex_lock(&pstCQueue->m_stMutex);
    if(_CQDequeue(_pCQHandle, _pstElement) < 0)
    {
        if(CQ_BLOCK == pstCQueue->m_iBlock)
        {
            CQLog_Error("call _CQDequeue failed, cond wait");
            pthread_cond_wait(&pstCQueue->m_stCond, &pstCQueue->m_stMutex);
            if(_CQDequeue(_pCQHandle, _pstElement) < 0)
            {
                pthread_mutex_unlock(&pstCQueue->m_stMutex);
                iRet = -1;
                goto end;
            }
        }else
        {
            CQLog_Error("call _CQDequeue failed");
            pthread_mutex_unlock(&pstCQueue->m_stMutex);
            iRet = -1;
            goto end;
        }
    }
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    
end:
    return iRet;
}


static int _CQGetElementCount(CQueue *_pstCQueue)
{    
    int iRet = 0;

    if(NULL == _pstCQueue)
    {
        CQLog_Error("invalid _pstCQueue(%p)", _pstCQueue);
        iRet = -1;
        goto end;
    }

    iRet = (_pstCQueue->m_iElementCount + _pstCQueue->m_iRear - _pstCQueue->m_iFront) % _pstCQueue->m_iElementCount;

end:
    return iRet;
}

int CQGetElementCount(void *_pCQHandle)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle)
    {
        CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;

    pthread_mutex_lock(&pstCQueue->m_stMutex);
    iRet = _CQGetElementCount(pstCQueue);
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    
end:
    return iRet;
}

int CQGetMode(void *_pCQHandle)
{
	int iRet = 0;
	CQueue *pstCQueue = NULL;
	
	if(NULL == _pCQHandle)
	{
		CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
		iRet = -1;
		goto end;
	}
	pstCQueue = (CQueue *)_pCQHandle;

	iRet = pstCQueue->m_iBlock;
	
end:
	return iRet;
}

int CQIsFull(void *_pCQHandle)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle)
    {
        CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;

    pthread_mutex_lock(&pstCQueue->m_stMutex);
    iRet = _CQIsFull(pstCQueue);
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    
end:
    return iRet;
}


int CQIsEmpty(void *_pCQHandle)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle)
    {
        CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;

    pthread_mutex_lock(&pstCQueue->m_stMutex);
    iRet = _CQIsEmpty(pstCQueue);
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    
end:
    return iRet;
}

static int _CQUninit(void *_pCQHandle)
{
    int iRet = 0;
    CQueue *pstCQueue = NULL;
    
    if(NULL == _pCQHandle)
    {
        CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
        iRet = -1;
        goto end;
    }
    pstCQueue = (CQueue *)_pCQHandle;
    
    pthread_mutex_lock(&pstCQueue->m_stMutex);
    pstCQueue->m_iFront = 0;
    pstCQueue->m_iRear = 0;
    pstCQueue->m_iElementCount = 0;
    free((void *)(pstCQueue->m_pstElement));
    pstCQueue->m_pstElement = NULL;
    pthread_mutex_unlock(&pstCQueue->m_stMutex);
    pthread_mutex_destroy(&pstCQueue->m_stMutex);
    free(_pCQHandle);
    _pCQHandle = NULL;
    
    CQLog_Debug("free successful");
end:
    return iRet;
}

int CQUninit(void *_pCQHandle)
{
    int iRet = 0;
    
    if(NULL == _pCQHandle)
    {
        CQLog_Error("invalid _pCQHandle(%p)", _pCQHandle);
        iRet = -1;
        goto end;
    }
    
    if(_CQUninit(_pCQHandle) < 0)
    {
        CQLog_Error("call _CQUninit failed");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}

