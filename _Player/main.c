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

#include "player.h"

int main()
{
    //WAIT FOR SERVER TO START RUNNIG
    printf("Waiting for server!\n");
    int fd;
    
    int failure;
    for(failure=0; failure<50; failure++)
    {
        fd = shm_open("MANAGER", O_RDWR, 0600);
        if (fd > 0) break;
        usleep(200000);
    }
    if(failure==50)
    {
        puts("Connection timeout\n");
        return 0;
    }

    

    //CONNECTION SETUP
    int err = ftruncate(fd, sizeof(struct connection_manager_t));
    if (err < 0) {perror("ftruncate"); return 1;}

    connection_manager =(struct connection_manager_t *) mmap(NULL, sizeof(struct connection_manager_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (connection_manager == MAP_FAILED) {perror("mmap"); return 1;}
    
    //GET ID
    int player_id=0;
    
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if((connection_manager->connected_players[i])==0)
        {
            (connection_manager->connected_players[i])=1;
            player_id=i+1;
        }
        if(player_id!=0) break; 
    }

    if(player_id==0) 
    {
        printf("\nSERVER FULL\n");
        return 0;
    }


    puts("\nCONNECTING TO INDIVIDUAL SHM\n");
    //CONNECT TO DESIGNED SHM
    char name[20];
    sprintf(name, "player_%d", player_id-1);

    int pfd = shm_open(name, O_CREAT | O_RDWR, 0600);
    if (pfd < 0) {perror("shm_open"); return 1;}

    err = ftruncate(fd, sizeof(struct player_t));
    if (err < 0) {perror("ftruncate"); return 1;}

    player= mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, pfd, 0);
    if (player == MAP_FAILED) {perror("mmap"); return 1;}

    player->active=1;
    player->p_ID=getpid();

    //SCREEN SETUP
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);


    //MAIN LOOP
    pthread_create(&board,NULL,&draw_board,NULL);

    pthread_create(&actions,NULL,&make_actions,NULL);

    pthread_join(actions,NULL);

    //ENDGAME
    endwin();

    return 0;
}