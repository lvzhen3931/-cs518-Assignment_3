# -cs518-Assignment_3
Remote files
## usage:
  $make
  on server side:
  $./server
  on client side:
  $./client [server address]
  example:
    $make
    $./server
    $./client rm.cs.rutgers.edu
## Client side:
  We implemented netserverinit, netopen, netread, netwrite and netclose on libnetfiles.c, also there's helper functions called start_session, end_session that basicly deal with connecting to server and disconnecting from server. Below are details of implementation of the remote file operations.
  ### netserverinit
  On the client side, the netserverinit function takes hostname and filemode as input, checks if the hostname is valid. It sets the errno if the hostname cannot be connected to. 
  ### netopen
  The netopen function first call start session to connect a socket session with the host, then it clear the buffer and write "open,[filemode],[filepath],[read write permission]" to the socket. Then it receives response from the server, it will tokenize it and parse the meaning. On success, it returns the file descriptor to the caller. On failure, it set the errno to the number received. The respose format would be "success[0],fd" or "failure[-1],[errno]". The function would call end_session to disconnect the socket.
  ### netread
  The netread function first call start session to connect a socket session with the host, then it clear the buffer and send "read,[filemode],[fd],[nbytes]" to the server. It receives response and tokenizes and parses the message. The response would be"-1,[errno]" on failure and "0,[nbytes_read],[data]" on success. The function then parse the values and copy data out, return the number of bytes actually read and stuff data into local buffer on success, or set the errno and return -1 on falure. Then it calls end_session to end connect.
  ### netwrite
  The netwrite function first call start session to connect a socket session with the host, then it clear the buffer and send "write,[filemode],[fd],[nbytes],[data]" to the server. It receives response "0,[nbytes_written]" on success and "-1, [errno]" on falure. Then it sets the return value to caller accordingly and call end_session.
  ### netclose
  It basically do the same thing as other functions, the message sent to server is "close,[fd]". The message received is "0" on success and "-1,[errno]" on failure. It then parse the message and set return values accordingly.
  ### tokenize
  This function is inside "libnetfiles.h" and takes a string and a delimeter as input and returns array of strings and number of tokens as output. In both server and client side, we ues comma as delimeter to seperate parameters as well as actual data.
  
## Server side
  On the server side, we use pthread to handle multiple request from different process. After the server accept a connection, we create a thread and register the "prcess" function to the thread and continue listen to other request in the main thread.
  ### process
  In the process function, the message received from the client is first tokenized. The first token received is the operation name. So for each operation, we have a if branch for the operation and if the operation is invalid, we set the errno and send error message back to client. In the each operation branch, a helper function is called to deal with these four operations: f_open, f_close, f_read and f_write.
  ### f_open
  In the f_open function, it takes the parsed tokens and do the open() operation accordingly. For example,"open,/path/to/file,filemode,O_RDONLY" is received. The f_open function will try to operate fd = open("/path/to/file",O_RDONLY) and stuff the response back to the caller i.e. "process" function.
  ### f_read,f_write,f_close
  Basically do the similar job. If error is occured, appropriate errno is stuffed into msg after -1(failure).The message mechenism is described above. The only thing need to mension is that the file descriptor in server sides are positive, but negative file descriptor is sent to the client so that clien won't accidentally operates on local files without error noting.
  
## Extension A
  Three file modes are supported by the lib. "UNRES","EXCLU",and "TRANS". We add global state file_metadata for each file opened. Inside a file_metadata, we maintain the filemode and read write permission for each file descriptor, so for every file opened, we have all the state of this file for different remote process. Thus we can keep track of every other thread's mode and permission on the same file and restrict the operation allowed for the file. If operation is not allowed, in f_open function the errno is set to INVALID_OPERATION_MODE. For example, if "test.c" is already opened in "UNRES" mode with "O_RDWR" permission in thread 1, and thread 2 is trying to open it in "EXCLU" mode, then it is not allowed and errno is set to let client know that the operation is invalid. After a thread close the file, that file's metadata get updated and other threads can access it now if a conflict was there. 
