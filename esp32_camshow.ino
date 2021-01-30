#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "more.h"


TFT_eSPI tft = TFT_eSPI();

// Return the minimum of two values a and b
#define minimum(a, b)     (((a) < (b)) ? (a) : (b))


// The buffer for content recieve--------------------------------------------------------
uint8_t receive_content[RECEIVE_SIZE] = {};
uint8_t buff[512] = {0};


//####################################################################################################
// Setup
//####################################################################################################
void setup() {
    Serial.begin(115200);
    tft.begin();

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    tft.setRotation(1);  // landscape
    tft.fillScreen(TFT_BLACK);
}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.println("[HTTP] begin...\n");
        // configure server and url
        http.begin(url);
        Serial.println("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                // get tcp stream
                WiFiClient *stream = http.getStreamPtr();

                int receive_len = 0;
                int begin = -1;
                int end = -1;

                // read all data from server
                while (http.connected()) {
                    // get available data size
                    size_t size = stream->available();
                    // Serial.printf("[stream] dsize: %d\n ",size);
                    if (size) {
                        if (receive_len > RECEIVE_SIZE) { //防止出错
                            return;
                        }
                        // read up to 128 byte
                        int buf_len = stream->read(buff, sizeof(buff));
//                        Serial.printf("[buf_len] dsize: %d\n ", buf_len);
                        memcpy(receive_content + receive_len, buff, buf_len * sizeof(uint8_t));
                        receive_len += buf_len;
                        if (begin == -1) {
                            begin = findmark(receive_content, receive_len, 0, 0); // find begin
                        }
                        if (begin > 0) {
                            end = findmark(receive_content, receive_len, begin, 1); // from begin to find end
                        }
//                        Serial.printf(" begin: %d end: %d receive_len: %d \n", begin, end, receive_len);
                        if (begin > 0 && end > 0) {
//                            Serial.printf(" find::::::: begin: %d end: %d \n", begin, end);
                            drawArrayJpeg(receive_content + begin, end - begin, 0, 4);
                            receive_len = receive_len - end;

//                            Serial.printf(" receive_left::::::: %d \n", receive_len);
                            memcpy(receive_content, receive_content + end, receive_len);
                            begin = -1;
                            end = -1;
//                            delay(20);
                        }
                    }
//                    delay(1);
                }
                Serial.print("[HTTP] connection closed or file end.\n");
            }
        } else {
            Serial.print("[HTTP] GET... failed, error: ");
            Serial.println(http.errorToString(httpCode).c_str());
        }

        http.end();
    } else {
        Serial.println("[wifi connected error]!!!!");
    }
    delay(2000);

}

//####################################################################################################
// Draw a JPEG on the TFT pulled from a program memory array
//####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

    int x = xpos;
    int y = ypos;

    JpegDec.decodeArray(arrayname, array_size);

//    jpegInfo(); // Print information from the JPEG file (could comment this line out)

    renderJPEG(x, y);

//    Serial.println("#########################");
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos) {

    // retrieve infomration about the image
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // read each MCU block until there are no more
    while (JpegDec.read()) {

        // save a pointer to the image block
        pImg = JpegDec.pImage;

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w) {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++) {
                p += mcu_w;
                for (int w = 0; w < win_w; w++) {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        tft.startWrite();

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height()) {

            // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

            // Write all MCU pixels to the TFT window
            while (mcu_pixels--) {
                // Push each pixel to the TFT MCU area
                tft.pushColor(*pImg++);
            }

        } else if ((mcu_y + win_h) >= tft.height())
            JpegDec.abort(); // Image has run off bottom of screen so abort decoding

        tft.endWrite();
    }

    // calculate how long it took to draw the image
//    drawTime = millis() - drawTime;

    // print the results to the serial port
//    Serial.print(F("Total render time was    : "));
//    Serial.print(drawTime);
//    Serial.println(F(" ms"));
//    Serial.println(F(""));
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
void jpegInfo() {
    Serial.println(F("==============="));
    Serial.println(F("JPEG image info"));
    Serial.println(F("==============="));
    Serial.print(F("Width      :"));
    Serial.println(JpegDec.width);
    Serial.print(F("Height     :"));
    Serial.println(JpegDec.height);
    Serial.print(F("Components :"));
    Serial.println(JpegDec.comps);
    Serial.print(F("MCU / row  :"));
    Serial.println(JpegDec.MCUSPerRow);
    Serial.print(F("MCU / col  :"));
    Serial.println(JpegDec.MCUSPerCol);
    Serial.print(F("Scan type  :"));
    Serial.println(JpegDec.scanType);
    Serial.print(F("MCU width  :"));
    Serial.println(JpegDec.MCUWidth);
    Serial.print(F("MCU height :"));
    Serial.println(JpegDec.MCUHeight);
    Serial.println(F("==============="));
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16 bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
    //tft.setCursor(0, 0);
    //tft.setTextFont(1);
    //tft.setTextSize(2);
    //tft.setTextColor(TFT_WHITE, TFT_BLACK);
    //tft.print(F(" JPEG drawn in "));
    //tft.print(msTime);
    //tft.println(F(" ms "));
    Serial.print(F(" JPEG drawn in "));
    Serial.print(msTime);
    Serial.println(F(" ms "));
}
