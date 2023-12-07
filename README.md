# crtsp_server

RTSP 协议开发



# ffmpeg 命令

推流

```bash

ffmpeg -re -stream_loop -1 -i input.h264 -vcodec copy -rtsp_transport tcp -f rtsp rtsp://127.0.0.1/live/chensong

```


提取h264码流命令

```bash

ffmpeg -i input.mp4 -codec copy -bsf: h264_mp4toannexb  -f h264 test.h264 
```


视频转换分辨率命令

```bash
ffmpeg -i .\input.mp4 -b:v 10000k -s 216x384 output4.mp4
```
