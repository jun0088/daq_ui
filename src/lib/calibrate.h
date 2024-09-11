#ifndef __CALIBRATE_H__
#define __CALIBRATE_H__

#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define NEEDS_ENDIAN_CONVERSION 0
#else
    #define NEEDS_ENDIAN_CONVERSION 1
#endif

#define TO_LITTLE_ENDIAN(val) ((val&0xff) << 24 | (val&0xff00) << 8 | (val&0xff0000) >> 8 | (val&0xff000000) >> 24)

void process_data_simd(uint32_t* data, size_t data_count, float gain, float* result) {
    const float normalization_factor = 4.096f / (float)0x7fffff;
    const size_t batch_count = data_count / 8;
    const size_t batch_total = batch_count * 8;
    const size_t remain = data_count % 8;
    __m256 norm_factor_vec = _mm256_set1_ps(normalization_factor);
    __m256 gain_vec = _mm256_set1_ps(gain);

    for (size_t i = 0; i < batch_count; i++) {
        __m256i data_vec_int = _mm256_loadu_si256((const __m256i*)&data[i * 8]);
        __m256 data_vec = _mm256_cvtepi32_ps(data_vec_int);
        data_vec = _mm256_mul_ps(data_vec, norm_factor_vec);
        data_vec = _mm256_mul_ps(data_vec, gain_vec);
        _mm256_storeu_ps(&result[i * 8], data_vec);
    }

    for (size_t i = 0; i < remain; i++) {
        float val = data[batch_total + i];
        result[batch_total + i] = (val * normalization_factor) * gain;
    }
}

void endian_convert_simd(const uint32_t* input, uint32_t* output, size_t count) {
    const size_t simd_count = count / 8;
    const size_t remain = count % 8;

    // 创建用于字节重排的掩码
    __m256i mask = _mm256_set_epi8(
        28, 29, 30, 31,  24, 25, 26, 27,  20, 21, 22, 23,  16, 17, 18, 19,
        12, 13, 14, 15,  8, 9, 10, 11,  4, 5, 6, 7,  0, 1, 2, 3
    );

    for (size_t i = 0; i < simd_count; ++i) {
        __m256i data_vec = _mm256_loadu_si256((__m256i*)&input[i * 8]);
        data_vec = _mm256_shuffle_epi8(data_vec, mask);

        _mm256_storeu_si256((__m256i*)&output[i * 8], data_vec);
    }
    // 处理剩余的值
    for (size_t i = simd_count * 8; i < count; i++) {

        output[i] = TO_LITTLE_ENDIAN(input[i]);

    }
}

#endif