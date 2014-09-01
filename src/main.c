// Written by Kenny Kim, August 2014.
// For HFASt Wearable Devices Project.
// Phone Call App.


/////////////////////////////////////////// HEADERS //////////////////////////////////////////
#include <pebble.h>
#include <pebble_fonts.h>  

  
/////////////////////////////////////////// VARIABLES //////////////////////////////////////////  
int exit_counter, call_counter, timeout_counter;
char name[32], number[16];


/////////////////////////////////////////// POINTERS //////////////////////////////////////////
Window *window, *reply_window;
TextLayer *incoming_text, *caller_name, *caller_number, *busy_text, *callback_text, *lol_text;
GBitmap *options_bitmap, *callicon_bitmap, *replyoptions_bitmap; // Contains data
BitmapLayer *options_layer, *replyoptions_layer; // Presents it as a layer


/////////////////////////////////////////// Structs //////////////////////////////////////////  
enum {
    KEY_BUTTON_EVENT = 0,
    BUTTON_EVENT_UP = 1,
    BUTTON_EVENT_DOWN = 2,
    BUTTON_EVENT_SELECT = 3,
    DISPLAY_CALL_CALLER = 4,
    DISPLAY_CALL_NUMBER = 5,
    LOG_MESSAGE = 6,
    STOP_VIBRATE = 7,
    REPLY_BUSY = 8,
    REPLY_CALLBACK = 9,
    REPLY_LOL = 10,
    KEY_BUTTON_REPLY = 11
};


////////////////////////////////////////// VIBRATOR //////////////////////////////////////////
void vibe()
{
  //Create an array of ON-OFF-ON etc durations in milliseconds (TOTAL: 30 seconds)
  uint32_t segments[] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                         1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                         1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                         1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                         1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                         1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
 
  //Create a VibePattern structure with the segments and length of the pattern as fields
  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
};
  //Trigger the custom pattern to be executed
  vibes_enqueue_custom_pattern(pattern);
}


void confirm_vibe()
{
  //Create an array of ON-OFF-ON etc durations in milliseconds (TOTAL: 30 seconds)
  uint32_t segments[] = {100, 100, 100};
 
  //Create a VibePattern structure with the segments and length of the pattern as fields
  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
};
  //Trigger the custom pattern to be executed
  vibes_enqueue_custom_pattern(pattern);
}


/////////////////////////////////////////// TIMER //////////////////////////////////////////
static void exit_timer(struct tm* tick_time, TimeUnits units_changed) {
        if (exit_counter-- <= 0) {
          vibes_cancel();
          window_stack_pop_all(true);
          vibes_cancel();
        }
}


static void timeout_timer(struct tm* tick_time, TimeUnits units_changed) {
        if (timeout_counter-- <= 0) {
          vibes_cancel();
          window_stack_pop(true);
        }
}


static void timer(struct tm* tick_time, TimeUnits units_changed) {
        if (call_counter-- <= 0) {
          text_layer_set_text(incoming_text, "Call Ended");
          exit_counter = 3;
          tick_timer_service_subscribe(SECOND_UNIT, exit_timer);
        }
}


static void reply_timer(struct tm* tick_time, TimeUnits units_changed) {
        if (call_counter-- <= 0) {
          text_layer_set_text(busy_text, "Call Ended");
          exit_counter = 3;
          tick_timer_service_subscribe(SECOND_UNIT, exit_timer);
        }
}


////////////////////////////////////////// APPMSG STUFF //////////////////////////////////////////
void send_int(uint8_t key, uint8_t cmd)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
      
    Tuplet value = TupletInteger(key, cmd);
    dict_write_tuplet(iter, &value);
      
    app_message_outbox_send();
}


void process_tuple(Tuple *t)
{
  //Get key
  int key = t->key;
 
  //Get integer value, if present
  int value = t->value->int32;
 
  //Get string value, if present
  char string_value[32];
  strcpy(string_value, t->value->cstring);
 
  //Decide what to do
  switch(key) {
    case DISPLAY_CALL_CALLER:
      //Caller name received
      snprintf(name, sizeof("firstname couldbereallylongname"), "%s", string_value);
      text_layer_set_text(caller_name, (char*) &name);  
      break;
    case DISPLAY_CALL_NUMBER:
      //Caller number received
      snprintf(number, sizeof("X-XXX-XXX-XXXX"), "%s", string_value);
      text_layer_set_text(caller_number, (char*) &number);  
      break;
    case STOP_VIBRATE:
      //Stop vibration command received
      timeout_counter = value;
      tick_timer_service_subscribe(SECOND_UNIT, timeout_timer);  
      break;
  }
}


void in_received_handler(DictionaryIterator *iter, void *context) 
{
  (void) context;
    
  vibe();
    
  //Get data
   Tuple *t = dict_read_first(iter);
   while(t != NULL)
   {
     process_tuple(t);
       
     //Get next
     t = dict_read_next(iter);
   }
} 

/*
void reply_in_received_handler(DictionaryIterator *iter, void *context) 
{
  (void) context;
  
  //Get data
  Tuple *t = dict_read_first(iter);
  while(t != NULL)
  {
    process_tuple(t);
         
    //Get next
    t = dict_read_next(iter);
  }
} 
*/


void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Dropped: %i", reason); 
    send_int(LOG_MESSAGE, reason);
  
}


/////////////////////////////////////////// WINDOWS //////////////////////////////////////////
void window_load(Window *window)
{
  // Creating texts. Pebble screen is 144x168 pixels
  // "Incoming Call" text
  incoming_text = text_layer_create(GRect(5, 0, 144, 168));
  text_layer_set_background_color(incoming_text, GColorClear);
  text_layer_set_text_color(incoming_text, GColorBlack);
  text_layer_set_font(incoming_text, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(incoming_text));
  text_layer_set_text(incoming_text, "Incoming Call");
  
  // Caller name text
  caller_name = text_layer_create(GRect(5, 47, 123, 168));
  text_layer_set_background_color(caller_name, GColorClear);
  text_layer_set_text_color(caller_name, GColorBlack);
  text_layer_set_font(caller_name, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(caller_name));
  //text_layer_set_text(caller_name, name);
  
  // Caller number text
  caller_number = text_layer_create(GRect(5, 120, 123, 168));
  text_layer_set_background_color(caller_number, GColorClear);
  text_layer_set_text_color(caller_number, GColorBlack);
  text_layer_set_font(caller_number, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(caller_number));
  //text_layer_set_text(caller_number, number);
  
  //Load bitmaps into GBitmap structures
  options_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OPTIONS);
  callicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CALLICON);
  // Create BitmapLayers to show GBitmaps and add to Window. "Option" image is 21 x 146 pixels
  options_layer = bitmap_layer_create(GRect(123, 4, 21, 146));
  bitmap_layer_set_bitmap(options_layer, options_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(options_layer));
  
   window_set_status_bar_icon(window, callicon_bitmap);
}


void window_unload(Window *window)
{
  //We will safely destroy the Window's elements here!
  text_layer_destroy(incoming_text);   // Destroy Text
  text_layer_destroy(caller_name);     // Destroy Text
  text_layer_destroy(caller_number);     // Destroy Text
  gbitmap_destroy(options_bitmap);     // Destroy GBitmaps
  gbitmap_destroy(callicon_bitmap);     // Destroy GBitmaps
  bitmap_layer_destroy(options_layer); // Destroy BitmapLayers
}


void replywindow_load(Window *reply_window)
{
  // Creating texts. Pebble screen is 144x168 pixels
  // "Busy" text
  busy_text = text_layer_create(GRect(5, 2, 144, 168));
  text_layer_set_background_color(busy_text, GColorClear);
  text_layer_set_text_color(busy_text, GColorBlack);
  text_layer_set_font(busy_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_get_root_layer(reply_window), text_layer_get_layer(busy_text));
  text_layer_set_text(busy_text, "1. Busy");
  
  // "Will call back" text
  callback_text = text_layer_create(GRect(5, 56, 144, 168));
  text_layer_set_background_color(callback_text, GColorClear);
  text_layer_set_text_color(callback_text, GColorBlack);
  text_layer_set_font(callback_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_get_root_layer(reply_window), text_layer_get_layer(callback_text));
  text_layer_set_text(callback_text, "2. Will call back");
  
  // "lol" text
  lol_text = text_layer_create(GRect(5, 115, 144, 168));
  text_layer_set_background_color(lol_text, GColorClear);
  text_layer_set_text_color(lol_text, GColorBlack);
  text_layer_set_font(lol_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_get_root_layer(reply_window), text_layer_get_layer(lol_text));
  text_layer_set_text(lol_text, "3. lol");
  
  //Load bitmaps into GBitmap structures
  replyoptions_bitmap = gbitmap_create_with_resource(RESOURCE_ID_REPLYOPTIONS);
  // Create BitmapLayers to show GBitmaps and add to Window. "Option" image is 21 x 146 pixels
  replyoptions_layer = bitmap_layer_create(GRect(123, 4, 21, 146));
  bitmap_layer_set_bitmap(replyoptions_layer, replyoptions_bitmap);
  layer_add_child(window_get_root_layer(reply_window), bitmap_layer_get_layer(replyoptions_layer)); 
}


void replywindow_unload(Window *message_window)
{
  text_layer_destroy(busy_text);   // Destroy Text
  text_layer_destroy(callback_text);   // Destroy Text
  text_layer_destroy(lol_text);   // Destroy Text
  gbitmap_destroy(replyoptions_bitmap);     // Destroy GBitmaps
  bitmap_layer_destroy(replyoptions_layer); // Destroy BitmapLayers
}


/////////////////////////////////////////// BUTTON BEHAVIOURS //////////////////////////////////////////
void reply_up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  text_layer_set_text(busy_text, "Sending Msg...");
  text_layer_set_text(callback_text, " ");
  text_layer_set_text(lol_text, " ");
  confirm_vibe();
  send_int(KEY_BUTTON_REPLY, REPLY_BUSY);
  call_counter = 2;
  tick_timer_service_subscribe(SECOND_UNIT, reply_timer);
}
 

void reply_down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  text_layer_set_text(busy_text, "Sending Msg...");
  text_layer_set_text(callback_text, " ");
  text_layer_set_text(lol_text, " ");
  confirm_vibe();
  send_int(KEY_BUTTON_REPLY, REPLY_LOL);
  call_counter = 2;
  tick_timer_service_subscribe(SECOND_UNIT, reply_timer);
  
}
 

void reply_select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  text_layer_set_text(busy_text, "Sending Msg...");
  text_layer_set_text(callback_text, " ");
  text_layer_set_text(lol_text, " ");
  confirm_vibe();
  send_int(KEY_BUTTON_REPLY, REPLY_CALLBACK);
  call_counter = 2;
  tick_timer_service_subscribe(SECOND_UNIT, reply_timer);

}


void reply_click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, reply_up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, reply_down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, reply_select_click_handler);
}


void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  text_layer_set_text(incoming_text, "Call Accepted");
  vibes_cancel();
  confirm_vibe();
  send_int(KEY_BUTTON_EVENT, BUTTON_EVENT_UP);
  call_counter = 10;
  tick_timer_service_subscribe(SECOND_UNIT, timer);
}
 

void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  text_layer_set_text(incoming_text, "Declining Call");
  vibes_cancel();
  confirm_vibe();
  send_int(KEY_BUTTON_EVENT, BUTTON_EVENT_DOWN);
  exit_counter = 2;
  tick_timer_service_subscribe(SECOND_UNIT, exit_timer);
}
 

void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  vibes_cancel();
  send_int(KEY_BUTTON_EVENT, BUTTON_EVENT_SELECT);
  confirm_vibe();
  
  reply_window = window_create();
  window_set_window_handlers(reply_window, (WindowHandlers) {
    .load = replywindow_load,
    .unload = replywindow_unload,
  });
  
  //Register AppMessage events
 // app_message_register_inbox_received(reply_in_received_handler);                   
 // app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());    //Large input and output buffer sizes
  
  window_set_click_config_provider(reply_window, reply_click_config_provider);
  
  window_stack_push(reply_window, true);
}


void click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}


/////////////////////////////////////////// INIT //////////////////////////////////////////
void init()
{ 
  //Initialize the app elements here!  
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
    
  //Register AppMessage events
  app_message_register_inbox_received(in_received_handler);           
  app_message_register_inbox_dropped(in_dropped_handler);           
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());    //Large input and output buffer sizes
  
  window_set_click_config_provider(window, click_config_provider);
  //window_counter = 1;
  //tick_timer_service_subscribe(SECOND_UNIT, window_timer);
  window_stack_push(window, true);
}
 

/////////////////////////////////////////// DEINIT //////////////////////////////////////////
void deinit()
{
  //De-initialize elements here to save memory!
  window_destroy(window);              // Destroy window
  window_destroy(reply_window);
  vibes_cancel();
}


/////////////////////////////////////////// MAIN //////////////////////////////////////////
int main(void)
{
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
  vibes_cancel();
}