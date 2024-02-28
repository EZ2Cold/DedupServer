## DedupServer
### Introduction
***
This project implements a prototype of a cloud storage server that performs client-side chunk-level deduplication. When multiple clients upload a same file to the server, only one copy of the file will be stored, thus saving the storage.

### Requirement
* MYSQL
* BOOST
* OPENSSL

### How to build?
1. create a table in the MYSQL to store names of files uploaded by clients
    ```
    create database dedup_server;
    create table FILES(
        ID int,
        FileName VARCHAR(256)
    );
    ```
2. modify the `address`, `user` and `passwd` to that of your MYSQL in the file `DedupServer.cc`
3. create a build directory
    ```
    mkdir build && cd build
    cmake ..
    make
    ```

### How to run?
The generated binaries are in the directory `bin`. Directory `contaienrs` and `recipes` stores the data and recipe of the uploaded file respectively. Directory `downloads` stores the files downloaded from the server. Log information are stored in the directory `logs`.

1. run the server
    ```
    cd bin
    ./server
    ```
2. output the usage of the client
    ```
    ./client -h
        Usage: client [--help] [--version] --identity VAR --operation VAR --file VAR

        Optional arguments:
        -h, --help            shows help message and exits 
        -v, --version         prints version information and exits 
        -i, --identity        client id [required]
        -o, --operation       specifies operations to be done
                                u:upload the file
                                d:download the file [required]
        -f, --file            (path of file to be uploaded) or (path of file to be saved) [required]
    ```
### Demo
1. generate a 1GB file
    ```
    mkdir dataset && cd dataset
    dd if=/dev/urandom of=test-1G bs=1M count=1000
    cp test-1G test-1G-a
    ```
2. start the server
    ```
    cd ../bin && ./server
    ```
3. upload the file `test-1G`
    ```
    ./client -i 1 -o u -f ../dataset/test-1G
    ```
    log output:
    ```
    2024-02-28 19-26-25 [info]:server starts to run
    2024-02-28 19-26-34 [info]:[1] thread 140506544563968 is serving the client with address 0.0.0.0:0
    2024-02-28 19-26-34 [info]:[1] receive upload request
    2024-02-28 19-26-34 [info]:fetch a mysql connection from the pool
    2024-02-28 19-26-34 [info]:put a mysql connection back into the pool
    2024-02-28 19-26-34 [info]:[1] file does not exist in the database
    2024-02-28 19-26-34 [info]:fetch a mysql connection from the pool
    2024-02-28 19-26-34 [info]:[1] add the file test-1G into the database
    2024-02-28 19-26-34 [info]:put a mysql connection back into the pool
    2024-02-28 19-26-48 [info]:[1] upload finished! receive 1048576000 bytes, 256000 unique chunks, 0 duplicated chunks
    2024-02-28 19-26-48 [info]:[1] receive quit message
    ```

 4. upload the file `test-1G-a` (to simulate another client upload a same file)
    ```
    ./client -i 2 -o u -f ../dataset/test-1G-a
    ```
    log output:
    ```
    2024-02-28 19-28-59 [info]:[2] thread 140506502600448 is serving the client with address 127.0.0.1:670
    2024-02-28 19-28-59 [info]:[2] receive upload request
    2024-02-28 19-28-59 [info]:fetch a mysql connection from the pool
    2024-02-28 19-28-59 [info]:put a mysql connection back into the pool
    2024-02-28 19-28-59 [info]:[2] file does not exist in the database
    2024-02-28 19-28-59 [info]:fetch a mysql connection from the pool
    2024-02-28 19-28-59 [info]:[2] add the file test-1G-a into the database
    2024-02-28 19-28-59 [info]:put a mysql connection back into the pool
    2024-02-28 19-29-23 [info]:[2] upload finished! receive 0 bytes, 0 unique chunks, 256000 duplicated chunks
    2024-02-28 19-29-23 [info]:[2] receive quit message
    ```
    From the log we can see that, no data of the file is transmitted and stored, since it has been uploaded before by client `1`.

5. download the file `test-1G`
    ```
    ./client -i 1 -o d -f downloads/test-1G
    cmp downloads/test-1G ../dataset/test-1G
    ```
    log output:
    ```
    2024-02-28 19-32-25 [info]:[1] thread 140506502600448 is serving the client with address 127.0.0.1:49893
    2024-02-28 19-32-25 [info]:[1] receive download request
    2024-02-28 19-32-25 [info]:fetch a mysql connection from the pool
    2024-02-28 19-32-25 [info]:put a mysql connection back into the pool
    2024-02-28 19-32-25 [info]:[1] file exists
    2024-02-28 19-32-26 [info]:[1] download finished. sent 1048576000 bytes, 256000 chunks
    2024-02-28 19-32-26 [info]:[1] receive quit message
    ```
 