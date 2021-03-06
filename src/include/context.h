#ifndef __HFM_CONTEXT_H__
#define __HFM_CONTEXT_H__

#include <libvmi/libvmi.h>
#include "private.h"

/**
  * @brief Read an address at a virtual addr
  * @param ctx context
  * @param addr virtual address
  * @return address, return 0 if operation fail
  */
addr_t hfm_read_addr(context_t *ctx, addr_t addr);

/**
  * @brief Read an 64 bit integer at a virtual addr
  * @param ctx context
  * @param addr virtual address
  */
uint64_t hfm_read_64(context_t *ctx, addr_t addr);

/**
  * @brief Read an 32 bit integer at a virtual addr
  * @param ctx context
  * @param addr virtual address
  */
uint32_t hfm_read_32(context_t *ctx, addr_t addr);

/**
  * @brief Read an 16 bit integer at a virtual addr
  * @param ctx context
  * @param addr virtual address
  */
uint16_t hfm_read_16(context_t *ctx, addr_t addr);

/**
  * @brief Read an 8 bit integer at a virtual addr
  * @param ctx context
  * @param addr virtual address
  */
uint8_t hfm_read_8(context_t *ctx, addr_t addr);

/**
  * @brief Read buffer at a virtual addr
  * @param ctx context
  * @param addr virtual address
  * @param buf Buffer address
  * @param count Number of bytes to read
  * @return Number of bytes read
  */
size_t hfm_read(context_t *ctx, addr_t addr, void *buf, size_t count);

/**
  * @brief Get current process address
  * @param ctx context
  * @return Address of process
  */
addr_t hfm_get_current_process(context_t *ctx);

/**
  * @brief Get file object address from handle
  * @param ctx context
  * @param handle File handle
  */
addr_t hfm_fileobj_from_handle(context_t *ctx, reg_t handle);

/**
  * @brief Read filename of File object from file handler
  * @param ctx context
  * @param object File object address
  * @param filename Address of buffer to write filename to
  * @return Lengh of filename read
  */
int hfm_read_filename_from_object(context_t *ctx, addr_t object, char *filename);

/**
  * @brief Extract file from the file object address
  * @param ctx context
  * @param object File object address
  * @param path Path
  * @return Number of file extracted
  */
int hfm_extract_file(context_t *ctx, addr_t object, char *path);

/**
  * @brief Read and convert a unicode string at address
  * @param ctx context
  * @param addr Address of unicode string
  * @param buffer Buffer to write the result to
  * @return Lengh of unicode string read
  */
int hfm_read_unicode(context_t *ctx, addr_t addr, char *buffer);

/**
  * @brief Get process pid
  * @param ctx context
  * @param process_addr Process address
  * @return PID of process if operation succeeds, return -1 if fails
  */
vmi_pid_t hfm_get_process_pid(context_t *ctx, addr_t process_addr);

/**
  * @brief Get process owner's SID
  * @param ctx context
  * @param process_addr Process address
  * @param out Buffer to store the result
  * 
  */
void hfm_get_process_sid(context_t *ctx, addr_t process_addr, char *out);

/**
  * Extract SID
  * @param[in] context Context
  * @param[in] sid_addr SID address
  * @param[out] sid SID text
  * Reference : https://github.com/libyal/libfwnt/wiki/Security-Descriptor
  */
void hfm_extract_sid(context_t *context, addr_t sid_addr, char *sid);

#endif
