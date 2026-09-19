#include "ring_buf.h"
#include <stdio.h>
#include <string.h>

void RingBuf_Init(RingBuf_t *rb,uint8_t *buf,size_t buf_size)
{
    if(rb == NULL || buf == NULL || buf_size ==0U)
    {
        return;
    }
    rb->buf = buf;
    rb->size = buf_size;
    rb->head = 0U;
    rb->tail = 0U;
    rb->count = 0U;
    rb->overflow_warn_flag = 0U;
}

int RingBuf_WriteByte(RingBuf_t *rb,uint8_t ch)
{
    if(rb == NULL || rb->buf == NULL)
    {
        return 1;
    }
    if(rb->count == rb->size)
    {
        if(rb->overflow_warn_flag == 0U)
        {
            fprintf(stderr,"[CRITICAL WARN] ring_buf overflow! drop incoming bytes.\n");
            rb->overflow_warn_flag = 1U;
        }
        return 1;
    }
    rb->buf[rb->head] = ch;
    rb->head = (rb->head + 1U) % rb->size;
    rb->count++;
    return 0;
}

int RingBuf_ReadByte(RingBuf_t *rb,uint8_t *ch)
{
    if(rb == NULL || rb->buf == NULL || ch ==NULL)
    {
        return 1;
    }
    if(rb->count == 0)
    {
        return 1;
    }
    *ch = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1U) % rb->size;
    rb->count--;
    return 0;
}

size_t RingBuf_GetCount(RingBuf_t *rb)
{
    if(rb == NULL)
    {
        return 0U;
    }
    return rb->count;
}

void RingBuf_Clear(RingBuf_t *rb)
{
    if(rb == NULL)
    {
        return;
    }
    rb->head = 0U;
    rb->tail = 0U;
    rb->overflow_warn_flag = 0U;
    rb->count = 0U;
}