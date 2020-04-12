region;
init;
adddata 0;
attachdata 0 1 1 1 0 0;
q;

integrate;
vifinit 1920 1080;
vpeinit 0 1920 1080 24;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 0 0 1280 720 11;
vpecreateport 0 1 1280 720 11;

divpinit 0 0 0 0 0 0 0 0; 
divpcreateport 0 11 1280 720;
divpstart 0;

divpinit 1 0 0 0 0 0 0 0; 
divpcreateport 1 11 1280 720;
divpstart 1;

vencinit 1280 720 0 2;
vencinit 1280 720 1 2;

vencstart 0;
vencstart 1;

#modid vpe:7, divp:12, venc: 2
setbind 7 0 0 0 30 12 0 0 0 30;
setbind 12 0 0 0 30 2 0 0 0 30;

setbind 7 0 0 1 30 12 0 1 0 30;
setbind 12 0 1 0 30 2 0 1 0 30;

rtspstart stream0 2;
rtspstart stream1 2;
q;
