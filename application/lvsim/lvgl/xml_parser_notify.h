/*
 * Copyright 2025 wtcat 
 */

#ifndef XML_PARSER_NOTIFY_H_
#define XML_PARSER_NOTIFY_H_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _XML_SOURCE_CODE
#define LV_XML_PARSER_EXTERN
#else
#define LV_XML_PARSER_EXTERN extern
#endif

typedef void (*lv_xml_parser_callback_t)(lv_xml_parser_state_t* state, 
    const char* type, const char** attrs);

LV_XML_PARSER_EXTERN lv_xml_parser_callback_t _font_parser_callback;
LV_XML_PARSER_EXTERN lv_xml_parser_callback_t _image_parser_callback;

static inline void lv_xml_register_image_parser_cb(lv_xml_parser_callback_t cb) {
    _image_parser_callback = cb;
}

static inline void lv_xml_register_font_parser_cb(lv_xml_parser_callback_t cb) {
    _font_parser_callback = cb;
}

#ifdef __cplusplus
}
#endif
#endif /* XML_PARSER_NOTIFY_H_ */

