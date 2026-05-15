#include <immintrin.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <iomanip>

int main() {
    int fd = open("global_weather_100M.bin", O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;
    
    // Tarik dari luar (Disk) masuk ke Virtual Memory
    float* wire_data = (float*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    long long N = size / sizeof(float);

    __m512i bit_accumulator = _mm512_setzero_si512();
    __m512i magic_pattern = _mm512_set1_epi32(0x55555555);

    std::cout << "--- EXTERNAL PULL + BIT ALCHEMY (REAL I/O) ---" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    for (long long i = 0; i < N; i += 16) {
        // TEKNIK WHISPERER: Stream load (Tarik terus dari luar, bypass cache)
        __m512i v = _mm512_stream_load_si512((__m512i*)&wire_data[i]);
        
        // Bit-Manipulation
        bit_accumulator = _mm512_ternarylogic_epi32(v, magic_pattern, bit_accumulator, 0x96);
        bit_accumulator = _mm512_rol_epi32(bit_accumulator, 7);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "LATENCY: " << (double)total_ns / N << " ns (" << ((double)total_ns / N) * 1000 << " ps)" << std::endl;
    
    unsigned int final_bits[16];
    _mm512_store_si512((__m512i*)final_bits, bit_accumulator);
    std::cout << "External Pull Result: 0x" << std::hex << final_bits[0] << std::dec << std::endl;

    munmap(wire_data, size);
    return 0;
}
