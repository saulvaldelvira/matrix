/**
 * Matrix rainfall animation
 * Author: Saúl Valdelvira (2023)
*/
#include <stdio.h>
#include <wchar.h>
#include "console.h"
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <stdbool.h>

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

static inline void gliph_set(int x, int y, wchar_t character, short color){
        struct gliph *gliph = &gliph_at(x,y);
        gliph->character = character;
        gliph->color = color;
        gliph->need_update = true;
}

#define abs(n) ((n) < 0 ? -(n) : (n))
#define rand_range(min,max) (rand() % ((max) - (min)) + (min))

// Default values
#define UNICODE_CHAR     0x30A1
#define UNICODE_STEP     89

#define ASCII_CHAR      0x23
#define ASCII_STEP      59

#define MIN_STREAM_LENGTH 4
#define MAX_STREAM_LENGTH 12

#define MIN_STREAM_SPEED 5
#define MAX_STREAM_SPEED 30

void check_screen_size();
void update(double elapsed_time);
void resize(int width, int hwight);
void cleanup();
void rand_stream(struct stream *str);
void sleep_for(unsigned long millis);
void help();

static inline double get_time(){
#ifdef __unix__
        static struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1.0 + tv.tv_usec / 1000000.0;
#elif _WIN32
        return GetTickCount() * 1.0 / 1000.0;
#endif
}

struct {
        bool unicode;
        wchar_t seed_char;
        int step;
        wchar_t *stream;
        int stream_length;
        int nstreams;
} config = {
        .unicode = true,
        .seed_char = UNICODE_CHAR,
        .step = UNICODE_STEP
};

#ifdef _WIN32
HANDLE win_console;
void win_handle_signal(DWORD signal);
void win_setup_console();
#endif

int main(int argc, char *argv[]){
        for (int i = 1; i < argc; i++){
                if (argv[i][0] == '-'){
                        if (argv[i][1] == '-'){
                                if (strcmp(&argv[i][2], "ascii") == 0){
                                        config.unicode = false;
                                        config.seed_char = ASCII_CHAR;
                                        config.step = ASCII_STEP;
                                }
                                else if (strcmp(&argv[i][2], "char-seed") == 0){
                                        if (i == argc - 1){
                                                fprintf(stderr, "Missing argument to --char-seed\n");
                                                exit(1);
                                        }
                                        sscanf(argv[++i], "%x", &config.seed_char);
                                }
                                else if(strcmp(&argv[i][2], "step") == 0){
                                        if (i == argc - 1){
                                                fprintf(stderr, "Missing argument to --char-seed\n");
                                                exit(1);
                                        }
                                        sscanf(argv[++i], "%u", &config.step);
                                }
                                else if(strcmp(&argv[i][2], "stream") == 0){
                                        if (i == argc - 1){
                                                fprintf(stderr, "Missing argument to --stream\n");
                                                exit(1);
                                        }
                                        size_t length = strlen(argv[++i]);
                                        config.stream_length = length;
                                        config.stream = malloc(length * sizeof(wchar_t));
                                        char *src = argv[i];
                                        wchar_t *dst = config.stream;
                                        while (*src != '\0'){
                                                *dst++ = (wchar_t) *src++;
                                        }
                                }
                                else if (strcmp(&argv[i][2], "number-of-streams") == 0){
                                        if (i == argc - 1){
                                                fprintf(stderr, "Missing argument to --number-of-streams\n");
                                                exit(1);
                                        }
                                        config.nstreams = atoi(argv[++i]);
                                }
                                else if(strcmp(&argv[i][2], "help") == 0){
                                        help();
                                }
                                else{
                                        fprintf(stderr, "Invalid argument: %s\nUse --help or -h to get a list of possible arguments\n", &argv[i][2]);
                                        exit(1);
                                }
                        }else{
                                switch (argv[i][1]){
                                case 'h':
                                        help();
                                        break;
                                }
                        }
                }
        }

        if (config.unicode){
                setlocale(LC_CTYPE, "en_US.UTF-8");
        }

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

        double last_time = get_time();
        double elapsed_time;

        screen_clear();

#ifdef __unix__
        struct timespec ts = {
                .tv_sec = 0,
                .tv_nsec = 10000000 // 10ms
        };
#endif
        for (;;){
                elapsed_time = get_time() - last_time;
                last_time = get_time();

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

                stream->y += stream->speed * elapsed_time * (screen_height / 55.0);
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
}

void rand_stream(struct stream *stream){
        stream->length = rand_range(MIN_STREAM_LENGTH, MAX_STREAM_LENGTH);
        if (config.stream != NULL && stream->length > config.stream_length){
                stream->length = config.stream_length;
        }
        if (!stream->str || stream->length > stream->str_size){
                free(stream->str);
                stream->str = malloc(stream->length * sizeof(wchar_t));
                stream->str_size = stream->length;
        }

        for (int i = 0; i < stream->length; i++){
                if (config.stream == NULL){
                        stream->str[i] = rand() % config.step + config.seed_char;
                }else{
                        stream->str[i] = config.stream[i];
                }
        }

        if (config.unicode){
                // If we're displaying unicode, use only pair values of x
                // to account for full width unicode characters.
                // Also, leave a few columns free to the rigth of the
                // screen, to avoid overflow in some terminals
                stream->x = rand_range(0, screen_width-3);
                if (stream->x % 2 != 0){
                        stream->x++;
                }
        }else{
                stream->x = rand_range(0, screen_width);
        }
        stream->y = 0.0;

        stream->speed = rand_range(MIN_STREAM_SPEED, MAX_STREAM_SPEED);
}

void check_screen_size(){
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
        if (w != screen_width || h != screen_height){
                resize(w, h);
        }
}

void resize(int width, int height){
        fflush(stdout);

        screen_width = width;
        screen_height = height;

        size_t buffer_size = sizeof(wchar_t) * screen_width * screen_height * 500;
        setvbuf(stdout, NULL, _IOFBF, buffer_size);

        free(gliphs);
        gliphs = calloc(screen_height * screen_width, sizeof(struct gliph));

        if (streams){
                for (int i = 0; i < n_streams; i++){
                        free(streams[i].str);
                }
                free(streams);
        }
        if (config.nstreams == 0){
                n_streams = screen_width + (screen_width * 0.3);
        }else{
                n_streams = config.nstreams;
        }
        streams = calloc(n_streams, sizeof(struct stream));
        for (int i = 0; i < n_streams; i++){
                rand_stream(&streams[i]);
        }

        screen_clear();
}

void cleanup(){
        console_restore_state();
        cursor_on();
        buffer_show();
        fflush(stdout);
        if (streams){
                for (int i = 0; i < n_streams; i++){
                        free(streams[i].str);
                }
                free(streams);
        }
        exit(0);
}

void help(){
        printf("Matrix Rain\n"
                "Parameters:\n"
                "  --ascii: use ascii characters only.\n"
                "  --char-seed <hex-code>: uses the given char as the \"seed\".\n"
                "  --step <number>: Sets how many characters since the \"char seed\" to use for the stream generation.\n"
                "  --stream <string>: use the given string as the stream.\n"
                "  --number-of-streams <number>: sets the number of streams on the screen\n"
                "  --help (or -h): display this help guide.\n");
        exit(0);
}

#ifdef _WIN32
void win_handle_signal(DWORD signal){
        if (signal == CTRL_C_EVENT){
                cleanup();
        }
}

void win_setup_console(){
        win_console = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        SetConsoleOutputCP(CP_UTF8);
        GetConsoleMode(win_console, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(win_console, mode);
}
#endif
