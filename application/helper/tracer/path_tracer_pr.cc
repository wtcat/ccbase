/*
 * Copyright 2023 wtcat 
 */
#include <stdint.h>
#include <stdio.h>

extern "C" void uiview_heap_user_print(void *fout, const void *user) {
    FILE* fp = (FILE *)fout;
    fprintf(fp, "ViewID(%d)", (uint16_t)(uint32_t)user);
}
