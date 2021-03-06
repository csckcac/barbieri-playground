#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <Ecore_Evas.h>
#include <Ecore.h>
#include <Edje.h>
#include <sys/time.h>
#include <math.h>
#include "vlist.h"
#include "key_monitor.h"
#include "icon.h"

#define WIDTH 800
#define HEIGHT 480
#define THEME "default.edj"
#define THEME_GROUP "main"
#define TITLE "Text List Test"
#define WM_NAME "TextList"
#define WM_CLASS "main"

#define F_PRECISION 0.00001


typedef struct app
{
    char *theme;
    Evas_Object *edje_main;
    Evas_Object *arrow_up;
    Evas_Object *arrow_down;
    Evas_Object *list;
    Evas_List *items;
    Ecore_Evas  *ee;
    Evas *evas;
    struct {
        key_monitor_t down;
        key_monitor_t up;
    } keys;
} app_t;

static inline int
eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static void
key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev;
    app_t *app;
    const char *k;

    ev = (Evas_Event_Key_Down *)event_info;
    app = (app_t *)data;

    k = ev->keyname;

    if (eq(k, "Down"))
        key_monitor_down(&app->keys.down);
    else if (eq(k, "Up"))
        key_monitor_down(&app->keys.up);
    else if (eq(k, "Escape"))
        ecore_main_loop_quit();
    else if (eq(k, "f") || eq(k, "F6")) {
        if (ecore_evas_fullscreen_get(app->ee)) {
            ecore_evas_fullscreen_set(app->ee, 0);
            ecore_evas_cursor_set(app->ee, NULL, 0, 0, 0);
        } else {
            ecore_evas_fullscreen_set(app->ee, 1);
            ecore_evas_cursor_set(app->ee, " ", 999, 0, 0);
        }
    }
}

static void
key_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Up *ev;
    app_t *app;
    const char *k;

    ev = (Evas_Event_Key_Up *)event_info;
    app = (app_t *)data;

    k = ev->keyname;

    if (eq(k, "Down"))
        key_monitor_up(&app->keys.down);
    else if (eq(k, "Up"))
        key_monitor_up(&app->keys.up);
}

static void
move_up_start(void *d)
{
    app_t *app = (app_t *)d;

    vlist_scroll_start(app->list, VLIST_SCROLL_DIR_UP);
}

static void
move_up_stop(void *d)
{
    app_t *app = (app_t *)d;

    vlist_scroll_stop(app->list, VLIST_SCROLL_DIR_UP);
}

static void
key_up_start(void *d, key_monitor_t *m)
{
    move_up_start(d);
}

static void
key_up_stop(void *d, key_monitor_t *m)
{
    move_up_stop(d);
}

static void
mouse_down_arrow_up(void *d, Evas *e, Evas_Object *obj, void *event_info)
{
    move_up_start(d);
}

static void
mouse_up_arrow_up(void *d, Evas *e, Evas_Object *obj, void *event_info)
{
    move_up_stop(d);
}

static void
move_down_start(void *d)
{
    app_t *app = (app_t *)d;

    vlist_scroll_start(app->list, VLIST_SCROLL_DIR_DOWN);
}

static void
move_down_stop(void *d)
{
    app_t *app = (app_t *)d;

    vlist_scroll_stop(app->list, VLIST_SCROLL_DIR_DOWN);
}

static void
key_down_start(void *d, key_monitor_t *m)
{
    move_down_start(d);
}

static void
key_down_stop(void *d, key_monitor_t *m)
{
    move_down_stop(d);
}

static void
mouse_down_arrow_down(void *d, Evas *e, Evas_Object *obj, void *event_info)
{
    move_down_start(d);
}

static void
mouse_up_arrow_down(void *d, Evas *e, Evas_Object *obj, void *event_info)
{
    move_down_stop(d);
}

static void
mouse_down_back_button(void *d, Evas *e, Evas_Object *obj, void *event_info)
{
    ecore_main_loop_quit();
}

static void
dim_arrows(app_t *app, int index)
{
    double p;
    int c, total;

    total = vlist_count(app->list);
    if (total < 1)
        return;

    p = ((float)(index + 1)) / ((float)total);
    c = 255 * p;
    if (c > 255)
        c = 255;
    if (c < 0)
        c = 0;
    evas_object_color_set(app->arrow_up, c, c, c, c);

    p = (float)index / ((float)total);
    c = 255 * (1.0 - p);
    if (c > 255)
        c = 255;
    if (c < 0)
        c = 0;
    evas_object_color_set(app->arrow_down, c, c, c, c);
}

static void
list_selection_changed(Evas_Object *o, void *item_data, int index,
                       void *user_data)
{
    app_t *app = (app_t *)user_data;

    dim_arrows(app, index);

    fprintf(stderr, "now selected: %p (%d/%d)\n", item_data, index,
            vlist_count(o));
}

static int
app_signal_exit(void *data, int type, void *event)
{

    ecore_main_loop_quit();
    return 1;
}

static void
resize_cb(Ecore_Evas *ee)
{
    app_t *app;
    Evas_Coord w, h;

    app = ecore_evas_data_get(ee, "app");
    evas_output_viewport_get(app->evas, NULL, NULL, &w, &h);
    evas_object_resize(app->edje_main, w, h);
}

static Evas_Object *
list_row_new(Evas_Object *list, Evas *evas, void *user_data)
{
    return icon_new(evas);
}

static void
list_row_set(Evas_Object *list, Evas_Object *row, void *data, void *user_data)
{
    if (data) {
        icon_image_set(row, (char*)data);
        icon_text_set(row, (char*)data);
        evas_object_show(row);
    } else {
        evas_object_hide(row);
    }
}

static void
list_row_freeze(Evas_Object *list, Evas_Object *row, void *user_data)
{
    edje_object_freeze(row);
}

static void
list_row_thaw(Evas_Object *list, Evas_Object *row, void *user_data)
{
    edje_object_thaw(row);
}

int
main(int argc, char *argv[])
{
    app_t app;
    int i, selected_offset, item_h;
    double speed, accel;
    Evas_Object *o;
    Evas_Hash *hash;
    Evas_List *itr;
    vlist_row_ops_t row_ops = {
        .new = list_row_new,
        .set = list_row_set,
        .freeze = list_row_freeze,
        .thaw = list_row_thaw
    };

    ecore_init();
    ecore_app_args_set(argc, (const char **)argv);
    ecore_evas_init();
    edje_init();

    edje_frametime_set(1.0 / 30.0);

    memset(&app, 0, sizeof(app));

    app.ee = ecore_evas_software_x11_new(NULL, 0,  0, 0, WIDTH, HEIGHT);
    ecore_evas_data_set(app.ee, "app", &app);
    ecore_evas_title_set(app.ee, TITLE);
    ecore_evas_name_class_set(app.ee, WM_NAME, WM_CLASS);
    app.theme = THEME;
    speed = 0.1;
    accel = 0.00001;
    selected_offset = 1;

    for (i=1; i < argc; i++)
        if (strcmp (argv[i], "-fs") == 0)
            ecore_evas_fullscreen_set(app.ee, 1);
        else if (strncmp (argv[i], "-theme=", sizeof("-theme=") - 1) == 0)
            app.theme = argv[i] + sizeof("-theme=") - 1;
        else if (strncmp (argv[i], "-speed=", sizeof("-speed=") - 1) == 0)
            speed = atof(argv[i] + sizeof("-theme=") - 1);
        else if (strncmp (argv[i], "-accel=", sizeof("-accel=") - 1) == 0)
            accel = atof(argv[i] + sizeof("-accel=") - 1);
        else if (strncmp (argv[i], "-offset=", sizeof("-offset=") - 1) == 0)
            selected_offset = atoi(argv[i] + sizeof("-offset=") - 1);
        else if (argv[i][0] != '-')
            app.items = evas_list_append(app.items, argv[i]);

    app.evas = ecore_evas_get(app.ee);

    app.edje_main = edje_object_add(app.evas);
    hash = evas_hash_add(NULL, "edje_file", app.theme);
    evas_data_attach_set(app.evas, hash);
    if (!edje_object_file_set(app.edje_main, app.theme, THEME_GROUP)) {
        fprintf(stderr, "Failed to load file \"%s\", part \"%s\".\n",
                app.theme, THEME_GROUP);
        return 1;
    }

    o = icon_new(app.evas);
    evas_object_geometry_get(o, NULL, NULL, NULL, &item_h);
    evas_object_del(o);

    app.list = vlist_new(app.evas, item_h, row_ops, &app);
    vlist_conf_set(app.list, 0, selected_offset);
    vlist_scroll_conf_set(app.list, speed, accel, 0.2);
    vlist_connect_selection_changed(app.list, list_selection_changed, &app);

    evas_object_move(app.edje_main, 0, 0);
    evas_object_resize(app.edje_main, WIDTH, HEIGHT);

    edje_object_part_swallow(app.edje_main, "contents_swallow", app.list);

    evas_object_show(app.edje_main);
    evas_object_show(app.list);
    ecore_evas_show(app.ee);

    vlist_freeze(app.list);
    for (itr = app.items; itr != NULL; itr = itr->next)
        vlist_append(app.list, itr->data);
    vlist_thaw(app.list);

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, app_signal_exit, NULL);
    ecore_evas_callback_resize_set(app.ee, resize_cb);

    evas_object_event_callback_add(app.edje_main, EVAS_CALLBACK_KEY_DOWN,
                                   key_down, &app);
    evas_object_event_callback_add(app.edje_main, EVAS_CALLBACK_KEY_UP,
                                   key_up, &app);

    key_monitor_setup(&app.keys.down, key_down_start, key_down_stop, &app);
    key_monitor_setup(&app.keys.up, key_up_start, key_up_stop, &app);

    evas_object_focus_set(app.edje_main, 1);

    app.arrow_down = edje_object_part_object_get(app.edje_main, "arrow_down");
    o = edje_object_part_object_get(app.edje_main, "arrow_down_area");
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                   mouse_down_arrow_down,
                                   &app);
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
                                   mouse_up_arrow_down,
                                   &app);

    app.arrow_up = edje_object_part_object_get(app.edje_main, "arrow_up");
    o = edje_object_part_object_get(app.edje_main, "arrow_up_area");
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                   mouse_down_arrow_up,
                                   &app);
    evas_object_event_callback_add(app.arrow_up, EVAS_CALLBACK_MOUSE_UP,
                                   mouse_up_arrow_up,
                                   &app);

    o = edje_object_part_object_get(app.edje_main, "back_button");
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                   mouse_down_back_button,
                                   &app);

    dim_arrows(&app, 0);

    ecore_main_loop_begin();

    return 0;
}
