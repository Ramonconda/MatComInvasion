// Definiendo estructuras del juego

// Jugador
struct spaceship
{
    int r, c; // rows columns
    char ch;  // caracter que lo representa
};

// Disparo del jugador
struct shoot
{
    int r, c;
    int active; // 1=act 0=inact
    char ch;
};

// Alien enemigo
struct alien
{
    int r, c;
    int pr, pc;     // Previus row , previus column
    int alive;      // 1-vivo
    int showed;     // 0-no ha aparecido    X-orden en que aparece
    int spawn_time; // tiempo para aparecer
    char direction; // l-left r-right d-down  's','S','z'=stop
    char pd;        // previus direction 
    char ch;
} t_alien;


// Bomba del enemigo
struct bomb
{
    int r, c;
    int active; // 1-act
    int loop;   // Usado para prevenir al alien que flashee cuando es arrojada
    char ch;
};

// Parametros modificables del juego
struct options
{
    int overall_speed, alien_speed, shots_speed, bombs_speed, bombchance, alien_amount, shots_amount, bombs_amount, ingame_aliens;
};

// TODAS LAS FUNCIONES DEL JUEGO DIVIDIDAS POR CATEGORIAS

// FUNCIONES QUE ADMINISTRAN MENUS
void gameover(int gamestate);
void game();

// FUNCIONES QUE DIBUJAN EN PANTALLA
void act_screen();
void move_aliens();
void move_shots();
void move_bombs();
void move_player();
void show_stats();

// OTRAS FUNCIONES
void write_input();
void init_config();
void starting_game();
