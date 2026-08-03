#include "pins_config.h"
#include "LovyanGFX_Driver.h"
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

#define SD_MOSI 6
#define SD_MISO 4
#define SD_SCK  5
#define SD_CS   7

/*---------------------------------------------------------------
 * BMP image files on the SD card
 * The lesson expects these files to be stored in the root directory
 * of the card so each image can be loaded by its absolute path.
 *--------------------------------------------------------------*/
#define IMAGE_1 "/1.bmp"
#define IMAGE_2 "/2.bmp"
#define IMAGE_3 "/3.bmp"
#define IMAGE_4 "/4.bmp"
#define IMAGE_5 "/5.bmp"

// Uses the HSPI peripheral for SD card communication.
SPIClass SD_SPI = SPIClass(HSPI);

// Provides LCD drawing, DMA transfer, and display configuration.
LGFX gfx;

/**
 * @brief Show a short status message on the LCD.
 *
 * The lesson uses this helper after SD card initialization so students can
 * confirm the result without relying only on the serial monitor.
 *
 * @param lcd_w Display width reserved for future layout adjustments.
 * @param lcd_h Display height reserved for future layout adjustments.
 * @param x Text start position on the X axis.
 * @param y Text start position on the Y axis.
 * @param text Null-terminated message to draw.
 * @return None.
 */
void show_test(int lcd_w, int lcd_h, int x, int y, const char * text)
{
  gfx.fillScreen(TFT_BLACK);
  gfx.setTextSize(2);
  gfx.setTextColor(TFT_RED);
  gfx.setCursor(x, y);
  gfx.print(text); 
}

/**
 * @brief Initialize the LCD, backlight, and SD card.
 *
 * Arduino calls this function once after reset. The display is started first
 * so SD card success or failure can be reported directly on the screen.
 *
 * @param None.
 * @return None.
 */
void setup()
{
  Serial.begin(115200);

  /*---------------------------------------------------------------
   * Prepare the LCD for status messages and image drawing
   * Backlight control is separate from LCD initialization; both must
   * be correct before the user can see the displayed image.
   *--------------------------------------------------------------*/
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);
  delay(500);
  
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

 if (SD_init() == 0)
  {
    Serial.println("TF_Card initialization succeeded");
    show_test(LCD_H_RES, LCD_V_RES, 75, 100, "SD_Card OK");
    delay(3000);
  } else {
    Serial.println("TF card initialization failed");
    show_test(LCD_H_RES, LCD_V_RES, 75, 100, "SD_Card Failed");
    delay(3000);
  }
  gfx.setRotation(2);
  gfx.fillScreen(TFT_BLACK);
  Serial.println( "----- Setup done -----" );
}

/**
 * @brief Display each prepared BMP image in a repeating slideshow.
 *
 * Arduino calls this function repeatedly after setup() finishes. Each image is
 * shown for five seconds so students can verify that the SD card, file names,
 * BMP format, and LCD drawing path are all working.
 *
 * @param None.
 * @return None.
 */
void loop()
{
  Serial.println("Refreshing image...1");
  displayImage(SD, IMAGE_1, 320, 240);
  delay(5000);

  Serial.println("Refreshing image...2");
  displayImage(SD, IMAGE_2, 320, 240);
  delay(5000);

  Serial.println("Refreshing image...3");
  displayImage(SD, IMAGE_3, 320, 240);
  delay(5000);

  Serial.println("Refreshing image...4");
  displayImage(SD, IMAGE_4, 320, 240);
  delay(5000);

  Serial.println("Refreshing image...5");
  displayImage(SD, IMAGE_5, 320, 240);
  delay(5000);
}

/**
 * @brief Mount the SD card and print its directory contents.
 *
 * The function configures the SPI pins used by the card slot, mounts the file
 * system, reports the card size, and lists files so missing image resources
 * can be diagnosed before displayImage() tries to open them.
 *
 * @param None.
 * @return 0 when the SD card is mounted, 1 when mounting fails.
 */
int SD_init()
{
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, SD_SPI, 80000000))
  {
    Serial.println(F("ERROR: File system mount failed!"));
    SD_SPI.end();
    return 1;
  }
  else
  {
    Serial.println("Card Mount Successed");
    Serial.printf("SD Size： %lluMB \n", SD.cardSize() / (1024 * 1024));
  }
  listDir(SD, "/", 2);
  Serial.println("**** TF Card init finished ****.");
  return 0;
}

/**
 * @brief Print files and subdirectories under a directory.
 *
 * Listing the card contents helps confirm that the expected BMP files are
 * present and named correctly before the image display loop starts.
 *
 * @param fs File system object used for the directory scan.
 * @param dirname Directory path to list.
 * @param levels Number of nested directory levels to print.
 * @return None.
 */
void listDir(fs::FS & fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname); 
    File root = fs.open(dirname);
    if (!root) { 
        Serial.println("Failed to open directory"); 
        return; 
    }
    if (!root.isDirectory()) { 
        Serial.println("Not a directory"); 
        return; 
    }

    File file = root.openNextFile();
    while (file) { 
        if (file.isDirectory()) { 
            Serial.print("  DIR : "); 
            Serial.println(file.name());
            if (levels) { 
                listDir(fs, file.name(), levels - 1);
            }
        } 
        else { 
            Serial.print("  FILE: "); 
            Serial.print(file.name());
            Serial.print("  SIZE: "); 
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

/**
 * @brief Read a 24-bit BMP file and draw it row by row.
 *
 * The BMP header is skipped and each row is read as RGB888 data. Drawing one
 * row at a time keeps the temporary buffer small while still making the full
 * image appear on the LCD.
 *
 * @param fs File system object that contains the BMP file.
 * @param filename BMP file path.
 * @param x Image width in pixels.
 * @param y Image height in pixels.
 * @return 0 after the display attempt finishes.
 */
int displayImage(fs::FS &fs, String filename, int x, int y)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        f.close();
        return 0;
    }

    f.seek(54);
    int X = x;
    int Y = y;
    uint8_t RGB[3 * X];

    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);
        gfx.pushImage(0, row, X, 1, (lgfx::rgb888_t *)RGB);
    }
    f.close();
    return 0; 
}
