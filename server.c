#include "browser.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <errno.h>

#define PORT 3001
#define STATE_FILE "browser_state.json"
#define RESP(buf) send(sock, buf, strlen(buf), 0)

TabManager *tm;


void save_state(void);
void load_state(void);


void ok_json(int sock, const char *json) {
    char hdr[256];
    snprintf(hdr, sizeof hdr,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %zu\r\n\r\n",
        strlen(json));
    send(sock, hdr, strlen(hdr), 0);
    send(sock, json, strlen(json), 0);
}


json_object *historynode_to_json_obj(const HistoryNode *n) {
    json_object *node = json_object_new_object();
    json_object_object_add(node, "url", json_object_new_string(n->url));
    return node;
}

json_object *tab_to_json_obj(const Tab *t) {
    json_object *tab = json_object_new_object();
    json_object_object_add(tab, "id", json_object_new_int(t->id));
    json_object_object_add(tab, "current",
        t->current ? historynode_to_json_obj(t->current) : NULL);

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

char *tm_to_json_str(void) {
    json_object *obj = tm_to_json_obj(tm);
    char *out = strdup(json_object_to_json_string(obj));
    json_object_put(obj);
    return out;
}


Tab *find_tab(int id) {
    for (Tab *t = tm->first; t; t = t->next)
        if (t->id == id) return t;
    return NULL;
}


void respond_tab(int sock, int id) {
    Tab *tab = find_tab(id);
    if (tab) {
        char *json = tab_to_json_str(tab);
        ok_json(sock, json);
        free(json);
        save_state();
    } else {
        RESP("HTTP/1.1 404 Not Found\r\n\r\n");
    }
}


void respond_all_tabs(int sock) {
    char *json = tm_to_json_str();
    ok_json(sock, json);
    free(json);
}


void save_state(void) {
    json_object *root = tm_to_json_obj(tm);
    FILE *fp = fopen(STATE_FILE, "w");
    if (!fp) { perror("Failed to open state file for writing"); }
    else {
        fputs(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY), fp);
        fclose(fp);
    }
    json_object_put(root);
}

void load_state(void) {
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

        json_object *history_obj;
        if (json_object_object_get_ex(tab_obj, "history", &history_obj)) {
            int hlen = json_object_array_length(history_obj);
            for (int j = 0; j < hlen; j++) {
                json_object *node_obj = json_object_array_get_idx(history_obj, j);
                json_object *url_obj;
                if (json_object_object_get_ex(node_obj, "url", &url_obj))
                    tab_visit(tab, json_object_get_string(url_obj));
            }
        }

        json_object *current_obj;
        if (json_object_object_get_ex(tab_obj, "current", &current_obj) &&
            current_obj &&
            json_object_get_type(current_obj) == json_type_object) {
            json_object *url_obj;
            if (json_object_object_get_ex(current_obj, "url", &url_obj)) {
                const char *cur_url = json_object_get_string(url_obj);
                for (HistoryNode *h = tab->head; h; h = h->next)
                    if (strcmp(h->url, cur_url) == 0) { tab->current = h; break; }
            }
        }
    }

    json_object_put(root);
}


void handle_request(int sock) {
    char buf[2048] = {0};
    if (read(sock, buf, sizeof buf - 1) <= 0) return;

    
    if (strncmp(buf, "OPTIONS ", 8) == 0) {
        RESP("HTTP/1.1 204 No Content\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Content-Length: 0\r\n\r\n");
        return;
    }

    
    if (strncmp(buf, "POST /tab/", 10) == 0 && strstr(buf, "/navigate")) {
        int id; sscanf(buf, "POST /tab/%d/navigate", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char direction[16] = {0};
            sscanf(strstr(body, "\"direction\":"), "\"direction\":\"%15[^\"]\"", direction);
            Tab *tab = find_tab(id);
            if (tab) {
                if      (strcmp(direction, "back")    == 0) tab_go_back(tab);
                else if (strcmp(direction, "forward") == 0) tab_go_forward(tab);
            }
        }
        respond_tab(sock, id);
        return;
    }

    
    if (strncmp(buf, "DELETE /tab/", 12) == 0 && strstr(buf, "/entry")) {
        int id; sscanf(buf, "DELETE /tab/%d/entry", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char url[256] = {0};
            sscanf(strstr(body, "\"url\":"), "\"url\":\"%255[^\"]\"", url);
            Tab *tab = find_tab(id);
            if (tab) tab_delete_entry(tab, url);
        }
        respond_tab(sock, id);
        return;
    }

    
    if (strncmp(buf, "DELETE /tab/", 12) == 0) {
        int id; sscanf(buf, "DELETE /tab/%d", &id);
        tm_close_tab(tm, id);
        respond_all_tabs(sock);
        save_state();
        return;
    }

    
    if (strncmp(buf, "POST /tabs", 10) == 0) {
        tm_new_tab(tm);
        respond_all_tabs(sock);
        save_state();
        return;
    }

    
    if (strncmp(buf, "PUT /tab/", 9) == 0 && strstr(buf, "/entry")) {
        int id; sscanf(buf, "PUT /tab/%d/entry", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (!body) { RESP("HTTP/1.1 400 Bad Request\r\n\r\n"); return; }
        body += 4;
        char old_url[256] = {0}, new_url[256] = {0};
        sscanf(strstr(body, "\"url\":"),    "\"url\":\"%255[^\"]\"",    old_url);
        sscanf(strstr(body, "\"newUrl\":"), "\"newUrl\":\"%255[^\"]\"", new_url);
        Tab *tab = find_tab(id);
        if (tab) tab_rename_url(tab, old_url, new_url);
        respond_tab(sock, id);
        return;
    }

    
    if (strncmp(buf, "GET /tabs", 9) == 0) {
        respond_all_tabs(sock);
        return;
    }

    
    if (strncmp(buf, "POST /tab/", 10) == 0 && strstr(buf, "/visit")) {
        int id; sscanf(buf, "POST /tab/%d/visit", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char url[256] = {0};
            sscanf(strstr(body, "\"url\":"), "\"url\":\"%255[^\"]\"", url);
            Tab *tab = find_tab(id);
            if (tab) tab_visit(tab, url);
        }
        respond_tab(sock, id);
        return;
    }

    RESP("HTTP/1.1 404 Not Found\r\n\r\n");
}


int main(void) {
    tm = tm_init();
    load_state();
    if (!tm->first) tm_new_tab(tm);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(server, (struct sockaddr *)&addr, sizeof addr);
    listen(server, 10);
    printf("Listening on http://localhost:%d\n", PORT);

    while (1) {
        int sock = accept(server, NULL, NULL);
        if (sock >= 0) { handle_request(sock); close(sock); }
    }
}