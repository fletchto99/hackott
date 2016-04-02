#include <pebble.h>

static Window *s_window;
static Layer *s_window_layer, *s_dots_layer, *s_progress_layer, *s_average_layer;
static TextLayer *s_time_layer, *s_step_layer;

static char s_current_time_buffer[8], s_current_steps_buffer[16];
static int s_step_count = 0, s_step_goal = 0, s_step_average = 0;

//Color when you are below your avg step count
GColor color_loser;

//color when you are above your avg step count
GColor color_winner;

// Is step data available?
bool step_data_is_available() {
  return HealthServiceAccessibilityMaskAvailable &
    health_service_metric_accessible(HealthMetricStepCount,
      time_start_of_today(), time(NULL));
}

// Daily step goal
static void get_step_goal() {
  const time_t start = time_start_of_today();
  const time_t end = start + SECONDS_PER_DAY;
  s_step_goal = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

// Todays current step count
static void get_step_count() {
//   s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
    
    //Hardcoded value for testing on cloud pebble, no way to fake data... yet.
     s_step_count = 100; 
}

// Average daily step count for this time of day
static void get_step_average() {
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  s_step_average = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

/*
 * Writes the step count to the screen
 */
static void display_step_count() {
  int thousands = s_step_count / 1000;
  int hundreds = s_step_count % 1000;
  static char s_emoji[5];

  if(s_step_count >= s_step_average) {
    text_layer_set_text_color(s_step_layer, color_winner);
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F60C");
  } else {
    text_layer_set_text_color(s_step_layer, color_loser);
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F4A9");
  }

  if(thousands > 0) {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d,%03d %s", thousands, hundreds, s_emoji);
  } else {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d %s", hundreds, s_emoji);
  }

  text_layer_set_text(s_step_layer, s_current_steps_buffer);
}

static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventSignificantUpdate) {
    get_step_goal();
  }

  //if we have a step event then let's updated the layers by marking them as dirty
  if(event != HealthEventSleepUpdate) {
    //retrieve new values
    get_step_count();
    get_step_average();
    
    //Update the displayed text
    display_step_count();
    
    //mark the progress layers as dirty, which will call their update proc
    layer_mark_dirty(s_progress_layer);
    layer_mark_dirty(s_average_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  strftime(s_current_time_buffer, sizeof(s_current_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%l:%M", tick_time);

  text_layer_set_text(s_time_layer, s_current_time_buffer);
}

static void dots_layer_update_proc(Layer *layer, GContext *ctx) {
  const GRect inset = grect_inset(layer_get_bounds(layer), GEdgeInsets(6));

  const int num_dots = 12;
  for(int i = 0; i < num_dots; i++) {
    GPoint pos = gpoint_from_polar(inset, GOvalScaleModeFitCircle,
      DEG_TO_TRIGANGLE(i * 360 / num_dots));
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_circle(ctx, pos, 2);
  }
}

static void progress_layer_update_proc(Layer *layer, GContext *ctx) {
  const GRect inset = grect_inset(layer_get_bounds(layer), GEdgeInsets(2));

  graphics_context_set_fill_color(ctx,
    s_step_count >= s_step_average ? color_winner : color_loser);

  graphics_fill_radial(ctx, inset, GOvalScaleModeFitCircle, 12,
    DEG_TO_TRIGANGLE(0),
    DEG_TO_TRIGANGLE(360 * (s_step_count / s_step_goal)));
}

/*
 * The process which is called when the layer needs to be updated (marked as dirty)
 */
static void average_layer_update_proc(Layer *layer, GContext *ctx) {
  //don't draw anything if the avg doesn't make any sense
  if(s_step_average < 1) {
    return;
  }

  //set some variables
  const GRect inset = grect_inset(layer_get_bounds(layer), GEdgeInsets(2));
  graphics_context_set_fill_color(ctx, GColorYellow);

  //draw progress to avg
  int trigangle = DEG_TO_TRIGANGLE(360 * s_step_average / s_step_goal);
  int line_width_trigangle = 1000;
  // draw a very narrow radial (it's just a line)
  graphics_fill_radial(ctx, inset, GOvalScaleModeFitCircle, 12,
    trigangle - line_width_trigangle, trigangle);
}

static void window_load(Window *window) {
  GRect window_bounds = layer_get_bounds(s_window_layer);

  // Dots for the progress indicator
  s_dots_layer = layer_create(window_bounds);
  layer_set_update_proc(s_dots_layer, dots_layer_update_proc);
  layer_add_child(s_window_layer, s_dots_layer);

  // Progress indicator
  s_progress_layer = layer_create(window_bounds);
  layer_set_update_proc(s_progress_layer, progress_layer_update_proc);
  layer_add_child(s_window_layer, s_progress_layer);

  // Average indicator
  s_average_layer = layer_create(window_bounds);
  layer_set_update_proc(s_average_layer, average_layer_update_proc);
  layer_add_child(s_window_layer, s_average_layer);

  // Create a layer to hold the current time
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(82, 78), window_bounds.size.w, 38));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));

  // Create a layer to hold the current step count
  s_step_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 54), window_bounds.size.w, 38));
  text_layer_set_background_color(s_step_layer, GColorClear);
  text_layer_set_font(s_step_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_step_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_step_layer));

  // Subscribe to health events if we can, otherwise just a basic watchface
  if(step_data_is_available()) {
    health_service_events_subscribe(health_handler, NULL);
  }
}

static void window_unload(Window *window) {
  //Destroy all of the layers
  layer_destroy(text_layer_get_layer(s_time_layer));
  layer_destroy(text_layer_get_layer(s_step_layer));
  layer_destroy(s_dots_layer);
  layer_destroy(s_progress_layer);
  layer_destroy(s_average_layer);
}

void init() {
  //Color when below avg step count
  color_loser = GColorPictonBlue;

  //color when above avg step count
  color_winner = GColorJaegerGreen;

  s_window = window_create();
  s_window_layer = window_get_root_layer(s_window);
  window_set_background_color(s_window, GColorBlack);

  //Handlers for window events
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  //pushes the window to the stack
  window_stack_push(s_window, true);

  //Subscribes to the timer service, for each minute
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

void deinit() {
    //Nothing to do here for strider.... Could do things like saving data to persistent storage
}

/*
 * App Entrypoint
 */
int main() {
  //Setup
  init();
    
  //Blocking until the windowstack is empty (app is ready to exit)
  app_event_loop();
    
  //Removal
  deinit();
}
