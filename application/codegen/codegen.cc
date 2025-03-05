/*
 * Copyright 2025 wtcat 
 */

#include <cstdlib>
#include <locale>
#include <codecvt>
#include <algorithm>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "application/codegen/codegen.h"

namespace app {
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

size_t StringToUpper(const char* instr, char* outstr, size_t maxsize) {
    if (instr == nullptr || outstr == nullptr)
        return 0;

    const char* s = instr;
    while (*s != '\0' && maxsize > 1) {
        *outstr++ = toupper(*s);
        s++;
        maxsize--;
    }

    return (size_t)(s - instr);
}

//Class ViewCodeFactory
bool ViewCodeFactory::GenerateViewCode(const FilePath &in) {
    //Parse resource file
    if (!ResourceParser::GetInstance()->ParseInput(in)) {
        printf("Failed to parse input file(re_output.json)\n");
        return false;
    }

    //Create resource code builder
    builders_.push_back(
        new app::ResourceCodeBuilder(FilePath(L"ui_template_resource.c"))
    );

    //Create resource id builder
    builders_.push_back(
        new app::ViewIDCodeBuilder(FilePath(L"ui_template_ids.h"))
    );

    //Create view template code builder 
    ResourceParser::GetInstance()->ForeachViewItem(
        base::Bind(&ViewCodeFactory::CallBack, this)
    );

    //Generate all code
    for (auto builder : builders_) {
        if (!builder->GenerateCode())
            return false;
    }

    return true;
}

void ViewCodeFactory::CallBack(const ResourceParser::ViewData& view) {
    char name[256];

    size_t len = StringToLower(view.name.c_str(), name, sizeof(name) - 3);
    name[len]     = '.';
    name[len + 1] = 'c';
    name[len + 2] = '\0';

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    FilePath path(converter.from_bytes(name));

    name[len] = '\0';
    builders_.push_back(
        new ViewCodeBuilder(view, path, name)
    );
}

//Class ViewCodeBuilder
void ViewCodeBuilder::AddHeaderFile(std::string& code) {
    code.append(
        "/*\n"
        " * Copyright 2025 Code-Generator\n"
        " */\n"
        "\n"
        "#include \"ui_template.h\"\n"
        "#include \"app_ui_view.h\"\n"
        "\n"
    );
}

void ViewCodeBuilder::AddPrivateData(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);

    snprintf(buffer.get(), BUFFER_SIZE,
        "#define PIC_NUMBERS %d\n"
        "#define STR_NUMBERS %d\n"
        "\n",
        (uint32_t)view_.pictures.size(), (uint32_t)view_.strings.size());
    code.append(buffer.get());

    snprintf(buffer.get(), BUFFER_SIZE,
        "typedef struct {\n"
        "    lv_obj_t* obj;\n"
        "    //...\n"
        "    // \n"
        "    //ui_font_t font;\n"
        "    %s\n"
        "    %s\n"
        "} %s_t;\n"
        "\n\n",
        view_.pictures.size() > 0? "lv_img_dsc_t res_img[PIC_NUMBERS];": "",
        view_.strings.size() > 0 ? "ui_string_t  res_txt[STR_NUMBERS];" : "",
        view_name_.c_str());
    code.append(buffer.get());
}

bool ViewCodeBuilder::CodeWriteHeader(std::string& code) {
    AddHeaderFile(code);
    AddPrivateData(code);
    return true;
}

bool ViewCodeBuilder::CodeWriteFoot(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    const char* pname = view_name_.c_str();

    snprintf(buffer.get(), BUFFER_SIZE,
        "\n\n"
        "UI_VIEW_DEFINE(%s_view) = {\n"
        "    .on_create        = %s_create,\n"
        "    .on_focus_change  = %s_focus_change,\n"
        "    .on_paint         = %s_paint,\n"
        "    .on_destroy       = %s_destroy,\n"
        "    .on_key           = %s_key,\n"
        "    .user_size        = sizeof(%s_t),\n"
        "    VIEW_PRIV(\"general\") //TODO: Maybe it needs to change\n"
        "};\n",
        pname,
        pname,
        pname,
        pname,
        pname,
        pname,
        pname);
    code.append(buffer.get());

    char idbuf[256];
    size_t len = StringToUpper(pname, idbuf, sizeof(idbuf));
    idbuf[len] = '\0';
    snprintf(buffer.get(), BUFFER_SIZE,
        "\n"
        "LVGL_VIEW_DEFINE(uID__%s, &%s_view);\n",
        idbuf,
        pname);
    code.append(buffer.get());

    return true;
}

bool ViewCodeBuilder::CodeWriteBody(std::string& code) {
    std::string tcode;

    tcode.reserve(1024);
    AddResourceCode(tcode);
    AddMethod(code, "create", 
        "ui_context_t* ctx", 
        tcode.c_str()
    );

    AddMethod(code, "focus_change", 
        "ui_context_t* ctx", 
        "\t//TODO: implement\n"
        "\treturn 0;\n"
    );

    AddMethod(code, "paint", 
        "ui_context_t* ctx", 
        "\t//TODO: implement\n"
        "\treturn 0;\n"
    );

    AddMethod(code, "destroy", 
        "ui_context_t* ctx", 
        "\t//TODO: implement\n"
        "\treturn 0;\n"
    );

    AddMethod(code, "key",
        "ui_context_t* ctx, int keyid, int keyevt, bool* done", 
        "\t//TODO: implement\n"
        "\treturn 0;\n"
    );

    return true;
}

void ViewCodeBuilder::AddMethod(std::string& code, const char* suffix,
    const char* args_list, const char* content) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);

    snprintf(buffer.get(), BUFFER_SIZE,
        "static int %s_%s(%s) {\n",
        view_name_.c_str(), 
        suffix,
        args_list);
    code.append(buffer.get());

    if (content != nullptr)
        code.append(content);
    code.append("}\n\n");
}

void ViewCodeBuilder::AddResourceCode(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);

    snprintf(buffer.get(), BUFFER_SIZE,
        "\t%s_t *priv = ui_context_get_user(ctx);\n"
        "\tint err;\n\n",
        view_name_.c_str());
    code.append(buffer.get());

    //Picture resource code
    int pic_numbers = (int)view_.pictures.size();
    if (pic_numbers > 0) {
        code.append(
            "\t/*\n"
            "\t * Get picture resources\n"
            "\t */\n"
            "\terr = ui_context_get_picture(ctx, priv->res_img, PIC_NUMBERS,\n"
        );

        for (int i = 0; i < pic_numbers; i++) {
            snprintf(buffer.get(), BUFFER_SIZE,
                "\t\t\"%s\"%s //%d\n",
                view_.pictures.at(i).name.c_str(),
                (i < pic_numbers - 1) ? "," : ");",
                i);
            code.append(buffer.get());
        }
        code.append(
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n"
        );
    }

    // String resource code
    int str_numbers = (int)view_.strings.size();
    if (str_numbers > 0) {
        code.append(
            "\t/*\n"
            "\t * Get text resources\n"
            "\t */\n"
            "\terr = ui_conext_get_text(ctx, priv->res_txt, STR_NUMBERS,\n"
        );

        for (int i = 0; i < str_numbers; i++) {
            snprintf(buffer.get(), BUFFER_SIZE,
                "\t\t\"%s\"%s //%d\n",
                view_.strings.at(i).name.c_str(),
                (i < str_numbers - 1) ? "," : ");",
                i);
            code.append(buffer.get());
        }
        code.append(
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n"
        );
    }

    // Font resource get code
    code.append(
        "\t//err = ui_context_get_font(ctx, DEF_FONTx_FILE, &priv->font);\n"
        "\t//if (err)\n"
        "\t\t//return err;\n"
        "\n"
    );

    // Lvgl widget create code
    snprintf(buffer.get(), BUFFER_SIZE,
        "\t/*\n"
        "\t * Create lvgl widgets\n"
        "\t */\n"
        "\tlv_obj_t *scr = lv_disp_get_scr_act(ui_context_get_display(ctx));\n"
        "\tconst %s_presenter_t *presenter = ui_context_get_presenter(ctx);\n"
        "\n"
        "\tpriv->obj = lv_obj_create(scr);\n"
        "\tlv_obj_set_pos(priv->obj, 0, 0);\n"
        "\tlv_obj_set_size(priv->obj, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);\n\n"
        "\t//TODO: implement\n"
        "\n"
        "\treturn 0;\n",
        view_name_.c_str());
    code.append(buffer.get());
}

//Class ViewIDCodeBuilder
bool ViewIDCodeBuilder::CodeWriteHeader(std::string& code) {
    code.append(
        "/*\n"
        " * Copyright 2025 wtcat (Don't edit it)\n"
        " */\n"
        "#ifndef UI_TEMPLATE_IDS_H_\n"
        "#define UI_TEMPLATE_IDS_H_\n\n"
    );
    return true;
}

bool ViewIDCodeBuilder::CodeWriteFoot(std::string& code) {
    code.append("\n#endif /* UI_TEMPLATE_IDS_H_ */\n");
    return true;
}

bool ViewIDCodeBuilder::CodeWriteBody(std::string& code) {
    ResourceParser* reptr = ResourceParser::GetInstance();
    size_t count = reptr->GetIdCount();
    for (size_t i = 0; i < count; i++) {
        char idname[8] = {0};
        itoa((int)i, idname, 10);
        code.append("#define ")
            .append(reptr->GetIdName((int)i))
            .append("  ")
            .append(itoa((int)i + reptr->GetIdBase(), idname, 10))
            .append("\n");
    }
    return true;
}

//Class ResourceCodeBuilder
bool ResourceCodeBuilder::CodeWriteHeader(std::string& code) {
    static const char header[] = {
        "/*\n"
        " * Copyright 2025 wtcat\n"
        " */\n"
        "\n"
        "#include <assert.h>\n"
        "#include \"ui_template.h\"\n"
        "\n"
        "/*\n"
        " * Helper Macro\n"
        " */\n"
        "#define SDK_RESOURCE_NUM(table) (sizeof(table) / sizeof(table[0]))\n"
        "#define SDK_RESOURCE_ITEM(view_id, scene_id, pic, txt, picset) \\\n"
        "     {picset, pic, txt, scene_id, view_id}\n"
    };
    code.clear();
    code.append(header);
    return true;
}

bool ResourceCodeBuilder::CodeWriteFoot(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    int id = ResourceParser::GetInstance()->GetIdBase();

    snprintf(buffer.get(), BUFFER_SIZE,
        "UI_PUBLIC_API\n"
        "const sdk_resources_t* _sdk_view_get_resource(uint16_t view_id) {\n"
        "    assert(view_id >= %d);\n"
        "    uint16_t offset = view_id - %d;\n"
        "    if (offset < SDK_RESOURCE_NUM(sdk_resource_table))\n"
        "        return &sdk_resource_table[view_id];\n"
        "    return NULL;\n"
        "}\n",
        id, id);

    ResourceTableFill(code);
    code.append(buffer.get());
    return true;
}

bool ResourceCodeBuilder::CodeWriteBody(std::string& code) {
    ResourceParser::GetInstance()->ForeachView(
        base::Bind(&ResourceCodeBuilder::ResourceCallback, this),
        code);
    return true;
}

void ResourceCodeBuilder::ResourceCallback(const ResourceParser::ViewData& view,
    std::string& code) {
    char symbol_name[256];

    auto generate_fn = [&](const std::string& name, 
        const std::vector<ResourceParser::ResourceType> &vec,
        const char* extname) ->void {
        if (vec.size() > 0) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "\n/* %s */\nSDK_RESOURCE_DEFINE(%s_%s,\n", 
                name.c_str(), 
                symbol_name,
                extname
            );
            code.append(buf);
            for (const auto& iter : vec)
                code.append("\t_RESOURCE_ITEM(").append(iter.name).append("),\n");
            code.append(");\n");
        }
    };

    size_t len = StringToLower(view.name.c_str(), symbol_name, sizeof(symbol_name) - 1);
    symbol_name[len] = '\0';

    //Picture resource
    generate_fn(view.name, view.pictures, "pic");

    //String resource
    generate_fn(view.name, view.strings, "txt");

    //Collect variable information
    scoped_ptr<ResourceNode> item(new ResourceNode());
    item->view = view_ids_++;
    item->scene = view.value;
    item->picture.append(symbol_name).append("_pic");
    item->text.append(symbol_name).append("_txt");
    nodes_.push_back(std::move(item));
}

void ResourceCodeBuilder::ResourceTableFill(std::string& code) {
    static const char header[] = {
        "\n\n"
        "/*\n"
        " * Resource table\n"
        " */\n"
        "static const sdk_resources_t sdk_resource_table[] = {\n"
    };

    //Append resource table header
    code.append(header);

    //Sort view ID
    std::sort(nodes_.begin(), nodes_.end(), Less());

    //Generate resource table item
    for (const auto& iter : nodes_) {
        code.append("\tSDK_RESOURCE_ITEM(")
            .append(ResourceParser::GetInstance()->GetIdName(iter->view)).append(", ")
            .append(iter->scene).append(", ")
            .append(iter->picture).append(", ")
            .append(iter->text).append(", ")
            .append(iter->anim.size() ? iter->anim : "NULL").append("),\n");
    }

    //Append resource table foot
    code.append("};\n\n\n");
}

//Class ResourceParser
bool ResourceParser::ParseInput(const FilePath& path) {
    JSONFileValueSerializer json(path);
    std::string  err_message;
    base::Value* value;

    if (valid())
        return true;

    //Parse resource information file (Generated by resource convert tool)
    value = json.Deserialize(nullptr, &err_message);
    if (value == nullptr) {
        printf("Failed to parse json: %s\n", err_message.c_str());
        return false;
    }

    //Get root node
    const base::DictionaryValue* dict_value;
    const base::ListValue* list_value;
    if (!value->GetAsDictionary(&dict_value))
        return false;

    //Check file signature
    std::string str;
    str.reserve(128);

    if (!dict_value->GetString("signature", &str)) {
        printf("Not found key: \"signature\"\n");
        return false;
    }
    if (str != "ResourceInterface") {
        printf("Invalid resource interface file\n");
        return false;
    }

    //Parse resource list
    if (!dict_value->GetList("views", &list_value)) {
        printf("Not found key: \"views\"\n");
        return false;
    }

    //Walk around resource node list
    ids_.reserve(80);
    resources_.reserve(50);
    for (auto iter = list_value->begin();
        iter != list_value->end();
        ++iter) {
        std::unique_ptr<ViewData> view_ptr = std::make_unique<ViewData>();

        if (!(*iter)->GetAsDictionary(&dict_value)) {
            printf("Invalid \"views\" value\n");
            return false;
        }

        //Get view name
        if (!dict_value->GetString("name", &view_ptr.get()->name)) {
            printf("Not found key: \"name\"\n");
            return false;
        }

        //Get resource id of the view
        if (!dict_value->GetString("value", &view_ptr.get()->value)) {
            printf("Not found key: \"value\"\n");
            return false;
        }

        //Get all pictures of the view
        ForeachListValue(dict_value, "pictures", view_ptr.get()->pictures);
        std::sort(view_ptr.get()->pictures.begin(), 
            view_ptr.get()->pictures.end(), ResourceLess());

        //Get all strings of the view
        ForeachListValue(dict_value, "strings", view_ptr.get()->strings);
        std::sort(view_ptr.get()->strings.begin(),
            view_ptr.get()->strings.end(), ResourceLess());

        ids_.push_back("uID__" + view_ptr.get()->name);
        resources_.push_back(std::move(view_ptr));
    }

    return true;
}

bool ResourceParser::ForeachListValue(const base::DictionaryValue* value,
    const std::string& key,
    std::vector<ResourceType>& vector) {
    const base::ListValue* list_value;
    if (!value->GetList(key, &list_value)) {
        printf("Not found key: \"%s\"\n", key.c_str());
        return false;
    }

    //Walk around resource node list
    ResourceType resource(64, 32);
    for (auto iter = list_value->begin();
        iter != list_value->end();
        ++iter) {
        const base::DictionaryValue* dict_value;
        if (!(*iter)->GetAsDictionary(&dict_value)) {
            printf("Invalid \"views\" value\n");
            return false;
        }

        resource.Clear();
        if (!dict_value->GetString("name", &resource.name)) {
            printf("Not found key: \"name\"\n");
            return false;
        }
        if (!dict_value->GetString("value", &resource.value)) {
            printf("Not found key: \"value\"\n");
            return false;
        }
        vector.push_back(resource);
    }
    return true;
}

} //namespace app
