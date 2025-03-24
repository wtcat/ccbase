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

//Class ViewPresenterBuilder
bool ViewPresenterBuilder::CodeWriteHeader(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    snprintf(buffer.get(), BUFFER_SIZE,
        "/*\n"
        " * Copyright wtcat 2025\n"
        " */\n"
        "\n"
        "#ifndef %s_presenter_h_\n"
        "#define %s_presenter_h_\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n", name_.c_str(), name_.c_str());
    code.append(buffer.get());
    return true;
}

bool ViewPresenterBuilder::CodeWriteBody(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    snprintf(buffer.get(), BUFFER_SIZE,
        "typedef struct {\n"
        "\t//TODO: implement\n"
        "}%s_presenter_t\n"
        "\n", name_.c_str());
    code.append(buffer.get());
    return true;
}

bool ViewPresenterBuilder::CodeWriteFoot(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    snprintf(buffer.get(), BUFFER_SIZE,
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "#endif /* %s_presenter_h_ */\n",
        name_.c_str());
    code.append(buffer.get());
    return true;
}

//Class CMakeBuilder
bool CMakeBuiler::CodeWriteHeader(std::string& code) {
    code.append(
        "# UI project files \n\n"
        "zephyr_library_named(NewUI)\n"
        "file(GLOB C_SOURCES \"*.c\")\n\n"

    );
    return true;
}

bool CMakeBuiler::CodeWriteBody(std::string& code) {
    //Add include directories
    AddCMakeOption(code, "zephyr_library_include_directories",
        [](CMakeBuiler* cls, std::string& xcode) -> std::string& {
            return xcode.append("\t${CMAKE_CURRENT_SOURCE_DIR}\n");
        });

    //Add compile options
    AddCMakeOption(code, "zephyr_library_compile_options",
        [](CMakeBuiler* cls, std::string& xcode) -> std::string& {
            return xcode.append("\t-Os\n");
        });

    //Add source files
    AddCMakeOption(code, "zephyr_library_sources",
        [](CMakeBuiler* cls, std::string& xcode) -> std::string& {
            xcode.append("\t${C_SOURCES}\n");
            return xcode;
        });

    return true;
}

bool CMakeBuiler::CodeWriteFoot(std::string& code) {
    return true;
}

void CMakeBuiler::AddCMakeOption(std::string& code, const char* function, 
    std::string& (*fill)(CMakeBuiler *cls, std::string& code)) {
    code.append(function)
        .append("(\n");
    fill(this, code).append(")\n\n");
}

//Class ViewCodeFactory
bool ViewCodeFactory::GenerateViewCode(const FilePath &in, bool overwrite) {
    ResourceParser* re_parser = ResourceParser::GetInstance();

    //Parse resource file
    if (!re_parser->ParseInput(in)) {
        printf("Failed to parse input file(re_output.json)\n");
        return false;
    }

    //Create resource code builder
    builders_.push_back(
        new app::ResourceCodeBuilder(
            re_parser->output_path().Append(L"ui_template_resource.c"))
    );

    //Create resource id builder
    builders_.push_back(
        new app::ViewIDCodeBuilder(
            re_parser->output_path().Append(L"ui_template_ids.h"))
    );

    //Create view template code builder 
    ResourceParser::GetInstance()->ForeachViewItem(
        base::Bind(&ViewCodeFactory::CallBack, this)
    );

    //Generate all code
    for (auto builder : builders_) {
        if (!builder->GenerateCode(overwrite))
            return false;
    }

    //Create cmake project file
    scoped_refptr<CodeBuilder> cmake(
        new CMakeBuiler(
            ResourceParser::GetInstance()->output_path().Append(L"CMakeLists.txt"),
            builders_));
    cmake->GenerateCode(overwrite);
    
    return true;
}

void ViewCodeFactory::CallBack(const ResourceParser::ViewData& view) {
    char name[256 + kAppendStringLength];
    size_t len = StringToLower(view.name.c_str(), name, sizeof(name) - kAppendStringLength);

    // View file
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    StringCopy(name + len, ".c", kAppendStringLength);
    FilePath path(ResourceParser::GetInstance()->output_path()
        .Append(converter.from_bytes(name)));
    name[len] = '\0';
    builders_.push_back(
        new ViewCodeBuilder(view, path, name)
    );

    // Presenter file
    StringCopy(name + len, "_presenter.h", kAppendStringLength);
    FilePath hdr_path(ResourceParser::GetInstance()->output_path()
        .Append(converter.from_bytes(name)));
    name[len] = '\0';
    builders_.push_back(
        new ViewPresenterBuilder(hdr_path, name)
    );
}

//Class ViewCodeBuilder
void ViewCodeBuilder::AddEnumList(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);


    //Picture index list
    code.append(
        "/* Picture index list */\n"
        "enum picture_index {\n"
    );
    for (size_t i = 0, size = view_.pictures.size();
        i < size; i++) {
        snprintf(buffer.get(), BUFFER_SIZE,
            "\tK_%s,\n",
            view_.pictures[i].name.c_str());
        code.append(buffer.get());
    }
    code.append("};\n\n");

    //String index list
    code.append(
        "/* Text index list */\n"
        "enum text_index {\n"
    );
    for (size_t i = 0, size = view_.strings.size();
        i < size; i++) {
        snprintf(buffer.get(), BUFFER_SIZE,
            "\tK_%s,\n",
            view_.strings[i].name.c_str());
        code.append(buffer.get());
    }
    code.append("};\n\n");
}

void ViewCodeBuilder::AddHeaderFile(std::string& code) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "#include \"%s_presenter.h\"\n",
        view_name_.c_str());
    code.append(
        "/*\n"
        " * Copyright 2025 Code-Generator\n"
        " */\n"
        "\n"
        "#include \"ui_template.h\"\n"
        "#include \"app_ui_view.h\"\n"
    );
    code.append(buffer).append("\n");
}

void ViewCodeBuilder::AddPrivateData(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    scoped_ptr<char> font(new char[BUFFER_SIZE]);

    snprintf(buffer.get(), BUFFER_SIZE,
        "#define FONT_NAME   %s\n"
        "#define PIC_NUMBERS %d\n"
        "#define STR_NUMBERS %d\n"
        "#define GRP_NUMBERS %d\n"
        "\n",
        ResourceParser::GetInstance()->default_font().c_str(),
        (uint32_t)view_.pictures.size(), 
        (uint32_t)view_.strings.size(),
        (uint32_t)view_.picgroups.size());
    code.append(buffer.get());

    AddEnumList(code);

    //Generate font member code
    int offset = 0;
    for (auto &iter : view_.fonts) {
        int len = snprintf(font.get() + offset, 
            (int)BUFFER_SIZE - offset,
            "\tui_font_t font%d;\n", atoi(iter.value.c_str()));
        offset += len;
    }

    snprintf(buffer.get(), BUFFER_SIZE,
        "typedef struct {\n"
        "\tlv_obj_t* obj;\n"
        "\t\n"
        "\t// Resource objects \n"
        "\t%s\n"
        "\t%s\n"
        "\t%s\n"
        "%s"
        "} %s_t;\n"
        "\n\n",
        view_.pictures.size()?  "lv_img_dsc_t res_img[PIC_NUMBERS];": "",
        view_.strings.size()?   "ui_string_t res_txt[STR_NUMBERS];" : "",
        view_.picgroups.size()? "ui_picture_set_t res_anim[GRP_NUMBERS];" : "",
        font.get(),
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

    AddExampleCode(code);

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
        "\t//if (keyevt == UIVIEW_KEY_RELEASE && keyid == UIVIEW_KEY_HOME) {\n"
        "\t\t//TODO: implement\n"
        "\t//}\n"
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

    //Animation resource code
    AddGroupCode(code, buffer.get(), BUFFER_SIZE);

    //Picture resource code
    AddPictureCode(code, buffer.get(), BUFFER_SIZE);

    // String resource code
    AddStringCode(code, buffer.get(), BUFFER_SIZE);

    // Font resource get code
    AddFontCode(code, buffer.get(), BUFFER_SIZE);

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

void ViewCodeBuilder::AddFontCode(std::string& code, char* buf, size_t size) {
    code.append(
        "\t/* Get font resource */\n"
    );
    for (auto &iter : view_.fonts) {
        int font_size = atoi(iter.value.c_str());
        snprintf(buf, size,
            "\terr = ui_context_get_font(ctx, FONT_NAME, %d, &priv->font%d);\n"
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n", 
            font_size, font_size);
        code.append(buf);
    }
}

void ViewCodeBuilder::AddPictureCode(std::string& code, char* buf, size_t size) {
    int pic_numbers = (int)view_.pictures.size();
    if (pic_numbers > 0) {
        code.append(
            "\t/*\n"
            "\t * Get picture resources\n"
            "\t */\n"
            "\terr = ui_context_get_picture(ctx, priv->res_img, PIC_NUMBERS,\n"
        );

        for (int i = 0; i < pic_numbers; i++) {
            snprintf(buf, size,
                "\t\t__RE(\"%s\")%s //%d\n",
                view_.pictures.at(i).name.c_str(),
                (i < pic_numbers - 1) ? "," : ");",
                i);
            code.append(buf);
        }
        code.append(
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n"
        );
    }
}

void ViewCodeBuilder::AddStringCode(std::string& code, char* buf, size_t size) {
    int str_numbers = (int)view_.strings.size();
    if (str_numbers > 0) {
        code.append(
            "\t/*\n"
            "\t * Get text resources\n"
            "\t */\n"
            "\terr = ui_conext_get_text(ctx, priv->res_txt, STR_NUMBERS,\n"
        );

        for (int i = 0; i < str_numbers; i++) {
            snprintf(buf, size,
                "\t\t__RE(\"%s\")%s //%d\n",
                view_.strings.at(i).name.c_str(),
                (i < str_numbers - 1) ? "," : ");",
                i);
            code.append(buf);
        }
        code.append(
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n"
        );
    }
}

void ViewCodeBuilder::AddGroupCode(std::string& code, char* buf, size_t size) {
    int grp_numbers = (int)view_.picgroups.size();
    if (grp_numbers > 0) {
        code.append(
            "\t/*\n"
            "\t * Get animation resources\n"
            "\t */\n"
            "\terr = ui_context_get_picture_set(ctx, priv->res_anim, GRP_NUMBERS,\n"
        );

        for (int i = 0; i < grp_numbers; i++) {
            snprintf(buf, size,
                "\t\t__RE(\"%s\")%s //%d\n",
                view_.picgroups.at(i).name.c_str(),
                (i < grp_numbers - 1) ? "," : ");",
                i);
            code.append(buf);
        }
        code.append(
            "\tif (err)\n"
            "\t\treturn err;\n"
            "\n"
        );
    }
}

void ViewCodeBuilder::AddExampleCode(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    snprintf(buffer.get(), BUFFER_SIZE,
        "#if 0\n"
        "//LVGL event callback example"
        "(Events: LV_EVENT_CLICKED, ...) \n"
        "static void xxxx_event_cb(lv_event_t * e) {\n"
        "    ui_context_t* ctx = lv_event_get_user_data(e);\n"
        "    %s_presenter_t* presenter = ui_context_get_presenter(ctx);\n"
        "    %s_t* priv = ui_context_get_user(ctx);\n"
        "    lv_obj_t* obj = lv_event_get_target(e);\n"
        "\n"
        "    //TODO: implement\n"
        "}\n"
        "lv_obj_add_event_cb(priv->xxxx_obj, xxxx_event_cb, LV_EVENT_xxxxx, ctx);\n"
        "#endif //if 0\n"
        "\n",
        view_name_.c_str(), 
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
    size_t count = reptr->id_count();
    for (size_t i = 0; i < count; i++) {
        char idname[8] = {0};
        itoa((int)i, idname, 10);
        code.append("#define ")
            .append(reptr->GetIdName((int)i))
            .append("  ")
            .append(itoa((int)i + reptr->id_base(), idname, 10))
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
        "#define SDK_RESOURCE_ITEM(view_id, scene_id, npic, ntxt, nset) \\\n"
        "     { scene_id, view_id, npic, nset, ntxt }\n"
    };
    code.clear();
    code.append(header);
    return true;
}

bool ResourceCodeBuilder::CodeWriteFoot(std::string& code) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    int id = ResourceParser::GetInstance()->id_base();

    snprintf(buffer.get(), BUFFER_SIZE,
        "UI_PUBLIC_API\n"
        "const sdk_resources_t* %s(uint16_t view_id) {\n"
        "    assert(view_id >= %d);\n"
        "    uint16_t offset = view_id - %d;\n"
        "    if (offset < SDK_RESOURCE_NUM(sdk_resource_table))\n"
        "        return &sdk_resource_table[offset];\n"
        "    return NULL;\n"
        "}\n",
        ResourceParser::GetInstance()->function_name().c_str(),
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
    //Collect variable information
    scoped_ptr<ResourceNode> item(new ResourceNode());

    item->view = view_ids_++;
    item->scene = view.value;
    item->picture_num = (uint16_t)view.pictures.size();
    item->text_num = (uint16_t)view.strings.size();
    item->anim_num = (uint16_t)view.picgroups.size();

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
        char buffer[32];
        code.append("\tSDK_RESOURCE_ITEM(")
            .append(ResourceParser::GetInstance()->GetIdName(iter->view)).append(", ")
            .append(iter->scene).append(", ")
            .append(itoa(iter->picture_num, buffer, 10)).append(", ")
            .append(itoa(iter->text_num, buffer, 10)).append(", ")
            .append(itoa(iter->anim_num, buffer, 10)).append("),\n");
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
    for (const auto iter: *list_value) {
        std::unique_ptr<ViewData> view_ptr = std::make_unique<ViewData>();

        if (!(iter)->GetAsDictionary(&dict_value)) {
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

        //Get all picture groups of the view
        ForeachListValue(dict_value, "groups", view_ptr.get()->picgroups);

        //Get all fonts
        ForeachListValue(dict_value, "fonts", view_ptr.get()->fonts);

        ids_.push_back("uID__" + view_ptr.get()->name);
        resources_.push_back(std::move(view_ptr));
    }

    return true;
}

bool ResourceParser::ForeachListValue(const base::DictionaryValue* value,
    const std::string& key,
    std::vector<ResourceType>& vector) {
    const base::ListValue* list_value;
    if (!value->GetList(key, &list_value))
        return false;

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
