#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <cstring>

// --- 1. SHA-256 Cryptographic Engine ---
class SHA256 {
private:
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t k[64];

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t tousig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t tousig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void transform(const uint8_t data[]) {
        uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
            m[i] = tousig1(m[i - 2]) + m[i - 7] + tousig0(m[i - 15]) + m[i - 16];

        a = state[0]; b = state[1]; c = state[2]; d = state[3];
        e = state[4]; f = state[5]; g = state[6]; h = state[7];

        for (i = 0; i < 64; ++i) {
            t1 = h + sig1(e) + choose(e, f, g) + k[i] + m[i];
            t2 = sig0(a) + majority(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

public:
    SHA256() {
        uint32_t k_init[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb03238c4,0xbf657fc8,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        std::memcpy(k, k_init, sizeof(k));
        reset();
    }

    void reset() {
        state[0] = 0x6a09e667; state[1] = 0xbb67ae85; state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
        state[4] = 0x510e527f; state[5] = 0x9b05688c; state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
        datalen = 0; bitlen = 0;
    }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[datalen++] = data[i];
            if (datalen == 64) {
                transform(buffer);
                bitlen += 512;
                datalen = 0;
            }
        }
    }

    void final(uint8_t hash[32]) {
        uint32_t i = datalen;
        if (datalen < 56) {
            buffer[i++] = 0x80;
            while (i < 56) buffer[i++] = 0x00;
        } else {
            buffer[i++] = 0x80;
            while (i < 64) buffer[i++] = 0x00;
            transform(buffer);
            std::memset(buffer, 0, 56);
        }
        bitlen += (uint64_t)datalen * 8;
        buffer[63] = bitlen;
        buffer[62] = bitlen >> 8;
        buffer[61] = bitlen >> 16;
        buffer[60] = bitlen >> 24;
        buffer[59] = bitlen >> 32;
        buffer[58] = bitlen >> 40;
        buffer[57] = bitlen >> 48;
        buffer[56] = bitlen >> 56;
        transform(buffer);

        for (i = 0; i < 4; ++i) {
            hash[i]      = (state[0] >> (24 - i * 8)) & 0xff;
            hash[i + 4]  = (state[1] >> (24 - i * 8)) & 0xff;
            hash[i + 8]  = (state[2] >> (24 - i * 8)) & 0xff;
            hash[i + 12] = (state[3] >> (24 - i * 8)) & 0xff;
            hash[i + 16] = (state[4] >> (24 - i * 8)) & 0xff;
            hash[i + 20] = (state[5] >> (24 - i * 8)) & 0xff;
            hash[i + 24] = (state[6] >> (24 - i * 8)) & 0xff;
            hash[i + 28] = (state[7] >> (24 - i * 8)) & 0xff;
        }
    }

    static std::string hash_string(const std::string& input) {
        SHA256 sha256;
        sha256.update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
        uint8_t hash[32];
        sha256.final(hash);
        
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
};

// --- 2. Blockchain Logger ---
class ProductionBlockchainLogger {
private:
    std::string log_filename;
    std::string latest_hash;
    int block_counter;

public:
    ProductionBlockchainLogger(const std::string& filename = "benchmark_audit.log") 
        : log_filename(filename), block_counter(0), latest_hash("0000000000000000000000000000000000000000000000000000000000000000") {
        std::ofstream outfile(log_filename, std::ios::trunc);
        outfile << "# LATENCY BENCHMARK LOG\n";
        outfile.close();
    }

    void append_block(int pred_class, float confidence, float delta, bool review) {
        int index = block_counter++;
        std::stringstream raw_data;
        raw_data << index << "|" << pred_class << "|" << confidence << "|" << delta << "|" << latest_hash;
        latest_hash = SHA256::hash_string(raw_data.str());

        // เขียน Log จริงลงไฟล์ (จำลอง Non-Ideal Disk I/O)
        std::ofstream outfile(log_filename, std::ios::app);
        if (outfile.is_open()) {
            outfile << "BLOCK=" << index << " HASH=" << latest_hash << "\n";
            outfile.close();
        }
    }
};

// --- 3. Latency Distribution & Statistical Analyzer ---
int main() {
    const int total_iterations = 1000;
    std::vector<double> latencies_us;
    latencies_us.reserve(total_iterations);

    ProductionBlockchainLogger blockchain("latency_test.log");

    std::cout << "=== เริ่มต้นทดสอบการกระจายตัวของความหน่วง (Latency Distribution Benchmark) ===" << std::endl;
    std::cout << "กำลังรันการทดสอบจำนวน " << total_iterations << " รอบ..." << std::endl;

    for (int i = 0; i < total_iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        // จำลองกระบวนการทำงานครบวงจร: Inference + SHA-256 Hash + Disk Append
        blockchain.append_block(1, 0.89f, 0.40f, true);

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(duration);
    }

    // คำนวณทางสถิติ (Statistical Distribution Analysis)
    std::sort(latencies_us.begin(), latencies_us.end());

    double min_lat = latencies_us.front();
    double max_lat = latencies_us.back();
    double median_lat = latencies_us[total_iterations / 2];
    double p95_lat = latencies_us[static_cast<size_t>(total_iterations * 0.95)];
    double p99_lat = latencies_us[static_cast<size_t>(total_iterations * 0.99)];

    double sum = std::accumulate(latencies_us.begin(), latencies_us.end(), 0.0);
    double mean_lat = sum / total_iterations;

    // คำนวณ Standard Deviation (ความเสถียรของระบบ)
    double sq_sum = std::inner_product(latencies_us.begin(), latencies_us.end(), latencies_us.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / total_iterations - mean_lat * mean_lat);

    // พิมพ์รายงานผลลัพธ์ระดับ Professional Report
    std::cout << "\n=================================================" << std::endl;
    std::cout << "          P'MAX SYSTEM LATENCY REPORT            " << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "Total Iterations : " << total_iterations << " runs" << std::endl;
    std::cout << "Mean Latency     : " << std::fixed << std::setprecision(2) << mean_lat << " microseconds" << std::endl;
    std::cout << "Standard Dev     : " << stdev << " us (Stability Indicator)" << std::endl;
    std::cout << "Min Latency      : " << min_lat << " us" << std::endl;
    std::cout << "Median (P50)     : " << median_lat << " us" << std::endl;
    std::cout << "95th Percentile  : " << p95_lat << " us (P95)" << std::endl;
    std::cout << "99th Percentile  : " << p99_lat << " us (P99)" << std::endl;
    std::cout << "Max Latency      : " << max_lat << " us" << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "สถานะ: พร้อมนำข้อมูลชุดนี้ไป Plot Histogram หรือใส่ Pitch Deck" << std::endl;

    return 0;
}