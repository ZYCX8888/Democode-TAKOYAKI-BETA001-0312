integrate;
vpevifinit 1920 1080;
setbind 6 0 0 0 30 7 0 0 0 30;
vpecreateport 352 288 10 0;
vencinit 352 288 0 2;
#modid vpe:7, divp:12, venc: 2
setbind 7 0 0 0 30 2 0 0 0 30;
vencstart 0;
q;

#565
region;
init;
injectpic /data/ut/565/128X96.rgb565 0 128 96 1 5 0 0 0 0 0;
q;

integrate;
vencwritefile /tmp/data.es 0 3;
q;

region;
injectpic /data/ut/565/128X96.rgb565 1 128 96 1 5 0 0 0 28 96;
q;

integrate;
vencwritefile /tmp/data.es 0 3;
exportfile /tmp/data.es /dev/mtd2;
q;
