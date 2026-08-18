/*---------------------------------------------------------------
 * Supported touch-controller libraries
 * Select exactly one controller below. This lesson enables GT911,
 * while the other profiles remain available for compatible panels.
 *--------------------------------------------------------------*/
#include <Arduino.h>

// Enable this profile when the panel uses an FT6X36 controller.
// #define TOUCH_FT6X36
// #define TOUCH_FT6X36_SCL 19
// #define TOUCH_FT6X36_SDA 18
// #define TOUCH_FT6X36_INT 39
// #define TOUCH_SWAP_XY
// #define TOUCH_MAP_X1 480
// #define TOUCH_MAP_X2 0
// #define TOUCH_MAP_Y1 0
// #define TOUCH_MAP_Y2 320

// GT911 wiring and coordinate calibration used by this lesson.
#define TOUCH_GT911
#define TOUCH_GT911_SCL 16
#define TOUCH_GT911_SDA 15
#define TOUCH_GT911_INT -1
#define TOUCH_GT911_RST -1
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 320
#define TOUCH_MAP_Y2 0

// Enable this profile when the panel uses an XPT2046 controller.
// #define TOUCH_XPT2046
// #define TOUCH_XPT2046_SCK 12
// #define TOUCH_XPT2046_MISO 13
// #define TOUCH_XPT2046_MOSI 11
// #define TOUCH_XPT2046_CS 38
// #define TOUCH_XPT2046_INT 18
// #define TOUCH_XPT2046_ROTATION 0
// #define TOUCH_MAP_X1 4000//4000
// #define TOUCH_MAP_X2 100 //100
// #define TOUCH_MAP_Y1 100//100
// #define TOUCH_MAP_Y2 4000//4000

// Stores the most recently mapped display coordinate.
int touch_last_x = 0, touch_last_y = 0;

#if defined(TOUCH_FT6X36)
#include <Wire.h>
#include <FT6X36.h>
FT6X36 ts(&Wire, TOUCH_FT6X36_INT);
bool touch_touched_flag = true, touch_released_flag = true;

#elif defined(TOUCH_GT911)
#include <Wire.h>
#include <TAMC_GT911.h>
TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

#elif defined(TOUCH_XPT2046)
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
XPT2046_Touchscreen ts(TOUCH_XPT2046_CS, TOUCH_XPT2046_INT);
// This alternative constructor can be used when no interrupt pin is connected.
// T2046_Touchscreen ts(TOUCH_XPT2046_CS);



#endif

#if defined(TOUCH_FT6X36)
/**
 * @brief Process a touch event reported by the FT6X36 library.
 *
 * The callback maps controller coordinates to the current screen orientation
 * and records whether the gesture represents a press or release.
 *
 * @param p Raw point supplied by the touch controller.
 * @param e Event type supplied by the touch controller.
 * @return None.
 */
void touch(TPoint p, TEvent e)
{
  if (e != TEvent::Tap && e != TEvent::DragStart && e != TEvent::DragMove && e != TEvent::DragEnd)
  {
    return;
  }
  // Swap axes only when the selected display orientation requires it.
#if defined(TOUCH_SWAP_XY)
  touch_last_x = map(p.y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touch_last_y = map(p.x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#else
  touch_last_x = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touch_last_y = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#endif
  switch (e)
  {
    case TEvent::Tap:
      Serial.println("Tap");
      touch_touched_flag = true;
      touch_released_flag = true;
      break;
    case TEvent::DragStart:
      Serial.println("DragStart");
      touch_touched_flag = true;
      break;
    case TEvent::DragMove:
      Serial.println("DragMove");
      touch_touched_flag = true;
      break;
    case TEvent::DragEnd:
      Serial.println("DragEnd");
      touch_released_flag = true;
      break;
    default:
      Serial.println("UNKNOWN");
      break;
  }
}
#endif

/**
 * @brief Start the selected touch controller.
 *
 * setup() calls this function after the controller reset sequence. Only the
 * branch selected by the profile macros is compiled into the lesson.
 *
 * @param _addr I2C address used by the GT911 controller.
 * @return None.
 */
void touch_init(uint8_t _addr)
{
#if defined(TOUCH_FT6X36)
  Wire.begin(TOUCH_FT6X36_SDA, TOUCH_FT6X36_SCL);
  ts.begin();
  ts.registerTouchHandler(touch);

#elif defined(TOUCH_GT911)
  Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
  // Pass the address selected by the hardware reset sequence.
  ts.begin(_addr);
  ts.setRotation(TOUCH_GT911_ROTATION);

#elif defined(TOUCH_XPT2046)
  SPI.begin(TOUCH_XPT2046_SCK, TOUCH_XPT2046_MISO, TOUCH_XPT2046_MOSI, TOUCH_XPT2046_CS);
  ts.begin();
  ts.setRotation(TOUCH_XPT2046_ROTATION);

#endif
}

/**
 * @brief Determine whether the controller has touch activity to process.
 *
 * Input code can call this helper before reading coordinates. The exact
 * signal source depends on the controller selected at compile time.
 *
 * @param None.
 * @return true when touch data may be available; otherwise false.
 */
bool touch_has_signal()
{
#if defined(TOUCH_FT6X36)
  ts.loop();
  return touch_touched_flag || touch_released_flag;

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  return ts.tirqTouched();

#else
  return false;
#endif
}

/**
 * @brief Read and map the latest pressed point.
 *
 * Raw controller coordinates are converted into the active display range and
 * saved in touch_last_x and touch_last_y for the caller.
 *
 * @param None.
 * @return true while a touch is active; otherwise false.
 */
bool touch_touched() {
#if defined(TOUCH_FT6X36)
  if (touch_touched_flag) {
    touch_touched_flag = false;
    return true;
  } else {
    return false;
  }

#elif defined(TOUCH_GT911)
  ts.read();
  if (ts.isTouched) {
#if defined(TOUCH_SWAP_XY)
    touch_last_x = ::map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx.width() - 1);
    touch_last_y = ::map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx.height() - 1);
#else
    touch_last_x = ::map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx.width() - 1);
    touch_last_y = ::map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx.height() - 1);
#endif
    return true;
  } else {
    return false;
  }

#elif defined(TOUCH_XPT2046)
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(p.y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd->width() - 1);
    touch_last_y = map(p.x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd->height() - 1);
#else
    touch_last_x = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd->width() - 1);
    touch_last_y = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd->height() - 1);
#endif
    return true;
  } else {
    return false;
  }

#else
  return false;
#endif
}

/**
 * @brief Report whether the current touch interaction has been released.
 *
 * The FT6X36 profile consumes a stored release event. The other supported
 * profiles use their existing always-ready behavior.
 *
 * @param None.
 * @return true when the selected controller reports a release.
 */
bool touch_released()
{
#if defined(TOUCH_FT6X36)
  if (touch_released_flag) {
    touch_released_flag = false;
    return true;
  } else {
    return false;
  }

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  return true;

#else
  return false;
#endif
}
