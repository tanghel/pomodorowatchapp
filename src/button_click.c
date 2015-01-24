#include <pebble.h>

static Window *window;
static TextLayer *text_pomodoro_count;
static TextLayer *text_pomodoro_duration;
static ActionBarLayer *action_bar;

static bool is_pomo_started = false;
static bool is_pomo_paused = false;
static bool is_pomo_break = false;

const int TARGET_SECONDS = 1500;
const int TARGET_BREAK_SECONDS = 300;
static int seconds_elapsed = 0;
static int pomodoro_count = 0;

static GBitmap *play_bitmap;
static GBitmap *pause_bitmap;
static GBitmap *stop_bitmap;

static void refresh_action_bar()
{
  if (is_pomo_started)
  {
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, stop_bitmap);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, pause_bitmap);
  }
  else if (is_pomo_break)
  {
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, stop_bitmap);
    action_bar_layer_clear_icon(action_bar, BUTTON_ID_DOWN);
  }
  else
  {
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, play_bitmap);
    action_bar_layer_clear_icon(action_bar, BUTTON_ID_DOWN);
  }
}

static int get_target_seconds()
{
  return is_pomo_break ? TARGET_BREAK_SECONDS : TARGET_SECONDS;
}

static void refresh_pomo_duration()
{
  int seconds_to_go = get_target_seconds() - seconds_elapsed;
  int minutes = seconds_to_go / 60;
  int seconds = seconds_to_go % 60;
  
  static char pomodoro_time_string[32];
  // Update the TextLayer
  snprintf(pomodoro_time_string, sizeof(pomodoro_time_string), "%02d:%02d", minutes, seconds);
  text_layer_set_text(text_pomodoro_duration, pomodoro_time_string);
}

static void refresh_pomo_count()
{
  static char pomodoro_count_string[32];
  // Update the TextLayer
  snprintf(pomodoro_count_string, sizeof(pomodoro_count_string), "%d", pomodoro_count);
  text_layer_set_text(text_pomodoro_count, pomodoro_count_string);
}

static void pomo_start()
{
  is_pomo_started = true;  
  is_pomo_break = false;
  
  refresh_action_bar();
  
  if (!is_pomo_paused)
  {
    seconds_elapsed = 0;
  }
    
  refresh_pomo_duration();
}

static void pomo_break()
{
  is_pomo_started = false;
  is_pomo_break = true;
  
  refresh_action_bar();
  
  seconds_elapsed = 0;
  refresh_pomo_duration();
  
  pomodoro_count++;
  refresh_pomo_count();
}

static void pomo_stop()
{
  is_pomo_started = false;
  is_pomo_break = false;
  
  refresh_action_bar();
  
  seconds_elapsed = 0;
  refresh_pomo_duration();
}

static void pomo_pause()
{
  is_pomo_started = false;
  is_pomo_paused = true;
  refresh_action_bar();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Select");
  if (is_pomo_started)
  {
    pomo_break();
  }
  else if (is_pomo_break)
  {
    pomo_stop();
  }
  else
  {
    pomo_start(); 
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) 
{
  if (is_pomo_started || is_pomo_break)
  {
    seconds_elapsed++;  
    refresh_pomo_duration();
    
    if (seconds_elapsed >= get_target_seconds())
    {
      if (is_pomo_break)
      {
        vibes_long_pulse();
        pomo_stop();
      }
      else
      {
        vibes_short_pulse();
        pomo_break();
      }
    }
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Down");
  if (is_pomo_started)
  {
    pomo_pause();
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  
  play_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ACTION_PLAY);
  pause_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ACTION_PAUSE);
  stop_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ACTION_STOP);
  
  refresh_action_bar();

  text_pomodoro_count = text_layer_create((GRect) { .origin = { 0, 23 }, .size = { bounds.size.w - 20, 42 } });
  text_layer_set_text_alignment(text_pomodoro_count, GTextAlignmentCenter);
  text_layer_set_font(text_pomodoro_count, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(text_pomodoro_count));
  
  refresh_pomo_count();

  text_pomodoro_duration = text_layer_create((GRect) { .origin = { 0, 87 }, .size = { bounds.size.w - 20, 34 } });
  text_layer_set_text_alignment(text_pomodoro_duration, GTextAlignmentCenter);
  text_layer_set_font(text_pomodoro_duration, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(text_pomodoro_duration));
  
  refresh_pomo_duration();
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void window_unload(Window *window) {
  text_layer_destroy(text_pomodoro_count);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
	.load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}