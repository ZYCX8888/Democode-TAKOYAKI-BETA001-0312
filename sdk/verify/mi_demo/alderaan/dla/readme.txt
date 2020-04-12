
build 方式
1.配置编译环境
2.在 sdk/verify/mi_demo/alderaan 目录下 make dla 会生成prog_dla文件


执行方式

1.	将 dla/resource/argb 或者 dla/resource/yuv 下的文件，sdk/verify/mi_demo/common/dla_fw/ipu_firmware.bin.shrink,以及生成的prog_dla 放在同一文件夹下
2．	执行prog_dla 2M_DLA_UI.ini  

3.	人脸数据库处理

3.1 录制人脸
a)	让被录制对象单独出现在屏幕中，并且检测框上方显示ID 号
b)	在控制台输入 小写字母a，控制台提示输入trackid，此trackid 为屏幕上显示的id号
c)	在控制台台输入名字
d)	录制成功时，控制台显示
add xxx to database
save database to bin 
可以对同一个人录制多个姿态的数据

3.2删除某人的数据库
a)	控制台输入 小写字母d
b)	根据提示输入人名
c)	删除成功时显示
del persion flynn.tong
save database to bin



