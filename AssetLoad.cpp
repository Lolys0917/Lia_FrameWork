#include "AssetLoad.h"
#include "Main.h" // あなたの既存関数（IN_LoadTexture, IN_LoadFBX等）が宣言されている場所
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

#define SafeRelease(p) if(p){ (p)->Release(); (p)=nullptr; }

namespace fs = std::filesystem;

// --- パッケージ内エントリ（テーブル保持用） ---
struct PackageEntry {
    std::string name;        // 相対パス/ファイル名（登録時の文字列）
    uint64_t offset = 0;     // pkg 内オフセット（保存時に決まる）
    uint64_t size = 0;       // バイナリサイズ
    std::vector<uint8_t> data; // バイナリ（Startup時のバッチ段階のみ使用、pkg書き出し後はクリア可能）
};

// --- Package管理構造体 ---
struct Package {
    std::string ext;         // "png", "fbx", ...
    KeyMap keymap;           // Manager.h の KeyMap を使う（ファイル名->index）
    std::vector<PackageEntry> entries;
    // 実行時ロード時に .pkg を開いた場合のファイルパス保持
    std::string pkgPath;
    std::ifstream pkgStream; // opened when AL_LoadPackageIndex called
};

static std::vector<Package> g_packages;

// ヘルパー: ext 小文字化
static std::string ToLowerExt(const std::string& s) {
    std::string e = s;
    for (auto& c : e) c = (char)tolower(c);
    return e;
}

// ヘルパー: パッケージ検索（by ext）
static Package* FindPackageByExt(const std::string& ext) {
    for (auto& p : g_packages) {
        if (p.ext == ext) return &p;
    }
    return nullptr;
}

// --- 初期化 / 終了 ---
void AL_Init() {
    g_packages.clear();
}

void AL_Shutdown() {
    // close streams
    for (auto& p : g_packages) {
        if (p.pkgStream.is_open()) p.pkgStream.close();
        KeyMap_Free(&p.keymap);
    }
    g_packages.clear();
}

// --- Startupでの登録（メモリにバイナリを保持） ---
bool AL_RegisterAssetToBatch(const char* filepath) {
    if (!filepath) return false;
    std::string path = filepath;
    std::string ext = fs::path(path).extension().string();
    if (ext.size() && ext[0] == '.') ext.erase(0, 1);
    ext = ToLowerExt(ext);
    if (ext.empty()) return false;

    // find or create package
    Package* pkg = FindPackageByExt(ext);
    if (!pkg) {
        Package np;
        np.ext = ext;
        KeyMap_Init(&np.keymap);
        g_packages.push_back(std::move(np));
        pkg = &g_packages.back();
    }

    // Check if already registered in this package (KeyMap_GetIndex)
    int existing = KeyMap_GetIndex(&pkg->keymap, path.c_str());
    if (existing != -1) return true; // already registered

    // read file binary
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    std::streamsize size = in.tellg();
    in.seekg(0);
    PackageEntry e;
    e.name = path;
    if (size > 0) {
        e.size = (uint64_t)size;
        e.data.resize((size_t)size);
        in.read((char*)e.data.data(), size);
    }
    else {
        e.size = 0;
    }
    in.close();

    // register in keymap and push
    KeyMap_Add(&pkg->keymap, e.name.c_str());
    pkg->entries.push_back(std::move(e));
    return true;
}

// --- 書き出し：各 ext ごとに .pkg を作る ---
// フォーマット（簡略）:
// [magic(8)] [count(uint32)] [tableOffset(uint64)]
// table: repeated ( nameLen(uint16) | name | offset(uint64) | size(uint64) )
// data: concatenated blobs
bool AL_SaveAllPackages(const char* outFolder) {
    if (!outFolder) return false;
    fs::create_directories(outFolder);
    for (auto& pkg : g_packages) {
        std::string outPath = std::string(outFolder) + "/Asset" + pkg.ext + ".pkg";

        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return false;

        // header placeholders
        const char magic[8] = "LIA_PKG";
        uint32_t count = (uint32_t)pkg.entries.size();
        uint64_t tableOffset = 0;

        out.write(magic, 8);
        out.write((char*)&count, sizeof(uint32_t));
        tableOffset = 8 + 4 + 8;
        out.write((char*)&tableOffset, sizeof(uint64_t));

        // calculate table size and jump
        std::streampos tablePos = out.tellp();
        size_t tableSize = 0;
        for (auto& e : pkg.entries) {
            tableSize += sizeof(uint16_t) + e.name.size() + sizeof(uint64_t) * 2;
        }
        out.seekp(tablePos + (std::streamoff)tableSize);

        // write data and record offsets
        for (auto& e : pkg.entries) {
            std::streampos dataPos = out.tellp();
            e.offset = (uint64_t)dataPos;
            if (!e.data.empty()) out.write((char*)e.data.data(), e.data.size());
            // after writing we can optionally clear e.data to save RAM
            // keep it for now
        }

        // write table
        out.seekp(tablePos);
        for (auto& e : pkg.entries) {
            uint16_t len = (uint16_t)e.name.size();
            out.write((char*)&len, sizeof(uint16_t));
            out.write(e.name.data(), len);
            out.write((char*)&e.offset, sizeof(uint64_t));
            out.write((char*)&e.size, sizeof(uint64_t));
        }

        out.close();
    }
    return true;
}

// --- 実行時：pkgを開いてメタを読み込む（データはまだ読み込まない） ---
bool AL_LoadPackageIndex(const char* ext, const char* pkgFilePath) {
    if (!ext || !pkgFilePath) return false;
    std::string sExt = ToLowerExt(ext);
    Package* pkg = FindPackageByExt(sExt);
    if (!pkg) {
        // create empty package
        Package np;
        np.ext = sExt;
        KeyMap_Init(&np.keymap);
        g_packages.push_back(std::move(np));
        pkg = &g_packages.back();
    }
    else {
        // if already opened, close
        if (pkg->pkgStream.is_open()) pkg->pkgStream.close();
        pkg->entries.clear();
        KeyMap_Free(&pkg->keymap);
        KeyMap_Init(&pkg->keymap);
    }

    pkg->pkgPath = pkgFilePath;
    pkg->pkgStream.open(pkgFilePath, std::ios::binary);
    if (!pkg->pkgStream.is_open()) return false;

    // read header
    char magic[8];
    pkg->pkgStream.read(magic, 8);
    uint32_t count = 0;
    uint64_t tableOffset = 0;
    pkg->pkgStream.read((char*)&count, sizeof(uint32_t));
    pkg->pkgStream.read((char*)&tableOffset, sizeof(uint64_t));
    // jump to tableOffset
    pkg->pkgStream.seekg(tableOffset);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t nameLen = 0;
        pkg->pkgStream.read((char*)&nameLen, sizeof(uint16_t));
        std::string name(nameLen, '\0');
        pkg->pkgStream.read(name.data(), nameLen);
        uint64_t offset = 0, size = 0;
        pkg->pkgStream.read((char*)&offset, sizeof(uint64_t));
        pkg->pkgStream.read((char*)&size, sizeof(uint64_t));
        PackageEntry e;
        e.name = name;
        e.offset = offset;
        e.size = size;
        // do not load binary blob here
        KeyMap_Add(&pkg->keymap, e.name.c_str());
        pkg->entries.push_back(std::move(e));
    }

    return true;
}

// helper: find package & entry index by name (search across ext packages)
static bool FindPackageEntryByName(const std::string& name, Package*& outPkg, int& outIndex) {
    for (auto& p : g_packages) {
        int idx = KeyMap_GetIndex(&p.keymap, name.c_str());
        if (idx >= 0) { outPkg = &p; outIndex = idx; return true; }
    }
    outPkg = nullptr; outIndex = -1; return false;
}

// --- 実行時：pkgから index 指定でデータ抽出、既存の loader を呼ぶ ---
// 実装方針：一時ファイルへ書き出して既存のファイル版ローダへ渡す（既存コード活かす）
static bool WriteTempAndCallLoader(const PackageEntry& e) {
    if (e.name.empty()) return false;
    // determine ext
    std::string ext = fs::path(e.name).extension().string();
    if (ext.size() && ext[0] == '.') ext.erase(0, 1);
    ext = ToLowerExt(ext);

    //std::string tmpFolder = "saved/tmp/";
    //fs::create_directories(tmpFolder);
    //// create unique temp name using entry name sanitized
    //std::string base = fs::path(e.name).filename().string(); // e.g. "Player.png"
    //std::string tmpPath = tmpFolder + base;
    //
    //// if file exists, append counter
    //int c = 0;
    //while (fs::exists(tmpPath) && c < 1000) {
    //    tmpPath = tmpFolder + std::to_string(c) + "_" + base;
    //    ++c;
    //}
    //
    //// write binary
    //std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
    //if (!out.is_open()) return false;
    //if (!e.data.empty()) {
    //    out.write((char*)e.data.data(), e.data.size());
    //}
    //else {
    //    // if .data is empty, we may be in case where entries were loaded from .pkg (no memory)
    //    // In that case caller must have filled e.data or we need to read from pkg stream.
    //    // This function expects e.data valid.
    //    out.close();
    //    return false;
    //}
    //out.close();

    // call loader based on ext
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
    {
        return IN_LoadTexture_Memory(e.name.c_str(), e.data.data(), e.data.size());
    }
    else if (ext == "obj")
    {
        return IN_LoadModelObj_Memory(e.name.c_str(), e.data.data(), e.data.size());
    }
    else if (ext == "fbx")
    {
        return IN_LoadFBX_Memory(e.name.c_str(), e.data.data(), e.data.size());
    }
    else 
    {
        return false;
    }
}

bool AL_LoadFromPackageByName(const char* name) {
    if (!name) return false;
    Package* pkg = nullptr;
    int idx = -1;
    if (!FindPackageEntryByName(name, pkg, idx)) return false;

    if (!pkg) return false;
    if (idx < 0 || idx >= (int)pkg->entries.size()) return false;

    PackageEntry& e = pkg->entries[idx];

    // If data is empty, try to read from pkgStream (if opened)
    if (e.data.empty()) {
        if (!pkg->pkgStream.is_open()) {
            // try open by stored path
            pkg->pkgStream.open(pkg->pkgPath, std::ios::binary);
            if (!pkg->pkgStream.is_open()) return false;
        }
        pkg->pkgStream.seekg(e.offset);
        e.data.resize((size_t)e.size);
        pkg->pkgStream.read((char*)e.data.data(), (std::streamsize)e.size);
    }

    return WriteTempAndCallLoader(e);
}

bool AL_LoadFromPackageByIndex(const char* ext, int index) {
    if (!ext) return false;
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return false;
    if (index < 0 || index >= (int)pkg->entries.size()) return false;
    PackageEntry& e = pkg->entries[index];

    if (e.data.empty()) {
        if (!pkg->pkgStream.is_open()) {
            pkg->pkgStream.open(pkg->pkgPath, std::ios::binary);
            if (!pkg->pkgStream.is_open()) return false;
        }
        pkg->pkgStream.seekg(e.offset);
        e.data.resize((size_t)e.size);
        pkg->pkgStream.read((char*)e.data.data(), (std::streamsize)e.size);
    }

    return WriteTempAndCallLoader(e);
}

int AL_GetIndexFromPackage(const char* ext, const char* name) {
    if (!ext || !name) return -1;
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return -1;
    return KeyMap_GetIndex(&pkg->keymap, name);
}

// helpers to enumerate
int AL_GetPackageCount() { return (int)g_packages.size(); }
const char* AL_GetPackageExt(int pkgIdx) {
    if (pkgIdx < 0 || pkgIdx >= (int)g_packages.size()) return nullptr;
    return g_packages[pkgIdx].ext.c_str();
}
int AL_GetPackageEntryCount(const char* ext) {
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return 0;
    return (int)pkg->entries.size();
}
const char* AL_GetPackageEntryName(const char* ext, int index) {
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return nullptr;
    if (index < 0 || index >= (int)pkg->entries.size()) return nullptr;
    return pkg->entries[index].name.c_str();
}
