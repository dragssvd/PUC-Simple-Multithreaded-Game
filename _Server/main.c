#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>

#include "server.h"


int main()
{   
    system("rm -rf /dev/shm/MANAGER");
    system("rm -rf /dev/shm/player_*"); 

    //CONNECTION MANAGER:

    int fd = shm_open("MANAGER", O_CREAT | O_RDWR, 0777);
    if (fd < 0) {perror("shm_open"); return 1;}

    int err = ftruncate(fd, sizeof(struct connection_manager_t));
    if (err < 0) {perror("ftruncate"); return 1;}

    struct connection_manager_t * connection_manager = (struct connection_manager_t *)mmap(NULL, sizeof(struct connection_manager_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (connection_manager == MAP_FAILED) {perror("mmap"); return 1;}
    

    //MANAGER SETUP
    memset(connection_manager->connected_players,0,MAX_PLAYERS);

    //BOARD SETUP
    read_file();

    //PLAYERS SHMS
    int pfd[4];
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        char name[20];
        sprintf(name, "player_%d", i);

        pfd[i] = shm_open(name, O_CREAT | O_RDWR, 0777);
        if (pfd[i] < 0) {perror("shm_open"); return 1;}

        int err = ftruncate(pfd[i], sizeof(struct player_t));
        if (err < 0) {perror("ftruncate"); return 1;}

        game.players_shm[i]= mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, pfd[i], 0);
        if (game.players_shm[i] == MAP_FAILED) {perror("mmap"); return 1;}
        
        
        //PLAYER SHM SETUP
        game.players_shm[i]->active=0;
        game.players_shm[i]->s_ID=getpid();
        game.players_shm[i]->ID=i+1;

        game.players_shm[i]->round=0;

        game.players_shm[i]->on_map=0;
        game.players_shm[i]->pos.x=-1;
        game.players_shm[i]->pos.y=-1;

        game.players_shm[i]->deaths=0;

        game.players_shm[i]->slowed=0;

        game.players_shm[i]->carried=0;
        game.players_shm[i]->brought=0;

        game.players_shm[i]->dir=IDLE;

        sem_init(&game.players_shm[i]->sender, 1, 0);
        sem_init(&game.players_shm[i]->reciever, 1, 1);
    }

    //GAME SETUP
    //SCREEN SETUP
    int max_rows, max_cols;
    initscr();
    noecho();
    curs_set(FALSE);
    raw();
    keypad(stdscr, TRUE);

    getmaxyx(stdscr, max_rows, max_cols);
    //THREADS SETUP
    pthread_t board, actions, connection;
    pthread_mutex_init(&game.game_mutx,NULL);

    //GAME LOOPS
    pthread_create(&board,NULL,&draw_board,NULL);
    pthread_create(&actions,NULL,&make_actions,NULL);

    sem_init(&terminal_sem,0,0);
    sem_wait(&terminal_sem);


    //ENDGAME
    for (int i = 0; i < MAX_BEASTS; i++)
    {
        if (game.beasts[i].active = 1)
        {
            sem_destroy(&game.beasts[i].sender);
            sem_destroy(&game.beasts[i].reciever);
            pthread_cancel(game.beasts[i].beast);
        }
    }


    for(int i=0; i<MAX_PLAYERS; i++)
    {
        sem_destroy(&game.players_shm[i]->sender);
        sem_destroy(&game.players_shm[i]->reciever);
        munmap(game.players_shm[i], sizeof(struct player_t));
        char name[20];
        sprintf(name, "player_%d", i);
        shm_unlink(name);
    }

    munmap(connection_manager, sizeof(struct connection_manager_t));
    shm_unlink("MANAGER");

    sem_destroy(&terminal_sem);
    printf("\nGAME TERMINATED\n\n");

    endwin();

    return 0;
};