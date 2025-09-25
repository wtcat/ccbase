/*
 * Copyright 2025 wtcat
 */

#include "plugin/bitmap/lvgl_bitmap_font.h"

#define BASE_IMPLEMENTATION
#include "base/base_export.h"

#include "plugin/plugin.h"
#include "base/linked_list.h"


class BitmapLoader : public lvsim::ResourceLoader {
public:
    struct BitmapFont : public base::LinkNode<BitmapFont> {
        BitmapFont(const std::string &font_name, int font_no) : 
            name(font_name), no(font_no), registered(false) {}
        std::string name;
        lv_font_t   font;
        uint32_t    no;
        bool        registered;
    };

    BitmapLoader(const std::string& name) : ResourceLoader(name) {
        lvgl_bitmap_font_init(NULL);
    }
    virtual ~BitmapLoader() {
        Clear(); 
        lvgl_bitmap_font_deinit();
    }

    ReHandle Load(const Attribute &attr) override {
        const char* path = attr.GetValue("src_path");
        if (path == nullptr)
            return nullptr;

        BitmapFont* bfont = NewFont(path, 0);
        int err = lvgl_bitmap_font_open(&bfont->font, path);
        if (err) {
            DeleteFont(bfont);
            return nullptr;
        }
        return bfont;
    }

    bool Get(ReHandle h, const std::string& name, Attribute& attr) override {
        BitmapFont* bfont = (BitmapFont*)h;

        if (IsFontOpend(bfont)) {
            if (!bfont->registered)
                bfont->registered = attr.RegisterFont(name.c_str(), &bfont->font);
            return bfont->registered;
        }
        return false;
    }

    void Unload(ReHandle h) override {
        BitmapFont* bfont = (BitmapFont*)h;
        if (IsFontOpend(bfont)) 
            UnloadFont(bfont);
    }

    void Clear() override {
        Destroy();
    }

private:
    BitmapFont* NewFont(const std::string& font_name, uint32_t font_no) {
        BitmapFont* font = new BitmapFont(font_name, font_no);
        font_list_.Append(font);
        return font;
    }
    void DeleteFont(BitmapFont* font) {
        font->RemoveFromList();
        delete font;
    }
    bool IsFontOpend(BitmapFont* bfont) {
        for (base::LinkNode<BitmapFont>* node = font_list_.head();
            node != font_list_.end(); 
            node = node->next()) {
            if (node->value() == bfont)
                return true;
        }
        return false;
    }
    void UnloadFont(BitmapFont* bfont) {
        lvgl_bitmap_font_close(&bfont->font);
        DeleteFont(bfont);
    }
    void Destroy() {
        base::LinkNode<BitmapFont>* node = font_list_.head();
        base::LinkNode<BitmapFont>* next;

        while (node != font_list_.end()) {
            next = node->next();
            UnloadFont(node->value());
            node = next;
        }
    }

private:
    base::LinkedList<BitmapFont> font_list_;
};

extern "C"
BASE_EXPORT bool LoaderCreate(std::vector<lvsim::ResourceLoader*> &loaders) {
    loaders.push_back(new BitmapLoader("bitmap"));
    return true;
}
