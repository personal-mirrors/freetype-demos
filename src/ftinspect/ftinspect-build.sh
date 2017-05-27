export TRASH_OFF=YES

make clean
qmake ftinspect-qt4.pro
make

make clean
qmake-qt5 -spec linux-clang ftinspect-qt5.pro
make
