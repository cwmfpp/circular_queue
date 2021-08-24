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

#ifndef _CIRCULAR_QUEUE_H
#define _CIRCULAR_QUEUE_H

typedef struct _Element{
    void *m_pvData;
    int m_iDataLen;
}Element;


#define CQ_NONBLOCK     0 /* call CQDequeue immediately return for nodata*/
#define CQ_BLOCK        1 /*call CQDequeue block util recv data*/

typedef struct _CQAttr{
    int m_iElementCount;/*circular queue count*/
    int m_iMode;/* CQ_NONBLOCK CQ_BLOCK*/
}CQAttr;

void *CQInit(CQAttr *_pstCQAttr);
int CQEnqueue(void *_pCQHandle, Element *_pstElement);
int CQDequeue(void *_pCQHandle, Element *_pstElement);
int CQGetElementCount(void *_pCQHandle);
int CQGetMode(void *_pCQHandle);
int CQIsFull(void *_pCQHandle);
int CQIsEmpty(void *_pCQHandle);
int CQUninit(void *_pCQHandle);

#endif /*_CIRCULAR_QUEUE_H*/

