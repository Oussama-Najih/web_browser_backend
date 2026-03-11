#ifndef BROWSER_H
#define BROWSER_H

typedef struct HistoryNode {
    char url[256];
    struct HistoryNode *prev, *next;
} HistoryNode;

typedef struct Tab {
    int id;
    int currentIndex;
    HistoryNode *head, *tail, *current;
    struct Tab *next;
} Tab;

typedef struct TabManager {
    Tab *first;
    int nextId;
} TabManager;

TabManager *tm_init();
void tm_free(TabManager *tm);

Tab *tm_new_tab(TabManager *tm);
void tm_close_tab(TabManager *tm, int id);

HistoryNode *tab_visit(Tab *t, const char *url,int onLoad);

void tab_go_back(Tab *t);
void tab_go_forward(Tab *t);


void tm_decrement_ids(Tab *t);

Tab *find_tab(TabManager *tm, int id);

#endif
