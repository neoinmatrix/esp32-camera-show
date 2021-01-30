# ESP32-camshow
 ESP32 is connected the wifi, and using http-client to receive the video from esp32-cam
 
## dependencies
git clone https://github.com/Bodmer/TFT_eSPI ~/Arduino/libraries/    
git clone https://github.com/Bodmer/JPEGDecoder ~/Arduino/libraries/
 
TFT_eSPI is the driver for tft display. JPEGDecoder is library to transfer the jpg file to matrix for display image.
 
## camera-server
```
 cd camera_server 
 pip install flask 
 pip install numpy 
 pip install python-opencv 
 python app.py 
```
The web stream server (visit http://0.0.0.0:9999/video_feed can get http stream for video) . Also you can use ESP32/CameraWebServer 
to create the same video-stream. 

