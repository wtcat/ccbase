/*
 * Copyright 2026 wtcat
 */

#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb/stb_image.h>

#include "application/regen/resource_file.h"
#include "application/helper/utils.h"

#include "base/bind.h"
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/linked_list.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/threading/thread.h"
#include "base/command_line.h"
#include "base/threading/simple_thread.h"

#include "thirdparty/lz4/lib/lz4.h"

namespace {

class RGBConvertor {
public:
    RGBConvertor(const std::string& s) : name_(s) {}
    const std::string& name() const {
        return name_;
    }
    virtual bool Convert(const uint8_t* src, int w, int h, int channels, int format, uint8_t* outpx) {
        switch (format) {
        case PIXEL_FORMAT_RGB888:
            for (int i = 0; i < w * h; i++) {
                outpx[i * 3 + 0] = src[i * channels + 0]; // R
                outpx[i * 3 + 1] = src[i * channels + 1]; // G
                outpx[i * 3 + 2] = src[i * channels + 2]; // B
            }
            break;
        case PIXEL_FORMAT_ARGB888:
            for (int i = 0; i < w * h; i++) {
                outpx[i * 4 + 0] = src[i * channels + 0]; // R
                outpx[i * 4 + 1] = src[i * channels + 1]; // G
                outpx[i * 4 + 2] = src[i * channels + 2]; // B
                outpx[i * 4 + 3] = src[i * channels + 3]; // A
            }
            break;
        case PIXEL_FORMAT_RGB565:
            for (int i = 0; i < w * h; i++) {
                uint8_t r = src[i * channels + 0];
                uint8_t g = src[i * channels + 1];
                uint8_t b = src[i * channels + 2];
                uint16_t rgb565 = ToRGB565(r, g, b);

                outpx[i * 2 + 0] = rgb565 & 0xFF;
                outpx[i * 2 + 1] = rgb565 >> 8;
            }
            break;
        case PIXEL_FORMAT_ARGB565:
            for (int i = 0; i < w * h; i++) {
                uint8_t r = src[i * channels + 0];
                uint8_t g = src[i * channels + 1];
                uint8_t b = src[i * channels + 2];
                uint16_t rgb565 = ToRGB565(r, g, b);

                outpx[i * 3 + 0] = rgb565 & 0xFF;
                outpx[i * 3 + 1] = rgb565 >> 8;
                outpx[i * 3 + 2] = src[i * channels + 3];
            }
            break;
        default:
            return false;
        }
        return true;
    }
    virtual bool Compress(const uint8_t* src, size_t size, uint8_t* dst, size_t* dst_size, int comp) {
        if (comp == REFILE_COMPRESS_LZ4) {
            int max_size = LZ4_compressBound((int)size);
            if (*dst_size < max_size)
                return false;

            int ret = LZ4_compress_default((char*)src, (char*)dst, (int)size, (int)*dst_size);
            if (ret <= 0)
                return false;

            *dst_size = ret;
            return true;
        }
        return false;
    }
    virtual size_t GetFormatSize(int *format, int channel) {
        bool alpha = channel == 4;
    _repeat:
        switch (*format) {
        case PIXEL_FORMAT_RGB888:
            if (alpha) {
                *format = PIXEL_FORMAT_ARGB888;
                goto _repeat;
            }
            return 3;

        case PIXEL_FORMAT_ARGB888:
            if (!alpha) {
                *format = PIXEL_FORMAT_RGB888;
                goto _repeat;
            }
            return 4;

        case PIXEL_FORMAT_RGB565:
            if (alpha) {
                *format = PIXEL_FORMAT_ARGB565;
                goto _repeat;
            }
            return 2;

        case PIXEL_FORMAT_ARGB565: 
            if (!alpha) {
                *format = PIXEL_FORMAT_RGB565;
                goto _repeat;
            }
            return 3;

        default: 
            return 0;
        }
    }

protected:
    inline uint16_t ToRGB565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

private:
    std::string name_;
};

class FileResource {
public:
    typedef struct refile_data PixelNode;
    enum { kMaxFileName = 64 };
    struct FileNode {
        FileNode(const FilePath& p) : path(p) {}
        FilePath path;
        size_t   size = 0;
        uint32_t key = 0;
        char     keyname[kMaxFileName] = {};
    };
    struct ImageNode : public FileNode {
        ImageNode(const FilePath& p) : FileNode(p) {}
        int16_t compress = -1;
        int16_t format = -1;
        PixelNode* payload = nullptr;
    };
    FileResource(RGBConvertor* rgb, int fmt, int comp)
        : rgb_impl_(rgb), format_(fmt), compress_(comp) {
        images_.reserve(256);
    }
    ~FileResource() {
        for (auto& iter : images_) {
            if (iter.payload != nullptr) {
                delete [] iter.payload;
                iter.payload = nullptr;
            }
        }
    }

    size_t CollectFiles(const FilePath& dir);
    FileResource& SubmitWork(size_t nr_threads);
    int GenerateResFile(const FilePath& path);

private:
    bool Format(ImageNode& img, int format, int comp);
    uint32_t NameHash(const uint8_t* key, uint32_t len);
    int GenerateImageSymbol(const FilePath& path);

private:
    friend class Worker;
    DISALLOW_COPY_AND_ASSIGN(FileResource);
    std::vector<ImageNode> images_;
    RGBConvertor* rgb_impl_;
    int format_ = 0;
    int compress_ = 0;
};

class Worker : public base::DelegateSimpleThread::Delegate {
public:
    Worker(FileResource* fres, size_t id, size_t nr) :
        fres_(fres), id_(id), nr_(nr) {
    }

    void Run() OVERRIDE {
        for (size_t i = id_; i < nr_; i++)
            fres_->Format(fres_->images_[i], fres_->format_, fres_->compress_);
        delete this;
    }

private:
    FileResource* fres_;
    size_t id_;
    size_t nr_;
};

uint32_t FileResource::NameHash(const uint8_t* key, uint32_t len) {
    uint32_t hash = 0;
    for (const uint8_t* end = key + len;
        key < end; key++) {
        hash *= 16777619;
        hash ^= (uint32_t)(*key);
    }
    return hash;
}

int FileResource::GenerateImageSymbol(const FilePath& path) {
    FilePath filename = path.RemoveExtension().AddExtension(L".h");
   
    std::sort(images_.begin(), images_.end(), [](const ImageNode& a, const ImageNode& b) {
        return std::strcmp(a.keyname, b.keyname) < 0;
        });

    std::string buf;
    buf.reserve(4096);
    std::string header = filename.BaseName().RemoveExtension().AsUTF8Unsafe();
    std::transform(header.begin(), header.end(), header.begin(), [](unsigned char c)
        { return std::toupper(c); });

    char temp[256];
    snprintf(temp, sizeof(temp), "#ifndef %s_H_\n#define %s_H_\n\n", 
        header.c_str(), header.c_str());
    buf.append(temp);

    for (const auto& iter : images_) {
        snprintf(temp, sizeof(buf), "#define %s 0x%x\n", iter.keyname, iter.key);
        buf.append(temp);
    }
    buf.append("\n#endif");

    return file_util::WriteFile(filename, buf.data(), (int)buf.size());
}

size_t FileResource::CollectFiles(const FilePath& dir) {
    helper::FileCollect(dir, true, ".*\\.(jpg|jpeg|png|bmp)$", [&](const FilePath& path) {
        images_.push_back(ImageNode(path));
        });
    return images_.size();
}

bool FileResource::Format(ImageNode& img, int format, int comp) {
    if (img.format >= 0)
        format = img.format;
    if (img.compress >= 0)
        comp = img.compress;

    // Load picture RGB channels 
    int width, height, channels;
    uint8_t* px = stbi_load(img.path.AsUTF8Unsafe().c_str(), &width, &height, &channels, 0);
    if (!px) {
        printf("stb error: %s", stbi_failure_reason());
        return false;
    }

    // Get the size of RGB format
    size_t alloc_size = rgb_impl_->GetFormatSize(&format, channels);
    if (alloc_size == 0)
        return false;

    alloc_size = alloc_size * width * height;

    // TODO: Should to aligned allocate
    int index = comp != REFILE_COMPRESS_NONE ? 1 : 0;
    PixelNode* ptr = (PixelNode*)(new uint8_t[sizeof(PixelNode) + alloc_size * (index + 1)]);
    uint8_t* cptr = (uint8_t*)(ptr + 1);
    uint8_t* optr = cptr + alloc_size * index;

    // Convert pixel format
    if (!rgb_impl_->Convert(px, width, height, channels, format, optr)) {
        printf("Failed to convert file(%s)\n", img.path.AsUTF8Unsafe().c_str());
        stbi_image_free(px);
        return false;
    }

    if (comp != REFILE_COMPRESS_NONE) {
        size_t dst_size = alloc_size * 2;
        if (!rgb_impl_->Compress(optr, alloc_size, cptr, &dst_size, comp)) {
            printf("Failed to compress file(%s)\n", img.path.AsUTF8Unsafe().c_str());
            stbi_image_free(px);
            return false;
        }
        alloc_size = dst_size;
    }

    ptr->width = (uint32_t)width;
    ptr->height = (uint32_t)height;
    ptr->size = (uint32_t)alloc_size;
    ptr->format = (uint16_t)format;
    ptr->compress = (uint16_t)comp;

    // Extract base name and convert to Upper
    std::string name = img.path.BaseName().RemoveExtension().AsUTF8Unsafe();
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) 
        { return std::toupper(c); });

    img.key = NameHash((uint8_t*)name.c_str(), (uint32_t)name.size());
    std::strncpy(img.keyname, name.c_str(), kMaxFileName - 1);
    img.size = sizeof(PixelNode) + alloc_size;
    img.payload = ptr;

    return true;
}

FileResource& FileResource::SubmitWork(size_t nr_threads) {
    if (nr_threads > 8)
        nr_threads = 8;

    size_t nr_images = images_.size();
    size_t avg = nr_images / nr_threads;
    if (nr_images % nr_threads)
        avg++;

    //Dispatch works
    base::DelegateSimpleThreadPool thread_pool("refile", (int)nr_threads);
    for (size_t i = 0, next = 0; i < nr_threads; i++) {
        size_t end = next + avg;
        if (end > nr_images)
            end = nr_images;
        thread_pool.AddWork(new Worker(this, next, end));
        if (end == nr_images)
            break;

        next = end;
    }
    thread_pool.Start();

    //Waiting for worker complete
    thread_pool.JoinAll();

    return *this;
}

int FileResource::GenerateResFile(const FilePath& path) {
    //Sort images
    std::sort(images_.begin(), images_.end(), [](const ImageNode& a, const ImageNode& b) {
        return a.key < b.key;
        });

    //Check key conflict
    size_t bin_size = images_[0].size;
    for (size_t i = 1, j = 0; i < images_.size(); i++, j++) {
        if (images_[i].key == images_[j].key) {
            printf("Error: File(%s : %s) hash conflict\n",
                images_[i].path.AsUTF8Unsafe().c_str(),
                images_[j].path.AsUTF8Unsafe().c_str());
            return false;
        }
        bin_size += images_[i].size;
    }

    bin_size += sizeof(struct refile_index) * images_.size();
    bin_size += sizeof(struct refile_header);

    auto ptr = std::make_unique<uint8_t[]>(bin_size);

    // Build header
    struct refile_header* pheader = (struct refile_header*)ptr.get();
    strncpy((char*)pheader->magic, REFILE_FILE_MAGIC, sizeof(pheader->magic) - 1);
    pheader->magic[sizeof(pheader->magic) - 1] = '\0';
    pheader->version = 0;
    pheader->count = (uint32_t)images_.size();

    uint32_t offset = sizeof(struct refile_header) + pheader->count * sizeof(struct refile_index);
    for (uint32_t i = 0; i < pheader->count; i++) {
        // Copy binary data
        size_t dsize = images_[i].size;
        memcpy(ptr.get() + offset, images_[i].payload, dsize);

        // Set binary offset and key
        pheader->indexs[i].namekey = images_[i].key;
        pheader->indexs[i].offset = offset;
        pheader->indexs[i].size = (uint32_t)dsize;

        // Update data offset
        offset += (uint32_t)dsize;
    }

    pheader->chksum = helper::crc32_ieee_update(
        0,
        ptr.get() + sizeof(struct refile_header),
        offset - sizeof(struct refile_header));

    /* Flush to binary file */
    int err = file_util::WriteFile(path, (const char*)ptr.get(), offset);
    if (err < 0)
        return err;

    /* Generate symbol file */
    return GenerateImageSymbol(path);
}

} //namespace

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();
        if (cmdline->HasSwitch("help")) {
            printf(
                "regen "
                "[--dir=path]"
                "[--out=file]\n"
                "[--job=number]\n"
                "[--format=value]"
                "[--compress=value]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
                "  --job      The number of threads\n"
                "  --out      The output files\n"
                "  --format   The target format((a)rgb565,(a)rgb888)\n"
                "  --compress Compress algorithm(none, lz4)\n"
            );

            return 0;
        }

        FilePath dir(L"IMG");
        FilePath out(L"res.bin");
        int format = PIXEL_FORMAT_RGB565;
        int compress = REFILE_COMPRESS_LZ4;
        int jobs = 2;

        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");

        if (!file_util::PathExists(dir)) {
            printf("Not found path(%s)\n", dir.AsUTF8Unsafe().c_str());
            return -1;
        }

        if (cmdline->HasSwitch("out"))
            out = cmdline->GetSwitchValuePath("out");

        if (cmdline->HasSwitch("job")) {
            int nr_jobs = std::stoi(cmdline->GetSwitchValueASCII("job"));
            if (nr_jobs != 0)
                jobs = nr_jobs;
        }

        if (cmdline->HasSwitch("format"))
            format = std::stoi(cmdline->GetSwitchValueASCII("format"));

        if (cmdline->HasSwitch("compress"))
            compress = std::stoi(cmdline->GetSwitchValueASCII("compress"));

        RGBConvertor conv("default");
        FileResource fres(&conv, format, compress);

        if (fres.CollectFiles(dir) == 0) {
            printf("Not found any pictures\n");
            return -EINVAL;
        }

        return fres.SubmitWork(jobs).GenerateResFile(out);
    }

    return 0;
}
