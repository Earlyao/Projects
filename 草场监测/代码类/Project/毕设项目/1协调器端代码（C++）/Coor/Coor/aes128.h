#ifndef AES_H
#define AES_H

#include "typedefs.h"

/**
 * 参数 p: 明文的字符串数组。
 * 参数 plen: 明文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
int aes(uint_8 *p, uint_16 plen, const uint_8 *key, uint_16 klen);

/**
 * 参数 c: 密文的字符串数组。
 * 参数 clen: 密文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
int deAes(uint_8 *c, uint_16 clen, const uint_8 *key, uint_16 klen);

#endif