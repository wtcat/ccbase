/*
 * Copyright 2025 wtcat 
 */

#include "ui_template.h"

/* 
 * Helper Macro 
 */
#define SDK_RESOURCE_NUM(table) (sizeof(table) / sizeof(table[0]))
#define SDK_RESOURCE_ITEM(view_id, scene_id, npic, ntxt, nset) \
    { scene_id, view_id, npic, nset, ntxt }

/*
 * Resource table
 */
static const sdk_resources_t sdk_resource_table[] = {
    SDK_RESOURCE_ITEM(FIND_PHONE_VIEW, SCENE_FIND_PHONE, 4, 2, 0),
    SDK_RESOURCE_ITEM(UI_SHARED_VIEW,  SCENE_PHONE_VIEW, 1, 1, 0),
    //TODO: 
};

UI_PUBLIC_API
const sdk_resources_t* _sdk_view_get_resource(uint16_t view_id) {
    //TODO: sort table and use binary search algorithm
    for (size_t i = 0; i < SDK_RESOURCE_NUM(sdk_resource_table); i++) {
        if (view_id == sdk_resource_table[i].view_id)
            return &sdk_resource_table[i];
    }
    return NULL;
}
