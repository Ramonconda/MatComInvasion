#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "Structures.h"

/////////////////////////////DECLARANDO VARIABLES Y ESTRUCTURAS GLOBALES
struct spaceship spaceship;
struct alien *aliens;
struct alien *ingame_aliens;
int *alien_out_time;
struct shoot shot[1000];
struct bomb bomb[1000];
struct options settings;
unsigned int input, loops, currentshots, currentbombs, currentaliens;
int score, gamestate, difficult = 1, current_alien, current_showed_alien;
char tellscore[30];

// Declarando exclusiones mutuas
pthread_mutex_t gamestatemutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spaceship_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alienmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shootsmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bombsmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t soremutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pointermutex = PTHREAD_MUTEX_INITIALIZER; // Este es el mutex del puntero de escritura de curses (biblioteca de dibujar en consola)

// Esta funcion muestra la pantalla de victoria/derrota
void gameover(int gamestate)
{
    pthread_mutex_lock(&pointermutex);
    // Configurando biblioteca
    nodelay(stdscr, 0);
    // Mostrando pantalla en dependencia de la condicion de victoria/derrota
    clear();
    if (gamestate == 0)
    {
        move((LINES / 2) - 1, 0);
        addstr("Game Over");
    }
    else if (gamestate == 1)
    {
        move((LINES / 2) - 1, (0));
        addstr("VIctoria");
    }
    else if (gamestate == 2)
    {
        move((LINES / 2) - 1, 0);
        addstr("Game Over");
    }
    else if (gamestate == 3)
    {
        move((LINES / 2) - 1, 0);
        addstr("Game Over");
    }
    move(0, COLS - 1);
    refresh();
    sleep(2); // Esperar dos segundos antes de cerrar
    getch();
    pthread_mutex_unlock(&pointermutex);
}

///////////////////IMPLEMENTANDO FUNCIONES DE PANTALLA

// Funcion que muestra la pantalla de estadisticas del juego
void show_stats()
{
    // Mostrar titulo, score
    pthread_mutex_lock(&pointermutex);
    pthread_mutex_lock(&soremutex);
    move(0, 0);
    move(0, COLS - 20);
    addstr("MATCOM INVASION");
    move(0, 1);
    addstr("SCORE: ");
    move(0, 15);
    addstr("REMAINING: ");
    move(0, COLS - 1);
    sprintf(tellscore, "%d", score);
    move(0, 8);
    addstr(tellscore);
    sprintf(tellscore, "%d", currentaliens);
    move(0, 26);
    addstr(tellscore);
    addstr("-");
    move(0, 0);
    refresh();
    pthread_mutex_unlock(&soremutex);
    pthread_mutex_unlock(&pointermutex);
}

// Funcion que mueve al jugador
void move_player()
{
    pthread_mutex_lock(&spaceship_mutex);
    pthread_mutex_lock(&pointermutex);
    //  Pone el puntero en la posicion actualizada del jugador y lo dibuja
    move(spaceship.r, spaceship.c);
    addch(spaceship.ch);
    // Verificar posicion valida de jugador
    if (spaceship.c > COLS - 1)
    {
        spaceship.c = COLS - 1;
    }
    else if (spaceship.c < 0)
    {
        spaceship.c = 0;
    }
    pthread_mutex_unlock(&pointermutex);
    pthread_mutex_unlock(&spaceship_mutex);
}

// Funcion que mueve las bombas de aliens
void move_bombs()
{
    pthread_mutex_lock(&bombsmutex);
    pthread_mutex_lock(&pointermutex);
    if (loops % settings.bombs_speed == 0) // La variable loops define la velocidad a la que va el juego.
    {
        for (int i = 0; i < settings.bombs_amount; ++i)
        {
            if (bomb[i].active == 1)
            {
                // Si la bomba esta activa, muevela de posicion
                if (bomb[i].r < LINES)
                {
                    // Poniendo en blanco la posicion anterior
                    if (bomb[i].loop != 0)
                    {
                        move(bomb[i].r - 1, bomb[i].c);
                        addch(' ');
                    }
                    else
                    {
                        ++bomb[i].loop;
                    }
                    // Dibujando bomba en nueva posicion
                    move(bomb[i].r, bomb[i].c);
                    addch(bomb[i].ch);
                    ++bomb[i].r;
                }
                // Elimina la bomba si llego al fondo del campo
                else
                {
                    bomb[i].active = 0;
                    bomb[i].loop = 0;
                    --currentbombs;
                    move(bomb[i].r - 1, bomb[i].c);
                    addch(' ');
                }
            }
        }
    }
    pthread_mutex_unlock(&pointermutex);
    pthread_mutex_unlock(&bombsmutex);
}

// Funcion que mueve los disparos del jugador
void move_shots()
{
    pthread_mutex_lock(&alienmutex);
    pthread_mutex_lock(&pointermutex);
    pthread_mutex_lock(&soremutex);
    pthread_mutex_lock(&shootsmutex);
    if (loops % settings.shots_speed == 0)
    {
        for (int i = 0; i < settings.shots_amount; ++i)
        {
            // Si el disparo del jugador esta activo
            if (shot[i].active == 1)
            {
                if (shot[i].r > 0)
                {
                    if (shot[i].r < LINES - 2)
                    {
                        // Eliminar disparo de la posicion anterior
                        move(shot[i].r + 1, shot[i].c);
                        addch(' ');
                    }
                    for (int j = 0; j < settings.ingame_aliens; ++j)
                    {
                        if (ingame_aliens[j].alive == 1 && ingame_aliens[j].r == shot[i].r && ingame_aliens[j].pc == shot[i].c)
                        {
                            // Si el disparo impacto a un enemigo, borra el disparo, mata al alien y aumenta el score
                            score += 20;
                            ingame_aliens[j].alive = 0;
                            shot[i].active = 0;
                            --currentshots;
                            --currentaliens;
                            move(ingame_aliens[j].pr, ingame_aliens[j].pc);
                            addch(' ');
                            break;
                        }
                    }
                    if (shot[i].active == 1)
                    {
                        // Dibujar disparo en la posicion siguiente (si esta activo)
                        move(shot[i].r, shot[i].c);
                        addch(shot[i].ch);
                        --shot[i].r;
                    }
                }
                else
                {
                    // Borrar disparo si llego al final
                    shot[i].active = 0;
                    --currentshots;
                    move(shot[i].r + 1, shot[i].c);
                    addch(' ');
                }
            }
        }
    }
    pthread_mutex_unlock(&shootsmutex);
    pthread_mutex_unlock(&soremutex);
    pthread_mutex_unlock(&pointermutex);
    pthread_mutex_unlock(&alienmutex);
}

// Funcion que mueve a los aliens
void move_aliens()
{
    if (loops % settings.alien_speed == 0)
    {
        pthread_mutex_lock(&alienmutex);
        pthread_mutex_lock(&pointermutex);
        // Actualizando tiempo de apairicion de cada alien del juego
        for (int i = 0; i < settings.alien_amount; i++)
        {
            // Si ya debe aparecer en pantalla, asignale una posicion de carga
            if (aliens[i].spawn_time == 0)
            {
                aliens[i].showed = current_alien;
                current_alien++;
            }
            // Si aun no debe aparecer en pantalla, disminuye su tiempo de aparicion
            if (aliens[i].spawn_time != -1)
            {
                aliens[i].spawn_time--;
            }
        }

        // Liberando paginas ocupadas por aliens muertos y asignando prioridad respectiva
        for (int i = 0; i < settings.ingame_aliens; i++)
        {
            // Verificando si existe alguna pagina ocupada por un alien obsoleto (ya muerto)
            if (ingame_aliens[i].alive == 0 && ingame_aliens[i].showed != 0 && alien_out_time[i] == -1)
            {
                // Hallando prioridad mas alta asignada a una pagina para definir la de la pagina actual
                int max = -1;

                // Asignando prioridad a la pagina actual y liberandola
                alien_out_time[i] = max + 1;
            }
        }

        // Ejecutando reemplazo de paginas e insertando aliens nuevos en pantalla
        for (int i = 0; i < settings.alien_amount; i++)
        {
            // Verificando si existe algun alien que deba aparecer en pantalla
            if (aliens[i].showed == current_showed_alien)
            {
                // Verificando si existe alguna pagina disponible para el alien (prioridad mas alta == valor mas bajo)
                for (int j = 0; j < settings.ingame_aliens; j++)
                {
                    // Reemplazando pagina y asignando al nuevo alien
                    if (alien_out_time[j] == 0)
                    {
                        alien_out_time[j] = -1;
                        ingame_aliens[j].alive = aliens[i].alive;
                        ingame_aliens[j].c = aliens[i].c;
                        ingame_aliens[j].ch = aliens[i].ch;
                        ingame_aliens[j].direction = aliens[i].direction;
                        ingame_aliens[j].pc = aliens[i].pc;
                        ingame_aliens[j].pr = aliens[i].pr;
                        ingame_aliens[j].r = aliens[i].r;
                        ingame_aliens[j].showed = aliens[i].showed;
                        ingame_aliens[j].spawn_time = aliens[i].spawn_time;
                        current_showed_alien++;
                    }
                    // Aumentando la prioridad de las otras paginas (disminuyendo valor)
                    else
                    {
                        if (alien_out_time[j] > 0)
                        {
                            alien_out_time[j]--;
                        }
                    }
                }
            }
        }

        // Moviendo a los aliens en pantalla a sus respectivas posiciones siguientes
        for (int i = 0; i < settings.ingame_aliens; ++i)
        {
            if (ingame_aliens[i].alive == 1)
            {
                // Borrar alien viejo
                move(ingame_aliens[i].pr, ingame_aliens[i].pc);
                addch(' ');
                // Poner alien nuevo
                move(ingame_aliens[i].r, ingame_aliens[i].c);
                addch(ingame_aliens[i].ch);
                // Actualizar posiciones viejas de alien
                ingame_aliens[i].pr = ingame_aliens[i].r;
                ingame_aliens[i].pc = ingame_aliens[i].c;

                // Verificar si deberia soltar una bomba ahora (los de tipo Y no tiran bombas)
                if (ingame_aliens[i].ch != 'Y')
                {
                    int n_random = 1 + (rand() % 100);
                    if ((settings.bombchance - n_random) >= 0 && currentbombs < settings.bombs_amount)
                    {
                        for (int j = 0; j < settings.bombs_amount; ++j)
                        {
                            if (bomb[j].active == 0)
                            {
                                bomb[j].active = 1;
                                ++currentbombs;
                                bomb[j].r = ingame_aliens[i].r + 1;
                                bomb[j].c = ingame_aliens[i].c;
                                break;
                            }
                        }
                    }
                }

                // Estableciendo posicion actual del alien
                if (ingame_aliens[i].direction == 'l')
                    --ingame_aliens[i].c;
                else if (ingame_aliens[i].direction == 'r')
                    ++ingame_aliens[i].c;
                else if (ingame_aliens[i].direction == 'd')
                    ++ingame_aliens[i].r;

                // Verificando proxima posicion del alien
                if (ingame_aliens[i].ch == 'Y')
                {
                    if (ingame_aliens[i].direction == 'd')
                    {
                        ingame_aliens[i].direction = 's';
                    }
                    else if (ingame_aliens[i].direction == 's')
                    {
                        ingame_aliens[i].direction = 'S';
                    }
                    else if (ingame_aliens[i].direction == 'S')
                    {
                        ingame_aliens[i].direction = 'z';
                    }
                    else if (ingame_aliens[i].direction == 'z')
                    {
                        ingame_aliens[i].direction = 'd';
                    }
                }
                else if (ingame_aliens[i].ch == 'V')
                {
                    int rdm = (rand() % 3) + 1;
                    if (rdm <= 1)
                    {
                        if (ingame_aliens[i].pd == 's')
                        {
                            ingame_aliens[i].direction = 'l';
                        }
                        else
                        {
                            ingame_aliens[i].direction = 's';
                        }
                    }
                    else if (rdm <= 2)
                    {
                        if (ingame_aliens[i].pd == 'l')
                        {
                            ingame_aliens[i].direction = 'r';
                        }
                        else
                        {
                            ingame_aliens[i].direction = 'l';
                        }
                    }
                    else if (rdm <= 3)
                    {
                        if (ingame_aliens[i].pd == 'r')
                        {
                            ingame_aliens[i].direction = 's';
                        }
                        else
                        {
                            ingame_aliens[i].direction = 'r';
                        }
                    }
                }
                // Definiendo accion en los bordes
                if (ingame_aliens[i].c == COLS - 2)
                {
                    ++ingame_aliens[i].r;
                    ingame_aliens[i].direction = 'l';
                }
                else if (ingame_aliens[i].c == 0)
                {
                    ++ingame_aliens[i].r;
                    ingame_aliens[i].direction = 'r';
                }
            }
        }
        pthread_mutex_unlock(&pointermutex);
        pthread_mutex_unlock(&alienmutex);
    }
}

// Actualizar pantalla
void act_screen()
{
    // Refrescar pantalla
    pthread_mutex_lock(&pointermutex);
    move(0, 0);
    refresh();
    pthread_mutex_unlock(&pointermutex);
    // Esperar tantos milisegundos como se haya definido en las opciones (velocidad del juego)
    usleep(settings.overall_speed);
    // Comenzar siguiente loop (ciclo de juego)
    ++loops;
}

//////////////////////////////////////IMPLEMENTANDO OTRAS FUNCIONES

// Recibir entrada del teclado
void write_input()
{
    // Recibir entrada
    input = getch();
    // Borrar posicion vieja de la nave
    pthread_mutex_lock(&pointermutex);
    pthread_mutex_lock(&spaceship_mutex);
    move(spaceship.r, spaceship.c);
    addch(' ');
    pthread_mutex_unlock(&spaceship_mutex);
    pthread_mutex_unlock(&pointermutex);

    // Actualizar posicion del jugador
    if (input == KEY_LEFT)
    {
        pthread_mutex_lock(&spaceship_mutex);
        --spaceship.c;
        pthread_mutex_unlock(&spaceship_mutex);
    }
    else if (input == KEY_RIGHT)
    {
        pthread_mutex_lock(&spaceship_mutex);
        ++spaceship.c;
        pthread_mutex_unlock(&spaceship_mutex);
    }
    // Efectuar disparo
    else if (input == ' ' && currentshots < settings.shots_amount)
    {
        pthread_mutex_lock(&shootsmutex);
        for (int i = 0; i < settings.shots_amount; ++i)
        {
            if (shot[i].active == 0)
            {
                // Dibujar nuevo disparo
                shot[i].active = 1;
                ++currentshots;
                --score;
                shot[i].r = LINES - 2;
                shot[i].c = spaceship.c;
                break;
            }
        }
        pthread_mutex_unlock(&shootsmutex);
    }
}

// Definir configuracion normal de las variables
void init_config()
{
    // Definiendo opciones por defecto
    settings.overall_speed = 15000;
    settings.alien_speed = 15;
    settings.shots_speed = 3;
    settings.bombs_speed = 10;
    settings.bombchance = 5;
    settings.alien_amount = 20;
    settings.shots_amount = 4;
    settings.bombs_amount = 1000;
    settings.ingame_aliens = 10;
}

// Definiendo configuracion al iniciar el juego
void starting_game()
{
    // Reestableciendo variables globales
    loops = 0;
    currentshots = 0;
    currentbombs = 0;
    currentaliens = settings.alien_amount;
    current_alien = 1;
    current_showed_alien = 1;
    score = 0;
    gamestate = -1;

    // Definiendo opciones del jugador
    spaceship.r = LINES - 1;
    spaceship.c = COLS / 2;
    spaceship.ch = '^';

    // Definiendo opciones de aliens
    aliens = malloc(sizeof(t_alien) * settings.alien_amount);
    ingame_aliens = malloc(sizeof(t_alien) * settings.ingame_aliens);
    alien_out_time = malloc(sizeof(int) * settings.ingame_aliens);

    for (int i = 0; i < settings.alien_amount; i++)
    {
        aliens[i].pd = 's';
        aliens[i].showed = 0;
        aliens[i].alive = 1;
        // Definiendo posicion inicial
        int rdm = (rand() % 3) + 1;
        aliens[i].r = rdm;
        rdm = (rand() % COLS - 1) + 1;
        aliens[i].c = rdm;
        // Posiciones anteriores son 0,0
        aliens[i].pr = 0;
        aliens[i].pc = 0;
        // Definiendo tipo de alien
        rdm = (rand() % 3) + 1;
        if (rdm <= 1)
        {
            aliens[i].ch = 'H';
        }
        else if (rdm <= 2)
        {
            aliens[i].ch = 'Y';
        }
        else
        {
            aliens[i].ch = 'V';
        }
        // Definiendo direccion de movimiento inicial
        rdm = (rand() % 3) + 1;
        // Aliens de tipo T
        if (aliens[i].ch == 'V')
        {
            if (rdm <= 1)
            {
                aliens[i].direction = 'r';
            }
            else if (rdm <= 2)
            {
                aliens[i].direction = 'l';
            }
            else
            {
                aliens[i].direction = 'd';
            }
        }
        // Aliens de tipo M
        else if (aliens[i].ch == 'H')
        {
            if (rdm <= 1.5)
            {
                aliens[i].direction = 'r';
            }
            else
            {
                aliens[i].direction = 'l';
            }
        }
        // Aliens de tipo V
        else if (aliens[i].ch == 'Y')
        {
            aliens[i].direction = 'd';
        }
        rdm = (rand() % settings.ingame_aliens) + 1;
        aliens[i].spawn_time = rdm;
    }

    // Definiendo paginas iniciales de aliens
    for (int i = 0; i < settings.ingame_aliens; i++)
    {
        ingame_aliens[i].alive = 0;
        ingame_aliens[i].showed = 0;
        alien_out_time[i] = i;
    }

    // Definiendo opciones de disparos
    for (int i = 0; i < settings.shots_amount; ++i)
    {
        shot[i].active = 0;
        shot[i].ch = 'I';
    }

    // Definiendo opciones de bombas (aliens)
    for (int i = 0; i < settings.bombs_amount; ++i)
    {
        bomb[i].active = 0;
        bomb[i].ch = 'I';
        bomb[i].loop = 0;
    }
}

////////////////////////////////////DECLARANDO HILOS

void *logicthread(void *arg)
{
    while (1)
    {

        // Mostrar puntuacion
        show_stats();
        // Mover jugador
        move_player();
        // Mover bombas
        move_bombs();
        // Mover disparos del jugador
        move_shots();
        // Mover aliens
        move_aliens();
        // Actualizar pantalla y loops
        act_screen();
        // Ejecutar entrada del teclado
        write_input();

        // Aqui no pongo mutex porque solo estoy leyendo el valor
        if (!(gamestate == -1))
        {
            break;
        }
    }
    return NULL;
}

void *eventsthread(void *arg)
{
    while (1)
    {
        // Verificar si el juego se gano o perdio:
        // Ya no quedan aliens
        pthread_mutex_lock(&alienmutex);
        pthread_mutex_lock(&bombsmutex);
        pthread_mutex_lock(&gamestatemutex);
        if (currentaliens == 0)
        {
            gamestate = 1;
        }
        // Algun alien llego al final
        for (int i = 0; i < settings.ingame_aliens; ++i)
        {
            if (ingame_aliens[i].r == LINES - 1)
            {
                gamestate = 2;
                pthread_mutex_unlock(&bombsmutex);
                pthread_mutex_unlock(&alienmutex);
                break;
            }
        }
        // Alguna bomba impacto al jugador
        for (int i = 0; i < settings.bombs_amount; ++i)
        {
            if (bomb[i].r == spaceship.r && bomb[i].c == spaceship.c)
            {
                gamestate = 0;
                pthread_mutex_unlock(&bombsmutex);
                pthread_mutex_unlock(&alienmutex);
                break;
            }
        }
        pthread_mutex_unlock(&bombsmutex);
        pthread_mutex_unlock(&alienmutex);
        // Comprobando si termino el juego
        if (!(gamestate == -1))
        {
            pthread_mutex_unlock(&gamestatemutex);
            break;
        }
        pthread_mutex_unlock(&gamestatemutex);
    }
    return NULL;
}

////////////////////////////////////FUNCIONES PRINCIPALES

// Funcion que maneja el gameplay
void game()
{
    // Creando hilos
    pthread_t event_th, game_th;
    // Inicializando funciones de hilos
    pthread_create(&event_th, NULL, eventsthread, NULL);
    pthread_create(&game_th, NULL, logicthread, NULL);
    // Ciclo de juego
    while (1)
    {
        pthread_mutex_lock(&gamestatemutex);
        if (!(gamestate == -1))
        {
            pthread_mutex_unlock(&gamestatemutex);
            break;
        }
        pthread_mutex_unlock(&gamestatemutex);
        sleep(1);
    }
    // Esperando a que terminen los hilos
    pthread_join(event_th, NULL);
    pthread_join(game_th, NULL);

    // Liberando memoria de los arrays reservados
    free(aliens);
    free(ingame_aliens);
    free(alien_out_time);
    // Terminando partida
    gameover(gamestate);
}

int main(int argc, char const *argv[])
{
    // Inicializar biblioteca curses
    initscr();
    clear();
    noecho(); // bloqueo la escritura
    cbreak();
    nodelay(stdscr, 1);
    keypad(stdscr, 1);
    srand(time(NULL));

    // configuracion inicial
    init_config();

    while (1)
    {
        clear();
        starting_game();
        game();
        clear();
        move(0, 0);
        endwin();
        exit(0);
    }
}