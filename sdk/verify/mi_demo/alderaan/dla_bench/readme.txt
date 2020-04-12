[Run: net one tensor ouput  100 times]
./prog_dla_bench \
-i /mnt/sit_resource/002_mobile_v2/002_mobile_v2_input.bin \
-m /mnt/sit_resource/002_mobile_v2/002_mobile_v2_net_offline.img \
-g /mnt/sit_resource/002_mobile_v2/002_mobile_v2_output.bin \
-n 100 \
-s  0.000030517578 \
-w /mnt/sit_resource/FreeRTOS_ddr_i_ddr_d.bin 

[Run: net multi tensors output 100 times]
./prog_dla_bench  \
-i /mnt/sit_resource/003_ssd_mobile/003_ssd_mobile_input.bin \
-m /mnt/sit_resource/003_ssd_mobile/003_ssd_mobile_net_offline.img \
-g /mnt/sit_resource/003_ssd_mobile/003_ssd_mobile_output.bin \
-w /mnt/sit_resource/FreeRTOS_ddr_i_ddr_d.bin  \
-n 100 \
-s  "0.000045310891;1.000000000000;0.000030518357;1.000000000000"

-s：parameter get from xxx_output_scalar.txt, eg:003_ssd_mobile_output_scalar.txt
