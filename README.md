# lz16
    LZ16 is a lossless compression algorithm, developed in C, suitable for real-time compression scenarios. 
    It has extremely fast compression and decompression speed.
    MacBook (macOS Ventura 13.7.8 , CPU Core i7 )Under the test, 
    it is faster than lz4 1.8.2 and the compression rate is higher. 
    And use [ryg_rans]（https://github.com/rygorous/ryg_rans)The second entropy compression.At present, 
    it can only run on Little Endian system.
License
======
      LZ16 is provided as open source software using the BSD-3-Clause license.
Benchmark
=======
MacBook (macOS Ventura 13.7.8 , CPU Core i7 @ 3.5GHz，2133MHz LPDDR3 16G,SSD 1T)Under the test.
Evaluation reference [Silesia Corpus].In the test, 
the compression algorithms of LZ4 v1.8.2, zstd and lzfse were compared.

| Silesia Corpus     | https://github.com/MiloszKrajewski/SilesiaCorpus    |
|-------------------:|-----------------------------------------------------|
| Lz4                | https://github.com/lz4/lz4/releases#release-v1.8.2  |
| Zstd               | https://github.com/facebook/zstd                    |
| LzFse              | https://github.com/lzfse/lzfse                      |

|(Level)   |[source length ] |{Ratio encode time,decode time} |{Ratio encode time,decode time} | (level)|
|----------|-----------------|--------------------------------|--------------------------------|--------|      
|lz16( 1)  | [sl: 211938580] | { 2.4769 et: 129.8,dt:  30.4}  |  { 2.1009 et: 159.9,dt:  33.2} |lz4     |
|lz16( 2)  | [sl: 211938580] | { 2.8756 et: 195.6,dt:  73.4}  |  { 3.0568 et: 292.5,dt:  93.3} |zstd( 2)|
|lz16( 3)  | [sl: 211938580] | { 2.5506 et: 233.0,dt:  30.9}  |  { 3.2045 et: 430.7,dt: 103.4} |zstd( 3)|
|lz16( 4)  | [sl: 211938580] | { 2.9490 et: 287.9,dt:  70.1}  |  { 3.2627 et: 442.2,dt: 106.0} |zstd( 4)|
|lz16( 4)  | [sl: 211938580] | { 2.9490 et: 287.9,dt:  70.1}  |  { 3.1346 et: 792.8,dt: 128.7} |lzfse   |

Remarks:

	lz16( 1, 3): Do not use the second entropy compression and decompression.
  
	lz16( 2, 4): use  [ryg_rans] second entropy compression and decompression.

MacBook (macOS Tahoe 26.6.2, CPU Apple M5 Pro )Under the test:
|(Level)   |[source length] |{Ratio encode time,decode time} |{Ratio encode time,decode time} | (level)|
|----------|-----------------|--------------------------------|-------------------------------|--------|      
|lz16( 1)  |[sl: 211938580]  |{ 2.4769 et:  22.8,dt:   7.5}   |{ 2.1009 et:  16.6,dt:   3.2}  | lz4    |
|lz16( 2)  |[sl: 211938580]  |{ 2.8756 et:  36.3,dt:  25.6}   |{ 3.0568 et:  24.7,dt:   8.4}  |zstd( 2)|
|lz16( 3)  |[sl: 211938580]  |{ 2.5506 et:  38.7,dt:   7.7}   |{ 3.2045 et:  31.8,dt:   8.6}  |zstd( 3)|
|lz16( 4)  |[sl: 211938580]  |{ 2.9490 et:  52.8,dt:  24.9}   |{ 3.2627 et:  35.3,dt:   8.6}  |zstd( 4)|
|lz16( 4)  |[sl: 211938580]  |{ 2.9490 et:  53.0,dt:  24.9}   |{ 3.1346 et:  81.6,dt:  10.3}  |lzfse   |

files
======
| lib/c/lz16*.*         | lz16 compression and decompression.                    |
|-----------------------|--------------------------------------------------------|
| lib/c/ans64me.c       |used [ryg rans] entropy compression and decompression.  |
| lib/rans/rans64*.h    |Original [ryg rans] process                             |
| lib/TPdynamic.* 		  |dynamic thread pool                                     |
| tests/common.*     	  |command line common subprocess                          |
| tests/main.c          |command line test process                               |
| data/silesia/*			  |Silesia Corpus                                          |
| data/other/*.*  	    |My development test files                               |

//++++++++ in order save space，only take library of other compression！++++++++

Other/lz4	/lib/*.*

Other/zstd/lib/*.*

Other/lzfse/src/*.*

//++++++++ in order save space，only take library of other compression！++++++++

Building and use
======
cmake

$Mkdir build 

$ cd build 

$ cmake .. 

$ make

$ /…../lz16rans/build/mylz16 -c 1 -S lz4

Development status
======
Improve the X64 asm program in the future. The compression and decompression speed is further increased by about 40%。
