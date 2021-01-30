//
// Created by neomi on 2021/1/28.
//

#define RECEIVE_SIZE  10 * 1024

int findmark(uint8_t *info, const int size, int begin, int type) {
//    fprintf(stdout, " info-len: %d %d  %d \n", strlen(info), begin, type);
    for (int i = begin; i < size - 1; i++) {
//        fprintf(stdout, " %#x", (__u_char) info[i]);
        if (type == 0) {
            if ((uint8_t) info[i] == 0xff && (uint8_t) info[i + 1] == 0xd8) {
                return i;
            }
        } else {
            if ((uint8_t) info[i] == 0xff && (uint8_t) info[i + 1] == 0xd9) {
//                flag_begin = i+2;
                return i + 2;
            }
        }
    }
    return -1;
}
const char *ssid = "neophone";
const char *password = "";
const char *url = "http://0.0.0.0:9999/video_feed";