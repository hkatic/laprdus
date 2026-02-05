// -*- coding: utf-8 -*-
// packer.cpp - Phoneme WAV to BIN packer tool
// Combines WAV files into a single packed binary with optional encryption

#define _CRT_SECURE_NO_WARNINGS  // Suppress sscanf warning

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <random>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#endif

// =============================================================================
// File Format Structures (matching types.hpp)
// =============================================================================

constexpr uint32_t PHONEME_FILE_MAGIC = 0x4C505244;  // "LPRD"
constexpr uint16_t PHONEME_FILE_VERSION = 1;

constexpr uint16_t PACKED_FLAG_ENCRYPTED = 0x0001;
constexpr uint16_t PHONEME_FLAG_TRUNCATED = 0x0004;

#pragma pack(push, 1)
struct PackedFileHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t phoneme_count;
    uint32_t index_offset;
    uint32_t data_offset;
    uint32_t total_size;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t channels;
    uint32_t checksum;
    uint8_t encryption_iv[16];
    uint8_t reserved[12];
};

struct PhonemeIndexEntry {
    uint32_t phoneme_id;
    uint32_t name_hash;
    uint32_t data_offset;
    uint32_t compressed_size;
    uint32_t original_size;
    uint32_t duration_samples;
    uint16_t flags;
    uint8_t reserved[6];
};

// WAV file structures
struct WavHeader {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct WavChunk {
    char id[4];
    uint32_t size;
};
#pragma pack(pop)

static_assert(sizeof(PackedFileHeader) == 64, "Header size mismatch");
static_assert(sizeof(PhonemeIndexEntry) == 32, "Index entry size mismatch");

// =============================================================================
// Phoneme Information
// =============================================================================

struct PhonemeInfo {
    uint32_t id;
    std::string name;
    std::string filename;
    bool truncate;
    uint32_t max_bytes;
};

std::vector<PhonemeInfo> get_phoneme_list() {
    return {
        {0,  "A",       "PHONEME_A.wav",    false, 0},
        {1,  "B",       "PHONEME_B.wav",    false, 0},
        {2,  "C",       "PHONEME_C.wav",    false, 0},
        {3,  "D",       "PHONEME_D.wav",    false, 0},
        {4,  "E",       "PHONEME_E.wav",    false, 0},
        {5,  "F",       "PHONEME_F.wav",    false, 0},
        {6,  "G",       "PHONEME_G.wav",    false, 0},
        {7,  "H",       "PHONEME_H.wav",    false, 0},
        {8,  "I",       "PHONEME_I.wav",    false, 0},
        {9,  "J",       "PHONEME_J.wav",    false, 0},
        {10, "K",       "PHONEME_K.wav",    false, 0},
        {11, "L",       "PHONEME_L.wav",    true,  2000},  // Truncated
        {12, "M",       "PHONEME_M.wav",    true,  2000},  // Truncated
        {13, "N",       "PHONEME_N.wav",    true,  2000},  // Truncated
        {14, "O",       "PHONEME_O.wav",    false, 0},
        {15, "P",       "PHONEME_P.wav",    false, 0},
        {16, "Q",       "PHONEME_Q.wav",    false, 0},
        {17, "R",       "PHONEME_R.wav",    false, 0},
        {18, "S",       "PHONEME_S.wav",    true,  2000},  // Truncated
        {19, "T",       "PHONEME_T.wav",    false, 0},
        {20, "U",       "PHONEME_U.wav",    false, 0},
        {21, "V",       "PHONEME_V.wav",    true,  2000},  // Truncated
        {22, "W",       "PHONEME_W.wav",    false, 0},
        {23, "X",       "PHONEME_X.wav",    false, 0},
        {24, "Y",       "PHONEME_Y.wav",    false, 0},
        {25, "Z",       "PHONEME_Z.wav",    true,  2000},  // Truncated
        // Croatian special characters
        {26, "CH",      "PHONEME_CH.wav",   false, 0},     // č
        {27, "TJ",      "PHONEME_TJ.wav",   false, 0},     // ć
        {28, "DJ",      "PHONEME_DJ.wav",   false, 0},     // đ, dž
        {29, "SH",      "PHONEME_SH.wav",   true,  2000},  // š (truncated)
        {30, "ZH",      "PHONEME_ZH.wav",   true,  2000},  // ž (truncated)
        {31, "LJ",      "PHONEME_LJ.wav",   false, 0},     // lj digraph
        {32, "NJ",      "PHONEME_NJ.wav",   false, 0},     // nj digraph
        // Special
        {33, "SILENCE", "-.wav",            false, 0},     // Pause
    };
}

// =============================================================================
// Utility Functions
// =============================================================================

uint32_t fnv1a_hash(const std::string& str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    static uint32_t table[256] = {0};
    static bool table_init = false;

    if (!table_init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c >> 1) ^ ((c & 1) ? 0xEDB88320 : 0);
            }
            table[i] = c;
        }
        table_init = true;
    }

    for (size_t i = 0; i < length; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

std::string path_join(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    if (dir.back() == PATH_SEPARATOR || dir.back() == '/') {
        return dir + file;
    }
    return dir + PATH_SEPARATOR + file;
}

// =============================================================================
// WAV File Reader
// =============================================================================

struct WavData {
    std::vector<uint8_t> samples;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t channels;
    uint32_t duration_samples;
};

bool read_wav_file(const std::string& path, WavData& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open " << path << std::endl;
        return false;
    }

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (std::memcmp(header.riff, "RIFF", 4) != 0 ||
        std::memcmp(header.wave, "WAVE", 4) != 0 ||
        std::memcmp(header.fmt, "fmt ", 4) != 0) {
        std::cerr << "Error: Invalid WAV header in " << path << std::endl;
        return false;
    }

    // Skip any extra fmt data
    if (header.fmt_size > 16) {
        file.seekg(header.fmt_size - 16, std::ios::cur);
    }

    // Find data chunk
    WavChunk chunk;
    while (file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {
        if (std::memcmp(chunk.id, "data", 4) == 0) {
            break;
        }
        // Skip unknown chunks
        file.seekg(chunk.size, std::ios::cur);
    }

    if (std::memcmp(chunk.id, "data", 4) != 0) {
        std::cerr << "Error: No data chunk in " << path << std::endl;
        return false;
    }

    out.samples.resize(chunk.size);
    file.read(reinterpret_cast<char*>(out.samples.data()), chunk.size);

    out.sample_rate = header.sample_rate;
    out.bits_per_sample = header.bits_per_sample;
    out.channels = header.num_channels;
    out.duration_samples = chunk.size / (header.bits_per_sample / 8) / header.num_channels;

    return true;
}

// =============================================================================
// XOR Obfuscation (simple encryption)
// =============================================================================

void xor_encrypt(std::vector<uint8_t>& data, const uint8_t* key, size_t key_len) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key_len];
    }
}

void generate_random_bytes(uint8_t* buffer, size_t len) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 gen(static_cast<unsigned>(seed));
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = static_cast<uint8_t>(dist(gen));
    }
}

bool parse_hex_key(const std::string& hex, uint8_t* key, size_t key_len) {
    if (hex.size() < key_len * 2) {
        return false;
    }
    for (size_t i = 0; i < key_len; ++i) {
        unsigned int byte;
        if (sscanf(hex.c_str() + i * 2, "%02x", &byte) != 1) {
            return false;
        }
        key[i] = static_cast<uint8_t>(byte);
    }
    return true;
}

// =============================================================================
// Main Packer Function
// =============================================================================

int pack_phonemes(const std::string& input_dir,
                  const std::string& output_file,
                  bool encrypt,
                  const std::string& key_hex) {

    std::vector<PhonemeInfo> phoneme_list = get_phoneme_list();
    std::vector<PhonemeIndexEntry> index;
    std::vector<uint8_t> audio_data;

    uint32_t expected_sample_rate = 22050;
    uint16_t expected_bits = 16;
    uint16_t expected_channels = 1;

    std::cout << "Packing phonemes from: " << input_dir << std::endl;
    std::cout << "Output file: " << output_file << std::endl;

    // Process each phoneme
    for (const auto& phoneme : phoneme_list) {
        std::string wav_path = path_join(input_dir, phoneme.filename);

        WavData wav;
        if (!read_wav_file(wav_path, wav)) {
            std::cerr << "Warning: Missing phoneme file " << wav_path << std::endl;
            continue;
        }

        // Verify format
        if (wav.sample_rate != expected_sample_rate) {
            std::cerr << "Warning: Sample rate mismatch in " << phoneme.filename
                      << " (expected " << expected_sample_rate
                      << ", got " << wav.sample_rate << ")" << std::endl;
        }

        // Apply truncation if specified
        std::vector<uint8_t> samples = std::move(wav.samples);
        if (phoneme.truncate && phoneme.max_bytes > 0 && samples.size() > phoneme.max_bytes) {
            samples.resize(phoneme.max_bytes);
        }

        // Create index entry
        PhonemeIndexEntry entry{};
        entry.phoneme_id = phoneme.id;
        entry.name_hash = fnv1a_hash(phoneme.name);
        entry.data_offset = static_cast<uint32_t>(audio_data.size());
        entry.original_size = static_cast<uint32_t>(samples.size());
        entry.compressed_size = entry.original_size;  // No compression
        entry.duration_samples = static_cast<uint32_t>(samples.size() / (expected_bits / 8) / expected_channels);
        entry.flags = 0;

        if (phoneme.truncate) {
            entry.flags |= PHONEME_FLAG_TRUNCATED;
        }

        // Append audio data
        audio_data.insert(audio_data.end(), samples.begin(), samples.end());
        index.push_back(entry);

        std::cout << "  Packed: " << phoneme.name
                  << " (" << samples.size() << " bytes)" << std::endl;
    }

    // Encryption
    uint8_t encryption_key[32] = {0};
    uint8_t encryption_iv[16] = {0};
    uint16_t header_flags = 0;

    if (encrypt) {
        header_flags |= PACKED_FLAG_ENCRYPTED;

        // Parse or generate key
        if (!key_hex.empty()) {
            if (!parse_hex_key(key_hex, encryption_key, 32)) {
                std::cerr << "Error: Invalid encryption key (need 64 hex chars)" << std::endl;
                return 1;
            }
        } else {
            generate_random_bytes(encryption_key, 32);
        }

        generate_random_bytes(encryption_iv, 16);

        // Apply XOR encryption
        xor_encrypt(audio_data, encryption_key, 32);

        std::cout << "Encryption key: ";
        for (int i = 0; i < 32; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(encryption_key[i]);
        }
        std::cout << std::dec << std::endl;
    }

    // Calculate offsets
    uint32_t index_offset = sizeof(PackedFileHeader);
    uint32_t data_offset = index_offset + static_cast<uint32_t>(index.size() * sizeof(PhonemeIndexEntry));
    uint32_t total_size = data_offset + static_cast<uint32_t>(audio_data.size());

    // Build header
    PackedFileHeader header{};
    header.magic = PHONEME_FILE_MAGIC;
    header.version = PHONEME_FILE_VERSION;
    header.flags = header_flags;
    header.phoneme_count = static_cast<uint32_t>(index.size());
    header.index_offset = index_offset;
    header.data_offset = data_offset;
    header.total_size = total_size;
    header.sample_rate = expected_sample_rate;
    header.bits_per_sample = expected_bits;
    header.channels = expected_channels;
    header.checksum = crc32(audio_data.data(), audio_data.size());
    std::memcpy(header.encryption_iv, encryption_iv, 16);

    // Write output file
    std::ofstream out(output_file, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Cannot create output file " << output_file << std::endl;
        return 1;
    }

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(index.data()),
              index.size() * sizeof(PhonemeIndexEntry));
    out.write(reinterpret_cast<const char*>(audio_data.data()), audio_data.size());

    out.close();

    std::cout << "\nSuccessfully packed " << index.size() << " phonemes" << std::endl;
    std::cout << "Total size: " << total_size << " bytes" << std::endl;

    return 0;
}

// =============================================================================
// Command Line Interface
// =============================================================================

void print_usage(const char* prog) {
    std::cout << "LaprdusTTS Phoneme Packer" << std::endl;
    std::cout << "Usage: " << prog << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --input-dir PATH    Input directory containing WAV files" << std::endl;
    std::cout << "  --output PATH       Output binary file path" << std::endl;
    std::cout << "  --encrypt           Enable XOR encryption" << std::endl;
    std::cout << "  --key HEXSTRING     Encryption key (64 hex chars, or auto-generate)" << std::endl;
    std::cout << "  --help              Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << prog << " --input-dir phonemes --output phonemes.bin" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string input_dir;
    std::string output_file;
    bool encrypt = false;
    std::string key;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--input-dir" && i + 1 < argc) {
            input_dir = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--encrypt") {
            encrypt = true;
        } else if (arg == "--key" && i + 1 < argc) {
            key = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_dir.empty() || output_file.empty()) {
        std::cerr << "Error: --input-dir and --output are required" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return pack_phonemes(input_dir, output_file, encrypt, key);
}
