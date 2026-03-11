#ifndef STATE_H
#define STATE_H

#include "../browser/browser.h"

#define STATE_FILE "browser_state.json"


void save_state(const TabManager *tm);

void load_state(TabManager *tm);

#endif