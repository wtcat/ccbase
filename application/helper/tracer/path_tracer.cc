/*
 * Copyright 2023 wtcat 
 */
#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "path_tracer.h"

#pragma comment (lib,"imagehlp.lib")

namespace tools {

class cc_pathtracer {
public:
    enum {
        BACKTRACE_MAX_LIMIT = 64
    };
    struct cc_pathnode {
        void* ip[BACKTRACE_MAX_LIMIT];
        size_t ip_size;
        void* ptr;
        size_t size;
        void *user;
    };
    struct cc_heapnode {
        std::string name;
        void* start;
        size_t size;
        size_t peak_size;
        size_t sum_size;
        std::vector<cc_pathnode *> vec;
        void (*print)(void *fout, const void *);
    };
    cc_pathtracer(int min_limit, int max_limit, const std::string &sep) :
        min_limit_(min_limit),
        max_limit_(max_limit),
        sep_(sep) {
    }
    cc_pathtracer() :
        min_limit_(1),
        max_limit_(20),
        sep_("/") {
    }
    ~cc_pathtracer() = default;
    cc_pathtracer(const cc_pathtracer&) = delete;
    cc_pathtracer& operator=(const cc_pathtracer&) = delete;

    void add_heap(const std::string& str, void* start, size_t size,
        void (*print)(void *fout, const void *));
    bool add(void* addr, size_t size, void *user_data);
    bool del(void* addr);
    cc_pathnode *find(void *addr);
    void dump(FILE *fp);
    void set_minlimit(int min_limit) {
        min_limit_ = min_limit;
    }
    void set_maxlimit(int max_limit) {
        max_limit_ = max_limit;
    }
    void set_sep(const std::string& s) {
        sep_ = s;
    }
    void set_memcheck_hook(const memcheck_hook_t hook) {
        memchk_hook_ = hook;
    }

private:
    void transform_node(cc_pathnode*, void *, std::string *);
    cc_heapnode* heap_match(void* start);

private:
    std::map<void *, cc_pathnode> container_;
    int min_limit_;
    int max_limit_;
    std::string sep_;
    std::vector<cc_heapnode> heap_;
    std::mutex mtx_;

    static memcheck_hook_t memchk_hook_;
};

memcheck_hook_t cc_pathtracer::memchk_hook_;

cc_pathtracer::cc_heapnode* cc_pathtracer::heap_match(void* start) {
    for (int i = 0; i < heap_.size(); i++) {
        if ((char*)start >= (char*)heap_[i].start &&
            (char*)start < (char*)heap_[i].start + heap_[i].size)
            return &heap_[i];
    }
    return nullptr;
}

void cc_pathtracer::add_heap(const std::string& str,
    void* start, size_t size,
    void (*print)(void *fout, const void *)) {
    cc_heapnode heap;
    heap.name = str;
    heap.start = start;
    heap.size = size;
    heap.print = print;
    heap.peak_size = 0;
    heap.sum_size = 0;
    std::lock_guard<std::mutex> lock(mtx_);
    heap_.push_back(heap);
}

bool cc_pathtracer::add(void* addr, size_t size, void *user_data) {
    void* ip_array[BACKTRACE_MAX_LIMIT];
    if (!addr)
        return false;
    std::lock_guard<std::mutex> lock(mtx_);
    size_t ret = CaptureStackBackTrace(min_limit_, max_limit_, ip_array, NULL);
    if (ret) {
        cc_heapnode *heap = heap_match(addr);
        cc_pathnode node;

        memcpy(node.ip, ip_array, sizeof(void*) * ret);
        heap->sum_size += size;
        if (heap->sum_size > heap->peak_size)
            heap->peak_size = heap->sum_size;
        node.ip_size = ret;
        node.ptr = addr;
        node.size = size;
        node.user = user_data;
        container_.insert(std::make_pair(addr, node));
        return true;
    }
    return false;
}

bool cc_pathtracer::del(void* addr) {
    if (!addr)
        return false;
    std::lock_guard<std::mutex> lock(mtx_);
    auto iter = container_.find(addr);
    if (iter != container_.end()) {
        cc_heapnode* heap = heap_match(addr);
        heap->sum_size -= iter->second.size;
        container_.erase(addr);
        return true;
    }
    return false;
}

cc_pathtracer::cc_pathnode *
cc_pathtracer::find(void *addr) {
    if (addr) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto iter = container_.find(addr);
        if (iter != container_.end())
            return &iter->second;
    }
    return nullptr;
}

void cc_pathtracer::dump(FILE *fp) {
    HANDLE hproc = GetCurrentProcess();
    std::map<std::string, cc_heapnode *> heap_container;
    std::string path;
    std::lock_guard<std::mutex> lock(mtx_);

    //Preppare
    for (auto& iter : container_) {
        cc_pathnode* node = &iter.second;
        cc_heapnode* heap = heap_match(node->ptr);
        if (heap != nullptr) {
            auto iter = heap_container.find(heap->name);
            if (iter == heap_container.end()) 
                heap_container.insert(std::make_pair(heap->name, heap));
            heap->vec.push_back(node);
        }
    }

    auto print = [&](cc_pathnode* node, const std::string &path,
        const cc_heapnode* heap) -> void {
        fprintf(fp, "** ( %s ) ", heap->name.c_str());
        if (heap->print)
            heap->print(fp, node->user);
        fprintf(fp, " Memory <%p  %d>\n", node->ptr, node->size);
        fprintf(fp, "  %s \n\n", path.c_str());
    };

    SymInitialize(hproc, NULL, TRUE);
    fprintf(fp, "******************************************************\n");
    fprintf(fp, "                   MemTracer Dump                     \n");
    fprintf(fp, "******************************************************\n");
    for (auto &iviter : heap_container) {
        size_t sum = 0;
        fprintf(fp, "\n************* %s *************\n\n", iviter.second->name.c_str());
        for (auto node : iviter.second->vec) {
            path.clear();
            transform_node(node, (void*)hproc, &path);
            print(node, path, iviter.second);
            if (memchk_hook_)
                memchk_hook_(path.c_str(), node->ptr, node->size);
            sum += node->size;
        }

        fprintf(fp, "******" "%s" "Size: %d B ", iviter.second->name.c_str(), sum);
        fprintf(fp, " %.2f KB  %.4f MB  PeakSize(%.2f KB)", (float)sum / 1024, 
            (float)sum / (1024 * 1024), (float)iviter.second->peak_size / 1024);
        fprintf(fp, " ******\n\n");

        //Clear container
        iviter.second->vec.clear();
    }
    heap_container.clear();
}

void cc_pathtracer::transform_node(cc_pathnode *node, void *ctx,std::string *out) {
    unsigned long buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO* symbol;
    
    symbol = (SYMBOL_INFO*)buffer;
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for (int i = (int)node->ip_size - 1; i >= 0; i--) {
        void* addr = node->ip[i];
        if (addr == nullptr)
            continue;
        SymFromAddr((HANDLE)ctx, (DWORD64)addr, 0, symbol);
        symbol->Name[symbol->NameLen] = '\0';
        out->append(symbol->Name);
        out->append(sep_);
    }
}

} //namespace tools


//C-Like API
static tools::cc_pathtracer _tracer;

extern "C" void mempath_init(int min_limit, int max_limit, const char *sep) {
    static bool inited;
    if (!inited) {
        inited = true;
        if (min_limit < max_limit) {
            _tracer.set_minlimit(min_limit);
            _tracer.set_maxlimit(max_limit - min_limit);
        }
        if (sep)
            _tracer.set_sep(sep);
    }
}

extern "C" void mempath_add_heap(const char* heap, void* addr, size_t size,
    void (*print)(void *fout, const void *)) {
    _tracer.add_heap(heap, addr, size, print);
}

extern "C" bool mempath_add(void* addr, size_t size, void *user) {
    return _tracer.add(addr, size, user);
}

extern "C" size_t mempath_getsize(void* addr) {
    const tools::cc_pathtracer::cc_pathnode *node = _tracer.find(addr);
    if (node)
        return node->size;
    return 0;
}

extern "C" bool mempath_del(void* addr) {
    return _tracer.del(addr);
}

extern "C" void mempath_dump_std(void) {
    return _tracer.dump(stdout);
}

extern "C" void mempath_dump_file(const char *filename) {
    if (!filename)
        return;
    FILE* fp = fopen(filename, "w");
    if (fp != nullptr) {
        _tracer.dump(fp);
        fclose(fp);
    }
}

extern "C" void mempath_set_memcheck_hook(const memcheck_hook_t hook) {
    _tracer.set_memcheck_hook(hook);
}
