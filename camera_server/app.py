#!/usr/bin/env python
from flask import Flask, Response
import cv2
import numpy as np

app = Flask(__name__)


@app.route('/test')
def test():
    """Video streaming home page."""
    return "test>>>>>"


@app.route('/')
def index():
    """Video streaming home page."""
    return "hello world"


def gen(camera):
    """Video streaming generator function."""
    while True:
        ret, frame = camera.read()
        # print("xx:",camera.set(0,0))
        if ret == False:
            camera.set(0, 0)
            ret, frame = camera.read()
            # camera.
        # print("frame", frame.shape, type(frame))
        h, w = frame.shape[:2]
        frame = cv2.resize(frame, (int(w * 0.3), int(h * 0.3)))
        frame = np.swapaxes(frame, 0, 1)
        ret, buf = cv2.imencode(".jpg", frame)
        # print("buf", type(buf))
        # d x = img.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + bytes(buf) + b'\r\n')


@app.route('/video_feed')
def video_feed():
    """Video streaming route. Put this in the src attribute of an img tag."""
    Camera = cv2.VideoCapture("./candle.avi")
    return Response(gen(Camera),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9999, threaded=True)
