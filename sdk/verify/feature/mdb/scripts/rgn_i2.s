integrate;
vpevifinit 1920 1080;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 352 288 10 0;
vencinit 352 288 0 2;
#modid vpe:7, divp:12, venc: 2
setbind 7 0 0 0 30 2 0 0 0 30;
vencstart 0;
q;

#i2
region;
init;
injectpic /data/ut/I2/64X48.i2 0 64 48 1 2 0 0 0 0 0;
q;

integrate;
vencwritefile /tmp/data.es 0 3;
q;

region;
injectpic /data/ut/I2/200X200.i2 1 200 200 1 2 0 0 0 28 200;
q;

integrate;
vencwritefile /tmp/data.es 0 3;
exportfile /tmp/data.es /dev/mtd2;
q;