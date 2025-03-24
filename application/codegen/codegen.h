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
        std::vector<ResourceType> picgroups;
        std::vector<ResourceType> fonts;
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
    void SetOptions(int id_base, const std::string& rfnname, const FilePath &outpath,
        const std::string &font) {
        id_base_ = id_base;
        resource_fnname_ = rfnname;
        outpath_ = outpath;
        default_font_ = font;
    }

    template<typename Function>
    void ForeachView(base::Callback<Function> &&callback,
        std::string &code) {
        for (const auto &iter : resources_)
            callback.Run(*iter.get(), code);
    }

    template<typename Function>
    void ForeachViewItem(base::Callback<Function>&& callback) {
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

    size_t id_count() const {
        return ids_.size();
    }
    int id_base() const {
        return id_base_;
    }
    const std::string& function_name() {
        return resource_fnname_;
    }
    const FilePath& output_path() {
        return outpath_;
    }
    const std::string& default_font() const {
        return default_font_;
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
    std::string default_font_;
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
    bool GenerateCode(bool overwrite) {
        if (!ResourceParser::GetInstance()->valid())
            return false;

        //Don't overwrite file if it exists
        if (!overwrite) {
            if (file_util::PathExists(file_))
                return true;
        }

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
    const FilePath& filename() const {
        return file_;
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
        unsigned int picture_num;
        unsigned int text_num;
        unsigned int anim_num;
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
    void AddEnumList(std::string& code);
    void AddHeaderFile(std::string& code);
    void AddPrivateData(std::string& code);
    void AddExampleCode(std::string& code);
    void AddMethod(std::string& code, const char* suffix, 
        const char *args_list, const char* content);
    void AddResourceCode(std::string& code);
    void AddFontCode(std::string& code, char *buf, size_t size);
    void AddPictureCode(std::string& code, char* buf, size_t size);
    void AddStringCode(std::string& code, char* buf, size_t size);
    void AddGroupCode(std::string& code, char* buf, size_t size);

    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;

private:
    const ResourceParser::ViewData& view_;
    std::string view_name_;
};

//Class ViewPresenterBuilder
class ViewPresenterBuilder : public CodeBuilder {
public:
    ViewPresenterBuilder(const FilePath& file, const std::string &view_name)
        : CodeBuilder(file), name_(view_name) {}
private:
    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;
    DISALLOW_COPY_AND_ASSIGN(ViewPresenterBuilder);

private:
    std::string name_;
};

//Class ViewCodeFactory
class ViewCodeFactory: public base::RefCounted<ViewCodeFactory> {
public:
    enum { kAppendStringLength = 32 };
    ViewCodeFactory() {
        builders_.reserve(30); 
    }
    ~ViewCodeFactory() = default;
    bool GenerateViewCode(const FilePath& in, bool overwrite = false);
    void SetOptions(int id_base, const std::string &rfnname, 
        const FilePath &outpath, 
        const std::string &font) {
        ResourceParser::GetInstance()->SetOptions(id_base, rfnname, outpath, font);
    }

private:
    void CallBack(const ResourceParser::ViewData& view);
private:
    std::vector<scoped_refptr<CodeBuilder>> builders_;
};

//Class cmake builer
class CMakeBuiler : public CodeBuilder {
public:
    CMakeBuiler(const FilePath &file, const std::vector<scoped_refptr<CodeBuilder>>& src)
        : CodeBuilder(file), src_vector_(src) {}

private:
    void AddCMakeOption(std::string& code, const char *function, 
        std::string& (*fill)(CMakeBuiler *cls, std::string& code));
    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;
    DISALLOW_COPY_AND_ASSIGN(CMakeBuiler);

private:
    const std::vector<scoped_refptr<CodeBuilder>>& src_vector_;
};


} //namespace app


#endif //CODEGEN_H_
