#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <netdb.h>
#include <fstream>

using namespace std;

#define CLIENTDATA "Transactions.txt"
#define SIZE 256

void validate_input_parameters(int argc);

int create_endpoint(int socketfd, const hostent *server);

using namespace std;

void errormsg(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    clock_t executiontime = clock(); //Clock timer set
    int socketfd, portno;
    struct sockaddr_in server_addr;
    struct hostent *server;

    char buf[SIZE];

    validate_input_parameters(argc);

    /*Reading from Transactions.txt file which contains
    timestamp, account number, transaction type (withdrawal/deposit/balance enquiry), amount (space separated) */
    FILE *transactions_file = fopen(CLIENTDATA, "r+");
    if (transactions_file == NULL) {
        cout << "File error." << endl;
    }

    portno = atoi(argv[2]); //TCP port 8888

    server = gethostbyname(argv[1]);

    socketfd = create_endpoint(socketfd, server);

    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    if (connect(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        cout << stderr << " Could not connect to server." << endl;
        exit(0);
    }

    int size = 0;
    double counter;
    double timestamp;
    double rateoftransaction = 0.1;

    memset(buf, 0, SIZE);

    while (fgets(buf, SIZE, transactions_file) != NULL) {
        counter = counter + 1;
        char *token;
        token = strtok(buf, "\n");
        if (token) {
            strcpy(buf, token);
        }

        size += strlen(buf);
        char transaction_record[SIZE];
        strcpy(transaction_record, buf);

        //timestamp is compared before sending each query to server
        token = strtok(transaction_record, " ");
        if (token) {
            timestamp = atoi(token);
        }

        /* Code logic citation
        Author: zeenatali5710
        Date: Feb 14 2021
        Availablity:https://github.com/zeenatali5710/centralized-multiuser-concurrent-Bank-Manager/blob/main/BankAcClient.cpp
        */

        while(counter!=timestamp)
    	{
    		if(counter>timestamp)
    		{
    			sleep((counter-timestamp)*rateoftransaction);
    			counter=counter+1;
    		}
    		else
    		{
    			sleep((timestamp-counter)*rateoftransaction);
    			counter=counter+1;
    		}
    	}

    	if(counter==timestamp) {
        
        int bytes_written;

        cout << "------Incoming client request------>" << buf << "\n" << endl;

        bytes_written = write(socketfd, buf, sizeof(buf));

        if (bytes_written < 0) {
            cout << stderr << "Could not write to socket\n" << endl;
            exit(1);
        }

        /* End of citation */

        memset(buf, 0, SIZE);

        //Acknowledgement from server
        bytes_written = read(socketfd, buf, SIZE);
        cout << "Server has signalled that message has been received..\n" << buf;
        //Calculate timestamp taken for execution of the transaction
        executiontime = clock() - executiontime;
        //Execution timestamp used for performance evaluation
        cout << "\nTransaction execution timestamp: " << ((float) executiontime / CLOCKS_PER_SEC) << " seconds\n" << endl;
        sleep(0);

        }
    }

    close(socketfd);
    return 0;
}

/**
 * Generates a socket
 * @param socketfd socket descriptor
 */
int create_endpoint(int socketfd, const hostent *server) {
    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0)
        errormsg("Could not create socket.");
    if (server == NULL) {
        cout << stderr << "No such host to connect to.\n" << endl;
        exit(0);
    }

    cout << "-----------NEW CLIENT CONNECTION-----------\n" << endl;

    return socketfd;
}

/**
 * Validates input parameters entered: Hostname, port number
 * @param argc number of arguments being passed
 */
void validate_input_parameters(int argc) {
    if (argc < 3) {
        cout << "Enter hostname, port number to connect to host!" << endl;
        exit(0);
    }
}