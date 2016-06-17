Self realization if the simple IO libevent NetWork Libray

Makefile 是test.c文件进行测试的代码编译


这个libevent 网络IO库是在libevent-release-1.4.15-stable版本的基础上从中提取出适用于类linux平台使用的网络库,纯粹在于将libevent的框架代码提取出来,此时平台之间的差异就不那么重要

//改动地方

*其中的事件默认在linux下使用的都是单调时间 也就是从1970开始距离现在的秒数

*将其中各种跨平台要使用的宏尽可能的去掉

*原本的网络库要包含一个静态库-levent 这里直接就将原理的代码提取出放在文件中，使用的时候只需要将这些文件放到工程同一个目录下直接包含k_event.h就可以使用了,相当于自己的文件一样

//TO　DO

*增添配置文件
*增添log