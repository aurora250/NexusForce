#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/encrypt/aes256.hpp>
#include <NeForce/core/memory/memory_view.hpp>
#include <openssl/evp.h>

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic ignored "-Wstringop-overread"
#endif

using namespace neforce;

namespace {
    enum class cipher_mode {
        ecb,
        cbc,
        gcm,
    };

    constexpr size_t block_size = 16;
    constexpr size_t gcm_tag_size = 16;
    constexpr size_t gcm_iv_size = 12;

    constexpr byte_t key[32] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
                                0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
                                0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
    constexpr byte_t iv[block_size] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    constexpr byte_t aad[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

    void fill_plain(vector<byte_t>& data) {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<byte_t>((i * 131U + 17U) & 0xffU);
        }
    }

    cbyte_view view_of(const vector<byte_t>& data) { return cbyte_view(data.data(), data.size()); }
    cbyte_view view_of(const byte_t* data, const size_t size) { return cbyte_view(data, size); }

    byte_vector neforce_encrypt(const cipher_mode mode, const vector<byte_t>& plain, byte_t* tag) {
        const cbyte_view key_view = view_of(key, sizeof(key));
        switch (mode) {
            case cipher_mode::ecb: {
                return AES256::encrypt_ecb(view_of(plain), key_view);
            }
            case cipher_mode::cbc: {
                return AES256::encrypt_cbc(view_of(plain), key_view, view_of(iv, sizeof(iv)));
            }
            case cipher_mode::gcm: {
                return AES256::encrypt_gcm(view_of(plain), key_view, view_of(iv, gcm_iv_size),
                                           view_of(aad, sizeof(aad)), tag, gcm_tag_size);
            }
        }
        return {};
    }

    byte_vector neforce_decrypt(const cipher_mode mode, const vector<byte_t>& cipher, const byte_t* tag) {
        const cbyte_view key_view = view_of(key, sizeof(key));
        switch (mode) {
            case cipher_mode::ecb: {
                return AES256::decrypt_ecb(view_of(cipher), key_view);
            }
            case cipher_mode::cbc: {
                return AES256::decrypt_cbc(view_of(cipher), key_view, view_of(iv, sizeof(iv)));
            }
            case cipher_mode::gcm: {
                return AES256::decrypt_gcm(view_of(cipher), key_view, view_of(iv, gcm_iv_size),
                                           view_of(aad, sizeof(aad)), view_of(tag, gcm_tag_size), gcm_tag_size);
            }
        }
        return {};
    }

    void require(const bool condition) {
        if (!condition) {
            abort();
        }
    }

    const EVP_CIPHER* openssl_cipher(const cipher_mode mode) noexcept {
        switch (mode) {
            case cipher_mode::ecb:
                return EVP_aes_256_ecb();
            case cipher_mode::cbc:
                return EVP_aes_256_cbc();
            case cipher_mode::gcm:
                return EVP_aes_256_gcm();
        }
        return nullptr;
    }

    void openssl_crypt(EVP_CIPHER_CTX* context, const cipher_mode mode, const bool encrypt, const vector<byte_t>& input,
                       vector<byte_t>& output, byte_t* tag) {
        const EVP_CIPHER* cipher = openssl_cipher(mode);
        require(cipher != nullptr);
        require(EVP_CipherInit_ex(context, cipher, nullptr, nullptr, nullptr, encrypt ? 1 : 0) == 1);
        if (mode == cipher_mode::gcm) {
            require(EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(gcm_iv_size), nullptr) == 1);
        } else {
            require(EVP_CIPHER_CTX_set_padding(context, 0) == 1);
        }
        require(EVP_CipherInit_ex(context, nullptr, nullptr, key, mode == cipher_mode::ecb ? nullptr : iv,
                                  encrypt ? 1 : 0) == 1);
        if (mode == cipher_mode::gcm && !encrypt) {
            require(EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, static_cast<int>(gcm_tag_size), tag) == 1);
        }
        int output_length = 0;
        if (mode == cipher_mode::gcm) {
            int aad_length = 0;
            require(EVP_CipherUpdate(context, nullptr, &aad_length, aad, static_cast<int>(sizeof(aad))) == 1);
        }
        require(EVP_CipherUpdate(context, output.data(), &output_length, input.data(),
                                 static_cast<int>(input.size())) == 1);
        int final_length = 0;
        require(EVP_CipherFinal_ex(context, output.data() + output_length, &final_length) == 1);
        if (mode == cipher_mode::gcm && encrypt) {
            require(EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, static_cast<int>(gcm_tag_size), tag) == 1);
        }
    }

    vector<byte_t> openssl_encrypt(const cipher_mode mode, const vector<byte_t>& plain, byte_t* tag) {
        EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
        require(context != nullptr);
        vector<byte_t> cipher(plain.size() + block_size);
        openssl_crypt(context, mode, true, plain, cipher, tag);
        EVP_CIPHER_CTX_free(context);
        cipher.resize(plain.size());
        return cipher;
    }

    void run_neforce(benchmark::State& state, const cipher_mode mode, const bool encrypt) {
        const size_t size = static_cast<size_t>(state.range(0));
        vector<byte_t> plain(size);
        fill_plain(plain);
        vector<byte_t> cipher;
        byte_t tag[gcm_tag_size] = {};
        if (!encrypt) {
            const byte_vector encrypted = neforce_encrypt(mode, plain, tag);
            cipher.resize(encrypted.size());
            for (size_t i = 0; i < encrypted.size(); ++i) {
                cipher[i] = encrypted[i];
            }
        }
        for (auto _: state) {
            if (encrypt) {
                byte_vector output = neforce_encrypt(mode, plain, tag);
                benchmark::DoNotOptimize(output.data());
                benchmark::DoNotOptimize(output.size());
            } else {
                byte_vector output = neforce_decrypt(mode, cipher, tag);
                benchmark::DoNotOptimize(output.data());
                benchmark::DoNotOptimize(output.size());
            }
        }
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(size));
    }

    void run_openssl(benchmark::State& state, const cipher_mode mode, const bool encrypt) {
        const size_t size = static_cast<size_t>(state.range(0));
        vector<byte_t> plain(size);
        fill_plain(plain);
        vector<byte_t> cipher;
        byte_t tag[gcm_tag_size] = {};
        if (!encrypt) {
            cipher = openssl_encrypt(mode, plain, tag);
        }
        EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
        require(context != nullptr);
        for (auto _: state) {
            vector<byte_t> output(size + block_size);
            if (encrypt) {
                openssl_crypt(context, mode, true, plain, output, tag);
            } else {
                openssl_crypt(context, mode, false, cipher, output, tag);
            }
            benchmark::DoNotOptimize(output.data());
            benchmark::DoNotOptimize(output.size());
        }
        EVP_CIPHER_CTX_free(context);
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(size));
    }

    void apply_sizes(benchmark::Benchmark* benchmark) {
        for (const int size: {16, 64, 256, 1024, 8192, 65536}) {
            benchmark->Arg(size);
        }
    }

    void BM_NeForceEcbEncrypt(benchmark::State& state) { run_neforce(state, cipher_mode::ecb, true); }
    void BM_NeForceEcbDecrypt(benchmark::State& state) { run_neforce(state, cipher_mode::ecb, false); }
    void BM_NeForceCbcEncrypt(benchmark::State& state) { run_neforce(state, cipher_mode::cbc, true); }
    void BM_NeForceCbcDecrypt(benchmark::State& state) { run_neforce(state, cipher_mode::cbc, false); }
    void BM_NeForceGcmEncrypt(benchmark::State& state) { run_neforce(state, cipher_mode::gcm, true); }
    void BM_NeForceGcmDecrypt(benchmark::State& state) { run_neforce(state, cipher_mode::gcm, false); }

    void BM_OpenSslEcbEncrypt(benchmark::State& state) { run_openssl(state, cipher_mode::ecb, true); }
    void BM_OpenSslEcbDecrypt(benchmark::State& state) { run_openssl(state, cipher_mode::ecb, false); }
    void BM_OpenSslCbcEncrypt(benchmark::State& state) { run_openssl(state, cipher_mode::cbc, true); }
    void BM_OpenSslCbcDecrypt(benchmark::State& state) { run_openssl(state, cipher_mode::cbc, false); }
    void BM_OpenSslGcmEncrypt(benchmark::State& state) { run_openssl(state, cipher_mode::gcm, true); }
    void BM_OpenSslGcmDecrypt(benchmark::State& state) { run_openssl(state, cipher_mode::gcm, false); }
} // namespace

BENCHMARK(BM_NeForceEcbEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NeForceEcbDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NeForceCbcEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NeForceCbcDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NeForceGcmEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NeForceGcmDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_OpenSslEcbEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OpenSslEcbDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OpenSslCbcEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OpenSslCbcDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OpenSslGcmEncrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OpenSslGcmDecrypt)->Apply(apply_sizes)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
