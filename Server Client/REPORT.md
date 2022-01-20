# How to Run the code
* 1st compile the server code using: `g++ server.cpp -pthread`
* Run the Server use: `./a.out <num of worker threads in the threadpool`
* Then compile the Client code using: `g++ client.cpp -pthread`
* To run the Client use : `./a.out`
* To stop server: `Ctrl+C`.

# Client.cpp
- In main()
  - We take the input regarding how many requests are going to be sent, time and command that corresponds to each thread and then create those many threads such that each thread corresponds to a single request and they run the thread_fun() to which we pass time and command that thread has to perform as input.
  - Then we wait till all threads join and exit the process.
- In thread_func()
  - thread sleeps for the time specified and then make connection with the server and send the command string on socket.
  - Then reads string from socket that is given back by the server.
  - Output message from the server is read with the help of 
      - tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);
  - Finally, We accuire a mutex_lock and print the information of thread and message recieved and unlock the mutex lock accuired.
  - The lock is accquired here so that other threads donot interrupt while printing the output. 

# Server.cpp
- In main() we get the no.of worker threads from `argv[1]` and then create threads for the worker threads.
- Each thread created will execute thr_func() and in the main thread we setup the server to listen to the clients using `bind() and `listen`.
- Then accept the request from clients using `accept()` and push the client socket fd onto queue. And send `work_signal` to the worker threads so that they can execute the command specified in handle_connection() function.

- In thr_fun() the worker threads waits till they recieve the `work_signal` (which is send when there is a client in the queue).And then get the client_Id and then excute the corresponding corresponding command in the `handle_connection()`. 
- Here we accuire mutex_lock so that other thread cannot pop or push while this thread tries to pop the queue and unlock after poping the queue.

- In `handle_connection()` function we get the command that is sent by client on socket and tokenize the input and the tokens are checked with the commands starting from `insert`, `delete`, `update` , `concat`and `fetch`. And corresponding output of the commad is sent back to the user client using send_string_on_socket(), closes the client and return back to the thr_fun() that called it.

- The workers threads run infinitely till the server process exits. 
