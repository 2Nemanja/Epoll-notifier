#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define EXT_ERR_INIT_MUTEX 1
#define EXT_ERR_INIT_SOCKET 2
#define EXT_ERR_INIT_EPOLL 3
#define EXT_ERR_INIT_THREAD 4

#define EXT_ERR_SOCKET_BIND 5
#define EXT_ERR_SOCKET_LISTEN 6
#define EXT_ERR_SOCKET_ACCEPT 7

#define EXT_ERR_REALLOC_TABLE 8

#define MAX_TABLE_SIZE 256
#define MAX_EPOLL_EVENTS 8

#define MAX_WORKERS 8
#define MAX_CLIENTS MAX_TABLE_SIZE/MAX_WORKERS

#define MAX_FILES 8
#define TABLE_CHUNK 8

#define PORT 5521

#define MATCH_SUBFD 0x01
#define MATCH_PUBFD 0x10
#define MATCH_PATH 0x100
#define MATCH_UNLOADED 0x1000
#define MATCH_AVILABLE 0x10000


char *ProgramTitle = 'Server';
pthread_mutex_t PrintLock;

int AnyPubMagicalVal = -2;

struct watchEntry {
    int subfd;
    char *watchPath;

    int pubfd;
    int *activeFiles;
};

struct sharedWorkerData {
    struct watchEntry *subTable;
    int subTableSize;
    pthread_mutex_t subTableLock;

};

struct privateWorkerData {
     int threadNum;
     pthread_t threadId;

     struct sharedWorkerData *sharedData;

     int epollfd;
     int eventQueue;

     int fileCounterTable[MAX_CLIENTS];

     struct epoll_event socketEvents[MAX_CLIENTS];
     struct epoll_event socketEventQueue[MAX_EPOLL_EVENTS];  
     pthread_mutex_t socketEventLock;
};

void watch_entry_factory(struct watchEntry *entry) {
    entry->subfd = 0;
    entry->watchPath = NULL;
    entry->pubfd = 0;
    entry->activeFiles = NULL;
}

void watch_entry_reset(struct watchEntry *entry) {
    if(entry->watchPath != NULL) {
        free(entry->watchPath);
    }
    watch_entry_factory(entry);
}

void private_worker_data_factory(struct privateWorkerData *threadData) {
    threadData->threadNum = -1;
    threadData->epollfd = -1;
    threadData->sharedData = NULL;

    for(int i = 0; i< MAX_CLIENTS; i++) {
        threadData->socketEvents[i].data.fd = 0;
        threadData->fileCounterTable[i] = -1;
    }

}

void initialise_data_table(int start, int end, struct watchEntry *table) {
    for (int i= start; i < end; i++) {
        watch_entry_factory(&table[i]);
    }
}

int find_table_entry(struct watchEntry *matchEntry, int tableSize, struct watchEntry *table, uint32_t matchMask, bool clear) {
    uint32_t matched = 0x000000;
    int matchCounter = -1;

    for( int i = 0; i < tableSize ; i++) {
        if(matchEntry->subfd == table[i].subfd) {
            matched |= MATCH_SUBFD;
        }
        
        if(matchEntry->watchPath == NULL) {
            if(table[i].watchPath == NULL) {
             
                matched |= MATCH_PATH;
            }
        }
        if(matchEntry->pubfd != table[i].pubfd || !matchEntry->pubfd == AnyPubMagicalVal) {
            matched |= MATCH_PUBFD;
        }
        if(matchEntry->activeFiles == table[i].activeFiles) {
            matched |= MATCH_AVILABLE;
        }
        if(matchEntry->activeFiles == NULL) {
            matched |= MATCH_AVILABLE;
        } else {
            if(*table[i].activeFiles < MAX_FILES) { //posto nije null, mozemo ga dereferencirati i ispitati dalje
                matched |= MATCH_UNLOADED;
            }
        }

        if((matched & matchMask) == matchMask) { //koristimo bistsko i jer radimo sa bitovima // proveravamo da li su bitovi u matched isti kao u matchMask
            matchCounter++;

            if(clear) {
                watch_entry_reset(&table[i]);
                continue; //koristimo continue da ne bi doslo do izvrsavanja koda ispod vec samo da se nastavi for petlja
            }

            watch_entry_factory(&matchEntry);
            return i;
        }
    }

    watch_entry_factory(&matchEntry);
    return matchCounter;
}

void *hq_worker_thread(void *voidThreadData) {

    struct privateWorkerData *workerData = (struct privateWorkerData *)voidThreadData;
    uint8_t socketBuffer;  // DRUGI KOD POTREBAN ZA NASTAVAK DALJE
}

int main() {
    int clientId = -1;
    int threadId = -1;
    int socketFd = -1;
    int clientFd = -1;

    struct sockaddr_in serverAddress, clientAddress;
    socklen_t adressSize = sizeof(struct sockaddr_in);

    struct sharedWorkerData sharedData;
    struct privateWorkerData threadWorkerData[MAX_CLIENTS];

    sharedData.subTable = (struct watchEntry *)malloc(sizeof(struct watchEntry) * TABLE_CHUNK);
    sharedData.subTable = TABLE_CHUNK;
    initialise_data_table(0, TABLE_CHUNK, sharedData.subTable);

    if(pthread_mutex_init(&PrintLock, NULL) || pthread_mutex_init(&sharedData.subTableLock, NULL)) {
         fpritnf(stderr, "%s: nemoguce krerirati mutekse...\n", ProgramTitle);
         exit(EXT_ERR_INIT_MUTEX);
    }

    if((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "%s: greska pri kreiranju soketa...\n", ProgramTitle);
        exit(EXT_ERR_INIT_SOCKET);
    }

    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    if(bind(socketFd, (struct socketaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        fprintf(stderr, "%s: nemoguce bindovati na ovaj port...\n", ProgramTitle);
        exit(EXT_ERR_SOCKET_BIND);
    }

    printf("%s: je u procesu stvaranja %d tredova od kojih svaki podrzava %d klijenata\n", ProgramTitle, MAX_WORKERS, MAX_CLIENTS);

    for(int i = 0; i < MAX_WORKERS; i++) {
        private_worker_data_factory(&threadWorkerData[i]); // izvrsavanje private worker data factori nad tabelom thread worker data kako bism izvrsili inicijalizaiju elemenata tabele
        threadWorkerData[i].epollfd = epoll_create1(0); // kreiranje epoll instance kako bismo posmatrali vise file descriptor eventova u isto vreme

        if(threadWorkerData[i].epollfd == -1) {
            fprintf(stderr, "%s: nemoguce inicijalizovati epoll...\n", ProgramTitle);
            exit(EXT_ERR_INIT_EPOLL);
        }

        threadWorkerData[i].sharedData = &sharedData;
        threadWorkerData[i].threadNum = i;

        if(pthread_mutex_init(&threadWorkerData[i].socketEventLock, NULL) != 0) {
            fprintf(stderr, "%s: nemoguce inicijalizovati thred soket event muteks...\n", ProgramTitle);
            exit(EXT_ERR_INIT_MUTEX);
        }

        if(pthread_create(&threadWorkerData[i].threadId, NULL, NULL, &threadWorkerData[i]) != 0) {
            fprintf(stderr, "%s: nemoguce krerati tred...\n", ProgramTitle);
            exit(EXT_ERR_INIT_THREAD);
        }

        pthread_mutex_lock(&PrintLock); // zakljucavamo mutex da bismo kreirali tred
        printf("%s je stvorio tred %d.\n", ProgramTitle, i);
        pthread_mutex_unlock(&PrintLock); // otkljucavamo ga onda kada se tred uspesno kreirao

    }
}
