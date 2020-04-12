demo说明：
1. 头文件
	3partylib中的include
	smarttalk\demoApp\modules\include
	以及对应source下的头文件
2. so库文件
	3partylib中的lib->第三方库，如omron、Cspotter等
	demoApp中的lib->基本都是编译生成的
3. 运行需要的文件
	DepFile目录：appres是程序运行的图片文件；CSpotter是训练过的dat等
4. 编译
	在demoApp下执行:
		./build_sample.sh
5. 运行
	需要拷贝minigui的cfg到/etc（MiniGUI.cfg中路径需要设定正确）
	对应cfg中的内容需要修改正确的路径：比如字体文件，DepFile\miniguiCFG\minigui\minigui\res\font
	
minigui sdk的编译：
	下载：svn://172.19.24.228/minigui_dev_kit
	其他操作参考下载后的文档“MiniGui 开发包指引.doc”
	SDK中已经包含了编译好的lib，所以可以跳过SDK的编译。

ALKAID的下载：
	git clone ssh://beal.wu@hcgit04:29518/mstar/alkaid/sdk
	git clone ssh://beal.wu@hcgit04:29518/mstar/alkaid/project.git
	branch：H2_Develop
	编译config：./setup_config.sh configs/misc/i2/spinand.glibc.A610X-006a.1g
DEMO position：
	/sdk/verify/feature/module_test/smarttalk
	
注意事项：
1. 运行minigui的程序时，需要创建/var/tmp；mkdir /var/tmp
2. 可能flash空间不是太大，所以需要挂载运行，因为 lib 和 资源文件有点多

运行说明：
1、拷贝DepFile/appres到程序运行目录
2、拷贝DepFile/CSpotter到程序运行目录
3、拷贝DepFile/miniguiCFG/etc到/etc目录，需要关注MiniGUI.CFG中的路径，需要对应自己目录修改
4、拷贝DepFile/miniguiCFG/minigui到运行目录，需要对应MiniGUI.CFG中的路径
5、当前配置文件是将nfs挂载到/customer目录
6、hscrollview.rc拷贝到程序运行目录，如果要改此名字，那么需要修改程序中对应的名称
7、如果想换主界面，拷贝替换ST_CreateMainWindow_New(); 为 ST_CreateMainWindow()，目前ST_CreateMainWindow_New(); 还需要优化效率。

2019-02-28 增加xml编译方法：
	下载：
		http://ftp.gnome.org/pub/GNOME/sources/libxml2/2.6/libxml2-2.6.0.tar.bz2
	编译：
		./configure --host=arm-linux-gnueabihf --target=arm --prefix=/home/beal.wu/4100/sdk/verify/feature/module_test/smarttalk/3partyLib/xml/ --with-python=/home/beal.wu/4100/sdk/verify/feature/module_test/smarttalk/3partyLib/libxml2-2.6.0/python
		make -j7
		make install
		然后strip一下即可
2019-03-15 增加xml的配置文件：
	拷贝DepFile中smartLayout.xml、smartBD.xml到板子的/config目录，部分界面已经使用了xml来配置UI的控件坐标
	