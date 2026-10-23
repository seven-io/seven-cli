:: MINGW32
g++ seven-cli.c minini_12b/minIni.c -DCURL_STATICLIB -o build/win/seven-cli.exe -Icurl-7.42.1/include -Lcurl-7.42.1/lib -Iopenssl-1.0.2a/include -Lopenssl-1.0.2a -Izlib-1.2.8 -Lzlib-1.2.8 -static-libgcc -static-libstdc++ -static -lcurl -lssl -lcrypto -lwsock32 -lwinmm -lwldap32 -lws2_32 -lz -lgdi32
