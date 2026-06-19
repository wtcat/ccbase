// Copyright 2026 wtcat

#pragma once

#include <algorithm>
#include <vector>

#include "sys/queue.h"
#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/threading/simple_thread.h"

#define RE_KEY_PREFIX   "reIMG__"
#define RE_GROUP_PREFIX "reIMG__GRP_"

class Worker;
class RGBConvertor;

class FileResource {
public:
    typedef struct refile_data PixelNode;
    enum { kMaxFileName = 128 };
    enum { kImageFileNode = 1, kImageGroupFileNode = 2 };

    struct FileNode {
        FileNode(const FilePath& p, int ftype) : path(p), type(ftype) {}
        TAILQ_ENTRY(FileNode) link = {};
        FilePath    path;
        std::string keyname;
        uint32_t    key = 0;
        int         type = 0;
        int         id = -1;
        void* payload = nullptr;
        size_t      size = 0;
    };
    TAILQ_HEAD(FileNodeList, FileNode);

    struct ImageNode : public FileNode, public base::RefCountedThreadSafe<ImageNode> {
        ImageNode(const FilePath& p) : FileNode(p, kImageFileNode) {}
        int16_t compress = -1;
        int16_t format = -1;
    };

    struct ImageGroup : public FileNode, public base::RefCountedThreadSafe<ImageGroup> {
        ImageGroup(const FilePath& path, int id) :
            FileNode(path, kImageGroupFileNode), gid(id) {
            images.reserve(32);

            keyname = path.BaseName().AsUTF8Unsafe();
            std::transform(keyname.begin(), keyname.end(), keyname.begin(),
                [](unsigned char c) {
                    return std::toupper(c);
                });
            keyname = RE_GROUP_PREFIX + keyname;
            key = FileResource::NameHash((const uint8_t*)keyname.c_str(), (uint32_t)keyname.size());
        }
        const std::vector<const ImageNode*>& sort_by_name() {
            std::sort(images.begin(), images.end(),
                [](const ImageNode* a, const ImageNode* b) {
                    return FileResource::NaturalLess(a->keyname, b->keyname);
                });
            return images;
        }
        const std::vector<const ImageNode*>& sort_by_key() {
            std::sort(images.begin(), images.end(),
                [](const ImageNode* a, const ImageNode* b) {
                    return a->key < b->key;
                });
            return images;
        }
        size_t header_size() const {
            return images.size() * sizeof(struct refile_bindex) + sizeof(struct refile_group);
        }
        size_t binary_size() const {
            size_t body_size = 0;
            for (const auto iter : images)
                body_size += iter->size;
            return header_size() + body_size;
        }

        std::vector<const ImageNode*> images;
        int gid;
    };

    FileResource(RGBConvertor* rgb, int fmt, int comp)
        : rgb_impl_(rgb), format_(fmt), compress_(comp) {
        images_.reserve(256);
    }
    ~FileResource() = default;

    size_t CollectFiles(const FilePath& dir);
    FileResource& SubmitWork(size_t nr_threads);
    int GenerateResFile(const FilePath& path);
    void CreateImageGroup(const FilePath& group_path) {
        scoped_refptr<ImageGroup> group(new ImageGroup(group_path, global_id_++));
        groups_.push_back(group);
    }
    void SetVerbose(bool en) {
        verbose_ = en;
    }

private:
    bool CheckConflict();
    bool Format(ImageNode& img, int format, int comp);
    int  GenerateImageSymbol(const FilePath& path);
    void CollectNodes(FileNodeList* list);
    void FillImageGroup();
    static uint32_t NameHash(const uint8_t* key, uint32_t len);
    static bool NaturalLess(const std::string& a, const std::string& b);

private:
    friend class Worker;
    friend struct ImageGroup;
    DISALLOW_COPY_AND_ASSIGN(FileResource);
    std::vector<scoped_refptr<ImageNode>> images_;
    std::vector<scoped_refptr<ImageGroup>> groups_;
    std::vector<FileNode*> sort_vector_;
    RGBConvertor* rgb_impl_;
    int format_ = 0;
    int compress_ = 0;
    int global_id_ = 0;
    bool verbose_ = false;
};

class Worker : public base::DelegateSimpleThread::Delegate {
public:
    Worker(FileResource* fres, size_t id, size_t nr) :
        fres_(fres), id_(id), nr_(nr) {
    }

    void Run() OVERRIDE {
        for (size_t i = id_; i < nr_; i++) {
            FileResource::ImageNode* node = fres_->images_[i].get();
            if (fres_->Format(*node, fres_->format_, fres_->compress_)) {
                for (const auto iter : fres_->groups_) {
                    // Allocate group id for image node 
                    if (iter.get()->path.IsParent(node->path))
                        node->id = iter.get()->gid;
                }
            }
        }
        delete this;
    }

private:
    FileResource* fres_;
    size_t id_;
    size_t nr_;
};