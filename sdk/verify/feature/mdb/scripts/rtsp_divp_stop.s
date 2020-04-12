region;
rmdata;
deinit;
q;

integrate;
rtspstop main_stream0;
rtspstop main_stream1;

#modid vpe:7, divp:12, venc: 2
setunbind  12 0 0 0 2 0 0 0;
setunbind  12 0 1 0 2 0 1 0;
setunbind  7 0 0 0 12 0 0 0;
setunbind  7 0 0 1 12 0 1 0;

vencstop 0;
vencdeinit 0;
divpstop 0;
divpdeinit 0;
vpedestroyport 0;

vencstop 1;
vencdeinit 1;
divpstop 1;
divpdeinit 1;
vpedestroyport 1;

vpevifdeinit;
q;