#include "browser.h"
#include <stdlib.h>
#include <stdio.h>
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
    t->currentIndex = -1;
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
    if (id==1)
    {
        return;
    }
    
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

HistoryNode *tab_visit(Tab *t, const char *url,int onLoad) {
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
    if (!onLoad)
    {
        t->currentIndex++;
    }
    return n;
}


void tab_go_back(Tab *t) {
    if (t->current && t->current->prev){
        t->current = t->current->prev;
        t->currentIndex--;
    }
}

void tab_go_forward(Tab *t) {
    if (t->current && t->current->next){
        t->current = t->current->next;
        t->currentIndex++;
    }
}


Tab *find_tab(TabManager *tm, int id) {
    for (Tab *t = tm->first; t; t = t->next)
        if (t->id == id) return t;
    return NULL;
}
