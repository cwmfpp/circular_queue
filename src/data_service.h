#ifndef DATA_SERVICE_H
#define DATA_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRANS_DATA = 1,        /* data --> service*/
    TRANS_DATA_END,        /* data --> service, end data*/
    TRANS_DATA_EAGAIN,        /* not service data, again*/
} TRANS_CMD;

typedef int (*data_service_callback)(void *data_addr, int data_len, void *priv);

void *create_data_service(data_service_callback call_func, int recv_block);
int send_data_to_service(void *phdl, int data_tid, int data_cmd, void *data_addr, int data_len, void *priv);
int recv_data_from_service(void *phdl, void *data_addr, int data_len, void **ppriv, int flush_data);
int destroy_data_service(void *phdl);
long get_tid(void);

#ifdef __cplusplus
}
#endif

#endif /* DATA_SERVICE_H */
