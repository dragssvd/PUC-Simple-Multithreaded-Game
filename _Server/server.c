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
#include <math.h>

#include "server.h"

struct game_t game;

//mnt/c/users/drags/desktop/studia/s3/projekty/projekt3
//26 13

int read_file(){
    FILE *file=fopen("map.txt","r");
    if(file==NULL) return -1;

    for(int i=0; i<MAP_HEIGHT; i++)
        fscanf(file,"%s",board[i]);

    return 0;
}

void* draw_board(void* nothing){

    pthread_t round;

    for(int i=0; i<MAX_TREASURES; i++)
        game.treasures[i].active=0;
    
    for(int i=0; i<MAX_BEASTS; i++)
        game.beasts[i].active=0;
    
    game.round=0;
    game.PID=getpid();
    game.campsite_pos.x=26;
    game.campsite_pos.y=13;
        
    FILE* bushes=fopen("hide_on_bush.txt","r");
    for(int i=0; i<MAP_HEIGHT; i++)
        fscanf(bushes,"%s",bush_tab[i]);

    while(1)
    {


        int active_beasts=0;
        for(int i=0; i<MAX_BEASTS; i++)
        {
            if(game.beasts[i].active==1) active_beasts++;
        }

        int active_players=0;
         for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(game.players_shm[i]->active==1) active_players++;
        }


        clear();
        //PLAYER STATS HEADER
        pthread_mutex_lock(&game.game_mutx);
        int row = 1;
        int col = 55;
        mvprintw(row++, col++, "Server's PID: %d", game.PID);
        mvprintw(row++, col, "Campside X/Y %d/%d ", game.campsite_pos.x,game.campsite_pos.y);
        mvprintw(row++, col, "Round number: %d", game.round);
        --col; row++;
        mvprintw(row++, col++, "Parameters");
        mvprintw(row++, col, "PID");
        mvprintw(row++, col, "Type");
        mvprintw(row++, col, "Curr X/Y");
        mvprintw(row++, col, "Deaths");

        //PLAYER STATS LOOP
        col+=12;
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            row-=5;
            mvprintw(row++, col+(i*13), "Player_%d",i+1);
            if(game.players_shm[i]->active==1)
            {
                mvprintw(row++, col+(i*13), "%d",game.players_shm[i]->p_ID);
                mvprintw(row++, col+(i*13), "HUMAN");
                mvprintw(row++, col+(i*13), "%d/%d", game.players_shm[i]->pos.x,game.players_shm[i]->pos.y);
                mvprintw(row++, col+(i*13), "%d", game.players_shm[i]->deaths);
            }
            else
            {
                mvprintw(row++, col+(i*13), "-");
                mvprintw(row++, col+(i*13), "-");
                mvprintw(row++, col+(i*13), "--/--");
                mvprintw(row++, col+(i*13), "-");               
            }
        }
        
        //Coin HEADER
        col-=13; row++;
        mvprintw(row++, col++, "Coins");
        mvprintw(row++, col, "carried");
        mvprintw(row++, col, "brought");
        col+=12;
        //Coin LOOP
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            row-=2;
            if(game.players_shm[i]->active==1)
            {
                mvprintw(row++, col+(i*13), "%d",game.players_shm[i]->carried);
                mvprintw(row++, col+(i*13), "%d",game.players_shm[i]->brought);
            }
            else
            {
                mvprintw(row++, col+(i*13), "-");
                mvprintw(row++, col+(i*13), "-");           
            }
        }


        pthread_mutex_unlock(&game.game_mutx);

        //memcpy(board,tmp_tab,MAP_WIDTH*MAP_HEIGHT);

        for (int i = 0; i < MAP_HEIGHT; i++)
        {
            for (int j = 0; j < MAP_WIDTH; j++)
            {
                pthread_mutex_lock(&game.game_mutx);

                if(board[i][j]=='.') board[i][j]=' ';
                mvprintw(i,j,"%c",board[i][j]);

                pthread_mutex_unlock(&game.game_mutx);
            }
        }

        mvprintw(13,26,"A");

        for(int i=0; i<MAX_TREASURES; i++)
        {
            pthread_mutex_lock(&game.game_mutx);

            if(game.treasures[i].active==1)
                mvprintw(game.treasures[i].pos.y, game.treasures[i].pos.x,"D");
                
            pthread_mutex_unlock(&game.game_mutx);
        }
        
        for(int i=0; i<MAX_BEASTS; i++)
        {
            pthread_mutex_lock(&game.game_mutx);

            if(game.beasts[i].active==1)
                mvprintw(game.beasts[i].pos.y, game.beasts[i].pos.x, "*");

            pthread_mutex_unlock(&game.game_mutx);
        }


        for(int i=0; i<MAX_PLAYERS; i++)
        {
            pthread_mutex_lock(&game.game_mutx);

            if(game.players_shm[i]->active==1)
                mvprintw(game.players_shm[i]->pos.y, game.players_shm[i]->pos.x, "%d",i+1);

            pthread_mutex_unlock(&game.game_mutx);
        }
    

        refresh();
        
        pthread_create(&round,NULL,&next_round,NULL);

        usleep(200000);
        pthread_join(round,NULL);
    }

}

void* next_round(void* nothing)
{
        //next_round ->
        game.round+=1;

        for(int i=0; i<MAX_BEASTS; i++)
        {
            if(game.beasts[i].active==1)
            {
                sem_post(&game.beasts[i].sender);
                sem_wait(&game.beasts[i].reciever);
            }
        }

        int player_move[MAX_PLAYERS]={0};
        

        for(int i=0; i<MAX_PLAYERS; i++)
        {
            pthread_mutex_lock(&game.game_mutx);
            //IF NEW PLAYER JOINS THE GAME
            if(game.players_shm[i]->active==1 && game.players_shm[i]->on_map==0)
            {
                game.players_shm[i]->on_map=1;
                while(1)
                {
                    int x=rand() % 51 + 1;
                    int y=rand() % 25 + 1;
                    if(board[y][x]==' ')
                    {
                        game.players_shm[i]->pos.x=x;
                        game.players_shm[i]->pos.y=y;
                        game.players_shm[i]->spawn.x=x;
                        game.players_shm[i]->spawn.y=y;
                        break;
                    }
                }
            }

            //PLAYER IS PLAYING - MOVE HIM?
            if(game.players_shm[i]->slowed==1)
            {
                game.players_shm[i]->slowed=0;
            }
            else
            {
                if(game.players_shm[i]->active==1 && game.players_shm[i]->on_map==1)
                {
                    if(sem_trywait(&game.players_shm[i]->sender)==0)
                    {
                        player_move[i]=1;
                        enum move_dir a=game.players_shm[i]->dir;
                    
                        struct pos_t new_pos;
                        new_pos.x=game.players_shm[i]->pos.x;
                        new_pos.y=game.players_shm[i]->pos.y;

                        if(a==UP)
                        {
                            if(board[new_pos.y-1][new_pos.x]!='|')
                            new_pos.y-=1;
                        }

                        if(a==DOWN)
                        {
                            if(board[new_pos.y+1][new_pos.x]!='|')
                            new_pos.y+=1;
                        }

                        if(a==LEFT)
                        {
                            if(board[new_pos.y][new_pos.x-1]!='|')
                            new_pos.x-=1;
                        }

                        if(a==RIGHT)
                        {
                            if(board[new_pos.y][new_pos.x+1]!='|')
                            new_pos.x+=1;
                        }
                        game.players_shm[i]->pos.x=new_pos.x;
                        game.players_shm[i]->pos.y=new_pos.y;
                    }

                }
            }

            //KONIEC
            pthread_mutex_unlock(&game.game_mutx);
        }
  
        //COIN COLLISIONS
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(player_move[i]=1)
            {
                pthread_mutex_lock(&game.game_mutx);

                int new_x=game.players_shm[i]->pos.x;
                int new_y=game.players_shm[i]->pos.y;
                char new_spot=board[new_y][new_x];

                if(new_spot=='c')
                {
                    game.players_shm[i]->carried+=1;
                    board[new_y][new_x]=' ';
                }

                if(new_spot=='C')
                {
                    game.players_shm[i]->carried+=10;
                    board[new_y][new_x]=' ';
                }

                if(new_spot=='T')
                {
                    game.players_shm[i]->carried+=50;
                    board[new_y][new_x]=' ';
                }

                if(new_spot=='A')
                {
                    game.players_shm[i]->brought+=game.players_shm[i]->carried;
                    game.players_shm[i]->carried=0;
                }

                if(new_spot=='#')
                {
                    game.players_shm[i]->slowed=1;
                }

                //SPOTTED THE D
                    for(int j=0; j<MAX_TREASURES; j++)
                    {
                        if(game.treasures[j].active==1)
                        if(game.treasures[j].pos.x==new_x && game.treasures[j].pos.y==new_y)
                        {
                            game.players_shm[i]->carried+=game.treasures[j].value;
                            game.treasures[j].value=0;
                            game.treasures[j].active=0;
                            game.treasures[j].pos.x=0;
                            game.treasures[j].pos.y=0;

                            board[new_y][new_x]=' ';                            
                        }
                    }
                pthread_mutex_unlock(&game.game_mutx);
            }
            
        }

        //BEAST COLLISIONS
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(game.players_shm[i]->active==1)
            for(int j=0; j<MAX_BEASTS; j++)
            {
                if(game.beasts[j].active==1)
                {
                    pthread_mutex_lock(&game.game_mutx);
                    int new_x=game.players_shm[i]->pos.x;
                    int new_y=game.players_shm[i]->pos.y;
                    char new_spot=board[new_y][new_x];

                    if(new_x==game.beasts[j].pos.x && new_y==game.beasts[j].pos.y)
                    {
                        int coins_dropped=game.players_shm[i]->carried;
                        game.players_shm[i]->carried=0;
                        game.players_shm[i]->deaths+=1;
                        game.players_shm[i]->pos.x=game.players_shm[i]->spawn.x;
                        game.players_shm[i]->pos.y=game.players_shm[i]->spawn.y;
                        

                        if(game.players_shm[i]->carried!=0)
                        {
                        board[new_x][new_y]='D';

                        for(int i=0; i<MAX_TREASURES; i++)
                        {
                            if(game.treasures[i].active==0)
                            {
                                game.treasures[i].active=1;
                                game.treasures[i].pos.x=new_x;
                                game.treasures[i].pos.y=new_y;
                                game.treasures[i].value=coins_dropped;
                                break;
                            }
                        }
                        }

                    }

                    pthread_mutex_unlock(&game.game_mutx);
                }

            }
        }

        //PLAYERS COLLISIONS
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(game.players_shm[i]->active==1)
            for(int j=0; j<MAX_PLAYERS; j++)
            {
                if(game.players_shm[j]->active==1 && j!=i)
                {
                    pthread_mutex_lock(&game.game_mutx);
                    int new_x=game.players_shm[i]->pos.x;
                    int new_y=game.players_shm[i]->pos.y;
                    char new_spot=board[new_y][new_x];

                    if(new_x==game.players_shm[j]->pos.x && new_y==game.players_shm[j]->pos.y)
                    {
                        int coins_dropped=game.players_shm[i]->carried+game.players_shm[j]->carried;
                        
                        game.players_shm[i]->carried=0;
                        game.players_shm[j]->carried=0;

                        game.players_shm[i]->deaths+=1;
                        game.players_shm[j]->deaths+=1;

                        game.players_shm[i]->pos.x=game.players_shm[i]->spawn.x;
                        game.players_shm[i]->pos.y=game.players_shm[i]->spawn.y;
                        game.players_shm[j]->pos.x=game.players_shm[j]->spawn.x;
                        game.players_shm[j]->pos.y=game.players_shm[j]->spawn.y;
                        
                        board[new_x][new_y]='D';

                        for(int i=0; i<MAX_TREASURES; i++)
                        {
                            if(game.treasures[i].active==0)
                            {
                                game.treasures[i].active=1;
                                game.treasures[i].pos.x=new_x;
                                game.treasures[i].pos.y=new_y;
                                game.treasures[i].value=coins_dropped;
                                break;
                            }
                        }
                    }
                    pthread_mutex_unlock(&game.game_mutx);
                }

            }
        }

        //NIE MAM POJĘCIA CZEMU ZNIKA NA MAPIE GRACZA??? MOZE BO JEST NA MAPIE W .TXT???
    
        for (int i = 0; i < MAP_HEIGHT; i++)
        {
            for (int j = 0; j < MAP_WIDTH; j++)
            {
                pthread_mutex_lock(&game.game_mutx);
                if(bush_tab[i][j]=='#') board[i][j]='#';
                pthread_mutex_unlock(&game.game_mutx);
            }
        }
        board[13][26]='A';
        
        //WRAP UP THE ROUND - SEND STATS BACK TO THE GAMERS
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            pthread_mutex_lock(&game.game_mutx);

            if(game.players_shm[i]->active==1)
            {
            //SETTING UP PLAYERS MAP
            int draw_x=game.players_shm[i]->pos.x-2;
            int draw_y=game.players_shm[i]->pos.y-2;

            //memcpy(tmp_tab,board,MAP_WIDTH*MAP_HEIGHT);

            for(int j=0; j<MAX_TREASURES; j++)    
                if(game.treasures[j].active==1) board[game.treasures[j].pos.y][game.treasures[j].pos.x]='D';
        
        
            for(int j=0; j<MAX_BEASTS; j++)
                if(game.beasts[j].active==1) board[game.beasts[j].pos.y][game.beasts[j].pos.x]='*';


            for(int j=0; j<MAX_PLAYERS; j++)
            {
                if(i!=j)
                {
                char player[2];
                sprintf(player,"%d",j+1);
                if(game.players_shm[j]->active==1) board[game.players_shm[j]->pos.y][game.players_shm[j]->pos.x]=player[1];
                }
            }
                
        
            for(int j=0; j<SIGHT; j++)
            {
                for(int k=0; k<SIGHT; k++)
                {
                    char spot=board[draw_y+j][draw_x+k];
                    if(spot=='A')
                    {
                        game.players_shm[i]->A_pos.x=game.campsite_pos.x;
                        game.players_shm[i]->A_pos.y=game.campsite_pos.y;
                    }
                    game.players_shm[i]->minimap[j][k]=spot;
                }
            }

            
            for(int j=0; j<MAX_TREASURES; j++)    
                if(game.treasures[j].active==1) board[game.treasures[j].pos.y][game.treasures[j].pos.x]=' ';
        
        
            for(int j=0; j<MAX_BEASTS; j++)
                if(game.beasts[j].active==1) board[game.beasts[j].pos.y][game.beasts[j].pos.x]=' ';


            for(int j=0; j<MAX_PLAYERS; j++)
                if(game.players_shm[j]->active==1) board[game.players_shm[j]->pos.y][game.players_shm[j]->pos.x]=' ';

            

            game.players_shm[i]->round=game.round;
            game.players_shm[i]->dir = IDLE;
            if(player_move[i]==1) sem_post(&game.players_shm[i]->reciever);                   
            }
            pthread_mutex_unlock(&game.game_mutx);
        }
}

void* beast_move(void* id){
    int new_id= *(int*)id;

    while(1)
    {
        sem_wait(&game.beasts[new_id].sender);
        //puts("Sem_wait");
        //SIMPLEST EVER CHASE ALGORITHM BY ME >w<
        
        int b_pos_x=game.beasts[new_id].pos.x;
        int b_pos_y=game.beasts[new_id].pos.y;

        int moved=0;

        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(game.players_shm[i]->active==1)
            {
                int p_pos_x=game.players_shm[i]->pos.x;
                int p_pos_y=game.players_shm[i]->pos.y;

                int dx=b_pos_x-p_pos_x;
                dx=abs(dx);
                int dy=b_pos_y-p_pos_y;
                dy=abs(dy);

                if(dx<=2 && dy<=2) 
                {
                    //chase
                    if(b_pos_x<p_pos_x && board[b_pos_y][b_pos_x+1]!='|'){
                        b_pos_x+=1;
                        moved=1;
                        game.beasts[new_id].pos.x=b_pos_x;
                        game.beasts[new_id].pos.y=b_pos_y;
                        break;
                    }
                    if(b_pos_x>p_pos_x && board[b_pos_y][b_pos_x-1]!='|'){
                        b_pos_x-=1;
                        moved=1;
                        game.beasts[new_id].pos.x=b_pos_x;
                        game.beasts[new_id].pos.y=b_pos_y;
                        break;
                    }
                    if(b_pos_y<p_pos_y && board[b_pos_y+1][b_pos_x]!='|'){
                        b_pos_y+=1;
                        moved=1;
                        game.beasts[new_id].pos.x=b_pos_x;
                        game.beasts[new_id].pos.y=b_pos_y;
                        break;
                    }
                    if(b_pos_y>p_pos_y && board[b_pos_y-1][b_pos_x]!='|'){
                        b_pos_y-=1;
                        moved=1;
                        game.beasts[new_id].pos.x=b_pos_x;
                        game.beasts[new_id].pos.y=b_pos_y;
                        break;
                    }                                                            
                }
            }
            if(moved==1) break;
        }

        if(moved==0)
        {

            //IF NO PLAYER VISIBLE
            int new_dir=rand() % 5 + 1;
        
            game.beasts[new_id].dir=new_dir;
    
            int curr_pos_y=game.beasts[new_id].pos.y;
            int curr_pos_x=game.beasts[new_id].pos.x;

            if(new_dir==UP)
            {
                if(board[curr_pos_y+1][curr_pos_x]!='|')
                    curr_pos_y+=1;
            }

            if(new_dir==DOWN)
            {
                if(board[curr_pos_y-1][curr_pos_x]!='|')
                    curr_pos_y-=1;    
            }

            if(new_dir==LEFT)
            {
                if(board[curr_pos_y][curr_pos_x-1]!='|')
                    curr_pos_x-=1;        
            }

            if(new_dir==RIGHT)
            {
                if(board[curr_pos_y][curr_pos_x+1]!='|')
                   curr_pos_x+=1;        
            }

            game.beasts[new_id].pos.y=curr_pos_y;
            game.beasts[new_id].pos.x=curr_pos_x;
        } 

        sem_post(&game.beasts[new_id].reciever);
    }

}

void* make_actions(void* nothing){
    char action;
    while(1)
    {
        action=getchar();
        if(action=='q')
        {
            sem_post(&terminal_sem);
            return NULL;
        } 

        if(action=='c')
        {
            while(1)
            {
                int x=rand() % 51 + 1;
                int y=rand() % 25 + 1;
                if(board[y][x]==' ')
                {
                    board[y][x]='c';
                    break;
                }
            }
        }

        if(action=='C')
        {
            while(1)
            {
                int x=rand() % 51 + 1;
                int y=rand() % 25 + 1;
                if(board[y][x]==' ')
                {
                    board[y][x]='C';
                    break;
                }
            }            
        }

        if(action=='T')
        {
            while(1)
            {
                int x=rand() % 51 + 1;
                int y=rand() % 25 + 1;
                if(board[y][x]==' ')
                {
                    board[y][x]='T';
                    break;
                }
            }
        }

        if(action=='b')
        {
            
            for(int i=0; i<MAX_BEASTS; i++)
            {
                if(game.beasts[i].active==0)
                {
                    sem_init(&game.beasts[i].sender,0,0);
                    sem_init(&game.beasts[i].reciever,0,0);
                    
                    pthread_mutex_lock(&game.game_mutx);

                    while(1)
                    {
                        int x=rand() % 51 + 1;
                        int y=rand() % 25 + 1;
                        if(board[y][x]==' ')
                        {
                            game.beasts[i].pos.x=x;
                            game.beasts[i].pos.y=y;
                            break;
                        }

                    }

                    game.beasts[i].active=1;
                    pthread_mutex_unlock(&game.game_mutx);
                    pthread_create(&game.beasts[i].beast,NULL,&beast_move,&i);
                    break;
                }
            }
            
        }
    }
}
