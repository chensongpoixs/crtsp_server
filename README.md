# crtsp_server

RTSP 协议开发



# ffmpeg 命令

推流

```bash

ffmpeg -re -stream_loop -1 -i input.h264 -vcodec copy -rtsp_transport tcp -f rtsp rtsp://127.0.0.1/live/chensong

```

