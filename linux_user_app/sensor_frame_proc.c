#include "sensor_frame_proc.h"
#include "csv_log.h"
#include "sensor_frame_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int parse_sensor_kv_payload(const char *raw_payload,SensorFrame_t *out_data)
{
    if(raw_payload == NULL || out_data == NULL)
    {
        return -1;
    }
    memset(out_data,0x00,sizeof(SensorFrame_t));
    char work_copy[PARSE_WORK_BUF_LEN];
    //strncpy原理:
    /*
        char *strncpy(char *dest,const char *src,size_t n);
            从src逐个字节复制到dest有以下两个分支
                复制过程中,复制n个字节前遇到'\0',停止复制且剩下的全部补0
                复制了n个字节依旧没有'\0',此时末尾不会追加'\0'
            返回值永远是dest首地址
    */
    strncpy(work_copy,raw_payload,sizeof(work_copy)-1U);
    work_copy[sizeof(work_copy)-1U] = '\0';
    //strtok_r原理:
    /*
        char *strtok_r(char *str,const char *delim,char **saveptr);
            str--第一次调用传入待分割字符串首地址,后续循环调用时填NULL
            delim--分隔符集合,任意字符都可,可以写多个如",\r\n".分割符会被改写为'\0'.
                连续的分隔符自动跳过,如"a,,b"-->"a"和"b",不会得到空字符串
            save_ptr--一般自己定义char *ctx类型,传入&ctx,函数内部用它保存分割完后剩下字符串的起始位置,函数
                外部由自己保管这个状态.同一条分割流程里循环调用时,不要修改ctx的值
        返回当前取出片段的首地址,分割完成的返回NULL
    */
    char *save_ptr = NULL;
    char *token = strtok_r(work_copy," ",&save_ptr);
    int parse_ok_cnt = 0;
   //strchr原理:
   /*
        char *strchr(const char *s,int c);
            在字符串s里从前往后查找第一个等于c的字符,c虽然传int,但只会取第一字节当做unsigned char对比
        返回该等于字符c的地址,找不到返回NULL
   */
  //样例T:25.1 L:1024 AX:120 AY:211 AZ:1630 GX:-11 GY:22 GZ:-33
    while(token != NULL)
    {
        char *colon_ptr = strchr(token,':');
        if(colon_ptr == NULL)
        {
            token = strtok_r(NULL," ",&save_ptr);
            continue;
        }
        *colon_ptr = '\0';
        const char *key = token;
        const char *val_str = colon_ptr + 1U;
    //atof原理:
    /*
        double atof(const char *nptr);
            跳过前置空白符,识别正负号,解析整除部分数字,如果遇到小数段则解析小数段
                支持识别科学技术如1.2E3,2.5e-2
            碰到不属于浮点数合法字符时停止,返回double
        (缺陷:)
            溢出和下溢没有错误标记
            无法区分出错和输入0.0
    */
    //atoi原理:
    /*
        int atoi(const char *nptr);
            跳过所有空白字符,识别正负号,往后读取数字字符,直到遇到非数字字符.
            返回int型整数
        (缺陷:)
            超出int所能表示的范围时,不报错,返回值乱掉
            无法区分出错和输出0
    */
        if(strcmp(key,"T") == 0)
        {
            out_data->temp = atof(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"L") == 0)
        {
            out_data->light = (uint16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AX") == 0)
        {
            out_data->ax = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AY") == 0)
        {
            out_data->ay = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AZ") == 0)
        {
            out_data->az = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GX") == 0)
        {
            out_data->gx = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GY") == 0)
        {
            out_data->gy = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GZ") == 0)
        {
            out_data->gz = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        token = strtok_r(NULL," ",&save_ptr);
    }
    if(parse_ok_cnt == 8)
    {
        out_data->valid = 1U;
        return 0;
    }
    fprintf(stderr,"[WARN] KV parse incomplete, valid field count=%d, drop frame\n",parse_ok_cnt);
    return -1;
}

int sensor_frame_process(const char *raw_payload,int csv_fd)
{
    SensorFrame_t final_data_frame;
    int kv_ret = parse_sensor_kv_payload(raw_payload, &final_data_frame);
    if(kv_ret != 0)
    {
        fprintf(stderr,"[WARN] KV parse fail ,drop this sensor frame\n");
        return -1;
    }
    fprintf(stderr,"[PARSED] T=%.1f L=%d AX=%d AY=%d AZ=%d GX=%d GY=%d GZ=%d\n",
        final_data_frame.temp,
        final_data_frame.light,
        final_data_frame.ax,
        final_data_frame.ay,
        final_data_frame.az,
        final_data_frame.gx,
        final_data_frame.gy,
        final_data_frame.gz
    );
    //tv.tv_sec:Unix时间戳,从1970-01-01 UTC到现在一共经过多少秒
    //tv.tv_usec:微妙,1s=1000000微妙
    struct timeval tv;
    gettimeofday(&tv,NULL);
    double ts = tv.tv_sec + tv.tv_usec / 1000000.0;
    int csv_ret = csv_log_write_frame(csv_fd, &final_data_frame, ts);
    if(csv_ret != 0)
    {
        fprintf(stderr,"[FATAL] csv log write fsync errno!\n");
        return -1;
    }
    return 0;
}
