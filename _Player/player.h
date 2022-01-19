#define SIGHT 5
#define MAX_BEASTS 6
#define MAX_PLAYERS 4

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
} * player;

struct connection_manager_t
{
    int connected_players[MAX_PLAYERS];
} * connection_manager;

//THREADS SETUP
pthread_t board, actions;

void* draw_board(void* nothing);

void* make_actions(void* nothing);