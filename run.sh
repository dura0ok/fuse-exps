gcc main.c -o main -lfuse -D_FILE_OFFSET_BITS=64
mkdir mntpoint
./main mntpoint &
ls mntpoint
