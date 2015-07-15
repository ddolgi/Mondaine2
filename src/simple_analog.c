#include "simple_analog.h"

#include "pebble.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;

//static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4], s_day_buffer[6];
static GPoint center;

static void setPointOnCenterCircle(GPoint * point, int32_t angle, uint16_t radius) {
  point->x = (int16_t)( sin_lookup(angle) * (int32_t)radius / TRIG_MAX_RATIO) + center.x;
  point->y = (int16_t)(-cos_lookup(angle) * (int32_t)radius / TRIG_MAX_RATIO) + center.y;
} 

static void setBackPointOnCenterCircle(GPoint * point, int32_t angle, uint16_t radius) {
  setPointOnCenterCircle(point, angle + TRIG_MAX_ANGLE / 2, radius);
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  GPoint innerPoint;
  GPoint outerPoint;
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int i = 0; i < 60; ++i) {
    setPointOnCenterCircle(&outerPoint, i * TRIG_MAX_ANGLE / 60, 72);  
    if(i % 5 == 0) {
      setPointOnCenterCircle(&innerPoint, i * TRIG_MAX_ANGLE / 60, 60);
      graphics_context_set_stroke_width(ctx, 3);
    }
    else {
      setPointOnCenterCircle(&innerPoint, i * TRIG_MAX_ANGLE / 60, 68);
      graphics_context_set_stroke_width(ctx, 1);
    }
    graphics_draw_line(ctx, innerPoint, outerPoint);        
  }
}
  
static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  int32_t hour_angle = TRIG_MAX_ANGLE * t->tm_hour / 12 + minute_angle / 12;
  
  GPoint back_hand;
  int16_t back_hand_length = 15;
  // hour hand
  GPoint hour_hand;
  setPointOnCenterCircle(&hour_hand, hour_angle, 45);
  setBackPointOnCenterCircle(&back_hand, hour_angle, back_hand_length);
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, hour_hand, back_hand);

  // minute hand
  GPoint minute_hand;
  setPointOnCenterCircle(&minute_hand, minute_angle, 64);
  setBackPointOnCenterCircle(&back_hand, minute_angle, back_hand_length);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, minute_hand, back_hand);

  // second hand
  GPoint second_hand;
  setPointOnCenterCircle(&second_hand, second_angle, 58);
  setBackPointOnCenterCircle(&back_hand, second_angle, back_hand_length);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_draw_line(ctx, second_hand, back_hand);
  graphics_draw_circle(ctx, second_hand, 3);
  
  // dot in the middle
  graphics_draw_circle(ctx, center, 1);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  s_day_label = text_layer_create(GRect(46, 104, 27, 20));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorBlack);
  text_layer_set_text_color(s_day_label, GColorWhite);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  s_num_label = text_layer_create(GRect(73, 104, 18, 20));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorBlack);
  text_layer_set_text_color(s_num_label, GColorWhite);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // init hand paths
//  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
//  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&bounds);
//  gpath_move_to(s_minute_arrow, center);
//  gpath_move_to(s_hour_arrow, center);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {
//  gpath_destroy(s_minute_arrow);
//  gpath_destroy(s_hour_arrow);

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
