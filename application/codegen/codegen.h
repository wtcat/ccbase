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

namespace app {

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
    void ForeachView(base::Callback<void(const ViewData &, std::string&)> &callback,
        std::string &code) {
        for (const auto &iter : resources_)
            callback.Run(*iter.get(), code);
    }
    bool valid() const {
        return resources_.size() > 0;
    }
    const std::string& GetIdName(unsigned int id) {
        if (id < ids_.size())
            return ids_.at(id);
        return "";
    }
    size_t GetIdCount() const {
        return ids_.size();
    }

private:
    ResourceParser() = default;
    bool ForeachListValue(const base::DictionaryValue* value,
        const std::string& key,
        std::vector<ResourceType>& vector);

private:
    std::vector<std::unique_ptr<ViewData>> resources_;
    std::vector<std::string> ids_;
    DISALLOW_COPY_AND_ASSIGN(ResourceParser);
};

class CodeBuilder: public base::RefCounted<CodeBuilder> {
public:

    virtual ~CodeBuilder() {}
    bool GenerateCode(const FilePath& out) {
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

        return file_util::WriteFile(out, code.c_str(), (int)code.length()) > 0;
    }

protected:
    size_t StringToLower(const char* instr, char* outstr, size_t maxsize) {
        if (instr == nullptr || outstr == nullptr)
            return 0;

        const char* s = instr;
        while (*s != '\0' && maxsize > 1) {
            *outstr++ = tolower(*s);
            s++;
            maxsize--;
        }

        return (size_t)(s - instr);
    }

private:
    virtual bool CodeWriteHeader(std::string& code) { return true; }
    virtual bool CodeWriteBody(std::string& code) { return true; }
    virtual bool CodeWriteFoot(std::string& code) { return true; }
};

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

    ResourceCodeBuilder() : CodeBuilder(), view_ids_(0) {
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

class ViewIDCodeBuilder : public CodeBuilder {
public:
    ViewIDCodeBuilder() : CodeBuilder() {}
private:
    bool CodeWriteHeader(std::string& code) override;
    bool CodeWriteBody(std::string& code) override;
    bool CodeWriteFoot(std::string& code) override;
};


} //namespace app


#endif //CODEGEN_H_
