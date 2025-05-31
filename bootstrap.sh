#!/bin/bash

gcc -Wall -O2 -c src/core/cli_parser.c -o test/build/cli_parser.o -Iinclude
gcc -Wall -O2 -c src/core/var_table.c -o test/build/var_table.o -Iinclude -Ilib/cjson
gcc -Wall -O2 -c src/core/loader.c -o test/build/loader.o -Iinclude -Ilib/cjson
gcc -Wall -O2 -c src/core/builder.c -o test/build/builder.o -Iinclude
gcc -Wall -O2 -c src/sigbuild.c -o test/build/sigbuild.o -Iinclude
gcc -Wall -O2 -c src/main.c -o test/build/main.o -Iinclude
gcc -Wall -O2 -c lib/cjson/cJSON.c -o test/build/cJSON.o -Iinclude
gcc -o sigbuild test/build/cli_parser.o test/build/var_table.o test/build/loader.o test/build/builder.o test/build/sigbuild.o test/build/main.o test/build/cJSON.o
