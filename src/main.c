#include <pebble.h>
#include <math.h>
Window *window, *menu, *record, *set;
TextLayer *welcome_screen, *text_layer, *bpm_text_layer, *record_text, *record_bpm;
SimpleMenuLayer *options_screen;
SimpleMenuSection sections[1];
SimpleMenuItem items[2];
static AppTimer* rep_timer;
static char accel_data[30];
const VibePattern short_tick = {
  .durations = (uint32_t []) {70},
      .num_segments = 1
};
static int running=0;
static int bpm=60;
static int frequency=0;
static char bpm_str[10];
static int tap_max = 10;
static int taps=0;
static int last_timestamp=0;
static int difference=0;

//
// Record clicking
void record_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_push(set,true);
}
void record_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, record_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, record_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, record_click_handler); 
}

// ----------------------------- FUNCTIONS ------------------------------
int magnitude_no_root(int16_t x,int16_t y,int16_t z){
    return x*x + y*y + z*z;
}

// Sets a vibration loop
void vibration_loop(void *data){
  //custom vibration
  vibes_enqueue_custom_pattern(short_tick);
  //sets timers to wait then call vibration_loop again
  rep_timer = app_timer_register(frequency,(AppTimerCallback) vibration_loop, NULL);
}

// Modifies bpm string
void set_bpm_string(){
  snprintf(bpm_str, 10,"BPM: %d", bpm);
}

// Updates frequency
void update_frequency(){
  frequency = 60000/bpm;
}

void accel_data_handler(AccelData *data, uint32_t num_samples) {
  for(uint32_t c = 0; c < num_samples && taps<=tap_max; c ++){
      if ((magnitude_no_root(data[c].x,data[c].y,data[c].z)-1000000) > 180000 && 
          ((int)(data[c].timestamp)>(last_timestamp+300))) {
          if(last_timestamp){
             difference+=data[c].timestamp - last_timestamp;
          }
          last_timestamp = data[c].timestamp;
          taps++;
      }
  }
  if (taps>tap_max) {
    bpm = 60000/((difference/(tap_max-1)));
    bpm = bpm - (bpm % 4);
    accel_data_service_unsubscribe();
    text_layer_set_text(record_text, "Approximate BPM:");
    set_bpm_string();
    text_layer_set_text(record_bpm, bpm_str);
    window_set_click_config_provider(record, record_click_config_provider);
  }
}



// -----------------------------CLICK HANDLING----------------------------
// Menu clicking
void window_click_handler(ClickRecognizerRef recognizer, void *context) {
  set_bpm_string();
  window_stack_push(menu,true);
}
void window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, window_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, window_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, window_click_handler); 
}

//Set clicking
void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(bpm<180){
      bpm+= 4;
    }
    update_frequency();
    set_bpm_string();
    text_layer_set_text(bpm_text_layer, bpm_str);
}
void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(bpm>20){
      bpm-= 4;
    }
    update_frequency();
    set_bpm_string();
    text_layer_set_text(bpm_text_layer, bpm_str);
}
void select_click_handler(ClickRecognizerRef recognizer, void *context) {  
    //if not running go to vibration loop, else stop 
    if(!running){
      update_frequency();
      running=1;
      rep_timer = app_timer_register(frequency, (AppTimerCallback) vibration_loop, NULL);
    }
    else{
      running=0;
      app_timer_cancel(rep_timer);
    }
}
void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}
// ---------------------------- CALLBACKS ----------------------------------
// Callback functionality, shows record when called
void set_callback(int index, void *ctx) {
  bpm = 60;
  window_stack_push(set,true);
}

void record_callback(int index, void *ctx) {
  window_stack_push(record,true);
}

// ---------------------- WINDOW LOADING/UNLOADING -------------------------
// Load elements into the window
void window_load(Window *window) {
  welcome_screen = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(welcome_screen, GColorClear);
  text_layer_set_text_color(welcome_screen, GColorBlack);
  text_layer_set_text_alignment(welcome_screen, GTextAlignmentCenter);
  text_layer_set_font(welcome_screen, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(welcome_screen, "Welcome to Metronome! \
\n \n From the next screen please select the mode you wish to use." );
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(welcome_screen));
}


// Unloading elements of window
void window_unload(Window *window) {
  text_layer_destroy(welcome_screen);
}

//-------------------------- MENU LOAD/UNLOAD -----------------------------
// Load elements into the menu
void menu_load(Window *menu) {
  // First menu item
  items[0] = (SimpleMenuItem) {
    .title = "Record Beat",
    .callback = record_callback
  };
  
  // Second menu item
  items[1] = (SimpleMenuItem) {
    .title = "Set BPM",
    .callback = set_callback
  };
  
  // Populating the section with items
  sections[0] = (SimpleMenuSection) {
    .title = "Modes",
    .num_items = 2,
    .items = items
  };
  
  // Initialzing simple menu layer
  options_screen = simple_menu_layer_create(GRect(0,0,144,168), menu, sections, 1, NULL);
  
  // Add options layer to menu root
  layer_add_child(window_get_root_layer(menu), simple_menu_layer_get_layer(options_screen));

}
// Destroy elements of the menu
void menu_unload(Window *menu) {
  simple_menu_layer_destroy(options_screen);
}

// ------------------------- SET LOAD/UNLOAD -------------------------
// Initializing set window elements
void set_load(Window *set) {
  //title layer
  text_layer = text_layer_create(GRect(0, 70, 144, 168));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorBlack);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(set), (Layer*) text_layer);
      
  //bpm text
  bpm_text_layer = text_layer_create(GRect(0,50,144,40));
  text_layer_set_background_color(bpm_text_layer, GColorClear);
  text_layer_set_text_color(bpm_text_layer, GColorBlack);
  text_layer_set_text_alignment(bpm_text_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(set), (Layer*) bpm_text_layer);  
  
  window_set_click_config_provider(set, click_config_provider);
  
  set_bpm_string();
  text_layer_set_text(bpm_text_layer, bpm_str);
  text_layer_set_text(text_layer, "Press select to start/stop counting! \n \n Up: +4 BPM \n Down: -4 BPM");
}

// Destroy set window elements
void set_unload(Window *set) {
  text_layer_destroy(text_layer);
  text_layer_destroy(bpm_text_layer);
  app_timer_cancel(rep_timer);
}
  
  
// ----------------------- RECORD LOAD/UNLOAD ---------------------------
// Initialize record window elements
void record_load(Window *record) {
  // Creating text 
  record_text = text_layer_create(GRect(0,43,144,50));
  text_layer_set_text(record_text, "Be the beat!");
  text_layer_set_text_alignment(record_text, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(record), text_layer_get_layer(record_text));
    
  record_bpm = text_layer_create(GRect(0,60,144,50));
  text_layer_set_text_alignment(record_bpm, GTextAlignmentCenter);

  
  //set up accel data service
  accel_data_service_subscribe(25,accel_data_handler);
  taps=0;
  difference=0;
  last_timestamp = 0;
  //set sampling rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  
  layer_add_child(window_get_root_layer(record), text_layer_get_layer(record_bpm));
  size_t heap_bytes_used(void);
}
  
void record_unload(Window *record) {
  text_layer_destroy(record_text);
  text_layer_destroy(record_bpm);
  app_timer_cancel(rep_timer);
}
  
  
// --------------------------- INIT/DEINIT ------------------------------
// Intialize main app's elements
void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  menu = window_create();
  window_set_window_handlers(menu, (WindowHandlers) {
    .load = menu_load,
    .unload = menu_unload
  });
  set = window_create();
  window_set_window_handlers(set, (WindowHandlers) {
    .load = set_load,
    .unload = set_unload
  });
  record = window_create();
  window_set_window_handlers(record, (WindowHandlers) {
    .load = record_load,
    .unload = record_load
  });
  window_stack_push(window,true);
  window_set_click_config_provider(window, window_click_config_provider);
}

// De-intialize main app's elements
void deinit() {
  window_destroy(window);
  window_destroy(menu);
  window_destroy(set);
  window_destroy(record);
}

// Main program loop
int main(void) {
  init();
  app_event_loop();
  deinit();
}