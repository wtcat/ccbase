/*
 * Copyright 2025 wtcat 
 */

#include <stdbool.h>
#include <stdio.h>

#include "driver/simulator.h"
#include "driver/file_watcher.h"
#include "lvgl/lvgl.h"

#include "lvgl/src/core/lv_obj_private.h"
#undef main

#if 0
static void reload_view(void) {
    lv_result_t rc;

    lv_obj_clean(lv_screen_active());
    rc = lv_xml_component_register_from_file("A:../../../xml/my_h3.xml");
    if (rc == LV_RESULT_INVALID)
        return;

    rc = lv_xml_component_register_from_file("A:../../../xml/my_card.xml");
    if (rc == LV_RESULT_INVALID)
        return;

    rc = lv_xml_component_register_from_file("A:../../../xml/my_button.xml");
    if (rc == LV_RESULT_INVALID)
        return;

    rc = lv_xml_component_register_from_file("A:../../../xml/view.xml");
    if (rc == LV_RESULT_INVALID)
        return;

    lv_xml_register_font(NULL, "lv_montserrat_18", &lv_font_montserrat_18);

    lv_obj_t* obj = (lv_obj_t*)lv_xml_create(lv_screen_active(), "view", NULL);
    lv_obj_set_pos(obj, 10, 10);

    //const char * my_button_attrs[] = {
    //    "x", "10",
    //    "y", "-10",
    //    "align", "bottom_left",
    //    "btn_text", "New button",
    //    NULL, NULL,
    //};

    //lv_xml_component_unregister("my_button");

    //lv_xml_create(lv_screen_active(), "my_button", my_button_attrs);

    //const char* slider_attrs[] = {
    //    "x", "200",
    //    "y", "-15",
    //    "align", "bottom_left",
    //    "value", "30",
    //    NULL, NULL,
    //};

    //lv_obj_t* slider = (lv_obj_t*)lv_xml_create(lv_screen_active(), "lv_slider", slider_attrs);
    //lv_obj_set_width(slider, 100);
}

static void view_reload(struct lvgl_message* msg) {
    lv_xml_component_unregister("my_h3");
    lv_xml_component_unregister("my_card");
    lv_xml_component_unregister("my_button");
    lv_xml_component_unregister("view");

    reload_view();
}

static void xml_file_update(const char* file) {
    printf("File(%s) update\n", file);
    lvgl_send_message(view_reload, 0, NULL);
}

static void show_layout(lv_obj_t* obj, int level) {
    int children = lv_obj_get_child_count(obj);

    for (int i = 0; i < level; i++)
        printf("    ");

    printf("=>obj(%p@%s) flags(0x%x) childcnt(%d) coord{(%d, %d) (%d, %d)} class_p(%p)\n",
        obj, 
        lv_obj_get_name(obj),
        obj->flags,
        children,
        obj->coords.x1, obj->coords.y1, obj->coords.x2, obj->coords.y2,
        obj->class_p);

    for (int i = 0; i < children; i++)
        show_layout(lv_obj_get_child(obj, i), level + 1);
}

static bool keypad_process(int code, bool pressed) {
    if (pressed) {
        switch (code) {
        case SDLK_ESCAPE:
            printf("Exit code: %d\n", code);
            return false;
        case SDLK_SPACE:
            show_layout(lv_screen_active(), 0);
            break;
        default:
            break;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    file_watcher_init(500);
    file_watcher_add("../../../xml/view.xml", xml_file_update);

	lvgl_runloop(400, 400, reload_view, keypad_process);
    return 0;
}

#else

static void ui_lvgen__my_card_grad1_grad_init(lv_grad_dsc_t* dsc) {
    dsc->extend = LV_GRAD_EXTEND_PAD;
    dsc->dir = LV_GRAD_DIR_HOR;
    dsc->stops[0].color = lv_color_hex(0x00ff0000);
    dsc->stops[0].opa = 255;
    dsc->stops[0].frac = 0;
    dsc->stops_count++;
    dsc->stops[1].color = lv_color_hex(0x0000ff00);
    dsc->stops[1].opa = 255;
    dsc->stops[1].frac = 255;
    dsc->stops_count++;
}

void ui_lvgen__my_card_gray_style_init(lv_style_t* style) {
    lv_style_init(style);
    static lv_grad_dsc_t gard_dsc0;
    static bool gard_dsc0_ready;
    if (!gard_dsc0_ready) {
        gard_dsc0_ready = true;
        ui_lvgen__my_card_grad1_grad_init(&gard_dsc0);
    }

    lv_style_set_bg_grad(style, &gard_dsc0);
}

#define MAX_STYLES 7
#define MAX_FONTS  0
#define MAX_IMAGES 0

typedef struct {
#if MAX_STYLES > 0
    lv_style_t     styles[MAX_STYLES];
#endif
#if MAX_FONTS > 0
    lv_font_t      fonts[MAX_FONTS];
#endif
#if MAX_IMAGES > 0
    lv_image_dsc_t images[MAX_IMAGES];
#endif
} lv_view__private_t;


static void ui_lvgen__view_btn_style_style_init(lv_style_t* style) {
    lv_style_init(style);
    lv_style_set_bg_color(style, lv_color_hex(0x00000080));
    lv_style_set_bg_opa(style, 255);
}

static void ui_lvgen__view_btn_pr_style_style_init(lv_style_t* style) {
    lv_style_init(style);
    lv_style_set_bg_opa(style, 255);
}

static void ui_lvgen__view_red_border_style_init(lv_style_t* style) {
    lv_style_init(style);
    lv_style_set_border_color(style, lv_color_hex(0x00ff0000));
}

static lv_obj_t* ui_lvgen__view_create(lv_obj_t* parent, lv_view__private_t* priv) {

    lv_obj_t* obj_0 = lv_obj_create(parent);
    lv_obj_set_width(obj_0, 380);
    lv_obj_set_height(obj_0, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(obj_0, lv_color_hex(0x00bbbbff), LV_PART_MAIN);

    lv_obj_t* obj_1 = lv_obj_create(obj_0);
    lv_obj_set_style_radius(obj_1, 3, LV_PART_MAIN);
    lv_obj_set_width(obj_1, lv_pct(100));
    lv_obj_set_height(obj_1, LV_SIZE_CONTENT);
    ui_lvgen__my_card_gray_style_init(&priv->styles[0]);
    lv_obj_add_style(obj_1, &priv->styles[0], LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_1, lv_color_hex(0x00cccccc), LV_PART_MAIN);

    lv_obj_t* label_0 = lv_label_create(obj_1);
    lv_obj_set_align(label_0, LV_ALIGN_LEFT_MID);
    lv_label_set_text(label_0, "Card 25");

    lv_obj_t* button_0 = lv_button_create(obj_1);
    lv_obj_set_width(button_0, 100);

    lv_obj_t* label_1 = lv_label_create(button_0);
    lv_obj_set_style_text_color(label_1, lv_color_hex(0x00ffa020), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_1, lv_font_get_default(), LV_PART_MAIN);
    lv_obj_set_align(label_1, LV_ALIGN_CENTER);
    lv_label_set_text(label_1, "No action");
    ui_lvgen__view_btn_style_style_init(&priv->styles[1]);
    lv_obj_add_style(button_0, &priv->styles[1], LV_PART_MAIN);
    ui_lvgen__view_btn_pr_style_style_init(&priv->styles[2]);
    lv_obj_add_style(button_0, &priv->styles[2], LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_align(button_0, LV_ALIGN_RIGHT_MID);
    lv_obj_set_y(obj_1, 0);
    ui_lvgen__view_red_border_style_init(&priv->styles[3]);
    lv_obj_add_style(obj_1, &priv->styles[3], LV_PART_MAIN);

    lv_obj_t* obj_2 = lv_obj_create(obj_0);
    lv_obj_set_style_radius(obj_2, 3, LV_PART_MAIN);
    lv_obj_set_width(obj_2, lv_pct(100));
    lv_obj_set_height(obj_2, LV_SIZE_CONTENT);
    ui_lvgen__my_card_gray_style_init(&priv->styles[4]);
    lv_obj_add_style(obj_2, &priv->styles[4], LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_2, lv_color_hex(0x00ffaaaa), LV_PART_MAIN);

    lv_obj_t* label_2 = lv_label_create(obj_2);
    lv_obj_set_align(label_2, LV_ALIGN_LEFT_MID);
    lv_label_set_text(label_2, "Card 2xxxxxxxxx");

    lv_obj_t* button_1 = lv_button_create(obj_2);
    lv_obj_set_width(button_1, 100);

    lv_obj_t* label_3 = lv_label_create(button_1);
    lv_obj_set_style_text_color(label_3, lv_color_hex(0x00ffa020), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_3, lv_font_get_default(), LV_PART_MAIN);
    lv_obj_set_align(label_3, LV_ALIGN_CENTER);
    lv_label_set_text(label_3, "Apply");
    ui_lvgen__view_btn_style_style_init(&priv->styles[5]);
    lv_obj_add_style(button_1, &priv->styles[5], LV_PART_MAIN);
    ui_lvgen__view_btn_pr_style_style_init(&priv->styles[6]);
    lv_obj_add_style(button_1, &priv->styles[6], LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_align(button_1, LV_ALIGN_RIGHT_MID);
    lv_obj_set_y(obj_2, 130);

    lv_obj_t* obj_3 = lv_obj_create(obj_0);
    lv_obj_set_y(obj_3, 300);
    lv_obj_set_width(obj_3, 50);
    lv_obj_set_height(obj_3, LV_SIZE_CONTENT);

    lv_obj_t* label_4 = lv_label_create(obj_3);
    lv_obj_set_align(label_4, LV_ALIGN_CENTER);
    lv_label_set_text(label_4, "Hello");
    lv_label_set_long_mode(label_4, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);

    return obj_0;
}


static void view_init(void) {
	static lv_view__private_t styles;
	ui_lvgen__view_create(lv_screen_active(), &styles);
}

int main(int argc, char* argv[]) {
	lvgl_runloop(400, 400, view_init, NULL);
	return 0;
}
#endif
