/*
 * Copyright 2024 wtcat 
 */
#ifndef CODEGEN_H_
#define CODEGEN_H_

#include <string>
#include <vector>
#include <memory>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/json/json_file_value_serializer.h"

#include "application/helper/helper.h"

namespace app {

// Class ResourceParser
class ResourceParser {
public:
    struct ResourceType {
        ResourceType(size_t namelen, size_t valuelen) {
            name.reserve(namelen);
            value.reserve(valuelen);
        }
        void Clear() {
            name.clear();
            value.clear();
        }
        std::string name;
        std::string value;
    };
    struct ViewData {
        std::string name;
        std::string value;
        std::vector<ResourceType> pictures;
        std::vector<ResourceType> strings;
    };
    struct ResourceLess {
        bool operator()(const ResourceType& l, const ResourceType& r) {
            return atoi(l.value.c_str()) < atoi(r.value.c_str());
        }
    };

    static ResourceParser* GetInstance() {
        static ResourceParser generator;
        return &generator;
    }
    bool ParseInput(const FilePath& path);
    void SetOptions(int id_base, const std::string& rfnname, const FilePath &outpath) {
        id_base_ = id_base;
        resource_fnname_ = rfnname;
        outpath_ = outpath;
    }

    void ForeachView(base::Callback<void(const ViewData &, std::string&)> &callback,
        std::string &code) {
        for (const auto &iter : resources_)
            callback.Run(*iter.get(), code);
    }
    void ForeachViewItem(base::Callback<void(const ViewData&)>& callback) {
        for (const auto& iter : resources_)
            callback.Run(*iter.get());
    }
    bool valid() const {
        return resources_.size() > 0;
    }
    const std::string& GetIdName(unsigned int id) {
        if (id < ids_.size())
            return ids_.at(id);
        return null_str_;
    }

    size_t GetIdCount() const {
        return ids_.size();
    }
    int GetIdBase() const {
        return id_base_;
    }
    const std::string& GetFunctionName() {
        return resource_fnname_;
    }
    const FilePath& GetOutputPath() {
        return outpath_;
    }

private:
    ResourceParser() : id_base_(0) {}
    bool ForeachListValue(const base::DictionaryValue* value,
        const std::string& key,
        std::vector<ResourceType>& vector);

private:
    std::vector<std::unique_ptr<ViewData>> resources_;
    std::vector<std::string> ids_;
    std::string null_str_;
    std::string resource_fnname_;
    FilePath outpath_;
    int id_base_;
    DISALLOW_COPY_AND_ASSIGN(ResourceParser);
};

// Class CodeBuilder
class CodeBuilder: public base::RefCounted<CodeBuilder> {
public:
    enum { BUFFER_SIZE = 4096 };
    CodeBuilder(const FilePath& file) : file_(file) {}
    virtual ~CodeBuilder() {}
    bool GenerateCode() {
        if (!ResourceParser::GetInstance()->valid())
            return false;

        std::string code;
        code.reserve(40960);

        if (!CodeWriteHeader(code))
            return false;

        if (!CodeWriteBody(code))
            return false;

        if (!CodeWriteFoot(code))
            return false;

        return file_util::WriteFile(file_, code.c_str(), (int)code.length()) > 0;
    }

private:
    virtual bool CodeWriteHeader(std::string& code) { return true; }
    virtual bool CodeWriteBody(std::string& code) { return true; }
    virtual bool CodeWriteFoot(std::string& code) { return true; }

private:
    const FilePath file_;
};

// Class ResourceCodeBuilder
class ResourceCodeBuilder : public CodeBuilder {
public:
    friend class ResourceParser;
    struct ResourceNode {
        std::string scene;
        std::string picture;
        std::string text;
        std::string anim;
        unsigned int view;
    };

    class Less {
    public:
        bool operator()(const scoped_ptr<ResourceNode>& l, const scoped_ptr<ResourceNode>& r) {
            return l->view < r->view;
        }
    };

    ResourceCodeBuilder(const FilePath& file) : 
        CodeBuilder(file), view_ids_(0) {
        nodes_.reserve(50);
    }
    ~ResourceCodeBuilder() = default;

private:
    void ResourceTableFill(std::string& code);
    void ResourceCallback(const ResourceParser::ViewData& view, std::string& code);
    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;

private:
    std::vector<scoped_ptr<ResourceNode>> nodes_;
    unsigned int view_ids_;
};

// Class ViewIDCodeBuilder
class ViewIDCodeBuilder : public CodeBuilder {
public:
    ViewIDCodeBuilder(const FilePath& file) : CodeBuilder(file) {}
private:
    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;
};

//Class ViewCodeBuiler
class ViewCodeBuilder : public CodeBuilder {
public:
    ViewCodeBuilder(const ResourceParser::ViewData& view, const FilePath& file, 
        const std::string &view_name)
        : CodeBuilder(file), view_(view), view_name_(view_name) {}

private:
    void AddHeaderFile(std::string& code);
    void AddPrivateData(std::string& code);
    void AddExampleCode(std::string& code);
    void AddMethod(std::string& code, const char* suffix, 
        const char *args_list, const char* content);
    void AddResourceCode(std::string& code);

    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;

private:
    const ResourceParser::ViewData& view_;
    std::string view_name_;
};

//Class ViewCodeFactory
class ViewCodeFactory: public base::RefCounted<ViewCodeFactory> {
public:

    ViewCodeFactory() { 
        builders_.reserve(30); 
    }
    ~ViewCodeFactory() = default;
    bool GenerateViewCode(const FilePath& in);
    void SetOptions(int id_base, const std::string &rfnname, 
        const FilePath &outpath) {
        ResourceParser::GetInstance()->SetOptions(id_base, rfnname, outpath);
    }

private:
    void CallBack(const ResourceParser::ViewData& view);
private:
    std::vector<scoped_refptr<CodeBuilder>> builders_;
};


} //namespace app


#endif //CODEGEN_H_
