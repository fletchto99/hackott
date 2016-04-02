// Copyright [2015] Pebble Technology

//Check if we are compiling for the round platform
#if defined(PBL_ROUND)

#include <pebble.h>
#include "./ui.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 60)
#define HOUR_ANGLE(hour) hour * (TRIG_MAX_ANGLE / 12)

// Draw an arc with given inner/outer radii
static void draw_arc(GContext *ctx,
                     GRect rect,
                     uint16_t thickness,
                     uint32_t end_angle) {
  
  //Handle the case where the time has just looped over, display as full circle
  if (end_angle == 0) {
    graphics_fill_radial(ctx,
                         rect,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         TRIG_MAX_ANGLE);
  } else {
    graphics_fill_radial(ctx,
                         rect,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         end_angle);
  }
}

//Calculates the size of the rectangle to be drawn in arc form
static GRect calculate_rect(Layer *layer, uint8_t arc_id) {
  uint8_t padding = 10;
  return grect_inset(layer_get_bounds(layer), GEdgeInsets(padding*(3*arc_id)));
}

// Handle representation for seconds
void draw_seconds(GContext *ctx, uint8_t seconds, Layer *layer) {
  draw_arc(ctx, calculate_rect(layer, 0), 20, TIME_ANGLE(seconds));
}

// Handle representation for minutes
void draw_minutes(GContext *ctx, uint8_t minutes, Layer *layer) {
  draw_arc(ctx, calculate_rect(layer, 1), 20, TIME_ANGLE(minutes));
}

// Handle representation for hours
void draw_hours(GContext *ctx, uint8_t hours, Layer *layer) {
  draw_arc(ctx, calculate_rect(layer, 2), 20, HOUR_ANGLE(hours));
}

#endif
