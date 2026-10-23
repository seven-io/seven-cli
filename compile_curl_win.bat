set OPENSSL_PATH=Y:\batchsend77\openssl-1.0.2a
set LIBSSH2_PATH=Y:\batchsend77\libssh2-1.5.0
set ZLIB_PATH=Y:\batchsend77\zlib-1.2.8

cd curl-7.42.1
mingw32-make mingw32-ssl-zlib
mingw32-make mingw32
cd ..
