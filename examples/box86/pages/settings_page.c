#include "settings_page.h"
#include "more_settings_page.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
    lv_obj_t*    btn_network;
    lv_obj_t*    lbl_network;
} SettingsPageData;

static void on_network_toggle(lv_event_t* e)
{
    SettingsPageData* d = lv_event_get_user_data(e);
    SystemModel sys;
    device_store_snapshot_system(d->store, &sys);

    AppMsg msg = {
        .type      = MSG_UI_SET_SYSTEM,
        .device_id = -1,
        .value     = sys.network_enabled ? 0 : 1,
    };
    strncpy(msg.field, "network", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);

    /* 乐观更新按钮颜色 */
    int new_val = sys.network_enabled ? 0 : 1;
    if (new_val) {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0xFFB300), 0);
    } else {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0x607D8B), 0);
    }
    lv_label_set_text(d->lbl_network, new_val ? "Network: ON" : "Network: OFF");
}

static void on_more_settings(lv_event_t* e)
{
    SettingsPageData* d = lv_event_get_user_data(e);
    MoreSettingsPageParams params = { .bus = d->bus, .store = d->store };
    /* 使用 page_manager 跳转 */
    extern void page_manager_open(const char* name, void* params);
    page_manager_open("MoreSettings", &params);
}

lv_obj_t* settings_page_create(lv_obj_t* parent, SettingsPageParams* params)
{
    SettingsPageData* d = calloc(1, sizeof(SettingsPageData));
    d->bus   = params->bus;
    d->store = params->store;

    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_user_data(cont, d);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Network toggle button */
    d->btn_network = lv_button_create(cont);
    lv_obj_set_size(d->btn_network, 140, 60);
    d->lbl_network = lv_label_create(d->btn_network);
    lv_obj_center(d->lbl_network);
    lv_obj_add_event_cb(d->btn_network, on_network_toggle, LV_EVENT_CLICKED, d);

    /* More settings button */
    lv_obj_t* btn_more = lv_button_create(cont);
    lv_obj_set_size(btn_more, 140, 60);
    lv_obj_t* lbl_more = lv_label_create(btn_more);
    lv_label_set_text(lbl_more, "More Settings");
    lv_obj_center(lbl_more);
    lv_obj_add_event_cb(btn_more, on_more_settings, LV_EVENT_CLICKED, d);

    settings_page_refresh(cont);
    return cont;
}

void settings_page_refresh(lv_obj_t* page)
{
    SettingsPageData* d = lv_obj_get_user_data(page);
    if (!d) return;
    SystemModel sys;
    device_store_snapshot_system(d->store, &sys);
    if (sys.network_enabled) {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0xFFB300), 0);
        lv_label_set_text(d->lbl_network, "Network: ON");
    } else {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0x607D8B), 0);
        lv_label_set_text(d->lbl_network, "Network: OFF");
    }
}

void settings_page_destroy(lv_obj_t* page)
{
    SettingsPageData* d = lv_obj_get_user_data(page);
    if (d) { free(d); lv_obj_set_user_data(page, NULL); }
}
