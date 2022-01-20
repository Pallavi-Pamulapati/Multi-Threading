#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <bits/stdc++.h>

/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
#include <queue>
#include <pthread.h>
using namespace std;
/////////////////////////////

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

///////////////////////////////
#define MAX_CLIENTS 4
#define PORT_ARG 8001

const int initial_msg_len = 256;

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////

queue <long int> clients; 
pthread_cond_t work_signal = PTHREAD_COND_INITIALIZER;
pthread_mutex_t push_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pp_lock = PTHREAD_MUTEX_INITIALIZER;
int keys[101];
string values[101];

pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

///////////////////////////////

void handle_connection(int client_socket_fd)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;

    while (true)
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        // debug(ret_val);
        // printf("Read something\n");

        vector <string> tokens; 
        stringstream token(cmd);
        string temp;
        string msg_to_send_back="";

        while(getline(token, temp, ' '))
        {
            tokens.push_back(temp);
        }

        if(tokens[0] == "insert")
        {
            int given_key = stoi(tokens[1]);
            if(keys[given_key] != -1)
                msg_to_send_back="Key Already exists\n";
            else 
            {
                keys[given_key] = given_key;
                values[given_key] = tokens[2];
                msg_to_send_back="Insertion Successful\n";
            }
        }
        else if(tokens[0] == "delete")
        {
            int given_key = stoi(tokens[1]);
            if(keys[given_key] == -1)
                msg_to_send_back="No such key exists\n";
            else 
            {
                keys[given_key] = -1;
                msg_to_send_back="Deletion successful\n";
            }    
        }
        else if(tokens[0] == "update")
        {
            int given_key = stoi(tokens[1]);
            if(keys[given_key] == -1)
                msg_to_send_back="Key does not exist\n";
            else 
            {
                values[given_key] = tokens[2];
                // printf(BGRN"%s\n", values[given_key]);
                // cout << values[given_key] << endl;
                msg_to_send_back = values[given_key] + "\n";
            }
        }
        else if(tokens[0] == "concat")
        {
            int key1 = stoi(tokens[1]);
            int key2 = stoi(tokens[2]);

            if(key1 == -1 || key2 == -1)
                msg_to_send_back="Concat failed as at least one of the keys does not exist\n";
            else 
            {
                string new_value1 = values[key1]+values[key2];
                string new_value2 = values[key2] + values[key1];
                values[key1] = new_value1;
                values[key2] = new_value2;
                
                // cout << values[key2] << endl;
                msg_to_send_back = values[key2]+ "\n";
                // printf(BGRN"%s\n",values[key2]);
            }
        }
        else if(tokens[0] == "fetch")
        {
            int given_key = stoi(tokens[1]);
            if(keys[given_key] == -1)
                msg_to_send_back="Key does not exist\n";
            else 
            {
                // printf(BGRN"%s\n", values[given_key]);
                // cout << values[given_key] << endl;
                msg_to_send_back = values[given_key]+"\n";
            }
        }
        else 
        {
            msg_to_send_back="Invalid Command\n"; 
        }

        if (ret_val <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            goto close_client_socket_ceremony;
        }
        cout << "Client sent : " << cmd << endl;
        if (cmd == "exit")
        {
            cout << "Exit pressed by client" << endl;
            goto close_client_socket_ceremony;
        }

        ////////////////////////////////////////
        // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
        // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

        int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
        // debug(sent_to_client);
        if (sent_to_client == -1)
        {
            perror("Error while writing to client. Seems socket has been closed");
            goto close_client_socket_ceremony;
        }
        break;
    }

close_client_socket_ceremony:
    close(client_socket_fd);
    cout<<"Disconnected from client"<<endl;
    // return NULL;
}

void* thr_func(void* arg)
{
    pthread_mutex_lock(&pp_lock);
    while(1)
    {
        if(clients.size() == 0)
        {
            pthread_cond_wait(&work_signal, &pp_lock);
        }
        else 
        {
            pthread_mutex_lock(&push_lock);
            long int clint = clients.front();
            clients.pop();
            pthread_mutex_unlock(&push_lock);
            pthread_mutex_unlock(&pp_lock);
            handle_connection(clint);
        }
    }
}

int main(int argc, char *argv[])
{
    int  j, k, t, n;

    for(int i = 0;i < 101; i++)
    {
        keys[i] = -1;
    }

    int num_workers;
    num_workers = atoi(argv[1]);
    // cout << num_workers << endl;

    pthread_t work_thr[num_workers];

    for(int i = 0;i < num_workers;i++)
        pthread_create(&work_thr[i], NULL, thr_func, NULL);


    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact 
    from a client process running on an arbitrary host
    */
    //get welcoming socket
    //get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); //process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    //CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
of the server process. When the server “hears” the knocking, it creates a new door—
more precisely, a new socket that is dedicated to that particular client. 
        */
        //accept is a blocking call
        printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        pthread_mutex_lock(&push_lock);
        clients.push(client_socket_fd);
        pthread_mutex_unlock(&push_lock);

        pthread_cond_signal(&work_signal);
        // handle_connection(client_socket_fd);
    }

    close(wel_socket_fd);
    return 0;
}