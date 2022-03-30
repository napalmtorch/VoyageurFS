rm -r "bin"
mkdir "bin"

gcc -ggdb -m32 -Iinclude -c "src/main.c" -o "bin/main.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/ata.c" -o "bin/ata.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/fs.c" -o "bin/fs.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/vfs.c" -o "bin/vfs.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/util.c" -o "bin/util.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/cli.c" -o "bin/cli.o" -Wall
gcc -ggdb -m32 -Iinclude -c "src/tests.c" -o "bin/tests.o" -Wall

gcc -ggdb -m32 -o "bin/voy_fs" "bin/main.o" "bin/ata.o" "bin/fs.o" "bin/util.o" "bin/cli.o" "bin/vfs.o" "bin/tests.o" -Wall

./bin/voy_fs testscript