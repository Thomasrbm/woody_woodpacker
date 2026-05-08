#include "woody.h"

void xtea_encrypt_block(uint32_t key_crypt_block[2], const uint32_t key[4])
{
    uint32_t kcb0 = key_crypt_block[0];
    uint32_t kcb1 = key_crypt_block[1];
    uint32_t sum = 0;
    const uint32_t delta = 2654435769;
    
    for (int i = 0; i < 32; i++) 
    {
        kcb0 += (((kcb1 << 4) ^ (kcb1 >> 5)) + kcb1) ^ (sum + key[sum & 3]);
        sum += delta;
        kcb1 += (((kcb0 << 4) ^ (kcb0 >> 5)) + kcb0) ^ (sum + key[(sum >> 11) & 3]);
    }
    
    key_crypt_block[0] = kcb0;
    key_crypt_block[1] = kcb1;
}

void xtea_iter(uint8_t *data, size_t len, const uint32_t key[4])
{
    uint64_t counter = 0;
    
    for (size_t i = 0; i < len; i += 8)
    {
        uint32_t key_crypt_block[2];
        // 32 bit haut et bas
        key_crypt_block[0] = (uint32_t)(counter & 0xFFFFFFFF);
        key_crypt_block[1] = (uint32_t)(counter >> 32);
        
        xtea_encrypt_block(key_crypt_block, key);
        
        size_t remaining;
        
        if (len - i < 8)
            remaining = len - i;
        else
            remaining = 8;

        // xor la data avec le block de de cryptage
        for (size_t j = 0; j < remaining; j++) 
            data[i + j] ^= ((uint8_t *)key_crypt_block)[j];
        
        counter++;
    }
}