region;
init;
adddata 0;
attachdata 0 1 1 1 0 0;
q;

integrate;
vifinit 1920 1080 0;

vpeinit 0 1920 1080 8;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 0 0 1920 1080 11;

vpeinit 1 1920 1080 16;
setbind 7 0 0 0 30 7 0 1 0 30;
vpecreateport 1 0 1920 1080 11;


vencinit 1920 1080 0 2;
vencstart 0;

#modid vpe:7, divp:12, venc: 2
setbind 7 0 1 0 30 2 0 0 0 30;

rtspstart stream0 2;
rtspstart stream1 2;
q;