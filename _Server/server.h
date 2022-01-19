#define SIGHT 5
#define MAX_BEASTS 6
#define MAX_PLAYERS 4
#define MAX_TREASURES 20
#define MAP_HEIGHT 27
#define MAP_WIDTH 53
enum move_dir{
    UP=1,
    DOWN,
    LEFT,
    RIGHT,
    IDLE
};

struct pos_t{
    int x;
    int y;
};

struct wild_beast_t{
    
    int active;
    pthread_t beast;
    struct pos_t pos;

    enum move_dir dir;

    sem_t sender;
    sem_t reciever;
};

struct treasure_t
{
    int active;

    struct pos_t pos;
    int value;
};

struct player_t
{
    int active;
    int s_ID;
    int ID;

    int round;

    int on_map;
    struct pos_t pos;  
    
    int deaths;

    int slowed;

    int carried;
    int brought;

    struct pos_t A_pos;
    struct pos_t spawn;
    char minimap[SIGHT][SIGHT];

    int p_ID;
    enum move_dir dir;

    sem_t sender;
    sem_t reciever;
};

struct game_t
{
    int PID;
    int round;

    struct pos_t campsite_pos;

    struct wild_beast_t beasts[MAX_BEASTS];
    
    struct player_t* players_shm[MAX_PLAYERS];
    //COINS AND STUFF...

    //Server supports up to 20 individual dropped treasures despite c\C\T treasures
    struct treasure_t treasures[MAX_TREASURES];

    pthread_mutex_t game_mutx;
} game;

char board[MAP_HEIGHT][MAP_WIDTH];
char bush_tab[MAP_HEIGHT][MAP_WIDTH];


sem_t terminal_sem;

struct connection_manager_t
{
    int connected_players[MAX_PLAYERS];
};

int read_file(void);

void* draw_board(void* nothing);

void* make_actions(void* nothing);

void* next_round(void* nothing);
