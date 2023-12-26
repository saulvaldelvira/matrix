#ifndef CONSOLE_H
#define CONSOLE_H

#define cursor_on()	wprintf(L"\x1b[?25h")
#define cursor_off()	wprintf(L"\x1b[?25l")

#define screen_clear()          wprintf(L"\x1b[2J")

#define console_save_state()    wprintf(L"\x1b[?1049h")
#define console_restore_state() wprintf(L"\x1b[?1049l")

#define cursor_home()		wprintf(L"\x1b[H")
#define cursor_move(x,y) 	wprintf(L"\x1b[%d;%dH",(y)+1,(x)+1)
#define cursor_up(n) 		wprintf(L"\x1b[%dA", n)
#define cursor_down(n) 		wprintf(L"\x1b[%dB", n)
#define cursor_right(n) 	wprintf(L"\x1b[%dC", n)
#define cursor_left(n) 		wprintf(L"\x1b[%dD", n)

#define buffer_save()           wprintf(L"\x1b[?47h")
#define buffer_show()           wprintf(L"\x1b[?47l")

#define cursor_request_pos()	wprintf(L"\x1b[6n")
#define cursor_push()		wprintf(L"\x1b[7")
#define cursor_pop()		wprintf(L"\x1b[8")

#define console_setcolor256_fg(id) wprintf(L"\x1b[38;5;%dm",id)
#define console_setcolor256_bg(id) wprintf(L"\x1b[48;5;%dm",id)
#define console_put(wchar) wprintf(L"%lc\x1b[1D",wchar)
#define console_put_in(wchar,x,y) wprintf(L"\x1b[%d;%dH%lc",(y)+1,(x)+1, wchar)
#define console_print_in(wstr,x,y) wprintf(L"\x1b[%d;%dH%ls",(y)+1,(x)+1, wstr)
#define console_printcolor(color, wstr, ...) wprintf(color wstr Color_Reset __VA_ARGS__)

#endif // CONSOLE_H
