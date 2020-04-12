integrate;
vifinit 1920 1080 1;

vpeinit 0 1920 1080 6;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 0 0 1920 1080 11;

vencinit 1920 1080 0 2;
vencstart 0;

#modid vpe:7, divp:12, venc: 2
setbind 7 0 0 0 30 2 0 0 0 30;

rtspstart stream0 2;
rtspstart stream1 2;
q;