integrate;
vifinit 1920 1080 0;
vpeinit 0 1920 1080 0x18;
setbind2 6 0 0 0 30 7 0 0 0 30 0x4 0;
#[2] :PORT
vpecreateport 0 3 352 288 11;
q;

#i2
region;
init;
#PORT 是倒数第四个参数
injectpic ./testFile/I2/64X48.i2 0 64 48 1 2 0 0 3 0 0 0;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
injectpic ./testFile/I2/64X48.i2 1 64 48 1 2 0 0 3 20 80 1;
injectpic ./testFile/I2/64X48.i2 2 64 48 1 2 0 0 3 80 20 2;
injectpic ./testFile/I2/64X48.i2 3 64 48 1 2 0 0 3 20 180 3;
injectpic ./testFile/I2/64X48.i2 4 64 48 1 2 0 0 3 180 20 4;
injectpic ./testFile/I2/64X48.i2 5 64 48 1 2 0 0 3 100 100 5;
injectpic ./testFile/I2/64X48.i2 6 64 48 1 2 0 0 3 280 20 6;
injectpic ./testFile/I2/200X200.i2 7 200 200 1 2 0 0 3 200 150 7;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
#PORT 是第五个参数
setosddisplayattr 1 0 0 0 3 1 32 24 1 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 2 0 0 0 3 1 64 48 2 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 3 0 0 0 3 1 96 72 3 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 4 0 0 0 3 1 128 86 4 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 5 0 0 0 3 1 152 110 5 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 6 0 0 0 3 1 176 134 6 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 7 0 0 0 3 1 200 158 7 0 0 0 0 0 0 0 0xFF;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

#i4
region;
destroy 0;
destroy 1;
destroy 2;
destroy 3;
destroy 4;
destroy 5;
destroy 6;
destroy 7;
#PORT 是倒数第四个参数
injectpic ./testFile/I4/64X48.i4 0 64 48 1 3 0 0 3 0 0 0;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
injectpic ./testFile/I4/64X48.i4 1 64 48 1 3 0 0 3 20 80 1;
injectpic ./testFile/I4/64X48.i4 2 64 48 1 3 0 0 3 80 20 2;
injectpic ./testFile/I4/64X48.i4 3 64 48 1 3 0 0 3 20 180 3;
injectpic ./testFile/I4/64X48.i4 4 64 48 1 3 0 0 3 180 20 4;
injectpic ./testFile/I4/64X48.i4 5 64 48 1 3 0 0 3 100 100 5;
injectpic ./testFile/I4/64X48.i4 6 64 48 1 3 0 0 3 280 20 6;
injectpic ./testFile/I4/200X200.i4 7 200 200 1 3 0 0 3 200 150 7;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
#PORT 是第五个参数
setosddisplayattr 1 0 0 0 3 1 32 24 1 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 2 0 0 0 3 1 64 48 2 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 3 0 0 0 3 1 96 72 3 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 4 0 0 0 3 1 128 86 4 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 5 0 0 0 3 1 152 110 5 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 6 0 0 0 3 1 176 134 6 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 7 0 0 0 3 1 200 158 7 0 0 0 0 0 0 0 0xFF;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

#1555
region;
destroy 0;
destroy 1;
destroy 2;
destroy 3;
destroy 4;
destroy 5;
destroy 6;
destroy 7;
#PORT 是倒数第四个参数
injectpic ./testFile/1555/128X96.argb1555 0 128 96 1 0 0 0 3 0 0 0;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
injectpic ./testFile/1555/128X96.argb1555 1 128 96 1 0 0 0 3 133 0 1;
injectpic ./testFile/1555/128X96.argb1555 2 128 96 1 0 0 0 3 266 0 2;
injectpic ./testFile/1555/128X96.argb1555 3 128 96 1 0 0 0 3 0 100 3;
injectpic ./testFile/1555/128X96.argb1555 4 128 96 1 0 0 0 3 133 100 4;
injectpic ./testFile/1555/128X96.argb1555 5 128 96 1 0 0 0 3 266 100 5;
injectpic ./testFile/1555/128X96.argb1555 6 128 96 1 0 0 0 3 0 200 6;
injectpic ./testFile/1555/128X96.argb1555 7 128 96 1 0 0 0 3 133 200 7;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
#PORT 是第五个参数
setosddisplayattr 1 0 0 0 3 1 64 48 1 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 2 0 0 0 3 1 128 96 2 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 3 0 0 0 3 1 192 144 3 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 4 0 0 0 3 1 10 100 4 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 5 0 0 0 3 1 74 164 5 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 6 0 0 0 3 1 138 228 6 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 7 0 0 0 3 1 202 228 7 0 0 0 0 0 0 0 0xFF;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

#4444
region;
destroy 0;
destroy 1;
destroy 2;
destroy 3;
destroy 4;
destroy 5;
destroy 6;
destroy 7;
#PORT 是倒数第四个参数
injectpic ./testFile/4444/128X96.argb4444 0 128 96 1 1 0 0 3 0 0 0;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
injectpic ./testFile/4444/128X96.argb4444 1 128 96 1 1 0 0 3 133 0 1;
injectpic ./testFile/4444/128X96.argb4444 2 128 96 1 1 0 0 3 266 0 2;
injectpic ./testFile/4444/128X96.argb4444 3 128 96 1 1 0 0 3 0 100 3;
injectpic ./testFile/4444/128X96.argb4444 4 128 96 1 1 0 0 3 133 100 4;
injectpic ./testFile/4444/128X96.argb4444 5 128 96 1 1 0 0 3 266 100 5;
injectpic ./testFile/4444/128X96.argb4444 6 128 96 1 1 0 0 3 0 200 6;
injectpic ./testFile/4444/128X96.argb4444 7 128 96 1 1 0 0 3 133 200 7;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
#PORT 是第五个参数
setosddisplayattr 1 0 0 0 3 1 64 48 1 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 2 0 0 0 3 1 128 96 2 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 3 0 0 0 3 1 192 144 3 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 4 0 0 0 3 1 10 100 4 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 5 0 0 0 3 1 74 164 5 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 6 0 0 0 3 1 138 228 6 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 7 0 0 0 3 1 202 228 7 0 0 0 0 0 0 0 0xFF;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

#8888
#region;
#destroy 0;
#destroy 1;
#destroy 2;
#destroy 3;
#destroy 4;
#destroy 5;
#destroy 6;
#destroy 7;
##PORT 是倒数第四个参数
#injectpic ./testFile/8888/128X96.argb8888 0 128 96 1 6 0 0 3 0 0 0;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;
#region;
#injectpic ./testFile/8888/128X96.argb8888 1 128 96 1 6 0 0 3 133 0 1;
#injectpic ./testFile/8888/128X96.argb8888 2 128 96 1 6 0 0 3 266 0 2;
#injectpic ./testFile/8888/128X96.argb8888 3 128 96 1 6 0 0 3 0 100 3;
#injectpic ./testFile/8888/128X96.argb8888 4 128 96 1 6 0 0 3 133 100 4;
#injectpic ./testFile/8888/128X96.argb8888 5 128 96 1 6 0 0 3 266 100 5;
#injectpic ./testFile/8888/128X96.argb8888 6 128 96 1 6 0 0 3 0 200 6;
#injectpic ./testFile/8888/128X96.argb8888 7 128 96 1 6 0 0 3 133 200 7;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;
#region;
##PORT 是第五个参数
#setosddisplayattr 1 0 0 0 3 1 64 48 1 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 2 0 0 0 3 1 128 96 2 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 3 0 0 0 3 1 192 144 3 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 4 0 0 0 3 1 10 100 4 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 5 0 0 0 3 1 74 164 5 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 6 0 0 0 3 1 138 228 6 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 7 0 0 0 3 1 202 228 7 0 0 0 0 0 0 0 0xFF;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;

#565
#region;
#destroy 0;
#destroy 1;
#destroy 2;
#destroy 3;
#destroy 4;
#destroy 5;
#destroy 6;
#destroy 7;
##PORT 是倒数第四个参数
#injectpic ./testFile/565/128X96.rgb565 0 128 96 1 5 0 0 3 0 0 0;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;
#region;
#injectpic ./testFile/565/128X96.rgb565 1 128 96 1 5 0 0 3 133 0 1;
#injectpic ./testFile/565/128X96.rgb565 2 128 96 1 5 0 0 3 266 0 2;
#injectpic ./testFile/565/128X96.rgb565 3 128 96 1 5 0 0 3 0 100 3;
#injectpic ./testFile/565/128X96.rgb565 4 128 96 1 5 0 0 3 133 100 4;
#injectpic ./testFile/565/128X96.rgb565 5 128 96 1 5 0 0 3 266 100 5;
#injectpic ./testFile/565/128X96.rgb565 6 128 96 1 5 0 0 3 0 200 6;
#injectpic ./testFile/565/128X96.rgb565 7 128 96 1 5 0 0 3 133 200 7;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;
#region;
##PORT 是第五个参数
#setosddisplayattr 1 0 0 0 3 1 64 48 1 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 2 0 0 0 3 1 128 96 2 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 3 0 0 0 3 1 192 144 3 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 2 0 0 0 3 1 10 100 4 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 1 0 0 0 3 1 74 164 5 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 2 0 0 0 3 1 138 228 6 0 0 0 0 0 0 0 0xFF;
#setosddisplayattr 3 0 0 0 3 1 202 228 7 0 0 0 0 0 0 0 0xFF;
#q;
#integrate;
#yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#q;

#i8
region;
destroy 0;
destroy 1;
destroy 2;
destroy 3;
destroy 4;
destroy 5;
destroy 6;
destroy 7;
#PORT 是倒数第四个参数
injectpic ./testFile/I8/64X48.i8 0 64 48 1 4 0 0 3 0 0 0;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
injectpic ./testFile/I8/64X48.i8 1 64 48 1 4 0 0 3 20 80 1;
injectpic ./testFile/I8/64X48.i8 2 64 48 1 4 0 0 3 80 20 2;
injectpic ./testFile/I8/64X48.i8 3 64 48 1 4 0 0 3 20 180 3;
injectpic ./testFile/I8/64X48.i8 4 64 48 1 4 0 0 3 180 20 4;
injectpic ./testFile/I8/64X48.i8 5 64 48 1 4 0 0 3 100 100 5;
injectpic ./testFile/I8/64X48.i8 6 64 48 1 4 0 0 3 280 20 6;
injectpic ./testFile/I8/200X200.i8 7 200 200 1 4 0 0 3 200 150 7;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;
region;
#PORT 是第五个参数
setosddisplayattr 1 0 0 0 3 1 32 24 1 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 2 0 0 0 3 1 64 48 2 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 3 0 0 0 3 1 96 72 3 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 4 0 0 0 3 1 128 86 4 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 5 0 0 0 3 1 152 110 5 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 6 0 0 0 3 1 176 134 6 0 0 0 0 0 0 0 0xFF;
setosddisplayattr 7 0 0 0 3 1 200 158 7 0 0 0 0 0 0 0 0xFF;
q;
integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

#COVER
region;
destroy 0;
destroy 1;
destroy 2;
destroy 3;
destroy 4;
destroy 5;
destroy 6;
destroy 7;
#PORT 是第六个参数
injectcover 0 2000 2000 0 0 3 1000 1000 0 0xFF;
q;

integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
q;

region;
injectcover 1 2500 2500 0 0 3 1500 1500 1 0xFF00;
injectcover 2 3000 1000 0 0 3 2000 2000 2 0xFF0000;
injectcover 3 1000 2000 0 0 3 2500 2500 3 0x0;
q;

integrate;
yuvwritefile ./data_352x288_vpe_p3.yuv 7 0 3 0 3;
#exportfile /tmp/data.es /dev/mtd2;
q;


#integrate;
#vencdeinit 0 1;
#vpedestroyport 0;
#vpevifdeinit;
#q;