#ifndef CRYPT_UTIL_H
#define CRYPT_UTIL_H

#include "Arduino.h"

typedef unsigned long MD5_u32plus;

typedef struct {
  MD5_u32plus   lo, hi;
  MD5_u32plus   a, b, c, d;
  unsigned char buffer[64];
  MD5_u32plus   block[16];
} MD5_CTX;

class CryptUtil {
public:

  CryptUtil();
  static String         rc4Paradox(String text,
                                   String key);
  static String         md5SumHex(String arg);

  // static String         md5SumHex(const char *arg);
  static unsigned char* md5Sum(const char *arg);
  static unsigned char* md5Sum(const char *arg,
                               size_t      size);
  static char         * makeDigest(const unsigned char *digest,
                                   int                  len);
  static const void   * body(void       *ctxBuf,
                             const void *data,
                             size_t      size);
  static void           MD5Init(void *ctxBuf);
  static void           MD5Final(unsigned char *result,
                                 void          *ctxBuf);
  static void           MD5Update(void       *ctxBuf,
                                  const void *data,
                                  size_t      size);
};

#endif // ifndef CRYPT_UTIL_H
