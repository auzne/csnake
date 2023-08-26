#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define SNAKE_HEAD 1
#define SNAKE_BODY 2

typedef struct {
    int x;
    int y;
} position;

typedef struct __snake_part {
    position pos;
    struct __snake_part* next;
} snake_part;

typedef enum {
    SNAKE_IS_FINE,
    SNAKE_IS_DEAD,
    SNAKE_HAS_EATEN,
} snake_situation;

typedef void (*print_snake_func)(snake_part*);
typedef void (*remove_last_func)(position);

snake_part* createSnake(int size);
snake_part* increaseSnake(snake_part* snake, position pos);
position createFruit(snake_part* snake);
void printScore(int score);
void printAscii(snake_part* snake);
void printColor(snake_part* snake);
void printFruit(position fruit);
void removeChar(position pos);
void removeColor(position pos);
snake_situation parseMove(snake_part* snake, position fruit, int key, remove_last_func removeLast);
snake_situation moveSnake(snake_part* snake, position fruit, position inc, remove_last_func removeLast);
void snakeDied(void);
void cleanSnake(snake_part* snake);

int main(void) {
    srand(time(NULL));
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    timeout(300);

    print_snake_func printSnake;
    remove_last_func removeLast;
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(SNAKE_HEAD, COLOR_RED, COLOR_WHITE);
        init_pair(SNAKE_BODY, COLOR_GREEN, COLOR_WHITE);
        printSnake = printColor;
        removeLast = removeColor;
    } else {
        printSnake = printAscii;
        removeLast = removeChar;
    }

    box(stdscr, 0, 0);
    mvprintw(0, 1, "Snake");
    mvprintw(0, 8, "Score: ");
    snake_part* snake = createSnake(3);
    position fruit = createFruit(snake);
    
    int score = 0;
    printScore(score);
    printFruit(fruit);

    int key = 0;
    do {
        snake_situation sit = parseMove(snake, fruit, key, removeLast);
        if (sit == SNAKE_IS_DEAD) break;
        else if (sit == SNAKE_HAS_EATEN) {
            ++score;
            fruit = createFruit(snake);

            printScore(score);
            printFruit(fruit);
        }
        printSnake(snake);
        key = getch();
    } while (key != 'q');

    snakeDied();
    cleanSnake(snake);
    endwin();

    return 0;
}

snake_part* createSnake(int size) {
    position middle = {
        (getmaxx(stdscr) / 2),
        (getmaxy(stdscr) / 2)
    };

    /* head */
    snake_part* snake = malloc(sizeof(snake_part));
    snake->pos.x = middle.x;
    snake->pos.y = middle.y;
    snake->next = NULL;

    /* body */
    snake_part* last = snake;
    for (int count = 1; count < size; ++count) {
        last = increaseSnake(last, (position) { middle.x - count, middle.y });
    }
    return snake;
}

snake_part* increaseSnake(snake_part* snake, position pos) {
    snake_part* last = snake;
    while (last->next != NULL) last = last->next;

    snake_part* curr = malloc(sizeof(snake_part));
    curr->pos.x = pos.x;
    curr->pos.y = pos.y;
    curr->next = NULL;
    last->next = curr;

    return curr;
}

position createFruit(snake_part* snake) {
    position max = {
        getmaxx(stdscr) - 3,
        getmaxy(stdscr) - 3
    };
    position pos = {
        (rand() % max.x) + 1,
        (rand() % max.y) + 1
    };

    snake_part* curr = snake;
    while (curr != NULL) {
        while (pos.x == curr->pos.x && pos.y == curr->pos.y) {
            pos.x = (rand() % max.x) + 1;
            pos.y = (rand() % max.y) + 1;
        }
        curr = curr->next;
    }

    return pos;
}

void printScore(int score) {
    mvprintw(0, 15, "%d", score);
}

void printAscii(snake_part* snake) {
    mvaddch(snake->pos.y, snake->pos.x, '@');
    for (snake_part* part = snake->next; part != NULL; part = part->next) {
        mvaddch(part->pos.y, part->pos.x, '#');
    }
}

void printColor(snake_part* snake) {
    mvchgat(snake->pos.y, snake->pos.x, 1, A_REVERSE, SNAKE_HEAD, NULL);
    for (snake_part* part = snake->next; part != NULL; part = part->next) {
        mvchgat(part->pos.y, part->pos.x, 1, A_REVERSE, SNAKE_BODY, NULL);
    }
}

void printFruit(position fruit) {
    mvaddch(fruit.y, fruit.x, '*');
}

void removeChar(position pos) {
    mvaddch(pos.y, pos.x, ' ');
}

void removeColor(position pos) {
    mvchgat(pos.y, pos.x, 1, A_NORMAL, 0, NULL);
}

snake_situation parseMove(snake_part* snake, position fruit, int key, remove_last_func removeLast) {
    static int last = KEY_RIGHT;
    switch (key) {
        case KEY_UP:
            if (last == KEY_DOWN) return parseMove(snake, fruit, last, removeLast);
            last = key;
            return moveSnake(snake, fruit, (position) { 0, -1 }, removeLast);
        case KEY_RIGHT:
            if (last == KEY_LEFT) return parseMove(snake, fruit, last, removeLast);
            last = key;
            return moveSnake(snake, fruit, (position) { 1, 0 }, removeLast);
        case KEY_DOWN:
            if (last == KEY_UP) return parseMove(snake, fruit, last, removeLast);
            last = key;
            return moveSnake(snake, fruit, (position) { 0, 1 }, removeLast);
        case KEY_LEFT:
            if (last == KEY_RIGHT) return parseMove(snake, fruit, last, removeLast);
            last = key;
            return moveSnake(snake, fruit, (position) { -1, 0 }, removeLast);
        default:
            return parseMove(snake, fruit, last, removeLast);
    }
}

snake_situation moveSnake(snake_part* snake, position fruit, position inc, remove_last_func removeLast) {
    position max =  {
        getmaxx(stdscr) - 1,
        getmaxy(stdscr) - 1
    };
    position last = {
        snake->pos.x,
        snake->pos.y
    };

    snake->pos.x += inc.x;
    snake->pos.y += inc.y;

    if (((snake->pos.x >= max.x) || (snake->pos.x < 1)) ||
        ((snake->pos.y >= max.y) || (snake->pos.y < 1))) return SNAKE_IS_DEAD; /* snake hit the border */
    for (snake_part* curr = snake->next; curr != NULL; curr = curr->next) {
        position temp = {
            curr->pos.x,
            curr->pos.y
        };
        
        curr->pos.x = last.x;
        curr->pos.y = last.y;

        last.x = temp.x;
        last.y = temp.y;
        if (snake->pos.x == last.x && snake->pos.y == last.y) return SNAKE_IS_DEAD; /* snake hit itself */
    }
    if (snake->pos.x == fruit.x && snake->pos.y == fruit.y) {
        increaseSnake(snake, last);
        removeChar(fruit);
        return SNAKE_HAS_EATEN;
    }
    removeLast(last);
    return SNAKE_IS_FINE; /* everything went fine */
}

void snakeDied(void) {
    position pos = {
        (getmaxx(stdscr) / 2) - 4,
        (getmaxy(stdscr) / 2)
    };

    mvprintw(pos.y, pos.x, "you died");
    mvchgat(pos.y, pos.x, 8, A_UNDERLINE | A_REVERSE, 0, NULL);

    timeout(-1);
    getch();
}

void cleanSnake(snake_part* snake) {
    for (snake_part* curr = snake; curr != NULL;) {
        snake_part* next = curr->next;
        free(curr);
        curr = next;
    }
}
