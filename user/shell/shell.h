/* user/shell/shell.h — Luna OS Tactical Shell */
#ifndef LUNA_SHELL_H
#define LUNA_SHELL_H

#include "../../include/types.h"

void shell_init(void);
void shell_run(void);
void shell_on_keyboard_event(char c);

#endif /* LUNA_SHELL_H */