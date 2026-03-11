#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <json-c/json.h>
#include "state.h"
#include "../serialize/json-serialize.h"
#include "../browser/browser.h"


void save_state(const TabManager *tm) {
    json_object *root = tm_to_json_obj(tm);
    FILE *fp = fopen(STATE_FILE, "w");
    if (!fp) { perror("Failed to open state file for writing"); }
    else {
        fputs(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY), fp);
        fclose(fp);
    }
    json_object_put(root);
}

void load_state(TabManager *tm) {
    FILE *fp = fopen(STATE_FILE, "r");
    if (!fp) {
        if (errno != ENOENT) perror("Failed to open state file for reading");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) { perror("malloc"); fclose(fp); return; }
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    json_object *root = json_tokener_parse(buffer);
    free(buffer);
    if (!root) { fprintf(stderr, "Failed to parse state file\n"); return; }

    while (tm->first)
        tm_close_tab(tm, tm->first->id);

    int array_len = json_object_array_length(root);
    for (int i = 0; i < array_len; i++) {
        json_object *tab_obj = json_object_array_get_idx(root, i);
        Tab *tab = tm_new_tab(tm);

        json_object *id_obj;
        if (json_object_object_get_ex(tab_obj, "id", &id_obj))
            tab->id = json_object_get_int(id_obj);
            
        json_object *currentIndex;
        if (json_object_object_get_ex(tab_obj, "currentIndex", &currentIndex))
            tab->currentIndex = json_object_get_int(currentIndex);

        json_object *history_obj;
        if (json_object_object_get_ex(tab_obj, "history", &history_obj)) {
            int hlen = json_object_array_length(history_obj);
            int count = 0;
            HistoryNode *currentNode = NULL;
            for (int j = 0; j < hlen; j++) {
                json_object *node_obj = json_object_array_get_idx(history_obj, j);
                json_object *url_obj;
                if (json_object_object_get_ex(node_obj, "url", &url_obj)) {
                    HistoryNode *tmp = tab_visit(tab, json_object_get_string(url_obj), 1);
                    if (count == tab->currentIndex) {
                        currentNode = tmp;
                    }
                    count++;
                }
            }
            tab->current = currentNode;
        }

    }

    json_object_put(root);
}
