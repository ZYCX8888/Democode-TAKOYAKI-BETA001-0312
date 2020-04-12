vif;
#  [1] 0:BT656, 1:Digital Cam, 4:MIPI
#  [2] 3: RealTime, 4: Came Frame
createdev 0 4 3;
# portid rect(x,y,width,height)
# pixel 12bppGR:23  207sensor(I5), 10bppRG: 18  317sensor(I2)
# FrameRate 0:Full, 1:Half
createport 0 0 0 0 3840 2160 18 0;
startport 0 0;
q;
vpe;
createchannel 0 3840 2160 4;
setcrop 0 0 0 0 0;
createport 0 0 0 1920 1080;
startchannel 0;
q;
sys;
setbind 6 0 0 0 30 7 0 0 0 30;
#mod dev chn port userdepth totaldepth
setdepth 7 0 0 0 2 5;
q;
vpe;
#chnl port bufNum FilePath
writevpefile 0 0 10 /mnt/vpetest.yuv;
q;
sys;
setunbind 6 0 0 0 7 0 0 0;
q;
vpe;
stopport 0 0;
stopchannel 0;
destroychannel 0;
q;
vif;
stopport 0 0;
disabledev 0;
q
