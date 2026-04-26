#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr const char* kMagic = "HUF1";
constexpr std::uint32_t kVersion = 1;
std::string escape_json(const std::string& input) {
    std::ostringstream out;
    for (char ch : input) {
        switch (ch) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                } else {
                    out << ch;
                }
        }
    }
    return out.str();
}

void print_json_error(const std::string& message) {
    std::cout << "{"
              << "\"ok\":false,"
              << "\"error\":\"" << escape_json(message) << "\""
              << "}" << std::endl;
}

void ensure(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class SHA256 {
  public:
    SHA256() { reset(); }

    void update(const std::uint8_t* data, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            buffer_[buffer_length_++] = data[i];
            bit_length_ += 8;
            if (buffer_length_ == 64) {
                transform();
                buffer_length_ = 0;
            }
        }
    }

    void update(const std::vector<std::uint8_t>& data) {
        update(data.data(), data.size());
    }

    std::array<std::uint8_t, 32> digest() {
        auto buffer_len = buffer_length_;
        buffer_[buffer_len++] = 0x80;

        if (buffer_len > 56) {
            while (buffer_len < 64) {
                buffer_[buffer_len++] = 0x00;
            }
            transform();
            buffer_len = 0;
        }

        while (buffer_len < 56) {
            buffer_[buffer_len++] = 0x00;
        }

        for (int i = 7; i >= 0; --i) {
            buffer_[buffer_len++] = static_cast<std::uint8_t>((bit_length_ >> (i * 8)) & 0xff);
        }
        transform();

        std::array<std::uint8_t, 32> hash{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            hash[i * 4] = static_cast<std::uint8_t>((state_[i] >> 24) & 0xff);
            hash[i * 4 + 1] = static_cast<std::uint8_t>((state_[i] >> 16) & 0xff);
            hash[i * 4 + 2] = static_cast<std::uint8_t>((state_[i] >> 8) & 0xff);
            hash[i * 4 + 3] = static_cast<std::uint8_t>(state_[i] & 0xff);
        }
        return hash;
    }

    static std::string hex(const std::array<std::uint8_t, 32>& bytes) {
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (auto byte : bytes) {
            out << std::setw(2) << static_cast<int>(byte);
        }
        return out.str();
    }

    static std::array<std::uint8_t, 32> from_hex(const std::string& value) {
        ensure(value.size() == 64, "Invalid checksum length.");
        std::array<std::uint8_t, 32> result{};
        for (std::size_t i = 0; i < 32; ++i) {
            const auto slice = value.substr(i * 2, 2);
            result[i] = static_cast<std::uint8_t>(std::stoul(slice, nullptr, 16));
        }
        return result;
    }

  private:
    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffer_length_ = 0;
    std::uint64_t bit_length_ = 0;

    static std::uint32_t rotr(std::uint32_t value, std::uint32_t count) {
        return (value >> count) | (value << (32 - count));
    }

    void reset() {
        state_ = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
        };
        buffer_.fill(0);
        buffer_length_ = 0;
        bit_length_ = 0;
    }

    void transform() {
        static constexpr std::array<std::uint32_t, 64> k = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
            0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
            0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
            0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
            0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };

        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            words[i] = (static_cast<std::uint32_t>(buffer_[i * 4]) << 24) |
                       (static_cast<std::uint32_t>(buffer_[i * 4 + 1]) << 16) |
                       (static_cast<std::uint32_t>(buffer_[i * 4 + 2]) << 8) |
                       (static_cast<std::uint32_t>(buffer_[i * 4 + 3]));
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = rotr(words[i - 15], 7) ^ rotr(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const auto s1 = rotr(words[i - 2], 17) ^ rotr(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        auto a = state_[0];
        auto b = state_[1];
        auto c = state_[2];
        auto d = state_[3];
        auto e = state_[4];
        auto f = state_[5];
        auto g = state_[6];
        auto h = state_[7];

        for (std::size_t i = 0; i < 64; ++i) {
            const auto s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const auto ch = (e & f) ^ ((~e) & g);
            const auto temp1 = h + s1 + ch + k[i] + words[i];
            const auto s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }
};

std::vector<std::uint8_t> read_file(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    ensure(input.good(), "Unable to open input file: " + path.string());
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

void write_file(const fs::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream output(path, std::ios::binary);
    ensure(output.good(), "Unable to open output file: " + path.string());
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    ensure(output.good(), "Failed writing output file: " + path.string());
}

template <typename T>
void write_number(std::ostream& out, T value) {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xff));
    }
}

template <typename T>
T read_number(std::istream& in) {
    T value = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        const int ch = in.get();
        ensure(ch != EOF, "Unexpected end of archive.");
        value |= static_cast<T>(static_cast<std::uint64_t>(static_cast<unsigned char>(ch)) << (i * 8));
    }
    return value;
}

struct Node {
    std::uint8_t symbol = 0;
    std::uint64_t frequency = 0;
    bool leaf = false;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
};

struct NodeCompare {
    bool operator()(const std::shared_ptr<Node>& lhs, const std::shared_ptr<Node>& rhs) const {
        if (lhs->frequency == rhs->frequency) {
            return lhs->symbol > rhs->symbol;
        }
        return lhs->frequency > rhs->frequency;
    }
};

std::shared_ptr<Node> build_tree(const std::array<std::uint64_t, 256>& frequencies) {
    std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, NodeCompare> queue;
    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        if (frequencies[i] > 0) {
            auto node = std::make_shared<Node>();
            node->symbol = static_cast<std::uint8_t>(i);
            node->frequency = frequencies[i];
            node->leaf = true;
            queue.push(node);
        }
    }

    if (queue.empty()) {
        return nullptr;
    }

    if (queue.size() == 1) {
        auto parent = std::make_shared<Node>();
        parent->frequency = queue.top()->frequency;
        parent->left = queue.top();
        parent->right = nullptr;
        parent->leaf = false;
        return parent;
    }

    while (queue.size() > 1) {
        auto left = queue.top();
        queue.pop();
        auto right = queue.top();
        queue.pop();

        auto parent = std::make_shared<Node>();
        parent->frequency = left->frequency + right->frequency;
        parent->symbol = std::min(left->symbol, right->symbol);
        parent->left = left;
        parent->right = right;
        parent->leaf = false;
        queue.push(parent);
    }

    return queue.top();
}

void build_codes(const std::shared_ptr<Node>& node, const std::string& prefix,
                 std::array<std::string, 256>& codes) {
    if (!node) {
        return;
    }
    if (node->leaf) {
        codes[node->symbol] = prefix.empty() ? "0" : prefix;
        return;
    }
    build_codes(node->left, prefix + "0", codes);
    build_codes(node->right, prefix + "1", codes);
}

struct CompressionResult {
    std::vector<std::uint8_t> archive;
    std::array<std::uint64_t, 256> frequencies{};
    std::uint64_t original_size = 0;
    std::uint64_t compressed_bits = 0;
    std::string checksum;
};

CompressionResult compress_bytes(const std::vector<std::uint8_t>& data) {
    CompressionResult result;
    result.original_size = data.size();

    for (auto byte : data) {
        result.frequencies[byte]++;
    }

    SHA256 sha;
    sha.update(data);
    result.checksum = SHA256::hex(sha.digest());

    std::array<std::string, 256> codes;
    auto tree = build_tree(result.frequencies);
    build_codes(tree, "", codes);

    std::vector<std::uint8_t> payload;
    std::uint8_t current = 0;
    int bit_index = 0;

    for (auto byte : data) {
        const auto& code = codes[byte];
        for (char bit : code) {
            if (bit == '1') {
                current |= static_cast<std::uint8_t>(1u << (7 - bit_index));
            }
            ++bit_index;
            ++result.compressed_bits;
            if (bit_index == 8) {
                payload.push_back(current);
                current = 0;
                bit_index = 0;
            }
        }
    }

    if (bit_index != 0) {
        payload.push_back(current);
    }

    std::ostringstream archive_stream(std::ios::binary);
    archive_stream.write(kMagic, 4);
    write_number<std::uint32_t>(archive_stream, kVersion);
    write_number<std::uint64_t>(archive_stream, result.original_size);
    write_number<std::uint64_t>(archive_stream, result.compressed_bits);
    const auto checksum_bytes = SHA256::from_hex(result.checksum);
    archive_stream.write(reinterpret_cast<const char*>(checksum_bytes.data()), checksum_bytes.size());
    const auto active_symbols = static_cast<std::size_t>(std::count_if(
        result.frequencies.begin(), result.frequencies.end(), [](std::uint64_t value) { return value > 0; }));
    write_number<std::uint16_t>(archive_stream, static_cast<std::uint16_t>(active_symbols));
    for (std::size_t i = 0; i < result.frequencies.size(); ++i) {
        if (result.frequencies[i] == 0) {
            continue;
        }
        archive_stream.put(static_cast<char>(i));
        write_number<std::uint64_t>(archive_stream, result.frequencies[i]);
    }
    archive_stream.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));

    const auto raw = archive_stream.str();
    result.archive.assign(raw.begin(), raw.end());
    return result;
}

struct DecompressionResult {
    std::vector<std::uint8_t> bytes;
    std::array<std::uint64_t, 256> frequencies{};
    std::uint64_t original_size = 0;
    std::uint64_t compressed_bits = 0;
    std::string stored_checksum;
    std::string computed_checksum;
    bool integrity_ok = false;
};

DecompressionResult decompress_bytes(const std::vector<std::uint8_t>& archive_data) {
    std::istringstream archive(std::string(archive_data.begin(), archive_data.end()), std::ios::binary);
    char magic[4];
    archive.read(magic, 4);
    ensure(archive.gcount() == 4 && std::string(magic, 4) == kMagic, "Input is not a valid HUF archive.");

    const auto version = read_number<std::uint32_t>(archive);
    ensure(version == kVersion, "Unsupported archive version.");

    DecompressionResult result;
    result.original_size = read_number<std::uint64_t>(archive);
    result.compressed_bits = read_number<std::uint64_t>(archive);

    std::array<std::uint8_t, 32> checksum_bytes{};
    archive.read(reinterpret_cast<char*>(checksum_bytes.data()), checksum_bytes.size());
    ensure(static_cast<std::size_t>(archive.gcount()) == checksum_bytes.size(), "Archive checksum is truncated.");
    result.stored_checksum = SHA256::hex(checksum_bytes);

    const auto active_symbols = read_number<std::uint16_t>(archive);
    for (std::uint16_t i = 0; i < active_symbols; ++i) {
        const int symbol = archive.get();
        ensure(symbol != EOF, "Archive symbol table is truncated.");
        result.frequencies[static_cast<std::uint8_t>(symbol)] = read_number<std::uint64_t>(archive);
    }

    auto tree = build_tree(result.frequencies);
    if (!tree) {
        result.bytes.clear();
    } else {
        auto current = tree;
        std::uint64_t bits_read = 0;
        while (bits_read < result.compressed_bits && result.bytes.size() < result.original_size) {
            const int ch = archive.get();
            ensure(ch != EOF, "Archive payload ended unexpectedly.");
            const auto byte = static_cast<std::uint8_t>(ch);
            for (int i = 0; i < 8 && bits_read < result.compressed_bits && result.bytes.size() < result.original_size; ++i) {
                const bool bit = ((byte >> (7 - i)) & 1u) != 0;
                bits_read++;
                current = bit ? current->right : current->left;
                ensure(current != nullptr, "Archive tree traversal failed; data may be corrupted.");
                if (current->leaf) {
                    result.bytes.push_back(current->symbol);
                    current = tree;
                }
            }
        }
    }

    ensure(result.bytes.size() == result.original_size, "Decompressed byte count does not match archive metadata.");

    SHA256 sha;
    sha.update(result.bytes);
    result.computed_checksum = SHA256::hex(sha.digest());
    result.integrity_ok = result.computed_checksum == result.stored_checksum;
    ensure(result.integrity_ok, "Integrity verification failed: checksum mismatch.");

    return result;
}

std::size_t unique_symbols(const std::array<std::uint64_t, 256>& frequencies) {
    return static_cast<std::size_t>(std::count_if(frequencies.begin(), frequencies.end(),
                                                  [](std::uint64_t value) { return value > 0; }));
}

std::string top_symbols(const std::array<std::uint64_t, 256>& frequencies) {
    std::vector<std::pair<std::uint8_t, std::uint64_t>> entries;
    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        if (frequencies[i] > 0) {
            entries.emplace_back(static_cast<std::uint8_t>(i), frequencies[i]);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second;
    });

    std::ostringstream out;
    out << "[";
    const auto count = std::min<std::size_t>(entries.size(), 5);
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto symbol = entries[i].first;
        const auto printable = std::isprint(symbol) ? std::string(1, static_cast<char>(symbol)) : "0x" + [&]() {
            std::ostringstream hex;
            hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(symbol);
            return hex.str();
        }();
        out << "{"
            << "\"symbol\":\"" << escape_json(printable) << "\","
            << "\"count\":" << entries[i].second
            << "}";
    }
    out << "]";
    return out.str();
}

void print_compress_json(const fs::path& input_path, const fs::path& output_path, const CompressionResult& result) {
    const auto input_size = static_cast<double>(result.original_size);
    const auto output_size = static_cast<double>(result.archive.size());
    const auto ratio = result.original_size == 0 ? 1.0 : output_size / input_size;
    const auto savings = result.original_size == 0 ? 0.0 : (1.0 - ratio) * 100.0;

    std::cout << "{"
              << "\"ok\":true,"
              << "\"action\":\"compress\","
              << "\"input\":\"" << escape_json(input_path.filename().string()) << "\","
              << "\"output\":\"" << escape_json(output_path.filename().string()) << "\","
              << "\"original_size\":" << result.original_size << ","
              << "\"output_size\":" << result.archive.size() << ","
              << "\"compression_ratio\":" << std::fixed << std::setprecision(4) << ratio << ","
              << "\"space_saved_percent\":" << std::fixed << std::setprecision(2) << savings << ","
              << "\"checksum\":\"" << result.checksum << "\","
              << "\"unique_symbols\":" << unique_symbols(result.frequencies) << ","
              << "\"top_symbols\":" << top_symbols(result.frequencies)
              << "}" << std::endl;
}

void print_decompress_json(const fs::path& input_path, const fs::path& output_path, const DecompressionResult& result,
                           std::size_t archive_size) {
    const auto archive_size_double = static_cast<double>(archive_size);
    const auto restored_size = static_cast<double>(result.original_size);
    const auto expansion = archive_size == 0 ? 0.0 : restored_size / archive_size_double;

    std::cout << "{"
              << "\"ok\":true,"
              << "\"action\":\"decompress\","
              << "\"input\":\"" << escape_json(input_path.filename().string()) << "\","
              << "\"output\":\"" << escape_json(output_path.filename().string()) << "\","
              << "\"archive_size\":" << archive_size << ","
              << "\"restored_size\":" << result.original_size << ","
              << "\"expansion_ratio\":" << std::fixed << std::setprecision(4) << expansion << ","
              << "\"checksum\":\"" << result.computed_checksum << "\","
              << "\"integrity_ok\":" << (result.integrity_ok ? "true" : "false") << ","
              << "\"unique_symbols\":" << unique_symbols(result.frequencies) << ","
              << "\"top_symbols\":" << top_symbols(result.frequencies)
              << "}" << std::endl;
}

void print_inspect_json(const fs::path& input_path, const DecompressionResult& result, std::size_t archive_size) {
    std::cout << "{"
              << "\"ok\":true,"
              << "\"action\":\"inspect\","
              << "\"input\":\"" << escape_json(input_path.filename().string()) << "\","
              << "\"archive_size\":" << archive_size << ","
              << "\"original_size\":" << result.original_size << ","
              << "\"checksum\":\"" << result.stored_checksum << "\","
              << "\"integrity_ok\":" << (result.integrity_ok ? "true" : "false") << ","
              << "\"unique_symbols\":" << unique_symbols(result.frequencies) << ","
              << "\"top_symbols\":" << top_symbols(result.frequencies)
              << "}" << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        ensure(argc >= 3, "Usage: compressor <compress|decompress|inspect> <input> [output]");
        const std::string command = argv[1];
        const fs::path input_path = argv[2];

        if (command == "compress") {
            ensure(argc >= 4, "Compress mode requires an output path.");
            const fs::path output_path = argv[3];
            auto input_data = read_file(input_path);
            auto result = compress_bytes(input_data);
            write_file(output_path, result.archive);
            print_compress_json(input_path, output_path, result);
            return 0;
        }

        if (command == "decompress") {
            ensure(argc >= 4, "Decompress mode requires an output path.");
            const fs::path output_path = argv[3];
            auto archive_data = read_file(input_path);
            auto result = decompress_bytes(archive_data);
            write_file(output_path, result.bytes);
            print_decompress_json(input_path, output_path, result, archive_data.size());
            return 0;
        }

        if (command == "inspect") {
            auto archive_data = read_file(input_path);
            auto result = decompress_bytes(archive_data);
            print_inspect_json(input_path, result, archive_data.size());
            return 0;
        }

        throw std::runtime_error("Unknown command: " + command);
    } catch (const std::exception& ex) {
        print_json_error(ex.what());
        return 1;
    }
}
