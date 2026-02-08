#ifndef BROWSER_H
#define BROWSER_H

typedef struct HistoryNode {
    char url[256];
    struct HistoryNode *prev, *next;
} HistoryNode;

typedef struct Tab {
    int id;
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

void tab_visit(Tab *t, const char *url);
void tab_delete_entry(Tab *t, const char *url);

void tab_go_back(Tab *t);
void tab_go_forward(Tab *t);

void tab_rename_url(Tab *t, const char *old_url, const char *new_url);

void tm_decrement_ids(Tab *t);


#endif
