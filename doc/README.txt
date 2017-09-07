>>>Required: 

	ndn-cpp
	FFMPEG
	libconfig
	glog


	1. ndn-cpp
		https://github.com/named-data/ndn-cpp

	2. FFMPEG (min version 3.0.2)
		sudo apt-get -y install libsdl1.2-dev
		sudo apt-get -y install libsdl-image1.2-dev
		sudo apt-get -y install libsdl-mixer1.2-dev
		sudo apt-get -y install libsdl-ttf2.0-dev
		sudo apt-get -y install gfx1.2-dev

		sudo apt-get install yasm
		sudo apt-get install x264
		sudo apt-get install libx264-dev

		sudo apt-get install -y build-essential libfaac-dev libfaad-dev libmp3lame-dev libsdl1.2-dev libtheora-dev libx11-dev libxvidcore4 libxvidcore-dev zlib1g-dev libopencore-amrnb-dev libopencore-amrwb-dev

		./configure --prefix=/usr/local --enable-static --enable-shared --enable-gpl --enable-nonfree --enable-pthreads --enable-libfaac --enable-libmp3lame --enable-libtheora --enable-libx264 --enable-libxvid --enable-x11grab --enable-libopencore-amrnb --enable-libopencore-amrwb  --enable-version3 --disable-optimizations --disable-asm

		make && make install


	3. libconfig
		$PRO_ROOT/third-party/libconfig-1.5.tar.gz

	4. glog
		$PRO_ROOT/third-party/glog-master.zip




>>>Build：

	1. create build path:
		mkdir -p server/bin/
		cd server/bin

	2. generate Makefile
		cmake ../src

	3. make
		make

