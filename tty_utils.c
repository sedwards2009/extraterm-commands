#include <stdio.h>
#include <unistd.h>
#include <termios.h>

struct termios old_tty_settings;

void restore_tty() {
    tcsetattr(STDIN_FILENO, TCSADRAIN, &old_tty_settings);
    fflush(stderr);
}

void turn_off_echo() {
    /* Turn off echo on the tty. */
    if (!isatty(STDIN_FILENO)) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &old_tty_settings) == -1) {
        perror("tcgetattr");
        return;
    }

    struct termios new_tty_settings;
    tcgetattr(STDIN_FILENO, &new_tty_settings);
    new_tty_settings.c_lflag = new_tty_settings.c_lflag & ~ECHO;

    tcsetattr(STDIN_FILENO, TCSADRAIN, &new_tty_settings);

    /* Set up a hook to restore the tty settings at exit. */
    atexit(restore_tty);
}
