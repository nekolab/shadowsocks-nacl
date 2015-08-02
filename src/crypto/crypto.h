/*
 * Copyright (C) 2015  Sunny <ratsunny@gmail.com>
 *
 * This file is part of Shadowsocks-NaCl.
 *
 * Shadowsocks-NaCl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Shadowsocks-NaCl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SS_CRYPTO_H_
#define _SS_CRYPTO_H_

#include <map>
#include <vector>
#include <string>
#include <openssl/evp.h>
#include <sodium.h>

class Crypto {
  public:
    enum class Library { OPENSSL, SODIUM };
    enum class OpCode  { DECRYPTION, ENCRYPTION };
    enum class Cipher  { BF_CFB, RC2_CFB, RC4_MD5,
                         IDEA_CFB, SEED_CFB, CAST5_CFB,
                         AES_128_CFB, AES_192_CFB, AES_256_CFB,
                         AES_128_OFB, AES_192_OFB, AES_256_OFB,
                         AES_128_CTR, AES_192_CTR, AES_256_CTR,
                         AES_128_CFB1, AES_192_CFB1, AES_256_CFB1,
                         AES_128_CFB8, AES_192_CFB8, AES_256_CFB8,
                         CAMELLIA_128_CFB, CAMELLIA_192_CFB,
                         CAMELLIA_256_CFB, SALSA20, CHACHA20 };

    typedef const EVP_CIPHER *(*OpenSSLCipher)(void);
    typedef int (*SodiumCipher)(unsigned char*, const unsigned char*,
                                unsigned long long,
                                const unsigned char*, uint64_t,
                                const unsigned char*);

    typedef struct {
      int iv_size;
      int key_size;
      Library library;
      union {
        SodiumCipher sodium_cipher;
        OpenSSLCipher openssl_cipher;
      };
    } CipherInfo;

    virtual ~Crypto() {}

    static const Cipher *GetCipher(std::string name);
    static const CipherInfo *GetCipherInfo(Cipher cipher);

    virtual void Update(std::vector<uint8_t> &out,
                        const std::vector<uint8_t> &in) = 0;

  private:
    static const std::map<Cipher, CipherInfo> cipher_details_;
    static const std::map<std::string, Cipher> supported_cipher_;
};

#endif
