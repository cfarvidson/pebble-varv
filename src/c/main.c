#include <pebble.h>

// Varv: a 24-hour one-hand watchface for Pebble Time 2 (emery, 200x228),
// after the look of Nifty by Fnord Prefect. One long white hand that turns
// once a day, 12:00 at the top and 00:00 at the bottom. Battery on the left,
// weekday and date on the right, connection and quiet time shown at the top.

static Window *s_window;
static Layer  *s_canvas_layer;
static BatteryChargeState charge_state;

// Noon = top = 0; midnight = bottom.
static int32_t minutes_to_angle(int local_min) {
  int shifted = (local_min + 12 * 60) % (24 * 60);
  return (int32_t)((int64_t)TRIG_MAX_ANGLE * shifted / (24 * 60));
}

static GPoint polar(GPoint center, int r, int32_t angle) {
  return GPoint(
    center.x + (r * sin_lookup(angle)) / TRIG_MAX_RATIO,
    center.y - (r * cos_lookup(angle)) / TRIG_MAX_RATIO
  );
}

static void draw_centered(GContext *ctx, const char *text, GFont font, GPoint at, int w) {
  graphics_draw_text(ctx, text, font, GRect(at.x - w / 2, at.y - 11, w, 22),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect  bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  // Slightly larger than the screen width, like Nifty, so the side ticks
  // touch the edge.
  int radius = bounds.size.w / 2 + 2;

  time_t     now   = time(NULL);
  struct tm *local = localtime(&now);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // 96 quarter-hour ticks: white on the hour (bigger at 0, 6, 12, 18),
  // dark grey in between.
  for (int i = 0; i < 96; i++) {
    int32_t angle    = minutes_to_angle(i * 15);
    bool    on_hour  = (i % 4 == 0);
    bool    on_sixth = (i % 24 == 0);
    int     len      = on_sixth ? 16 : on_hour ? 12 : 8;
    graphics_context_set_stroke_color(ctx, on_hour ? GColorWhite : GColorDarkGray);
    graphics_context_set_stroke_width(ctx, on_sixth ? 3 : on_hour ? 2 : 1);
    graphics_draw_line(ctx, polar(center, radius, angle), polar(center, radius - len, angle));
  }

  // Even-hour numerals inside the ticks, 12 at the top and 00 at the bottom.
  GFont small = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  graphics_context_set_text_color(ctx, GColorLightGray);
  for (int h = 0; h < 24; h += 2) {
    GPoint pos = polar(center, radius - 27, minutes_to_angle(h * 60));
    char num_str[3];
    snprintf(num_str, sizeof(num_str), "%02d", h);
    graphics_draw_text(ctx, num_str, small, GRect(pos.x - 10, pos.y - 9, 20, 16),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  graphics_context_set_text_color(ctx, GColorWhite);

  // Top label: connection loss and quiet time, otherwise "pebble".
  const char *label = !connection_service_peek_pebble_app_connection() ? "no link"
                    : quiet_time_is_active() ? "quiet" : "pebble";
  draw_centered(ctx, label, font, GPoint(center.x, center.y - 44), 80);

  // Battery on the left.
  char batt[6];
  snprintf(batt, sizeof(batt), "%d%%", charge_state.charge_percent);
  draw_centered(ctx, batt, font, GPoint(center.x - 44, center.y), 50);

  // Weekday and date on the right.
  char day[8], date[8];
  strftime(day, sizeof(day), "%a", local);
  strftime(date, sizeof(date), "%d.%m.", local);
  draw_centered(ctx, day,  font, GPoint(center.x + 42, center.y - 10), 60);
  draw_centered(ctx, date, font, GPoint(center.x + 42, center.y + 10), 60);

  // The one long white hand, rounded ends, one turn per day.
  int32_t angle = minutes_to_angle(local->tm_hour * 60 + local->tm_min);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 9);
  graphics_draw_line(ctx, center, polar(center, radius - 26, angle));

  graphics_context_set_fill_color(ctx, GColorMelon);
  graphics_fill_circle(ctx, center, 7);
}

static void battery_handler(BatteryChargeState state) {
  charge_state = state;
  layer_mark_dirty(s_canvas_layer);
}

static void connection_handler(bool connected) {
  if (!connected) vibes_double_pulse();
  layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  charge_state = battery_state_service_peek();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load   = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = connection_handler,
  });
}

static void prv_deinit(void) {
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
