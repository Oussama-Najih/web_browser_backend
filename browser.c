#include "browser.h"
#include <stdlib.h>
#include <string.h>


HistoryNode *make_node(const char *url) {
    HistoryNode *n = calloc(1, sizeof(HistoryNode));
    strncpy(n->url, url, sizeof n->url - 1);
    return n;
}


TabManager *tm_init() {
    TabManager *tm = calloc(1, sizeof *tm);
    tm->nextId = 1;
    return tm;
}


void tm_free(TabManager *tm) {
    Tab *currentTab = tm->first;
    while (currentTab) {
        Tab *nextTab = currentTab->next;

        HistoryNode *node = currentTab->head;
        while (node) {
            HistoryNode *nextNode = node->next;
            free(node);
            node = nextNode;
        }

        free(currentTab);
        currentTab = nextTab;
    }

    free(tm);
}

Tab *tm_new_tab(TabManager *tm) {
    Tab *t = calloc(1, sizeof *t);
    t->id = tm->nextId;
    tm->nextId ++;
    Tab* tmp = tm->first;
    if (!tmp){
        tm->first = t;
    }
    else
    {
        while (tmp->next)
        {
            tmp=tmp->next;
        }
        tmp->next = t;
    }
    return t;
}

void tm_close_tab(TabManager *tm, int id) {
    Tab **tmp = &tm->first;
    while (*tmp && (*tmp)->id != id) tmp = &(*tmp)->next;
    if (!*tmp) return;

    tm_decrement_ids((*tmp)->next);
    tm->nextId --;

    Tab *toDelete = *tmp; 
    *tmp = toDelete->next;

    HistoryNode *n = toDelete->head;
    while (n) {
        HistoryNode *next = n->next;
        free(n);
        n = next;
    }
    free(toDelete);
}

void tm_decrement_ids(Tab *newer_tab) {
        while (newer_tab)
        {
            newer_tab->id--;
            newer_tab = newer_tab->next;
        }

}

void tab_visit(Tab *t, const char *url) {
    if (t->current && t->current->next) {
        HistoryNode *n = t->current->next;
        while (n) {
            HistoryNode *next = n->next;
            free(n);
            n = next;
        }
        t->current->next = NULL;
        t->tail = t->current;
    }
    HistoryNode *n = make_node(url);
    if (!t->head) t->head = n;
    n->prev = t->tail;
    if (t->tail) t->tail->next = n;
    t->tail = n;
    t->current = n;
}

void tab_delete_entry(Tab *t, const char *url) {
    HistoryNode *current = t->head;
    while (current) {
        if (strcmp(current->url, url) == 0) {
            if (current->prev) current->prev->next = current->next;
            if (current->next) current->next->prev = current->prev;
            if (t->head == current) t->head = current->next;
            if (t->tail == current) t->tail = current->prev;
            if (t->current == current)
                t->current = current->next ? current->next : current->prev;
            free(current);
            return;
        }
        current = current->next;
    }
}


void tab_go_back(Tab *t) {
    if (t->current && t->current->prev)
        t->current = t->current->prev;
}


void tab_go_forward(Tab *t) {
    if (t->current && t->current->next)
        t->current = t->current->next;
}


void tab_rename_url(Tab *t, const char *old_url, const char *new_url) {
    for (HistoryNode *n = t->head; n; n = n->next) {
        if (strcmp(n->url, old_url) == 0) {
            strncpy(n->url, new_url, sizeof n->url - 1);
            n->url[sizeof n->url - 1] = '\0'; 
            return;
        }
    }
}
