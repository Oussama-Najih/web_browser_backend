#include <stdlib.h>
#include <string.h>
#include "json-serialize.h"
#include "../browser/browser.h"
#include <json-c/json.h>

json_object *historynode_to_json_obj(const HistoryNode *n) {
    json_object *node = json_object_new_object();
    json_object_object_add(node, "url", json_object_new_string(n->url));
    return node;
}

json_object *tab_to_json_obj(const Tab *t) {
    json_object *tab = json_object_new_object();
    json_object_object_add(tab, "id", json_object_new_int(t->id));
    json_object_object_add(tab, "currentIndex",json_object_new_int(t->currentIndex));

    json_object *history = json_object_new_array();
    for (HistoryNode *h = t->head; h; h = h->next)
        json_object_array_add(history, historynode_to_json_obj(h));
    json_object_object_add(tab, "history", history);

    return tab;
}

json_object *tm_to_json_obj(const TabManager *tm) {
    json_object *tabs = json_object_new_array();
    for (const Tab *t = tm->first; t; t = t->next)
        json_object_array_add(tabs, tab_to_json_obj(t));
    return tabs;
}


char *tab_to_json_str(const Tab *t) {
    json_object *obj = tab_to_json_obj(t);
    char *out = strdup(json_object_to_json_string(obj));
    json_object_put(obj);
    return out;
}

char *tm_to_json_str(const TabManager *tm) {
    json_object *obj = tm_to_json_obj(tm);
    char *out = strdup(json_object_to_json_string(obj));
    json_object_put(obj);
    return out;
}
