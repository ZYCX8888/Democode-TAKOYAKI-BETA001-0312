integrate;
vpevifinit 1920 1080;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 352 288 10 0;
vencinit 352 288 0 2;
#modid vpe:7, divp:12, venc: 2
setbind 7 0 0 0 30 2 0 0 0 30;
vencstart 0;
q;

#COVER
region;
init;
injectcover 0 2000 2000 0 0 0 1000 1000 0 0xFF;
q:

integrate;
vencwritefile /tmp/data.es 0 3;
q;

region;
injectcover 1 2500 2500 0 0 0 1500 1500 1 0xFF00;
injectcover 2 3000 1000 0 0 0 2000 2000 2 0xFF0000;
injectcover 3 1000 2000 0 0 0 2500 2500 3 0x0;
q;

integrate;
vencwritefile /tmp/data.es 0 3;
exportfile /tmp/data.es /dev/mtd2;
q;
