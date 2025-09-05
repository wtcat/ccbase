/*
 * Copyright 2025 wtcat 
 */

#include "lvgen_cinsn.h"

#include <vector>
#include <filesystem>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/memory/singleton.h"
#include "lvgen.h"

namespace app {
namespace fs = std::filesystem;

LvCodeGenerator::LvCodeGenerator() {
    lvgen_context_init();
}

LvCodeGenerator::~LvCodeGenerator() {
    for (auto iter : lv_props_)
        delete iter.second;
    lvgen_context_destroy();
}

LvCodeGenerator* LvCodeGenerator::GetInstance() {
    return Singleton<LvCodeGenerator>::get();
}

bool LvCodeGenerator::LoadViews(const FilePath& dir) {
    if (!file_util::PathExists(dir)) {
        printf("Invalid path(%s)\n", dir.AsUTF8Unsafe().c_str());
        return false;
    }

    // Parse components
    FilePath component_dir = dir.Append(L"components");
    if (file_util::PathExists(component_dir))
        ScanDirectory(component_dir, 0, false);

    return ScanDirectory(dir, 0, true);
}

bool LvCodeGenerator::Generate(const FilePath &outdir) const{
    if (lvgen_generate()) {
        std::string buf;
        buf.reserve(8192);

        LvGlobalContext* ctx = lvgen_get_context();
        void* ll_ptr;
        LV_LL_READ(&ctx->ll_modules, ll_ptr) {
            buf.clear();
            GenerateModule((const LvModuleContext*)ll_ptr, buf, outdir);
        }

        return true;
    }

    return false;
}

bool LvCodeGenerator::LoadAttributes(const FilePath& file) {
    // Make sure the file is valid 
    if (!file_util::PathExists(file)) {
        printf("Not found file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Parse doc
    xml::XMLDocument doc;
    if (doc.LoadFile(file.AsUTF8Unsafe().c_str()) != xml::XML_SUCCESS) {
        printf("Failed to parse file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Get root element
    xml::XMLElement* root_node = doc.RootElement();

    // Parse all types
    std::vector<const char*> type_vector;
    type_vector.reserve(15);

    xml::XMLElement* type_nodes = FindChild(root_node, "types");
    ForeachChild(type_nodes, [&](xml::XMLElement* e) {
        type_vector.push_back(e->Name());
    });

    for (auto iter : type_vector) {
        LvMap* lvmap = new LvMap(iter);
        xml::XMLElement* child = FindChild(type_nodes, iter);
        ForeachChild(child, [&](xml::XMLElement* e) {
            LvAttribute *attr = new LvAttribute(e->Attribute("name"), e->Attribute("value"), "?");
            lvmap->container.insert(std::make_pair(attr->name, attr));
        });

        lv_props_.insert(std::make_pair(lvmap->name, lvmap));
    }

    // Parse styles
    xml::XMLElement* styles_node = FindChild(root_node, "styles");
    LvMap* lvmap = new LvMap("styles");
    ForeachChild(styles_node, [&](xml::XMLElement* e) {
        LvAttribute* attr = new LvAttribute(e->Attribute("name"), e->Attribute("value"), e->Attribute("type"));
        lvmap->container.insert(std::make_pair(attr->name, attr));
        });
    lv_props_.insert(std::make_pair(lvmap->name, lvmap));

    //TODO: remove 
    const LvAttribute* p = FindAttribute("lv_border_side_t", "bottom");
    if (p != nullptr) {
        printf("%s\n", p->value.c_str());
    }
    
    return true;
}

const LvCodeGenerator::LvAttribute* LvCodeGenerator::FindAttribute(const std::string& ns,
    const std::string& key) {
    auto ns_iter = lv_props_.find(ns);
    if (ns_iter != lv_props_.end()) {
        LvMap* lvmap = ns_iter->second;
        auto key_iter = lvmap->container.find(key);
        if (key_iter != lvmap->container.end())
            return key_iter->second;
    }
    return nullptr;
}

xml::XMLElement* LvCodeGenerator::FindChild(xml::XMLElement* parent,
    const char* elem) {
    for (xml::XMLElement* node = parent->FirstChildElement();
        node != nullptr;
        node = node->NextSiblingElement()) {
        if (!strcmp(node->Name(), elem))
            return node;
    }
    return nullptr;
}

bool LvCodeGenerator::ScanDirectory(const FilePath& dir, int level, bool ignore_components) {
    const fs::path root_path(dir.MaybeAsASCII());
    bool okay = false;
    bool view;

    // Max path recursive deepth must be less than 2
    if (level > 2)
        return true;

    view = (dir.BaseName() != FilePath(L"components"))? true: false;
    if (ignore_components && !view)
        return true;

    level++;

    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (fs::is_directory(entry)) {
            okay = ScanDirectory(FilePath::FromUTF8Unsafe(entry.path().string()), level, 
                ignore_components);
            if (!okay)
                break;
        }

        if (fs::is_regular_file(entry)) {
            if (entry.path().filename().extension() == ".xml") {
                okay = ParseView(entry.path().string(), view);
                if (!okay) {
                    printf("Failed to parse file(%s)\n", entry.path().string().c_str());
                    break;
                }
            }
        }
    }

    level--;
    return okay;
}

bool LvCodeGenerator::ParseView(const std::string& file, bool is_view) {
    return lvgen_parse(file.c_str(), is_view) == 0;
}

bool LvCodeGenerator::GenerateModule(const LvModuleContext* mod, std::string &buf,
    const FilePath &outdir) const {
    if (lv_ll_get_head(&mod->ll_funs) != nullptr) {

        //Generate module header file
        if (GenerateModuleHeader(mod, buf)) {
            char tbuf[128];
            snprintf(tbuf, sizeof(tbuf), "%s.h", mod->name);
            if (file_util::WriteFile(outdir.Append(FilePath::FromUTF8Unsafe(tbuf)),
                buf.data(), (int)buf.size()) < 0)
                return false;

            //Generate module source file
            buf.clear();
            if (GenerateModuleSource(mod, buf)) {
                snprintf(tbuf, sizeof(tbuf), "%s.c", mod->name);
                return file_util::WriteFile(outdir.Append(FilePath::FromUTF8Unsafe(tbuf)),
                    buf.data(), (int)buf.size()) > 0;
            }
        }
    }

    return false;
}

bool LvCodeGenerator::GenerateModuleHeader(const LvModuleContext* mod, std::string &buf) const {
    char tbuf[256];

    //Add copyright information
    GenerateCopyright(buf);

    // Head file begin marker
    snprintf(tbuf, sizeof(tbuf), "#ifndef autogen_%s__h_\n#define autogen_%s__h_\n\n",
        mod->name, mod->name);
    buf.append(tbuf);
    buf.append("#ifdef __cplusplus\n");
    buf.append("extern \"C\"{\n");
    buf.append("#endif\n\n");

    // Function declare
    void* ll_ptr;
    LV_LL_READ(&mod->ll_funs, ll_ptr) {
        LvFunctionContext* fn = (LvFunctionContext*)ll_ptr;
        if (fn->export_cnt > 0 && GenerateFunctionSignature(fn, tbuf, sizeof(tbuf)))
            buf.append(tbuf).append(";\n");
    }
    buf.append("\n");

    //Head file end marker
    buf.append("#ifdef __cplusplus\n");
    buf.append("}\n");
    buf.append("#endif\n");
    snprintf(tbuf, sizeof(tbuf), "#endif /* autogen_%s__h_ */\n",
        mod->name);
    buf.append(tbuf);
    
    return true;
}

bool LvCodeGenerator::GenerateModuleSource(const LvModuleContext* mod, std::string& buf) const {
    scoped_ptr<char> strbuf(new char[kStringBufferSize]);
    void* ll_ptr;

    //Add copyright information
    GenerateCopyright(buf);

    //Add header file depends
    buf.append("#include \"lvgl.h\"\n");
    buf.append("#include \"lvgen_cdefs.h\"\n");
    LV_LL_READ(&mod->ll_deps, ll_ptr) {
        LvModuleDepend* mdep = (LvModuleDepend*)ll_ptr;
        snprintf(strbuf.get(), kStringBufferSize, "#include \"%s.h\"\n", mdep->mod->name);
        buf.append(strbuf.get());
    }
    buf.append("\n\n");

    //Add function definition
    int max_styles = 1, max_images = 0, max_fonts = 0;
    std::string fn_text;
    fn_text.reserve(8192);

    LV_LL_READ(&mod->ll_funs, ll_ptr) {
        LvFunctionContext* fn = (LvFunctionContext*)ll_ptr;

        //if (!mod->is_view && fn->ref_cnt == 0)
        //    continue;

        GenerateFunction(fn, fn_text);
        if (fn->style_num > max_styles)
            max_styles = fn->style_num;

        if (fn->font_num > max_fonts)
            max_fonts = fn->font_num;

        if (fn->image_num > max_images)
            max_images = fn->image_num;
    }

    //Add type definition
    if (mod->is_view) {
        snprintf(strbuf.get(), kStringBufferSize,
            "#define MAX_STYLES %d\n"
            "#define MAX_FONTS  %d\n"
            "#define MAX_IMAGES %d\n"
            "\n"
            "typedef struct {\n"
            "#if MAX_STYLES > 0\n"
            "\tlv_style_t     styles[MAX_STYLES];\n"
            "#endif\n"

            "#if MAX_FONTS > 0\n"
            "\tlv_font_t      fonts[MAX_FONTS];\n"
            "#endif\n"

            "#if MAX_IMAGES > 0\n"
            "\tlv_image_dsc_t images[MAX_IMAGES];\n"
            "#endif\n"
            "} lv_view__private_t;"
            "\n\n\n",
            max_styles,
            max_fonts,
            max_images);
        buf.append(strbuf.get());
    }
    buf.append(fn_text);

    return true;
}

bool LvCodeGenerator::GenerateFunction(const LvFunctionContext* fn, std::string& buf) const  {
    char tbuf[256];

    if (!GenerateFunctionSignature(fn, tbuf, sizeof(tbuf)))
        return false;

    buf.append(tbuf).append(" {\n");

    GenerateFunctionInstruction(fn, buf, "\t");

    buf.append("}\n\n");

    return true;
}

bool LvCodeGenerator::GenerateFunctionSignature(const LvFunctionContext* fn, char* tbuf, 
    size_t maxsize) const  {
    if (fn == nullptr || tbuf == nullptr)
        return false;

    // Format function signature
    int remain = (int)maxsize;
    int offset = 0;

    if (fn->export_cnt == 0)
        offset += snprintf(tbuf + offset, remain - offset, "static ");

    offset += snprintf(tbuf + offset, remain - offset, "%s %s",
        lv_type_to_name(fn->rtype), fn->signature);

    switch (fn->args_num) {
    case 0:
        offset += snprintf(tbuf + offset, remain - offset, "(void)");
        break;
    case 1:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s)",
            fn->args[0].type, fn->args[0].name
        );
        break;
    case 2:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s, %s %s)",
            fn->args[0].type, fn->args[0].name,
            fn->args[1].type, fn->args[1].name
        );
        break;
    case 3:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s, %s %s, %s %s)",
            fn->args[0].type, fn->args[0].name,
            fn->args[1].type, fn->args[1].name,
            fn->args[2].type, fn->args[2].name
        );
        break;
    case 4:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s, %s %s, %s %s, %s %s)",
            fn->args[0].type, fn->args[0].name,
            fn->args[1].type, fn->args[1].name,
            fn->args[2].type, fn->args[2].name,
            fn->args[3].type, fn->args[3].name
        );
        break;
    case 5:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s, %s %s, %s %s, %s %s, %s %s)",
            fn->args[0].type, fn->args[0].name,
            fn->args[1].type, fn->args[1].name,
            fn->args[2].type, fn->args[2].name,
            fn->args[3].type, fn->args[3].name,
            fn->args[4].type, fn->args[4].name
        );
        break;
    case 6:
        offset += snprintf(tbuf + offset, remain - offset, "(%s %s, %s %s, %s %s, %s %s, %s %s, %s %s)",
            fn->args[0].type, fn->args[0].name,
            fn->args[1].type, fn->args[1].name,
            fn->args[2].type, fn->args[2].name,
            fn->args[3].type, fn->args[3].name,
            fn->args[4].type, fn->args[4].name,
            fn->args[5].type, fn->args[5].name
        );
        break;
    default:
        return false;
    }

    return true;
}

bool LvCodeGenerator::GenerateFunctionInstruction(const LvFunctionContext* fn, std::string &buf, 
    const char *indent) const  {
    char tbuf[512];
    void* ll_ptr;

    // Format function instruction
    LV_LL_READ(&fn->ll_insn, ll_ptr) {
        LvFunctionCallInsn* ins = (LvFunctionCallInsn*)ll_ptr;
        int remain = sizeof(tbuf);
        int offset = 0;

        if (indent) {
            if (!LV_IS_EXPR(ins->rtype) && ins->lvalue != nullptr)
                offset += snprintf(tbuf + offset, remain - offset, "\n");
            offset += snprintf(tbuf + offset, remain - offset, indent);
        }

        if (!LV_IS_EXPR(ins->rtype)) {
            if (ins->lvalue != nullptr) {
                offset += snprintf(tbuf + offset, remain - offset, "%s %s = ",
                    lv_type_to_name(ins->rtype), ins->lvalue);
            }

            switch (ins->args_num) {
            case 0:
                offset += snprintf(tbuf + offset, remain - offset, "%s();\n",
                    ins->insn);
                break;
            case 1:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s);\n",
                    ins->insn,
                    ins->args[0]
                );
                break;
            case 2:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s, %s);\n",
                    ins->insn,
                    ins->args[0],
                    ins->args[1]
                );
                break;
            case 3:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s, %s, %s);\n",
                    ins->insn,
                    ins->args[0],
                    ins->args[1],
                    ins->args[2]
                );
                break;
            case 4:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s, %s, %s, %s);\n",
                    ins->insn,
                    ins->args[0],
                    ins->args[1],
                    ins->args[2],
                    ins->args[3]
                );
                break;
            case 5:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s, %s, %s, %s, %s);\n",
                    ins->insn,
                    ins->args[0],
                    ins->args[1],
                    ins->args[2],
                    ins->args[3],
                    ins->args[4]
                );
                break;
            case 6:
                offset += snprintf(tbuf + offset, remain - offset, "%s(%s, %s, %s, %s, %s, %s);\n",
                    ins->insn,
                    ins->args[0],
                    ins->args[1],
                    ins->args[2],
                    ins->args[3],
                    ins->args[4],
                    ins->args[5]
                );
                break;
            default:
                return false;
            }
        } else {
            offset += snprintf(tbuf + offset, remain - offset, "%s\n", ins->expr);
        }
        
        buf.append(tbuf);
    }

    // Generate return instruction
    if (fn->rvar != nullptr) {
        int offset = 0;

        tbuf[offset++] = '\n';
        if (indent != nullptr)
            offset += snprintf(tbuf + offset, sizeof(tbuf) - offset, "%s", indent);
        snprintf(tbuf + offset, sizeof(tbuf) - offset, "return %s;\n", fn->rvar);
        buf.append(tbuf);
    }

    return true;
}

void LvCodeGenerator::GenerateCopyright(std::string& buf) const {
    base::Time time = base::Time::NowFromSystemTime();
    base::Time::Exploded explod;
    time.LocalExplode(&explod);

    char tbuf[256];
    snprintf(tbuf, sizeof(tbuf), 
        "/*\n"
        " * Copyright(c) %d Autogen @wtcat\n"
        " */\n\n",
        explod.year);
    buf.append(tbuf);
}

} //namespace app

