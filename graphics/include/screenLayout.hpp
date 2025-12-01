#pragma once

#include <esp_err.h>
#include <lvgl.h>
#include <vector>

namespace macdap
{
    class ScreenLayout
    {
    private:
        lv_obj_t *screen;
        lv_obj_t *header;
        lv_obj_t *left_grp;
        lv_obj_t *header_label;
        lv_obj_t *right_grp;
        lv_obj_t *main_area;
        lv_obj_t *footer;
        lv_obj_t *footer_label;
        
        int32_t screen_width;
        int32_t screen_height;
        int32_t header_height;
        int32_t footer_height;
        
        std::vector<lv_obj_t*> left_icons;
        std::vector<lv_obj_t*> right_icons;

    public:
        ScreenLayout(lv_display_t *display, 
                    lv_style_t *header_style = nullptr,
                    const char *header_text = "", 
                    lv_style_t *footer_style = nullptr,
                    const char *footer_text = "",
                    int32_t header_h = 16, 
                    int32_t footer_h = 16);
        ~ScreenLayout();

        // Icon management
        lv_obj_t *add_left_icon();
        lv_obj_t *add_left_icon(const lv_image_dsc_t *icon_src);
        lv_obj_t *add_right_icon();
        lv_obj_t *add_right_icon(const lv_image_dsc_t *icon_src);
        
        // Text management
        void set_header_text(const char *text);
        lv_obj_t *get_header_label();
        void set_footer_text(const char *text);
        lv_obj_t *get_footer_label();
        
        // Area getters
        lv_obj_t *get_main_area();
        lv_obj_t *get_header();
        lv_obj_t *get_footer();
        
        // Cleanup
        void clear_left_icons();
        void clear_right_icons();
    };
}
