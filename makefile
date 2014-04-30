# slave_openwrt: main.cpp slave.cpp timer.cpp tool.cpp
#	../../mips-openwrt-linux-g++ -std=c++0x -pthread -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib main.cpp slave.cpp timer.cpp tool.cpp -o slave_openwrt -lasound

slave_openwrt: main.o slave.o tool.o timer.o
	../../mips-openwrt-linux-g++ -std=c++0x -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib main.o slave.o timer.o tool.o -o slave_openwrt -lasound

main.o: main.cpp
	../../mips-openwrt-linux-g++ -std=c++0x -pthread -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib -c main.cpp

slave.o: slave.cpp slave.h tool.o timer.o
	../../mips-openwrt-linux-g++ -std=c++0x -pthread -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib -c slave.cpp -lasound

tool.o: tool.cpp tool.h
	../../mips-openwrt-linux-g++ -std=c++0x -c tool.cpp

timer.o: timer.cpp timer.h
	../../mips-openwrt-linux-g++ -std=c++0x -c timer.cpp

clear:
	rm slave_openwrt main.o slave.o tool.o timer.o
