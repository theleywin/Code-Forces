//Copyright © -> Diego Viera & Pablo Gómez
// Havana University

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

typedef struct ship {
    int startX ;
    int startY ;
    int width ;
    int height ;
    bool destroyed ;
} ship ;

typedef struct bullet {
    int positionX ;
    int positionY ;
    chtype bullet_image ;
    bool direction ; // 0 -> down , 1 -> up
    struct bullet * next ;
} bullet;

typedef struct enemy{
    int positionX ;
    int positionY ;
    int direction ; // 0 -> down , 1-> left , 2 -> right
    int width ;
    int height ;
    int memory_index ;
    int lives ;
    int rank ;
    bool shoot ;
    bool destroyed ;
}enemy;

//Blocks of the FreeList
typedef struct block {
    int index ;
    int space ;
    struct block * next ;
}block;

//Functions Prototypes
void Init_ship_params() ;
void Print_ship(int posX , int posY);
void * Player_input();
void * Move_bullets();
void Delete_bullet(bullet * delete);
void Add_bullet(chtype image , bool is_player , int x , int y);
void free_bullets();
void * Draw_game();
void Print_enemy1(enemy * e);
void  Create_enemys(int number_enemys , int rank);
void * Enemy_generator();
void * Move_enemys();
void Add_block(int space , int index);
void Delete_block(int index);
int FirstFit(int space);
int BestFit(int space);
int WorstFit(int space);
void GameLoop(int maxY , int maxX);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
void Presentation(int maxY , int maxX);
void PauseLoop();
void free_free_list();
void Print_enemy2(enemy * e);
void Print_enemy3(enemy * e);
void enqueue(int x);
void dequeue();
int see_front();
void gen_en();
int read_txt(const char *filename);
void safe_txt(const char *filename, int valor);

//Global Variables
enemy * enemys_memory[1000] ;
int MEMORY[1000][2] ; //Blocks 0 -> Space , 1 -> 0 or 1 (Available)
block * free_list = NULL ; //Free list of blocks
block * last_block = NULL ;

ship * player ;
bool finish = FALSE ;
int input = 0 ;
bullet * bullets_list = NULL ;
bullet * last_bullet = NULL;
int enemy_count = 0 ;
int score = 0 ;
int ship_rating_color = 3 ;
bool _pause = FALSE ;
int op = 0 ;
WINDOW * pause_menu_window ;
int gen_enem1=0 ;
int gen_enem2=0 ;
int gen_enem3=0 ;
const char *filename = "valor.txt";


//Dekker algorithm vars
int turn = 0 ;
bool flag[2] ;

//Mutex
pthread_mutex_t playerMutex ;
pthread_mutex_t bulletsMutex ;
pthread_mutex_t enemysMutex ;
pthread_mutex_t pauseMutex ;

int main(void)
{
    setlocale(LC_ALL , "UTF-8");
    initscr();//Init ncurses
    curs_set(0);//Hide cursor
    timeout(1);
    noecho();
    srand(time(NULL));
    cbreak();
    flag[0] = false ;
    flag[1] = false ;
    int maxY ;
    int maxX ;
    getmaxyx(stdscr , maxY , maxX);

    //colors
    start_color();
    init_pair(1 , COLOR_CYAN , COLOR_BLACK);
    init_pair(2 , COLOR_GREEN , COLOR_BLACK);
    init_pair(3 , COLOR_WHITE , COLOR_BLACK);
    init_pair(4 , COLOR_RED , COLOR_BLACK);
    init_pair(5 , COLOR_BLUE , COLOR_BLACK);
    init_pair(6 , COLOR_MAGENTA , COLOR_BLACK);
    init_pair(7 , COLOR_YELLOW , COLOR_BLACK);

    //init mutexs
    pthread_mutex_init(&playerMutex , NULL);
    pthread_mutex_init(&bulletsMutex , NULL);
    pthread_mutex_init(&enemysMutex , NULL);
    pthread:pthread_mutex_init(&pauseMutex , NULL);

    //init MEMORY
    int x = 249;
    int sp = 1 ;
    for(int i = 0 ; i < 1000 ; i++) {
        Add_block(sp , i);
        enemys_memory[i] = NULL ;
        MEMORY[i][0] = sp ;
        MEMORY[i][1] = 1 ;
        if(i == x) {
            x += 250 ;
            sp += 1 ;
        }
    }

    pthread_t thread_input , thread_bullets , thread_draw , thread_enemys , thread_move_enemys;
    Init_ship_params();
    Print_ship(player->startX , player->startY);
    //threads
    pthread_create(&thread_input , NULL , Player_input , NULL );
    Presentation( maxY , maxX);
    pthread_create(&thread_move_enemys , NULL , Move_enemys , NULL);
    pthread_create(&thread_bullets , NULL , Move_bullets , NULL);
    pthread_create(&thread_draw , NULL , Draw_game , NULL);
    pthread_create(&thread_enemys , NULL , Enemy_generator , NULL);

    GameLoop(maxY , maxX);

    //Cancel thread
    pthread_cancel(thread_bullets);
    pthread_cancel(thread_enemys);
    pthread_cancel(thread_move_enemys);
    pthread_cancel(thread_input);
    pthread_cancel(thread_draw);
    free(player);
    free_free_list();

    int best_score = read_txt(filename);
    if(score > best_score) {
        safe_txt(filename , score);
        clear();
        attron(COLOR_PAIR(2));
        attron(A_BLINK);
        mvprintw( maxY / 2 , maxX/ 2 - 5  , "NEW RECORD %d !" , score);
        attroff(A_BLINK);
        attroff(COLOR_PAIR(3));
        refresh();
        sleep(3);
    }
    clear();
    mvprintw( maxY / 2 , maxX/ 2 - 5  , "SCORE %d" , score);
    refresh();
    sleep(2);
    clear();
    mvprintw( maxY / 2 , maxX/ 2 - 5  , "GAME OVER");
    refresh();
    sleep(2);
    endwin();
    return 0;
}

void Presentation(int maxY , int maxX) {
    int best_score = read_txt(filename);
    attron(COLOR_PAIR(2));
    mvprintw( maxY / 2 - 2, maxX / 2 - 7  , "SPACE INVADER");
    mvprintw( maxY / 2 + 2, maxX / 2 - 7  , "Best score %d" , best_score);
    attroff(COLOR_PAIR(2));
    attron(A_BLINK);
    mvprintw( maxY / 2 , maxX / 2 - 12  , "- press enter to start -");
    attroff(A_BLINK);
    attron(A_UNDERLINE);
    mvprintw( maxY - 2, maxX / 2 - 25 , "By Diego Manuel Viera Martínez & Pablo Gómez Vidal");
    attroff(A_UNDERLINE);
    while (input != 10) {}
}

void PauseLoop() {

        pause_menu_window = newwin(10, 40, LINES / 2 - 5 , COLS / 2 - 20 );
        wclear(pause_menu_window);
        keypad(pause_menu_window, TRUE);
        box(pause_menu_window, 0, 0);
        print_in_middle(pause_menu_window, 1, 0, 40, "Pause", COLOR_PAIR(2));
        mvwaddch(pause_menu_window, 2, 0, ACS_LTEE);
        mvwhline(pause_menu_window, 2, 1, ACS_HLINE, 38);
        mvwaddch(pause_menu_window, 2, 39, ACS_RTEE);
        mvwprintw(pause_menu_window , 4 ,  16 , "Continue");
        mvwprintw(pause_menu_window , 6 ,  18 , "Exit");
        do {
            int selector = wgetch(pause_menu_window);
            switch (selector) {
                case KEY_UP :
                    op = 0 ;
                break;
                case KEY_DOWN :
                    op = 1 ;
                break;
                case 10:
                    if(op) {
                        finish = TRUE ;
                        wclear(pause_menu_window);
                    }
                    _pause = FALSE ;
                break ;
            }
            wattron(pause_menu_window,COLOR_PAIR(2));
            if(op == 0) {
                mvwprintw(pause_menu_window , 6 ,  15 , "  ");
                mvwprintw(pause_menu_window , 6 ,  23 , "  ");
                mvwprintw(pause_menu_window , 4 ,  13 , "->");
                mvwprintw(pause_menu_window , 4 ,  25 , "<-");
            }
            else {
                mvwprintw(pause_menu_window , 4 ,  13 , "  ");
                mvwprintw(pause_menu_window , 4 ,  25 , "  ");
                mvwprintw(pause_menu_window , 6 ,  15 , "->");
                mvwprintw(pause_menu_window , 6 ,  23 , "<-");
            }
            wattroff(pause_menu_window,COLOR_PAIR(2));
            wrefresh(pause_menu_window);
        }
        while (_pause && !finish);
        wclear(pause_menu_window);
        wrefresh(pause_menu_window);
        delwin(pause_menu_window);
}

void GameLoop(int maxY , int maxX) {
    do {
        switch (input) {
            case KEY_LEFT:
                if(player->startX - 2 > 8) {
                    player->startX-=2 ;
                }
            break;
            case KEY_RIGHT:
                if(player->startX + 2 < COLS - 8) {
                    player->startX+=2 ;
                }
            break;
            case KEY_UP:
                if(_pause) {
                    op = 0 ;
                }
                else if(player->startY - 2 > 0) {
                    player->startY-=2 ;
                }
            break;
            case KEY_DOWN:
                if(_pause) {
                    op = 1 ;
                }
                else if(player->startY + 2 < LINES - 4) {
                    player->startY+=2 ;
                }
            break;
            case 32 :
                Add_bullet('|' , TRUE , player->startX , player->startY - 1);
            break;
            case 27:
                _pause = !_pause;
            break;
        }
        input = 0 ;
        if(player->destroyed) finish = true ;
        usleep(500);
    }while(!finish);
}

void Init_ship_params() {
    //Ship position
    player = (ship*)malloc(sizeof(ship));
    player->height = 5 ;
    player->width = 17 ;
    player->startX = (COLS- player->width)/2 + 8;
    player->startY = (LINES - player->width)/2 + 18;
    player->destroyed = FALSE ;
}

void Print_ship(int posX , int posY) {

    int x = posX;
    int y = posY;

    if(player->destroyed) attron(A_BLINK);
    if(score < 25) {
        ship_rating_color = 3 ;
    }
    else if(score >= 25 && score < 50) {
        ship_rating_color = 2 ;
    }
    else if(score >= 50 && score < 75) {
        ship_rating_color = 1 ;
    }
    else if(score >= 75 && score < 100) {
        ship_rating_color = 5 ;
    }
    else if(score >= 100 && score < 150) {
        ship_rating_color = 6 ;
    }
    else if(score >= 150 && score < 200) {
        ship_rating_color = 7;
    }
    else {
        ship_rating_color = 4 ;
    }
    attron(COLOR_PAIR(1));
    mvaddch(y , x ,'-');
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(3));
    mvaddch(y , x-1 ,'/');
    mvaddch(y , x+1 ,'\\');
    mvaddch(y+1 , x ,'-');
    mvaddch(y+1 , x-1 ,'/');
    mvaddch(y+1 , x-2 ,'/');
    mvaddch(y+1 , x+1 ,'\\');
    mvaddch(y+1 , x+2 ,'\\');
    mvaddch(y+2 , x ,' ');
    mvaddch(y+2 , x-2 ,'_');
    mvaddch(y+2 , x-3 ,'/');
    mvaddch(y+2 , x-4 ,'_');
    mvaddch(y+2 , x-5 ,'-');
    mvaddch(y+2 , x-6 ,'_');
    mvaddch(y+2 , x-7 ,'|');
    mvaddch(y+2 , x+2 ,'_');
    mvaddch(y+2 , x+3 ,'\\');
    mvaddch(y+2 , x+4 ,'_');
    mvaddch(y+2 , x+5 ,'-');
    mvaddch(y+2 , x+6 ,'_');
    mvaddch(y+2 , x+7 ,'|');
    mvaddch(y+3 , x ,'-');
    mvaddch(y+3 , x-1 ,'-');
    mvaddch(y+3 , x-4 ,'-');
    mvaddch(y+3 , x-5 ,'_');
    mvaddch(y+3 , x-6 ,'-');
    mvaddch(y+3 , x-7 ,'|');
    mvaddch(y+3 , x+1 ,'-');
    mvaddch(y+3 , x+4 ,'-');
    mvaddch(y+3 , x+5 ,'_');
    mvaddch(y+3 , x+6 ,'-');
    mvaddch(y+3 , x+7 ,'|');
    mvaddch(y+4 , x-1 ,'\\');
    mvaddch(y+4 , x-2,'<');
    mvaddch(y+4 , x+1 ,'/');
    mvaddch(y+4 , x+2 ,'>');
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(ship_rating_color));
    mvaddch(y+2 , x-1 ,'[');
    mvaddch(y+2 , x+1 ,']');
    mvaddch(y+2 , x+8 ,'\\');
    mvaddch(y+2 , x-8 ,'/');
    mvaddch(y+3 , x-2 ,'|');
    mvaddch(y+3 , x-3 ,'|');
    mvaddch(y+3 , x-8 ,'\\');
    mvaddch(y+3 , x+8 ,'/');
    mvaddch(y+3 , x+2 ,'|');
    mvaddch(y+3 , x+3 ,'|');
    mvaddch(y+4 , x ,'-');
    attroff(COLOR_PAIR(ship_rating_color));
    if(player->destroyed) attroff(A_BLINK);
}

void * Player_input() {
    keypad(stdscr , true) ;
    int key ;
    // Dekker algorithm
    while (1) {
        flag[0] = true ;
        while (flag[1]) {
            flag[0] = false;
            while (turn!= 0){}
            flag[0] = true ;
        }
        input = getch() ;
        turn = 1 ;
        flag[0] = false ;
    }
    return NULL ;
}

void * Move_bullets() {
    while (1) {
        if(_pause) continue;
        pthread_mutex_lock(&bulletsMutex) ;
        bullet * current_bullet = bullets_list ;
        enemy * current_enemy ;
        while (current_bullet != NULL) {
            //if direction == 1 then up else down
            if(current_bullet->direction) {
                current_bullet->positionY -= 1 ;
            }else current_bullet->positionY += 1 ;

            //if bullet is out of bounds then delete it
            if(current_bullet->positionY < 0 || current_bullet->positionY > LINES) {
                Delete_bullet(current_bullet);
            }
            else if(!current_bullet->direction) {
                if(current_bullet->positionY == player->startY + 1
                    && current_bullet->positionX >= player->startX - 1
                    && current_bullet->positionX <= player->startX + 1
                    || current_bullet->positionY == player->startY + 2
                    && current_bullet->positionX >= player->startX - 2
                    && current_bullet->positionX <= player->startX + 2
                    || current_bullet->positionY == player->startY + 3
                    && current_bullet->positionX >= player->startX - 8
                    && current_bullet->positionX <= player->startX + 8) {
                    player->destroyed = TRUE ;
                    Delete_bullet(current_bullet);
                    beep();
                }
            }
            else {
                pthread_mutex_lock(&enemysMutex);
                for(int i = 0 ; i < 1000; i++) {
                    current_enemy = enemys_memory[i];
                    if(current_enemy == NULL) {
                        continue;
                    }
                    if(current_enemy->rank == 1 && !current_enemy->destroyed) {
                        if(current_bullet->positionY == current_enemy->positionY
                            && current_bullet->positionX >= current_enemy->positionX - 2
                            && current_bullet->positionX <= current_enemy->positionX + 2) {
                            current_enemy->lives -= 1 ;
                            if(current_enemy->lives == 0) {
                                current_enemy->destroyed = TRUE ;
                            }
                            score += 1 ;
                            Delete_bullet(current_bullet);
                        }
                    }
                    else if(current_enemy->rank == 2 && !current_enemy->destroyed) {
                        if(current_bullet->positionY == current_enemy->positionY
                            && current_bullet->positionX >= current_enemy->positionX - 4
                            && current_bullet->positionX <= current_enemy->positionX + 4) {
                            current_enemy->lives -= 1 ;
                            if(current_enemy->lives == 0) {
                                current_enemy->destroyed = TRUE ;
                            }
                            score += 1 ;
                            Delete_bullet(current_bullet);
                        }
                    }
                    else if (current_enemy->rank == 3 && !current_enemy->destroyed) {
                        if(current_bullet->positionY == current_enemy->positionY
                            && current_bullet->positionX >= current_enemy->positionX - 11
                            && current_bullet->positionX <= current_enemy->positionX + 11) {
                            current_enemy->lives -= 1 ;
                            if(current_enemy->lives == 0) {
                                current_enemy->destroyed = TRUE ;
                            }
                            score += 1 ;
                            Delete_bullet(current_bullet);
                        }
                    }
                }
                pthread_mutex_unlock(&enemysMutex);
            }
            current_bullet = current_bullet->next ;
        }
        pthread_mutex_unlock(&bulletsMutex);
        usleep(50000);
    }
    return NULL ;
}

void Delete_bullet(bullet * delete) {
    bullet * current = bullets_list ;
    bullet * prev = current ;
    if(delete == bullets_list) {
        bullets_list = delete->next ;
        free(delete);
    }
    else {
        while (current->next != delete) {
            current = current->next ;
        }
        prev = current ;
        current = current->next ;
        if(current == last_bullet) {
            last_bullet = prev ;
        }
        prev->next = current->next ;
        free(current);

    }
}

void Add_bullet(chtype image , bool is_player , int x , int y) {
    bullet * current = bullets_list;
    bool bullet_direction = FALSE ;
    if(is_player) bullet_direction = TRUE ;
    if(current == NULL) {
        bullets_list = (bullet*)malloc(sizeof(bullet));
        current = bullets_list ;
        last_bullet = bullets_list ;
        current->bullet_image = image;
        current->direction = bullet_direction;
        current->next = NULL ;
        current->positionX = x ;
        current->positionY = y;
    }
    else {
        current = last_bullet ;
        current->next = (bullet*)malloc(sizeof(bullet));
        current = current->next ;
        current->bullet_image = image;
        current->direction = bullet_direction ;
        current->next = NULL ;
        current->positionX = x ;
        current->positionY = y ;
        last_bullet = current ;
    }
}

void free_bullets() {
    bullet * current = bullets_list ;
    bullet * next ;
    while (current != NULL) {
        next = current->next ;
        free(current);
        current = next ;
    }
}

void free_free_list() {
    block * current = free_list ;
    block * next;
    while (current != NULL) {
        next = current->next ;
        free(current);
        current = next ;
    }
}

void * Draw_game() {
    while (1) {
        flag[1] = true ;
        while (flag[0]) {
            flag[1] = false;
            while (turn!= 1){};
            flag[1] = true ;
        }
        if(_pause) {
            PauseLoop();
        }
        if(!finish) {
            clear();
            //Draw player
            Print_ship(player->startX , player->startY);

            //Draw enemys
            for(int i = 0 ; i < 1000 ; i++) {
                if(enemys_memory[i] != NULL) {
                    if(enemys_memory[i]->rank == 1) {
                        Print_enemy1(enemys_memory[i]);
                    }
                    else if(enemys_memory[i]->rank == 2) {
                        Print_enemy2(enemys_memory[i]);
                    }
                    else if(enemys_memory[i]->rank == 3) {
                        Print_enemy3(enemys_memory[i]);
                    }
                }
            }

            //Draw bullets
            bullet * current_bullet = bullets_list ;
            int color ;
            while (current_bullet != NULL) {
                if(current_bullet->direction) {
                    color = 1 ;
                }
                else color = 4 ;
                attron(COLOR_PAIR(color)) ;
                mvaddch(current_bullet->positionY , current_bullet->positionX , current_bullet->bullet_image) ;
                attroff(COLOR_PAIR(color)) ;
                current_bullet = current_bullet->next ;
            }
            attron(COLOR_PAIR(ship_rating_color));
            mvprintw(2 , 2 , "SCORE %d" , score);
            attroff(COLOR_PAIR(ship_rating_color));
            refresh();
        }
        turn = 0 ;
        flag[1] = false ;
    }
    return  NULL ;
}

void Print_enemy1(enemy * e) {
    int x = e->positionX ;
    int y = e->positionY ;

    attron(COLOR_PAIR(4));
    mvaddch(y , x , '^');
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(2));
    mvaddch(y-1 , x , '-');
    mvaddch(y-1 , x-1 ,'-' );
    mvaddch(y-1 , x+1 , '-');
    if(e->shoot) {
        mvaddch(y , x-1 , '/');
        mvaddch(y , x+1 , '\\');
    }
    else {
        mvaddch(y , x-1 , '\\');
        mvaddch(y , x+1 , '/');
    }
    mvaddch(y , x-2 , '|');
    mvaddch(y , x+2 , '|');
    attroff(COLOR_PAIR(2));

}

void Print_enemy2(enemy * e) {
    int x = e->positionX ;
    int y = e->positionY ;
    attron(COLOR_PAIR(4));
    mvaddch(y , x , '^');
    mvaddch(y - 1, x , 'v');
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(2));
    if(e->shoot) {
        mvaddch(y , x-1 , '/');
        mvaddch(y , x+1 , '\\');
    }
    else {
        mvaddch(y , x-1 , '\\');
        mvaddch(y , x+1 , '/');
    }
    mvaddch(y - 1, x + 1, '\\');
    mvaddch(y - 1, x - 1 , '/');
    mvaddch(y , x + 2 , '|');
    mvaddch(y , x - 2, '|');
    mvaddch(y , x - 3, '-');
    mvaddch(y , x + 3 , '-');
    mvaddch(y , x  + 4, '/');
    mvaddch(y , x - 4, '\\');
    mvaddch(y - 1, x + 2 , '|');
    mvaddch(y - 1, x - 2, '|');
    mvaddch(y - 1, x + 3, '_');
    mvaddch(y - 1, x - 3, '_');
    mvaddch(y - 1, x + 4 , '\\');
    mvaddch(y - 1, x - 4, '/');
    attroff(COLOR_PAIR(2));

}

void Print_enemy3(enemy * e) {
    int x = e->positionX;
    int y = e->positionY;
    attron(COLOR_PAIR(4));
    mvaddch(y , x , 'O');
    mvaddch(y - 1, x , '.');
    mvaddch(y - 1 , x + 1 , '.');
    mvaddch(y - 1, x + 2, '.');
    mvaddch(y - 1, x - 1 , '.');
    mvaddch(y - 1, x - 2, '.');
    mvaddch(y - 3, x , 'X');
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(2));
    mvaddch(y , x - 1, '_');
    mvaddch(y , x + 1, '_');
    mvaddch(y , x + 2, '_');
    mvaddch(y , x - 2 , '_');
    mvaddch(y , x + 3 , '/');
    mvaddch(y , x - 3, '\\');
    mvaddch(y , x - 4, '-');
    mvaddch(y , x - 5, '-');
    mvaddch(y , x - 6, '-');
    mvaddch(y , x + 4, '-');
    mvaddch(y , x + 5, '-');
    mvaddch(y , x + 6, '-');

    mvaddch(y - 1, x + 3 , '\\');
    mvaddch(y - 1, x + 4, '_');
    mvaddch(y - 1, x + 5, '_');
    mvaddch(y - 1, x  + 6, '_');
    mvaddch(y - 1, x + 7, '/');
    mvaddch(y - 1, x + 8 , '_');
    mvaddch(y - 1, x + 9 , '_');
    mvaddch(y - 1, x  + 10, '=');
    mvaddch(y - 1, x + 11, ')');
    mvaddch(y - 1, x - 3 , '/');
    mvaddch(y - 1, x - 4, '_');
    mvaddch(y - 1, x - 5, '_');
    mvaddch(y - 1, x - 6, '_');
    mvaddch(y - 1, x - 7, '\\');
    mvaddch(y - 1, x - 8 , '_');
    mvaddch(y - 1, x - 9 , '_');
    mvaddch(y - 1, x  - 10, '=');
    mvaddch(y - 1, x - 11, '(');

    mvaddch(y - 2, x , '=');
    mvaddch(y - 2, x - 1, '=');
    mvaddch(y - 2, x - 2, '=');
    mvaddch(y - 2, x  - 3, '=');
    mvaddch(y - 2, x - 4 , '-');
    mvaddch(y - 2, x - 5, '-');
    mvaddch(y - 2, x - 6, '-');
    mvaddch(y - 2, x - 7, '_');
    mvaddch(y - 2, x - 8, '_');
    mvaddch(y - 2, x - 9, '_');

    mvaddch(y - 2, x + 1, '=');
    mvaddch(y - 2, x + 2, '=');
    mvaddch(y - 2, x  + 3, '=');
    mvaddch(y - 2, x + 4 , '-');
    mvaddch(y - 2, x + 5, '-');
    mvaddch(y - 2, x + 6, '-');
    mvaddch(y - 2, x + 7, '_');
    mvaddch(y - 2, x + 8, '_');
    mvaddch(y - 2, x + 9, '_');

    mvaddch(y - 3, x - 1 , '-');
    mvaddch(y - 3, x + 1, '-');
    mvaddch(y - 3, x + 2 , '\\');
    mvaddch(y - 3, x + 3, '\\');
    mvaddch(y - 3, x - 2, '/');
    mvaddch(y - 3, x - 3, '/');
    attroff(COLOR_PAIR(2));
}

/*
 *The function  "void * Enemy_generator()" implement a simulation of fifo algorithm from scheduling
 * We generate 3 random variables and introduce then in a queue in order of generation and in the same order
 * we generate the loot of corresponding enemies applying "First In - First Out" in enemy generation
 */
void * Enemy_generator() {
    while (1) {
        if(_pause) continue;
        pthread_mutex_lock(&enemysMutex);

        gen_en();
        enqueue(gen_enem1);
        enqueue(gen_enem2);
        enqueue(gen_enem3);

        if(score < 50 && see_front()==1){
            Create_enemys(3, 1);
            Create_enemys(1,2);
            dequeue();
        }
        if(score < 50 &&see_front() ==2){
            Create_enemys(2, 2);
            Create_enemys(2,1);
            dequeue();
        }
        if(score < 50 &&see_front() ==3){
            Create_enemys(2, 2);
            Create_enemys(3,1);
            dequeue();
        }
        if(score >= 50 && see_front()==1){
            Create_enemys(3, 1);
            Create_enemys(3, 2);
            dequeue();
        }
        if(score >= 50 && see_front()==2){
            Create_enemys(2, 2);
            Create_enemys(4, 1);
            dequeue();
        }
        if(score >= 50 && see_front()==3){
            Create_enemys(3, 3);
            dequeue();
        }
        pthread_mutex_unlock(&enemysMutex);
        sleep(10);

    }
    return NULL ;
}

void Create_enemys(int number_enemys , int rank) {
    int n = number_enemys ;
    enemy_count += number_enemys ;
    while (n != 0 ) {
        int index = FirstFit(rank);
        MEMORY[index][1] = 0 ;
        if(enemys_memory[index] == NULL) {
            enemys_memory[index] = (enemy*)malloc(sizeof(enemy));
        }
        enemys_memory[index]->direction = rand() % 3 ;
        enemys_memory[index]->lives = rank ;
        enemys_memory[index]->height = 2 ;
        enemys_memory[index]->width = 5 ;
        enemys_memory[index]->rank = rank ;
        enemys_memory[index]->positionY = 3 ;
        enemys_memory[index]->positionX = 10 + rand() % 140;
        enemys_memory[index]->shoot = FALSE ;
        enemys_memory[index]->memory_index = index ;
        enemys_memory[index]->destroyed = FALSE ;
        Delete_block(index);
        n-=1;
    }
}

int FirstFit(int space) {

    block * current = free_list;

    if(current == NULL) {
        return 0 ;
    }

    while (current->space < space) {
        current = current->next ;
    }

    return current->index ;
}

int BestFit(int space){

    block * current = free_list;

    if(current == NULL) {
        return 0 ;
    }

    block * best=free_list;
    int aux=30000;

    while (current!=NULL){
        int tmp = current->space - space;
        if(tmp>=0 && tmp < aux){
           aux=tmp;
           best=current;
        }
        current = current->next ;
    }
    return best->index;
}

int WorstFit(int space){

    block * current = free_list;

    if(current == NULL) {
        return 0 ;
    }

    block * best=free_list;
    int aux=30000;

    while (current!=NULL){
        int tmp = current->space - space;
        if(tmp>=0 && tmp > aux){
            aux=tmp;
            best=current;
        }
        current = current->next ;
    }
    return best->index;
}

void Add_block(int space , int index) {// add block to the free list if you delete an enemy

    block * current = free_list ;
    if(current == NULL) {
        free_list = (block*)malloc(sizeof(block));
        current = free_list ;
        last_block = free_list ;
        current->index = 0 ;
        current->space = space ;
        current->next = NULL ;
    }
    else {
        current = last_block ;
        current->next = (block*)malloc(sizeof(block));
        current = current->next ;
        current->index = index ;
        current->space = space ;
        current->next = NULL ;
        last_block = current ;
    }
}

void Delete_block(int index) { //Delete block of the free list when you create a new enemy
    block * current = free_list ;
    block * prev = current ;
    if(free_list == NULL) {
        return;
    }
    if(index == free_list->index) {
        block * delete = free_list ;
        free_list = free_list->next ;
        free(delete);
    }
    else {
        while (current->next->index != index) {
            current = current->next ;
        }
        prev = current ;
        current = current->next ;
        if(current == last_block) {
            last_block = prev ;
        }
        prev->next = current->next;
        free(current);
    }
}

void * Move_enemys() {
    while (1) {
        if(_pause) continue;
        pthread_mutex_lock(&enemysMutex);
        for(int i = 0 ; i < 1000 ; i++) {
            if(enemys_memory[i] != NULL) { //if there is an Enemy then move it
                    if(enemys_memory[i]->direction == 0) {
                        enemys_memory[i]->positionY+=1;
                    }
                    else if(enemys_memory[i]->direction == 1) {
                        if(enemys_memory[i]->positionX-=1 < 5) {
                            enemys_memory[i]->positionY+=1;
                        }
                        else enemys_memory[i]->positionX-=1;
                    }
                    else {
                        if(enemys_memory[i]->positionX + 1 > COLS - 5) {
                            enemys_memory[i]->positionY+=1;
                        }
                        else enemys_memory[i]->positionX+=1;
                    }
                    enemys_memory[i]->direction = rand() % 3;

                    if(enemys_memory[i]->positionY > LINES || enemys_memory[i]->destroyed) {
                        Add_block(MEMORY[i][0], i);
                        MEMORY[i][1] = 1 ;//Space memory available
                        enemys_memory[i] = NULL ;
                        enemy_count -= 1;
                    }
                    else if(rand() % 20 == 1) {
                        if(enemys_memory[i]->rank == 1) {
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX ,  enemys_memory[i]->positionY + 1 );
                        }
                        else if (enemys_memory[i]->rank == 2) {
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX + 1,  enemys_memory[i]->positionY + 1 );
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX - 1,  enemys_memory[i]->positionY + 1 );
                        }
                        else if(enemys_memory[i]->rank == 3) {
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX ,  enemys_memory[i]->positionY + 1 );
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX - 5,  enemys_memory[i]->positionY + 1 );
                            Add_bullet('|' , FALSE , enemys_memory[i]->positionX + 5,  enemys_memory[i]->positionY + 1 );
                        }
                        enemys_memory[i]->shoot = TRUE ;
                    }
                    else {
                         enemys_memory[i]->shoot = FALSE;
                    }
            }
        }

        pthread_mutex_unlock(&enemysMutex);
        usleep(500000);
    }
    return NULL ;
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color) {
    int length, x, y;
    float temp;

    if(win == NULL)
        win = stdscr;
    getyx(win, y, x);
    if(startx != 0)
        x = startx;
    if(starty != 0)
        y = starty;
    if(width == 0)
        width = 80;

    length = strlen(string);
    temp = (width - length)/ 2;
    x = startx + (int)temp;
    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    refresh();
}

//Node structure of our Queue
struct Node {
    int data;
    struct Node* next;
};
struct Node *front,*rear;

void enqueue(int x) {
    struct Node *temp;

    temp = (struct Node*)malloc(sizeof(struct Node));
    temp->data = x;
    temp->next = NULL;

    if(front==NULL && rear == NULL)
    {
        front =rear =temp;
        return;
    }
    rear->next = temp;
    rear = temp;
}

void dequeue() {
    struct Node *temp = front;

    if(front==NULL)
    {
        printf("Error : QUEUE is empty!!");
        return;
    }
    if(front==rear)
        front = rear = NULL;

    else
        front = front->next;

    free(temp);
}

// to have the value from the front of the queue but not delete it
int see_front(){
    int a;
    a = front->data;
    return(a);
}

// Generates random values of 1 or 2 for those 3 variables
void gen_en(){
    srand(time(NULL));
    gen_enem1 = (rand() % 3) + 1;
    gen_enem2 = (rand() % 3) + 1;
    gen_enem3 = (rand() % 3) + 1;
}

void safe_txt(const char *filename, int valor) {
    FILE *archivo;

    archivo = fopen(filename, "w");
    if (archivo == NULL) {
        perror("Error on save");
        return;
    }
    fprintf(archivo, "%d\n", valor);
    fclose(archivo);
}

int read_txt(const char *filemame) {
    FILE *archivo;
    int valor;

    archivo = fopen(filename, "r");

    if (archivo == NULL) {
        perror("Error on read");
        return -1;
    }
    if (fscanf(archivo, "%d", &valor) != 1) {
        printf("Error on read");
        fclose(archivo);
        return -1;
    }
    fclose(archivo);
    return valor;
}
