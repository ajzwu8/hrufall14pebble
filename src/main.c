#include <pebble.h>
 
static Window* window;
static TextLayer *text_layer;
static AppTimer* rep_timer;
const VibePattern short_tick = {
  .durations = (uint32_t []) {70},
      .num_segments = 1
};

static int running=0;
static int bpm=60;
static int frequency=0;
static TextLayer *bpm_text_layer;
static char bpm_str[10];

void vibration_loop(void *data){
  //custom vibration
  vibes_enqueue_custom_pattern(short_tick);
  //sets timers to wait then call vibration_loop again
  rep_timer = app_timer_register(frequency,(AppTimerCallback) vibration_loop, NULL);
}

//modifies bpm string
void set_bpm_string(){
  snprintf(bpm_str, 10,"bpm: %d", bpm);
  text_layer_set_text(bpm_text_layer, bpm_str);
}

void update_frequency(){
  frequency = 60000/bpm;
}

void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
    text_layer_set_text(text_layer, "You pressed UP!");
    if(bpm<180){
      bpm+= 4;
    }
    update_frequency();
    set_bpm_string();
}
 
void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
    text_layer_set_text(text_layer, "You pressed DOWN!");
    if(bpm>20){
      bpm-= 4;
    }
    update_frequency();
    set_bpm_string();
}
 
void select_click_handler(ClickRecognizerRef recognizer, void *context)
{  
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
    vibes_double_pulse();
    set_bpm_string();
    text_layer_set_text(text_layer, "You pressed SELECT!");
}

void click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}
 
/* Load all Window sub-elements */
void window_load(Window *window)
{
    text_layer = text_layer_create(GRect(0, 30, 144, 40));
    text_layer_set_background_color(text_layer, GColorClear);
    text_layer_set_text_color(text_layer, GColorBlack);
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), (Layer*) text_layer);
  
    bpm_text_layer = text_layer_create(GRect(0,50,144,40));
    text_layer_set_background_color(bpm_text_layer, GColorClear);
    text_layer_set_text_color(bpm_text_layer, GColorBlack);
    text_layer_set_text_alignment(bpm_text_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), (Layer*) bpm_text_layer);  

    window_set_click_config_provider(window, click_config_provider);
    
    text_layer_set_text(text_layer, "My first watchapp!");
}
 
/* Un-load all Window sub-elements */
void window_unload(Window *window)
{
    text_layer_destroy(text_layer);
}
 
/* Initialize the main app elements */
void init()
{
    window = window_create();
    WindowHandlers handlers = {
        .load = window_load,
        .unload = window_unload
    };
    window_set_window_handlers(window, (WindowHandlers) handlers);
    window_stack_push(window, true);
}
 
/* De-initialize the main app elements */
void deinit()
{
    window_destroy(window);
}
 
/* Main app lifecycle */
int main(void)
{
    init();
    app_event_loop();
    deinit();
}