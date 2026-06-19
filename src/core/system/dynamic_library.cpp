#include <NeForce/core/system/dynamic_library.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <libloaderapi.h>
#    include <winnt.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <linux/limits.h>
#    include <dlfcn.h>
#    include <elf.h>
#    include <link.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

void dynamic_library::open() {
    if (handle_ != nullptr) {
        return;
    }
    if (path_.empty()) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("trying to open a empty dynamic library."));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD flags = 0;
    const int mode_val = static_cast<int>(load_mode_);
    if ((mode_val & static_cast<int>(load_mode::now)) != 0) {
        flags |= LOAD_WITH_ALTERED_SEARCH_PATH;
    }

    handle_ = ::LoadLibraryExA(path_.data(), nullptr, flags);
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("dynamic library load failed."));
    }
#else
    int flags = RTLD_LAZY | RTLD_LOCAL;
    const int mode_val = static_cast<int>(load_mode_);

    if ((mode_val & static_cast<int>(load_mode::now)) != 0) {
        flags &= ~RTLD_LAZY;
        flags |= RTLD_NOW;
    }
    if ((mode_val & static_cast<int>(load_mode::global)) != 0) {
        flags &= ~RTLD_LOCAL;
        flags |= RTLD_GLOBAL;
    }
    if ((mode_val & static_cast<int>(load_mode::deep_bind)) != 0) {
        flags |= RTLD_DEEPBIND;
    }

    handle_ = ::dlopen(path_.data(), flags);
    if (handle_ == nullptr) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        const char* err = ::dlerror();
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception(err));
    }
#endif
}

void dynamic_library::close() {
    if (handle_ != nullptr) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::FreeLibrary(static_cast<::HMODULE>(handle_));
#else
        ::dlclose(handle_);
#endif
        handle_ = nullptr;
    }
}

dynamic_library::dynamic_library(string pth) :
path_(move(pth)) {
    open();
}

dynamic_library::dynamic_library(string pth, const load_mode mode) :
path_(move(pth)),
load_mode_(mode) {
    open();
}

dynamic_library::dynamic_library(dynamic_library&& other) noexcept :
handle_(other.handle_),
path_(move(other.path_)),
load_mode_(other.load_mode_) {
    other.handle_ = nullptr;
    other.load_mode_ = load_mode::default_;
}

dynamic_library& dynamic_library::operator=(dynamic_library&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();
    handle_ = other.handle_;
    path_ = move(other.path_);
    load_mode_ = other.load_mode_;
    other.handle_ = nullptr;
    other.load_mode_ = load_mode::default_;
    return *this;
}

dynamic_library::~dynamic_library() { close(); }

void* dynamic_library::symbol(const string& name) const {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("Library not loaded"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::FARPROC proc = ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data());
    if (proc == nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("GetProcAddress failed"));
    }
    return reinterpret_cast<void*>(proc);
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    ::dlerror();
    void* sym = ::dlsym(handle_, name.data());
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* error = ::dlerror();
    if (error != nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception(error));
    }
    return sym;
#endif
}

bool dynamic_library::has_symbol(const string& name) const noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data()) != nullptr;
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    ::dlerror();
    ignore = ::dlsym(handle_, name.data());
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* error = ::dlerror();
    return error == nullptr;
#endif
}

dynamic_library dynamic_library::load_self() {
    dynamic_library lib;
#ifdef NEFORCE_PLATFORM_WINDOWS
    lib.handle_ = ::GetModuleHandleA(nullptr);
    lib.path_ = "";
    lib.load_mode_ = load_mode::default_;
#else
    lib.handle_ = ::dlopen(nullptr, RTLD_LAZY | RTLD_LOCAL);
    lib.path_ = "";
    lib.load_mode_ = load_mode::default_;
#endif
    return lib;
}

dynamic_library dynamic_library::load_by_name(const string& name, load_mode mode) {
    string resolved_name = name;
    if (name.find('/') == string::npos && name.find('\\') == string::npos) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (name.size() < 4 || !name.ends_with(".dll")) {
            resolved_name = name + ".dll";
        }
#else
        if (!name.starts_with("lib")) {
            resolved_name = "lib" + name;
        }
        if (name.size() < 3 || (!name.ends_with(".so") && name.find(".so.") == string::npos)) {
            resolved_name += ".so";
        }
#endif
    }
    return {resolved_name, mode};
}

vector<string> dynamic_library::list_symbols(const string& name_filter) const {
    vector<string> symbols;
    if (!is_open()) {
        return symbols;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    ::link_map* lm = nullptr;
    if (::dlinfo(handle_, RTLD_DI_LINKMAP, &lm) != 0 || lm == nullptr) {
        return symbols;
    }

    const auto* dyn = reinterpret_cast<const ElfW(Dyn)*>(lm->l_ld);
    const char* strtab = nullptr;
    const ElfW(Sym)* symtab = nullptr;
    const ElfW(Word)* hash_table = nullptr;
    const ElfW(Word)* gnu_hash_table = nullptr;
    size_t syment = 0;

    for (; dyn->d_tag != DT_NULL; ++dyn) {
        if (dyn->d_tag == DT_STRTAB) {
            strtab = reinterpret_cast<const char*>(dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_SYMTAB) {
            symtab = reinterpret_cast<const ElfW(Sym)*>(dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_SYMENT) {
            syment = dyn->d_un.d_val;
        } else if (dyn->d_tag == DT_HASH) {
            hash_table = reinterpret_cast<const ElfW(Word)*>(dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_GNU_HASH) {
            gnu_hash_table = reinterpret_cast<const ElfW(Word)*>(dyn->d_un.d_ptr);
        }
    }

    size_t nchain = 0;
    if (hash_table != nullptr) {
        nchain = static_cast<size_t>(hash_table[1]);
    } else if (gnu_hash_table != nullptr) {
        const uint32_t nbuckets = gnu_hash_table[0];
        const uint32_t symoffset = gnu_hash_table[1];
        const uint32_t bloom_size = gnu_hash_table[2];

        const size_t bloom_words = bloom_size;
        const uint32_t* buckets = &gnu_hash_table[4 + bloom_words * (sizeof(ElfW(Addr)) / sizeof(uint32_t))];
        const uint32_t* chains = &buckets[nbuckets];

        uint32_t max_idx = symoffset;

        for (uint32_t i = 0; i < nbuckets; ++i) {
            const uint32_t bidx = buckets[i];
            if (bidx < symoffset) {
                continue;
            }
            if (bidx > max_idx) {
                max_idx = bidx;
            }
            uint32_t ci = bidx - symoffset;
            while ((chains[ci] & 1) == 0) {
                ++ci;
            }
            const uint32_t last_in_chain = symoffset + ci;
            if (last_in_chain > max_idx) {
                max_idx = last_in_chain;
            }
        }
        nchain = static_cast<size_t>(max_idx) + 1;
    }

    if (strtab != nullptr && symtab != nullptr && syment > 0 && nchain > 0) {
        for (size_t i = 1; i < nchain; ++i) {
            const auto* sym = reinterpret_cast<const ElfW(Sym)*>(reinterpret_cast<const char*>(symtab) + i * syment);
            string_view sym_name = strtab + sym->st_name;
            if (sym_name[0] != '\0' && ELF32_ST_TYPE(sym->st_info) == STT_FUNC) {
                if (name_filter.empty() || sym_name.contains(name_filter.view())) {
                    symbols.push_back(sym_name);
                }
            }
        }
    }
#else
    if (handle_ == nullptr) {
        return symbols;
    }

    const auto* base = static_cast<const ::BYTE*>(static_cast<const void*>(handle_));
    const auto* dos = reinterpret_cast<const ::IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return symbols;
    }

    const auto* nt = reinterpret_cast<const ::IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return symbols;
    }

    const ::DWORD export_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (export_rva == 0) {
        return symbols;
    }

    const auto* exports = reinterpret_cast<const ::IMAGE_EXPORT_DIRECTORY*>(base + export_rva);
    const auto* names = reinterpret_cast<const ::DWORD*>(base + exports->AddressOfNames);
    const auto* ordinals = reinterpret_cast<const ::WORD*>(base + exports->AddressOfNameOrdinals);
    const auto* functions = reinterpret_cast<const ::DWORD*>(base + exports->AddressOfFunctions);

    for (::DWORD i = 0; i < exports->NumberOfNames; ++i) {
        string_view sym_name = reinterpret_cast<const char*>(base + names[i]);
        if (sym_name[0] == '\0') {
            continue;
        }
        const ::DWORD func_rva = functions[ordinals[i]];
        if (func_rva >= export_rva &&
            func_rva < export_rva + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
            continue;
        }
        if (name_filter.empty() || sym_name.find(name_filter.view()) != string::npos) {
            symbols.push_back(sym_name);
        }
    }
#endif

    return symbols;
}

string dynamic_library::program_location() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    const ::DWORD len = ::GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    if (len == 0 || len >= sizeof(buffer)) {
        return "";
    }
    return {buffer, static_cast<size_t>(len)};
#else
    char buffer[PATH_MAX];
    const ssize_t len = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        return "";
    }
    buffer[len] = '\0';
    return {buffer, static_cast<size_t>(len)};
#endif
}

string dynamic_library::symbol_location(void* symbol_ptr) {
    if (symbol_ptr == nullptr) {
        return "";
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::HMODULE hModule = nullptr;
    if (::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             static_cast<const char*>(symbol_ptr), &hModule) == FALSE ||
        hModule == nullptr) {
        return "";
    }
    char buffer[MAX_PATH];
    const ::DWORD len = ::GetModuleFileNameA(hModule, buffer, sizeof(buffer));
    if (len == 0 || len >= sizeof(buffer)) {
        return "";
    }
    return {buffer, static_cast<size_t>(len)};
#else
    ::Dl_info info;
    if (::dladdr(symbol_ptr, &info) == 0 || info.dli_fname == nullptr) {
        return "";
    }
    return {info.dli_fname};
#endif
}

NEFORCE_END_NAMESPACE__
