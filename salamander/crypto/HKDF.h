/*
Copyright 2016 Silent Circle, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef HKDF_H
#define HKDF_H

/**
 * @file HKDF.h
 * @brief Implementation of HKDF
 * 
 * Refer to RFC 5869 for detailed description of this KDF.
 * 
 * @ingroup Salamander++
 * @{
 */

#include <stdint.h>
#include <stddef.h>

namespace salamander {
class HKDF
{
public:
    HKDF();
    ~HKDF();

    static void deriveSecrets(uint8_t* inputKeyMaterial, size_t ikmLength, 
                              uint8_t* info, size_t infoLength, 
                              uint8_t* output, size_t outputLength);

    static void deriveSecrets(uint8_t* inputKeyMaterial, size_t ikmLength, 
                              uint8_t* salt, size_t saltLen, 
                              uint8_t* info, size_t infoLength, 
                              uint8_t* output, size_t outputLength);

private:
    static const int HASH_OUTPUT_SIZE  = 32;
    static const int OFFSET = 1;

    static void extract(uint8_t* salt, size_t saltLen, uint8_t* inputKeyMaterial, size_t ikmLength, uint8_t* prkOut);

    static void expand(uint8_t* prk, size_t prkLen, uint8_t* info, size_t infoLen, uint8_t* output, size_t L);
};
} // namespace
/**
 * @}
 */

#endif // HKDF_H
