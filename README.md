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



ffmpeg 命令行 从mp4视频文件提取aac音频文件

```

ffmpeg -i input.mp4  -vn -acodec aac input.aac 


备注: -vm disable video  丢掉视频
     -acodec  设置音频编码格式
```


ffmpeg从acc音频文件解码为pcm音频文件

```
ffmpeg -i input.aac -f s16le input.pcm

备注:  -f 表示输出格式
```


ffplay播放 .pcm 音频文件

```
ffplay -ar 44100 -ac 2 -f s16le -i input.pcm 
```