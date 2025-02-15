/*
 * Matrix rainfall animation
 * Author: Saúl Valdelvira (2023)
 */
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE
#include <wchar.h>
#include <stdio.h>
#include <wctype.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <stdbool.h>
#include "console.h"

#if __has_include(<poll.h>)
#include <poll.h>
#endif

#if __has_include(<termios.h>)
#include <termios.h>
#endif

#ifdef __unix__
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

struct stream {
        wchar_t *str;
        int str_size;
        int length;
        int x;
        double y;
        int speed;
};

struct stream *streams = NULL;
int n_streams;

int screen_width = 0;
int screen_height = 0;

struct gliph {
        wchar_t character;
        short color;
        bool need_update;
};

struct gliph *gliphs;
#define gliph_at(x,y) gliphs[(y) * screen_width + (x)]

static inline
void gliph_set(int x, int y, wchar_t character, short color){
        struct gliph *gliph = &gliph_at(x,y);
        gliph->character = character;
        gliph->color = color;
        gliph->need_update = true;
}

#define abs(n) ((n) < 0 ? -(n) : (n))
#define rand_range(min,max) (rand() % ((max) - (min)) + (min))

#define UNICODE_CHAR     0x30A1
#define UNICODE_RANGE     89

#define ASCII_CHAR      0x23
#define ASCII_RANGE      59

#define MIN_STREAM_LENGTH 4
#define MAX_STREAM_LENGTH 12

#define MIN_STREAM_SPEED 5
#define MAX_STREAM_SPEED 25

void check_screen_size(void);
void update(double elapsed_time);
void resize(int width, int hwight);
void cleanup(int);
void rand_stream(struct stream *str);
void sleep_for(unsigned long millis);
void help(void);
void die(const char *msg);
static inline void* xmalloc(size_t nbytes);
static inline void* xcalloc(size_t n, size_t s);
static inline void* xrealloc(void *old, size_t nbytes);

static inline double get_time_ms(void){
#ifdef __unix__
        static struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#elif _WIN32
        return GetTickCount() * 1.0;
#endif
}

struct {
        bool full_width_unicode;
        wchar_t seed_char;
        int range;
        wchar_t *stream;
        int stream_length;
        int nstreams;
        wchar_t *message;
        int message_length;
        bool *message_shown;
        double message_delay;
        double last_message;
        int message_num_shown;
#if __has_include(<termios.h>)
        struct termios original_term;
#endif
} conf = {
        .full_width_unicode = false,
        .seed_char = UNICODE_CHAR,
        .range = UNICODE_RANGE,
        .message_delay = 700.0
};

#ifdef _WIN32
HANDLE win_console;
void win_handle_signal(DWORD signal);
void win_setup_console(void);
#endif

static wchar_t* readwstr(const char* src, int *len){
        size_t length = mbstowcs(NULL, src, 0); ;
        wchar_t *wstr = xmalloc((length + 1) * sizeof(wchar_t));
        *len = mbstowcs(wstr, src, length + 1);
        return wstr;
}

int main(int argc, char *argv[]){
        setlocale(LC_CTYPE, "");
        for (int i = 1; i < argc; i++){
                if (argv[i][0] == '-' && argv[i][1] == '-'){
                        if (strcmp(&argv[i][2], "ascii") == 0){
                                conf.seed_char = ASCII_CHAR;
                                conf.range = ASCII_RANGE;
                        }
                        else if (strcmp(&argv[i][2], "char-seed") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --char-seed\n");
                                        exit(1);
                                }
                                sscanf(argv[++i], "%x", &conf.seed_char);
                        }
                        else if(strcmp(&argv[i][2], "range") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --range\n");
                                        exit(1);
                                }
                                sscanf(argv[++i], "%u", &conf.range);
                        }
                        else if(strcmp(&argv[i][2], "stream") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --stream\n");
                                        exit(1);
                                }
                                conf.stream = readwstr(argv[++i], &conf.stream_length);
                        }
                        else if(strcmp(&argv[i][2], "message") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --message\n");
                                        exit(1);
                                }
                                conf.message = readwstr(argv[++i], &conf.message_length);
                                conf.message_shown = xcalloc(conf.message_length, sizeof(bool));
                        }
                        else if (strcmp(&argv[i][2], "message-delay") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --message-delay\n");
                                        exit(1);
                                }
                                conf.message_delay = atof(argv[++i]) * 1000;
                        }
                        else if (strcmp(&argv[i][2], "number-of-streams") == 0){
                                if (i == argc - 1){
                                        fprintf(stderr, "Missing argument to --number-of-streams\n");
                                        exit(1);
                                }
                                conf.nstreams = atoi(argv[++i]);
                        }
                        else if(strcmp(&argv[i][2], "help") == 0){
                                help();
                        }
                        else{
                                fprintf(stderr, "Invalid argument: %s\nUse --help to get a list of possible arguments\n", &argv[i][2]);
                                exit(1);
                        }
                }
        }

        if (wcwidth(conf.seed_char) > 1)
                conf.full_width_unicode = true;

#if __has_include(<poll.h>)
        /* Read stdin and use it as a source of characters for the streams.
           This allows us to pipe a file, or the ouput of a command */
        struct pollfd fds[] = {{0,POLLIN}};
        int p = poll(fds, 1, 5);
        if (p > 0){
                conf.full_width_unicode = false;
                size_t cap = 1024;
                conf.stream = xmalloc(cap * sizeof(wchar_t));
                conf.stream_length = 0;
                wint_t c;
                while ((c = getwchar()) != WEOF){
                        if (!iswprint(c) || c == ' ') continue;
                        if (wcwidth(c) > 1)
                                conf.full_width_unicode = true;
                        conf.stream[conf.stream_length++] = c;
                        if ((size_t)conf.stream_length == cap){
                                cap *= 1.5;
                                conf.stream = xrealloc(conf.stream, cap * sizeof(wchar_t));
                        }
                }
        }
#endif
        if (conf.stream && conf.stream_length == 0){
                conf.stream[0] = ' ';
                conf.stream_length = 1;
        }

#if __has_include(<termios.h>)
        tcgetattr(STDIN_FILENO, &conf.original_term);
        struct termios term = conf.original_term;
        term.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
#endif

        srand(time(NULL));
        check_screen_size();

#ifdef __unix__
        signal(SIGINT, &cleanup);
#elif _WIN32
        SetConsoleCtrlHandler((PHANDLER_ROUTINE) win_handle_signal, TRUE);
        win_setup_console();
#endif

        console_save_state();
        buffer_save();
        cursor_off();
        console_setcolor256_bg(232);

        double last_time = get_time_ms();
        double elapsed_time;

        screen_clear();

#ifdef __unix__
        const struct timespec ts = {
                .tv_sec = 0,
                .tv_nsec = 10000000 // 10ms
        };
#endif
        for (;;){
                elapsed_time = get_time_ms() - last_time;
                last_time = get_time_ms();

                check_screen_size();
                update(elapsed_time);
                for (int i = 0; i < screen_height; i++){
                        for (int j = 0; j < screen_width; j++){
                                if (gliph_at(j,i).need_update){
                                        console_setcolor256_fg(gliph_at(j,i).color);
                                        console_put_in(gliph_at(j,i).character, j, i);
                                        gliph_at(j,i).need_update = false;
                                }
                        }
                }
                fflush(stdout);
#ifdef __unix__
                nanosleep(&ts, NULL);
#elif _WIN32
                Sleep(10);
#endif
        }
        return 0;
}

void update(double elapsed_time){
        for (int i = 0; i < n_streams; i++){
                struct stream *stream = &streams[i];

                // Remove stream. This is done to avoid having to clear the whole
                // screen each frame. The streams clean after themselves.
                for (int j = 0; j < stream->length && stream->y + j < screen_height; j++){
                        gliph_set(stream->x, ((int)stream->y) + j, L' ', 0);
                }

                stream->y += stream->speed * elapsed_time / 1000 * (screen_height / 55.0);
                if (stream->y >= screen_height){
                        rand_stream(stream);
                }

                for (int j = 0; j < stream->length && stream->y + j < screen_height; j++){
                        short col;
                        // TODO: more shades of green
                        if (stream->speed <= MIN_STREAM_SPEED + (MAX_STREAM_SPEED - MIN_STREAM_SPEED) * 0.5){
                                col = 28;
                        }else{
                                col = 40;
                        }
                        if (j == stream->length - 1){
                                col = 255;
                        }else if (j >= stream->length - 3 && j < stream->length -1){
                                col = 249;
                        }

                        // A little trick to make the characters change positions
                        int char_index = abs(j + (int)stream->y) % stream->length;;
                        gliph_set(stream->x,((int)stream->y) + j, stream->str[char_index], col);
                }
        }
        if (conf.message){
                if (get_time_ms() >= conf.last_message + conf.message_delay && conf.message_num_shown < conf.message_length){
                        int pos;
                        do {
                                pos = rand_range(0, conf.message_length);
                        }while (conf.message_shown[pos]);
                        conf.message_shown[pos] = true;
                        conf.last_message = get_time_ms();
                        conf.message_num_shown++;
                }
                int midy = screen_height / 2 - 1;
                int startx = screen_width / 2 - conf.message_length / 2;

                for (int i = 0; i < conf.message_length; i++){
                        if (conf.message_shown[i])
                                gliph_set(startx + i, midy, conf.message[i], 255);
                }
        }
}

void rand_stream(struct stream *stream){
        stream->length = rand_range(MIN_STREAM_LENGTH, MAX_STREAM_LENGTH);
        if (!stream->str || stream->length > stream->str_size){
                stream->str = xrealloc(stream->str, stream->length * sizeof(wchar_t));
                stream->str_size = stream->length;
        }
        for (int i = 0; i < stream->length; i++){
                if (conf.stream == NULL)
                        stream->str[i] = rand() % conf.range + conf.seed_char;
                else
                        stream->str[i] = conf.stream[rand_range(0, conf.stream_length)];
        }
        if (conf.full_width_unicode){
                /* If we're displaying full width unicode characters, use
                   only pair values of x to avoid characters overlapping.
                   Also, leave a few columns free to the rigth of the
                   screen, to avoid overflow in some terminals */
                stream->x = rand_range(0, screen_width-2);
                if (stream->x % 2 != 0)
                        stream->x--;
        }else{
                stream->x = rand_range(0, screen_width);
        }
        stream->y = 0.0;
        stream->speed = rand_range(MIN_STREAM_SPEED, MAX_STREAM_SPEED);
}

void check_screen_size(void){
#ifdef __unix__
        struct winsize size;
        ioctl(1, TIOCGWINSZ, &size);
        int w = size.ws_col + 1;
        int h = size.ws_row + 1;
#elif _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        int columns, rows;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#endif
        if (w != screen_width || h != screen_height)
                resize(w, h);
}

void resize(int width, int height){
        fflush(stdout);
        screen_width = width;
        screen_height = height;
        size_t buffer_size = sizeof(wchar_t) * screen_width * screen_height * 500;
        setvbuf(stdout, NULL, _IOFBF, buffer_size);
        free(gliphs);
        gliphs = xcalloc(screen_height * screen_width, sizeof(struct gliph));
        if (streams){
                for (int i = 0; i < n_streams; i++)
                        free(streams[i].str);
                free(streams);
        }
        if (conf.nstreams == 0)
                n_streams = screen_width + (screen_width * 0.05);
        else
                n_streams = conf.nstreams;
        streams = xcalloc(n_streams, sizeof(struct stream));
        for (int i = 0; i < n_streams; i++)
                rand_stream(&streams[i]);
        if (conf.message){
                memset(conf.message_shown, 0, conf.message_length * sizeof(bool));
                conf.message_num_shown = 0;
        }
        screen_clear();
}

void cleanup(int signal){
        (void) signal;
        console_restore_state();
        cursor_on();
        buffer_show();
        fflush(stdout);
        if (streams){
                for (int i = 0; i < n_streams; i++)
                        free(streams[i].str);
                free(streams);
        }
#if __has_include(<termios.h>)
        tcsetattr(STDIN_FILENO, TCSANOW, &conf.original_term);
#endif
        exit(0);
}

void help(void){
        printf("Parameters:\n"
               "  --ascii: use ascii characters only.\n"
               "  --char-seed <hex-code>: uses the given char as the \"seed\".\n"
               "  --range <number>: Sets how many characters since the \"char seed\" to use for the stream generation.\n"
               "  --stream <string>: Use the given string as the stream.\n"
               "  --message <string>: Display the string as a message in the middle.\n"
               "  --message-delay <float>: Delay (in seconds) between 2 characters of the message.\n"
               "  --number-of-streams <number>: sets the number of streams on the screen\n"
               "  --help: display this help guide.\n");
        exit(0);
}

void die(const char *msg){
        fprintf(stderr, "ERROR: %s\n", msg);
        exit(1);
}

/// malloc wrappers ///////////////////////////////////////
static inline void* xmalloc(size_t nbytes){
        void *ptr = malloc(nbytes);
        if (!ptr) die("ran out of memory");
        return ptr;
}

static inline void* xcalloc(size_t n, size_t s){
        void *ptr = calloc(n,s);
        if (!ptr) die("ran out of memory");
        return ptr;
}

static inline void* xrealloc(void *old, size_t nbytes){
        void *ptr = realloc(old,nbytes);
        if (!ptr) die("ran out of memory");
        return ptr;
}
///////////////////////////////////////////////////////////

#ifdef _WIN32
void win_handle_signal(DWORD signal){
        if (signal == CTRL_C_EVENT)
                cleanup();
}

void win_setup_console(void){
        win_console = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        SetConsoleOutputCP(CP_UTF8);
        GetConsoleMode(win_console, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(win_console, mode);
}
#endif
