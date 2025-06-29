#include <immintrin.h>
#include <cstdio>
#include <windows.h>
#include <memory>

constexpr unsigned char XOR_KEY = 0xAA;

struct AlignedBufferDeleter {
    void operator()(unsigned char* ptr) const noexcept {
        if (ptr) {
            SecureZeroMemory(ptr, _aligned_msize(ptr, 32, 0));
            _aligned_free(ptr);
        }
    }
};

template <size_t N>
class XorStr {
    alignas(32) unsigned char encrypted[N];

public:
    explicit XorStr(const char(&str)[N]) noexcept : encrypted{} {
        for (size_t i = 0; i < N; ++i)
            encrypted[i] = str[i] ^ XOR_KEY;
    }

    XorStr(const XorStr&) = delete;
    XorStr& operator=(const XorStr&) = delete;

    void print() const noexcept {
        size_t len = N - 1;
        std::unique_ptr<unsigned char, AlignedBufferDeleter> buffer(
            (unsigned char*)_aligned_malloc(len + 1, 32));
        if (!buffer) return;

        decrypt_avx2(buffer.get(), encrypted, len, XOR_KEY);
        buffer.get()[len] = 0;

        std::printf("%s\n", buffer.get());
    }

private:
    static void decrypt_avx2(unsigned char* out, const unsigned char* in, size_t len, unsigned char key) noexcept {
        size_t i = 0;
        __m256i keyblock = _mm256_set1_epi8(key);

        for (; i + 32 <= len; i += 32) {
            __m256i block = _mm256_loadu_si256((const __m256i*)(in + i));
            __m256i decrypted = _mm256_xor_si256(block, keyblock);
            _mm256_storeu_si256((__m256i*)(out + i), decrypted);
        }

        for (; i < len; ++i)
            out[i] = in[i] ^ key;
    }
};

template <size_t N>
inline void XorPrint(const char(&str)[N]) noexcept {
    XorStr<N> xs(str);
    xs.print();
}

int main() {
    XorPrint("Example silvatweaks");
    return 0;
}
