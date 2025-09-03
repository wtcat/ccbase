/*
 * Copyright(c) 2025 wtcat
 */

#ifndef LVGEN_CDEFS_H_
#define LVGEN_CDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef lv_res_t                lv_result_t;
typedef lv_img_dsc_t            lv_image_dsc_t;
typedef lv_disp_t               lv_display_t;
typedef lv_disp_rotation_t      lv_display_rotation_t;
typedef lv_anim_ready_cb_t      lv_anim_completed_cb_t;
typedef lv_scr_load_anim_t      lv_screen_load_anim_t;

#if LV_USE_BUTTONMATRIX
typedef lv_buttonmatrix_ctrl_t  lv_btnmatrix_ctrl_t;
#endif /* LV_USE_BUTTONMATRIX */

/**********************
 *      MACROS
 **********************/
#define LV_DISPLAY_ROTATION_0                            LV_DISP_ROTATION_0              
#define LV_DISPLAY_ROTATION_90                           LV_DISP_ROTATION_90             
#define LV_DISPLAY_ROTATION_180                          LV_DISP_ROTATION_180            
#define LV_DISPLAY_ROTATION_270                          LV_DISP_ROTATION_270            
#define LV_DISPLAY_RENDER_MODE_PARTIAL                   LV_DISP_RENDER_MODE_PARTIAL     
#define LV_DISPLAY_RENDER_MODE_DIRECT                    LV_DISP_RENDER_MODE_DIRECT      
#define LV_DISPLAY_RENDER_MODE_FULL                      LV_DISP_RENDER_MODE_FULL        
#define LV_BUTTONMATRIX_BUTTON_NONE                      LV_BTNMATRIX_BTN_NONE           
#define LV_BUTTONMATRIX_CTRL_HIDDEN                      LV_BTNMATRIX_CTRL_HIDDEN        
#define LV_BUTTONMATRIX_CTRL_NO_REPEAT                   LV_BTNMATRIX_CTRL_NO_REPEAT     
#define LV_BUTTONMATRIX_CTRL_DISABLED                    LV_BTNMATRIX_CTRL_DISABLED      
#define LV_BUTTONMATRIX_CTRL_CHECKABLE                   LV_BTNMATRIX_CTRL_CHECKABLE     
#define LV_BUTTONMATRIX_CTRL_CHECKED                     LV_BTNMATRIX_CTRL_CHECKED       
#define LV_BUTTONMATRIX_CTRL_CLICK_TRIG                  LV_BTNMATRIX_CTRL_CLICK_TRIG    
#define LV_BUTTONMATRIX_CTRL_POPOVER                     LV_BTNMATRIX_CTRL_POPOVER       
#define LV_BUTTONMATRIX_CTRL_CUSTOM_1                    LV_BTNMATRIX_CTRL_CUSTOM_1      
#define LV_BUTTONMATRIX_CTRL_CUSTOM_2                    LV_BTNMATRIX_CTRL_CUSTOM_2      
#define LV_IMAGEBUTTON_STATE_RELEASED                    LV_IMGBTN_STATE_RELEASED        
#define LV_IMAGEBUTTON_STATE_PRESSED                     LV_IMGBTN_STATE_PRESSED         
#define LV_IMAGEBUTTON_STATE_DISABLED                    LV_IMGBTN_STATE_DISABLED        
#define LV_IMAGEBUTTON_STATE_CHECKED_RELEASED            LV_IMGBTN_STATE_CHECKED_RELEASED
#define LV_IMAGEBUTTON_STATE_CHECKED_PRESSED             LV_IMGBTN_STATE_CHECKED_PRESSED 
#define LV_IMAGEBUTTON_STATE_CHECKED_DISABLED            LV_IMGBTN_STATE_CHECKED_DISABLED
#define LV_RESULT_OK                                     LV_RES_OK                       
#define LV_RESULT_INVALID                                LV_RES_INV                      
#define LV_INDEV_STATE_PRESSED                           LV_INDEV_STATE_PR               
#define LV_INDEV_STATE_RELEASED                          LV_INDEV_STATE_REL              
#define lv_obj_delete                                    lv_obj_del                      
#define lv_obj_delete_async                              lv_obj_del_async                
#define lv_obj_remove_flag                               lv_obj_clear_flag               
#define lv_obj_remove_state                              lv_obj_clear_state              
#define lv_indev_set_display                             lv_indev_set_disp               
#define lv_indev_active                                  lv_indev_get_act                
#define lv_screen_active                                 lv_scr_act                      
#define lv_display_delete                                lv_disp_remove                  
#define lv_display_set_default                           lv_disp_set_default             
#define lv_display_get_default                           lv_disp_get_default             
#define lv_display_get_next                              lv_disp_get_next                
#define lv_display_set_rotation                          lv_disp_set_rotation            
#define lv_display_get_horizontal_resolution             lv_disp_get_hor_res             
#define lv_display_get_vertical_resolution               lv_disp_get_ver_res             
#define lv_display_get_physical_horizontal_resolution    lv_disp_get_physical_hor_res    
#define lv_display_get_physical_vertical_resolution      lv_disp_get_physical_ver_res    
#define lv_display_get_offset_x                          lv_disp_get_offset_x            
#define lv_display_get_offset_y                          lv_disp_get_offset_y            
#define lv_display_get_rotation                          lv_disp_get_rotation            
#define lv_display_get_dpi                               lv_disp_get_dpi                 
#define lv_display_get_antialiasing                      lv_disp_get_antialiasing        
#define lv_display_flush_ready                           lv_disp_flush_ready             
#define lv_display_flush_is_last                         lv_disp_flush_is_last           
#define lv_display_get_screen_active                     lv_disp_get_scr_act             
#define lv_display_get_screen_prev                       lv_disp_get_scr_prev            
#define lv_screen_load                                   lv_disp_load_scr                
#define lv_screen_load                                   lv_scr_load                     
#define lv_screen_load_anim                              lv_scr_load_anim                
#define lv_display_get_layer_top                         lv_disp_get_layer_top           
#define lv_display_get_layer_sys                         lv_disp_get_layer_sys           
#define lv_display_send_event                            lv_disp_send_event              
#define lv_display_set_theme                             lv_disp_set_theme               
#define lv_display_get_theme                             lv_disp_get_theme               
#define lv_display_get_inactive_time                     lv_disp_get_inactive_time       
#define lv_display_trigger_activity                      lv_disp_trig_activity           
#define lv_display_enable_invalidation                   lv_disp_enable_invalidation     
#define lv_display_is_invalidation_enabled               lv_disp_is_invalidation_enabled 
#define lv_display_refr_timer                            lv_disp_refr_timer              
#define lv_display_get_refr_timer                        lv_disp_get_refr_timer          
#define lv_timer_delete                                  lv_timer_del                    
#define lv_anim_delete                                   lv_anim_del                     
#define lv_anim_delete_all                               lv_anim_del_all                 
#define lv_anim_set_completed_cb                         lv_anim_set_ready_cb            
#define lv_group_delete                                  lv_group_del                    
#define lv_text_get_size                                 lv_txt_get_size                 
#define lv_text_get_width                                lv_txt_get_width                
#define lv_image_create                                  lv_img_create                   
#define lv_image_set_src                                 lv_img_set_src                  
#define lv_image_set_offset_x                            lv_img_set_offset_x             
#define lv_image_set_offset_y                            lv_img_set_offset_y             
#define lv_image_set_rotation                            lv_img_set_angle                
#define lv_image_set_pivot                               lv_img_set_pivot                
#define lv_image_set_scale                               lv_img_set_zoom                 
#define lv_image_set_antialias                           lv_img_set_antialias            
#define lv_image_get_src                                 lv_img_get_src                  
#define lv_image_get_offset_x                            lv_img_get_offset_x             
#define lv_image_get_offset_y                            lv_img_get_offset_y             
#define lv_image_get_rotation                            lv_img_get_angle                
#define lv_image_get_pivot                               lv_img_get_pivot                
#define lv_image_get_scale                               lv_img_get_zoom                 
#define lv_image_get_antialias                           lv_img_get_antialias            
#define lv_imagebutton_create                            lv_imgbtn_create                
#define lv_imagebutton_set_src                           lv_imgbtn_set_src               
#define lv_imagebutton_set_state                         lv_imgbtn_set_state             
#define lv_imagebutton_get_src_left                      lv_imgbtn_get_src_left          
#define lv_imagebutton_get_src_middle                    lv_imgbtn_get_src_middle        
#define lv_imagebutton_get_src_right                     lv_imgbtn_get_src_right         
#define lv_list_set_button_text                          lv_list_set_btn_text            
#define lv_list_get_button_text                          lv_list_get_btn_text            
#define lv_list_add_button                               lv_list_add_btn                 
#define lv_button_create                                 lv_btn_create    

#define lv_buttonmatrix_create                           lv_btnmatrix_create             
#define lv_buttonmatrix_set_map                          lv_btnmatrix_set_map            
#define lv_buttonmatrix_set_ctrl_map                     lv_btnmatrix_set_ctrl_map       
#define lv_buttonmatrix_set_selected_button              lv_btnmatrix_set_selected_btn   
#define lv_buttonmatrix_set_button_ctrl                  lv_btnmatrix_set_btn_ctrl       
#define lv_buttonmatrix_clear_button_ctrl                lv_btnmatrix_clear_btn_ctrl     
#define lv_buttonmatrix_set_button_ctrl_all              lv_btnmatrix_set_btn_ctrl_all   
#define lv_buttonmatrix_clear_button_ctrl_all            lv_btnmatrix_clear_btn_ctrl_all 
#define lv_buttonmatrix_set_button_width                 lv_btnmatrix_set_btn_width      
#define lv_buttonmatrix_set_one_checked                  lv_btnmatrix_set_one_checked    
#define lv_buttonmatrix_get_map                          lv_btnmatrix_get_map            
#define lv_buttonmatrix_get_selected_button              lv_btnmatrix_get_selected_btn   
#define lv_buttonmatrix_get_button_text                  lv_btnmatrix_get_btn_text       
#define lv_buttonmatrix_has_button_ctrl                  lv_btnmatrix_has_button_ctrl    
#define lv_buttonmatrix_get_one_checked                  lv_btnmatrix_get_one_checked

#define lv_tabview_get_tab_bar                           lv_tabview_get_tab_btns         
#define lv_tabview_get_tab_active                        lv_tabview_get_tab_act          
#define lv_tabview_set_active                            lv_tabview_set_act           

#define lv_tileview_get_tile_active                      lv_tileview_get_tile_act        
#define lv_tileview_set_tile_by_index                    lv_obj_set_tile_id              
#define lv_tileview_set_tile                             lv_obj_set_tile            

#define lv_roller_set_visible_row_count                  lv_roller_set_visible_row_cnt   
#define lv_roller_get_option_count                       lv_roller_get_option_cnt  

#define lv_table_set_column_count                        lv_table_set_col_cnt            
#define lv_table_set_row_count                           lv_table_set_row_cnt            
#define lv_table_get_column_count                        lv_table_get_col_cnt            
#define lv_table_get_row_count                           lv_table_get_row_cnt            
#define lv_table_set_column_width                        lv_table_set_col_width          
#define lv_table_get_column_width                        lv_table_get_col_width  

#define lv_dropdown_get_option_count                     lv_dropdown_get_option_cnt   

#define lv_obj_get_child_count                           lv_obj_get_child_cnt            
#define lv_obj_get_display                               lv_obj_get_disp                 
#define lv_obj_delete_anim_completed_cb                  lv_obj_delete_anim_ready_cb  

#define LV_STYLE_ANIM_DURATION                           LV_STYLE_ANIM_TIME              
#define LV_STYLE_IMAGE_OPA                               LV_STYLE_IMG_OPA                
#define LV_STYLE_IMAGE_RECOLOR                           LV_STYLE_IMG_RECOLOR            
#define LV_STYLE_IMAGE_RECOLOR_OPA                       LV_STYLE_IMG_RECOLOR_OPA        
#define LV_STYLE_SHADOW_OFFSET_X                         LV_STYLE_SHADOW_OFS_X           
#define LV_STYLE_SHADOW_OFFSET_Y                         LV_STYLE_SHADOW_OFS_Y           
#define LV_STYLE_TRANSFORM_ROTATION                      LV_STYLE_TRANSFORM_ANGLE  

#define lv_obj_get_style_anim_duration                   lv_obj_get_style_anim_time      
#define lv_obj_get_style_image_opa                       lv_obj_get_style_img_opa        
#define lv_obj_get_style_image_recolor                   lv_obj_get_style_img_recolor    
#define lv_obj_get_style_image_recolor_filtered          lv_obj_get_style_img_recolor_filtered
#define lv_obj_get_style_image_recolor_opa               lv_obj_get_style_img_recolor_opa
#define lv_obj_get_style_shadow_offset_x                 lv_obj_get_style_shadow_ofs_x   
#define lv_obj_get_style_shadow_offset_y                 lv_obj_get_style_shadow_ofs_y   
#define lv_obj_get_style_transform_rotation              lv_obj_get_style_transform_angle
#define lv_obj_get_style_bg_image_src                    lv_obj_get_style_bg_img_src     
#define lv_obj_get_style_bg_image_recolor                lv_obj_get_style_bg_img_recolor 
#define lv_obj_get_style_bg_image_recolor_opa            lv_obj_get_style_bg_img_recolor_opa

#define lv_obj_set_style_anim_duration                   lv_obj_set_style_anim_time      
#define lv_obj_set_style_image_opa                       lv_obj_set_style_img_opa        
#define lv_obj_set_style_image_recolor                   lv_obj_set_style_img_recolor    
#define lv_obj_set_style_image_recolor_opa               lv_obj_set_style_img_recolor_opa
#define lv_obj_set_style_shadow_offset_x                 lv_obj_set_style_shadow_ofs_x   
#define lv_obj_set_style_shadow_offset_y                 lv_obj_set_style_shadow_ofs_y   
#define lv_obj_set_style_transform_scale                 lv_obj_set_style_transform_zoom 
#define lv_obj_set_style_transform_rotation              lv_obj_set_style_transform_angle
#define lv_obj_set_style_bg_image_src                    lv_obj_set_style_bg_img_src     
#define lv_obj_set_style_bg_image_recolor                lv_obj_set_style_bg_img_recolor 
#define lv_obj_set_style_bg_image_recolor_opa            lv_obj_set_style_bg_img_recolor_opa

#define lv_style_set_anim_duration                       lv_style_set_anim_time          
#define lv_style_set_image_opa                           lv_style_set_img_opa            
#define lv_style_set_image_recolor                       lv_style_set_img_recolor        
#define lv_style_set_image_recolor_opa                   lv_style_set_img_recolor_opa    
#define lv_style_set_shadow_offset_x                     lv_style_set_shadow_ofs_x       
#define lv_style_set_shadow_offset_y                     lv_style_set_shadow_ofs_y       
#define lv_style_set_transform_rotation                  lv_style_set_transform_angle    
#define lv_style_set_transform_scale                     lv_style_set_transform_zoom     
#define lv_style_set_bg_image_src                        lv_style_set_bg_img_src         
#define lv_style_set_bg_image_recolor                    lv_style_set_bg_img_recolor     
#define lv_style_set_bg_image_recolor_opa                lv_style_set_bg_img_recolor_opa 

#define lv_keyboard_get_selected_button                  lv_keyboard_get_selected_btn    
#define lv_keyboard_get_button_text                      lv_keyboard_get_btn_text        
                 
#define lv_bin_decoder_open                              lv_image_decoder_built_in_open  
#define lv_bin_decoder_close                             lv_image_decoder_built_in_close 

#ifdef __cplusplus
}
#endif
#endif /* LVGEN_CDEFS_H_ */
