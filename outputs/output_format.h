/**
  * @file output_format.h
  * @author phuong.do
  * @date 2017-05-24
  * @brief Abstract class for the outputs
  */
#ifndef __HFM_OUTPUT_FORMAT_H__
#define __HFM_OUTPUT_FORMAT_H__

#include "private.h"

typedef enum {
    OUT_CONSOLE,
    OUT_CSV,
    OUT_ELASTICSEARCH
} output_type_t;

typedef struct _output_info {
    char time[PATH_MAX_LEN];
    int pid;
    int vmid;
    int policy_id;
    monitor_t action;
    char filepath[PATH_MAX_LEN];
    char extpath[PATH_MAX_LEN];
} output_info_t;

typedef struct _output {
    output_type_t type;
    void (*writefc)(struct _output *output, output_info_t *info);
    void (*closefc)(struct _output *output);
    FILE *fp;
} output_t;

/**
  * @brief Init output module
  *
  * @param type output type
  * @return Pointer to output_t
  */
output_t *out_init(output_type_t type, ...);

/**
  * @brief Write a record to output
  *
  * @param out Pointer to output
  * @param info Struct contains the information to log
  */
void out_write(output_t *out, output_info_t *info);

/**
  * @brief Close and release resources
  * @param out Output handler
  */
void out_close(output_t *out);
char *action_tostr(monitor_t type);

#endif
