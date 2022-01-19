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



//PLAN
/*
Utworzyć przy starcie servera sloty na graczy ze strukturami;
stworzyć shared memory dla każdego slota
wypełnić zerami active->0;
zmienna connected_players=0;
*/

void* draw_board(void* nothing)
{
    while(1)
    {
        clear();

        //PLAYER STATS HEADER
        int row = 1;
        int col = 55;
        mvprintw(row++, col++, "Server's PID: %d", player->s_ID);
        mvprintw(row++, col, "Campside X/Y %d/%d ", player->A_pos.x, player->A_pos.y);
        mvprintw(row++, col, "Round number: %d",player->round);
        --col; row++;
        mvprintw(row++, col++, "Parameters");
        mvprintw(row++, col, "PID");
        mvprintw(row++, col, "Type");
        mvprintw(row++, col, "Curr X/Y");
        mvprintw(row++, col, "Deaths");

        //PLAYER STATS LOOP
        col+=12;

        row-=4;
        mvprintw(row++, col+13, "%d",player->p_ID);
        mvprintw(row++, col+13, "HUMAN");
        mvprintw(row++, col+13, "%d/%d", player->pos.x ,player->pos.y);
        mvprintw(row++, col+13, "%d", player->deaths);

    
        
        //Coin HEADER
        col-=13; row++;
        mvprintw(row++, col++, "Coins");
        mvprintw(row++, col, "carried");
        mvprintw(row++, col, "brought");
        col+=12;

        //Coins
        row-=2;
        mvprintw(row++, col+13, "%d",player->carried);
        mvprintw(row++, col+13, "%d",player->brought);
        
        for (int i = 0; i < MAP_HEIGHT; ++i)
        {
            for (int j = 0; j < MAP_WIDTH; ++j)
            {
                mvprintw(i, j, " ");
            }
        }

            int draw_x=player->pos.x-2;
            int draw_y=player->pos.y-2;

            for(int j=0; j<SIGHT; j++)
            {
                for(int k=0; k<SIGHT; k++)
                {
                    mvprintw(draw_y+j,draw_x+k,"%c",player->minimap[j][k]);
                }
            }

        mvprintw(player->pos.y,player->pos.x,"%d",player->ID);
        refresh();
        usleep(200000);
    }
    

}

void* make_actions(void* nothing)
{
    char action;
    while(1)
    {
        action = getch();
        if(action=='q')
        {
            connection_manager->connected_players[player->ID-1]=0;
            player->active=0;
            pthread_cancel(board);
            return NULL;
        }    

        if(action=='w')
        {
            player->dir=UP;
        }

        if(action=='s')
        {
            player->dir=DOWN;
        }

        if(action=='a')
        {
            player->dir=LEFT;
        }

        if(action=='d')
        {
            player->dir=RIGHT; 
        }

        sem_post(&player->sender);
        sem_wait(&player->reciever);
    }       
}