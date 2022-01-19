#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sstream>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#define DATABASE "Records.txt"
#define SIZE 256
using namespace std;

int socketfd, clientsock, *newsock;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
char *buf[SIZE];
char transaction_data[SIZE];

int ack;

bool compare(char *no, const char *str);

void update_database();

void withdraw(int newsocket, int amount, int row);

void deposit(int newsocket, int amount, int row);

void balance_enquiry(int newsocket, int row);

void errmsg(int ack);

void clear_vectors();

std::vector<std::string> accountNo;
std::vector<std::string> name;
std::vector<std::int64_t> bal;

/** Handles interrupt signal
*@param signum SIGINT 2
*/
void kill_signal_handler(int signum) {
    update_database();
    cout << "\tDatabase updated!\n" << endl;
    clear_vectors();
    cout << "\tCaught signal. SIGINT " << signum << endl;
    cout << accountNo.size() << " " << name.size() << " " << bal.size() << endl;
    // Terminate program
    exit(signum);
}

/**
*Clears accountNo, name, bal vectors. 
*Called after database is updated
*/
void clear_vectors() {
    accountNo.clear();
    name.clear();
    bal.clear();
}

/**
*Writes all the updated information to the records database
*/
void update_database() {
    std::ofstream ofs(DATABASE, std::ofstream::trunc);
    for (int i = 0; i < accountNo.size(); ++i) {
        ofs << accountNo[i].c_str() << " " << name[i].c_str() << " " << bal[i] << "\n";
    }
    ofs.close();
}

/**
 *Reads a line from the file and splits based on spaces
 * @param line Each record in records database
 * @return
 */
std::vector<std::string> getData(string line) {
    // Vector of string to save tokens
    vector<string> tokens;

    // stringstream class check1
    stringstream check1(line);

    string intermediate;

    // Tokenizing w.r.t. space ' '
    while (getline(check1, intermediate, ' ')) {
        tokens.push_back(intermediate);
    }
    std::vector<std::string> data;
    for (int i = 0; i < tokens.size(); i++) {
        data.push_back(tokens[i]);
    }
    return data;
}

/**
 *Handles operations to be perfromed on the records database for the thread
 *and allows only one thread access to the database to make manipulations with a mutex lock
 * @param sock_desc socket descriptor
 * @return
 */
void *operations_handler(void *sock_desc) {
    memset(buf, 0, SIZE);
    int newsocket = *(int *) sock_desc;
    cout << "Thread: " << pthread_self() << "\n" << endl;
    memset(transaction_data, 0, SIZE);

    long bytes_read = -1;
    while ((bytes_read = (long) read(newsocket, transaction_data, SIZE))) {

        printf("Data received from the client---> %s\n", transaction_data);

        char *token = strtok(transaction_data, " ");

        int timestamp = atoi(token);
        token = strtok(NULL, " ");
        char *account_no = token;
        token = strtok(NULL, " ");
        char *operation = token;
        token = strtok(NULL, " ");
        int amount = atoi(token);

        bool flag = false;
        int size = 0;

        for (int row = 0; row < accountNo.size(); row++) {
            //Check to see if account number exists in the database
            if (compare(account_no, accountNo[row].c_str()))
            {
                //Proceed with type of transaction if it exists
                pthread_mutex_lock(&mutex1); //Acquiring lock on critical section
                cout << "Account number " << accountNo[row] << " is present in the database.\n" << endl;
                //Withdrawal type transaction makes call to withdraw function
                if (strcmp(operation, "w") == 0) {
                    withdraw(newsocket, amount, row);
                }
                //Deposit type transaction makes call to deposit function
                else if (strcmp(operation, "d") == 0) {
                    deposit(newsocket, amount, row);
                } 
                //Balance enquiry type transaction makes call to balance_enquiry function
                else if (strcmp(operation,"b") == 0) {
                    balance_enquiry(newsocket, row);
                }
                else 
                {
                    //In the case that transaction type is not w or d or b
                    cout << "Wrong transaction type..\n" << endl;

                    if (ack = write(newsocket, "Invalid transaction type\n", 100) < 0) {
                        errmsg(ack);
                    }
                }

                flag = true;
                //Release the lock on the critical section
                pthread_mutex_unlock(&mutex1);
                break;
            }
        }

        if (!flag) {
            cout << "\nAccount number does not exist in the database..\n" << endl;

            if (ack = write(newsocket, "Invalid request. Account number is not registered.\n", 100) < 0) //Acknowledgement to client
            {
                errmsg(ack);
            }
            else
            {
            cout << "Acknowledgement Sent.\n" << endl;
            cout << "------------------------" << endl;
            }

        }
    }

    return 0;
}

/**
 * Prints to stderr if writing to socket fails
 * @param ack number of bytes of buffer written to newsocket
 */
void errmsg(int ack) {
    cout << stderr << "Error writing to the socket\n" << endl;
}

/**
 * Handles deposit 'd' type transaction requests from client
 * @param newsocket connection b/w client and server
 * @param amount the amount from transaction_data being deposited
 * @param row index for iteration
 */
void deposit(int newsocket, int amount, int row) {
    cout << "Depositing amount..\n" << endl;
    
    bal[row] += amount;
    cout << "Updated balance after deposit: $" << bal[row] << endl;

    if (write(newsocket, "ACK:Amount deposited\n", 100) < 0)
    {
        cout << stderr << "Error writing to socket\n" << endl;
    }
    else
    {
            cout << "Acknowledgement Sent.\n" << endl;
            cout << "------------------------" << endl;
    } 
}

/**
 * Handles withdrawl 'w' type transaction requests from client
 * withdraws specified amount from balance and updates balance after transaction
 * @param newsocket connection b/w client and server
 * @param amount the amount from transaction_data being withdrawn from balance
 * @param row index for iteration
 */
void withdraw(int newsocket, int amount, int row) {
    if (bal[row] - amount > 0)
    {
        bal[row] -= amount;
        cout << "Withdrawing amount..\n" << endl;
        cout << "Updated balance after withdrawl: $" << bal[row] << endl;
        //cout << "Database updated" << endl;

        if (ack = write(newsocket, "ACK:Amount has been withdrawn\n", 100) < 0){
            errmsg(ack);
        }
        else
        {
            cout << "Acknowledgement Sent.\n" << endl;
            cout << "------------------------" << endl;
        }
    } 
    else 
    {
        //In the case account balance is insufficient to perform the withdrawal operation
        cout << "Insufficient balance. Cannot withdraw.\n" << endl;

        if (ack = write(newsocket, "Insufficient funds\n", 100) < 0)
        {
            errmsg(ack);
        }
        else{
            cout << "Acknowledgement Sent.\n" << endl;
            cout << "------------------------" << endl;
        }
    }
}

/**
 * Handles 'b' type transaction requests from client
 * Prints the balance of accountNo specified
 * @param newsocket connection b/w client and server
 * @param row index for iteration
 */
void balance_enquiry(int newsocket, int row) {
    cout << "Generating balance statement..\n" << endl;
    cout << "Balance: $" << bal[row] << endl;
    if (ack = write(newsocket, "ACK:Statement generated\n", 100) < 0){
            errmsg(ack);
        }
        else
        {
            cout << "Acknowledgement Sent.\n" << endl;
            cout << "------------------------" << endl;
        }
}

/**
 * Compares the accout numbers in the database and account numbers in the transaction data received from client side
 * @param no account_no
 * @param str accountNo[]
 */
bool compare(char *no, const char *str) {
    if (strlen(no) != strlen(str)) {
        return false;
    }
    for (int i = 0; i < strlen(no); ++i) {
        if (no[i] != str[i])
            return false;
    }
    return true;
}

//Main function
int main(int argc, char **argv) {
    signal(SIGINT, kill_signal_handler);

    std::ifstream file(DATABASE);

    std::string str;
 
    while (std::getline(file, str)) {

        if (str.size() == 1)
        {
            continue;
        }
        // Process str
        std::vector<std::string> records_data = getData(str);
        accountNo.push_back(records_data.at(0));
        name.push_back(records_data.at(1));
        bal.push_back(atoi(records_data.at(2).c_str()));

    }

    //Create a socket
    unsigned short portno = atoi(argv[1]); //TCP Port 8888
    struct sockaddr_in server_addr, clientaddr; //Declaration
    socklen_t size;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (socketfd < 0) {
        cout << "Error establishing socket.." << endl;
        exit(1);
    }

    cout << "Socket established.." << endl;

    /* Initialize socket structure */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(portno);

    /* Bind the host address(server_addr) to the socket referred to by the file descriptor socketfd using bind() call.*/
    if (bind(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        cout << "Error binding socket. ." << endl;
        exit(1);
    }

    //Listening for any incoming client connection requests upto a hundred
    cout << "Looking for clients.." << endl;

    int status = listen(socketfd, 100);

    if (status < 0) {
        cout << "Error listening" << endl;
    }

    /*The pthread_mutex_init() function initialises the
        mutex referenced by mutex with attributes specified by attr.
        If attr is NULL, the default mutex attributes are used*/

    pthread_mutex_init(&mutex1, NULL);

    while (true) {
        socklen_t clientlen = sizeof(clientaddr);
        //Accept client connection
        clientsock = accept(socketfd, (struct sockaddr *) &clientaddr, &clientlen);

        if (clientsock < 0) {
            //print stderr
            cout << stderr << "Error accepting client request.." << endl;
        } else {
            cout << "Connection established.." << endl;
        }

        pthread_t thread_id;
        newsock = (int *) malloc(1 * sizeof(int));
        *newsock = clientsock;

        //create new thread for each client
        if (pthread_create(&thread_id, NULL, operations_handler, (void *) newsock) < 0) {
            cout << "Could not create thread..\n" << endl;
            return (1);
        }

        cout << "Connection handler assigned..\n" << endl;
    }

    close(clientsock);
    pthread_exit(NULL);

    close(socketfd);

    return 0;
}