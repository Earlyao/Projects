#ifndef AES_H
#define AES_H

#include "typedefs.h"

/**
 * ���� p: ���ĵ��ַ������顣
 * ���� plen: ���ĵĳ���,���ȱ���Ϊ16�ı�����
 * ���� key: ��Կ���ַ������顣
 */
int aes(uint_8 *p, uint_16 plen, const uint_8 *key, uint_16 klen);

/**
 * ���� c: ���ĵ��ַ������顣
 * ���� clen: ���ĵĳ���,���ȱ���Ϊ16�ı�����
 * ���� key: ��Կ���ַ������顣
 */
int deAes(uint_8 *c, uint_16 clen, const uint_8 *key, uint_16 klen);

#endif