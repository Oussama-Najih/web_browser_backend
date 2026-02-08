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

 json_object *historynode_to_json_obj(const HistoryNode *n);
 json_object *tab_to_json_obj(const Tab *t);
 json_object *tm_to_json_obj(const TabManager *tm);
 void save_state();
 void load_state();

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

char *historynode_to_json(const HistoryNode *n) {
    char *out = malloc(512);
    snprintf(out, 512, "{\"url\":\"%s\"}", n->url);
    return out;
}

char *tab_to_json(const Tab *t) {
    char *out = malloc(4096);
    char *p = out;
    int rem = 4096;

    char *current_json = t->current ? historynode_to_json(t->current) : NULL;
    int n = snprintf(p, rem,
        "{\"id\":%d,\"current\":%s,\"history\":[",
        t->id,
        current_json ? current_json : "null");
    if (current_json) free(current_json);
    p += n; rem -= n;

    for (HistoryNode *h = t->head; h; h = h->next) {
        char *hjson = historynode_to_json(h);
        n = snprintf(p, rem, "%s%s", (h == t->head ? "" : ","), hjson);
        p += n; rem -= n;
        free(hjson);
    }
    snprintf(p, rem, "]}");
    return out;
}

char *tm_to_json(const TabManager *tm) {
    char *out = malloc(8192);
    char *p = out;
    int rem = 8192;

    int n = snprintf(p, rem, "[");
    p += n; rem -= n;

    for (const Tab *t = tm->first; t; t = t->next) {
        char *tjson = tab_to_json(t);
        n = snprintf(p, rem, "%s%s", (t == tm->first ? "" : ","), tjson);
        p += n; rem -= n;
        free(tjson);
    }
    snprintf(p, rem, "]");
    return out;
}

 json_object *historynode_to_json_obj(const HistoryNode *n) {
    json_object *node = json_object_new_object();
    json_object_object_add(node, "url", json_object_new_string(n->url));
    return node;
}

 json_object *tab_to_json_obj(const Tab *t) {
    json_object *tab = json_object_new_object();
    json_object_object_add(tab, "id", json_object_new_int(t->id));
    
    if (t->current) {
        json_object_object_add(tab, "current", historynode_to_json_obj(t->current));
    } else {
        json_object_object_add(tab, "current", NULL);
    }
    
    json_object *history = json_object_new_array();
    for (HistoryNode *h = t->head; h; h = h->next) {
        json_object_array_add(history, historynode_to_json_obj(h));
    }
    json_object_object_add(tab, "history", history);
    
    return tab;
}

 json_object *tm_to_json_obj(const TabManager *tm) {
    json_object *tabs = json_object_new_array();
    for (const Tab *t = tm->first; t; t = t->next) {
        json_object_array_add(tabs, tab_to_json_obj(t));
    }
    return tabs;
}

 void save_state() {
    json_object *root = tm_to_json_obj(tm);
    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    
    FILE *fp = fopen(STATE_FILE, "w");
    if (!fp) {
        perror("Failed to open state file for writing");
        json_object_put(root);
        return;
    }
    
    fputs(json_str, fp);
    fclose(fp);
    json_object_put(root);
}

 void load_state() {
    FILE *fp = fopen(STATE_FILE, "r");
    if (!fp) {
        if (errno != ENOENT) { 
            perror("Failed to open state file for reading");
        }
        return;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        perror("Failed to allocate memory for state file");
        fclose(fp);
        return;
    }
    
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);
    
    json_object *root = json_tokener_parse(buffer);
    free(buffer);
    
    if (!root) {
        fprintf(stderr, "Failed to parse state file\n");
        return;
    }
    
    while (tm->first) {
        tm_close_tab(tm, tm->first->id);
    }
    
    int array_len = json_object_array_length(root);
    for (int i = 0; i < array_len; i++) {
        json_object *tab_obj = json_object_array_get_idx(root, i);
        
        Tab *tab = tm_new_tab(tm);
        json_object *id_obj;
        if (json_object_object_get_ex(tab_obj, "id", &id_obj)) {
            tab->id = json_object_get_int(id_obj);
        }
        
        json_object *history_obj;
        if (json_object_object_get_ex(tab_obj, "history", &history_obj)) {
            int history_len = json_object_array_length(history_obj);
            for (int j = 0; j < history_len; j++) {
                json_object *node_obj = json_object_array_get_idx(history_obj, j);
                json_object *url_obj;
                if (json_object_object_get_ex(node_obj, "url", &url_obj)) {
                    const char *url = json_object_get_string(url_obj);
                    tab_visit(tab, url);
                }
            }
        }
        
        json_object *current_obj;
        if (json_object_object_get_ex(tab_obj, "current", &current_obj) && 
            current_obj && json_object_get_type(current_obj) == json_type_object) {
            json_object *current_url_obj;
            if (json_object_object_get_ex(current_obj, "url", &current_url_obj)) {
                const char *current_url = json_object_get_string(current_url_obj);
                for (HistoryNode *h = tab->head; h; h = h->next) {
                    if (strcmp(h->url, current_url) == 0) {
                        tab->current = h;
                        break;
                    }
                }
            }
        }
    }
    
    json_object_put(root);
}

 void handle_request(int sock) {
    char buf[2048] = {0};
    int r = read(sock, buf, sizeof buf - 1);
    if (r <= 0) return;

    if (strncmp(buf, "OPTIONS ", 8) == 0) {
        const char *res =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n\r\n";
        RESP(res);
        return;
    }

    else if (strncmp(buf, "POST /tab/", 10) == 0 && strstr(buf, "/navigate")) {
        int id;
        sscanf(buf, "POST /tab/%d/navigate", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char direction[16] = {0};
            sscanf(strstr(body, "\"direction\":"), "\"direction\":\"%15[^\"]\"", direction);
            for (Tab *t = tm->first; t; t = t->next)
                if (t->id == id) {
                    if (strcmp(direction, "back") == 0) tab_go_back(t);
                    else if (strcmp(direction, "forward") == 0) tab_go_forward(t);
                    break;
                }
            Tab *tab = tm->first;
            while (tab && tab->id != id) tab = tab->next;
            if (tab) {
                char *json = tab_to_json(tab);
                ok_json(sock, json);
                free(json);
                save_state(); 
            } else {
                RESP("HTTP/1.1 404 Not Found\r\n\r\n");
            }
        }
    }

    else if (strncmp(buf, "DELETE /tab/", 12) == 0 && strstr(buf, "/entry")) {
        int id;
        sscanf(buf, "DELETE /tab/%d/entry", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char url[256] = {0};
            sscanf(strstr(body, "\"url\":"), "\"url\":\"%255[^\"]\"", url);

            for (Tab *t = tm->first; t; t = t->next)
                if (t->id == id) { tab_delete_entry(t, url); break; }

            Tab *tab = tm->first;
            while (tab && tab->id != id) tab = tab->next;
            if (tab) {
                char *json = tab_to_json(tab);
                ok_json(sock, json);
                free(json);
                save_state(); 
            } else {
                RESP("HTTP/1.1 404 Not Found\r\n\r\n");
            }
        }
    }

    else if (strncmp(buf, "DELETE /tab/", 12) == 0 && !strstr(buf, "/entry")) {
        int id;
        sscanf(buf, "DELETE /tab/%d", &id);
        tm_close_tab(tm, id);
        char *json = tm_to_json(tm);
        ok_json(sock, json);
        free(json);
        save_state();
    }

    else if (strncmp(buf, "POST /tabs", 10) == 0) {
        tm_new_tab(tm);
        char *json = tm_to_json(tm);
        ok_json(sock, json);
        free(json);
        save_state();
    }
    
    else if (strncmp(buf, "PUT /tab/", 9) == 0 && strstr(buf, "/entry")) {
        int id;
        sscanf(buf, "PUT /tab/%d/entry", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char old_url[256] = {0}, new_url[256] = {0};
            sscanf(strstr(body, "\"url\":"), "\"url\":\"%255[^\"]\"", old_url);
            sscanf(strstr(body, "\"newUrl\":"), "\"newUrl\":\"%255[^\"]\"", new_url);
    
            for (Tab *t = tm->first; t; t = t->next)
                if (t->id == id) { tab_rename_url(t, old_url, new_url); break; }
    
            Tab *tab = tm->first;
            while (tab && tab->id != id) tab = tab->next;
    
            if (tab) {
                char *json = tab_to_json(tab);
                ok_json(sock, json);
                free(json);
                save_state();
            } else {
                RESP("HTTP/1.1 404 Not Found\r\n\r\n");
            }
        } else {
            RESP("HTTP/1.1 400 Bad Request\r\n\r\n");
        }
    }
    
    else if (strncmp(buf, "GET /tabs", 9) == 0) {
        char *json = tm_to_json(tm);
        ok_json(sock, json);
        free(json);
    }

    else if (strncmp(buf, "POST /tab/", 10) == 0 && strstr(buf, "/visit")) {
        int id;
        sscanf(buf, "POST /tab/%d/visit", &id);
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char url[256] = {0};
            sscanf(strstr(body, "\"url\":"), "\"url\":\"%255[^\"]\"", url);

            for (Tab *t = tm->first; t; t = t->next)
                if (t->id == id) { tab_visit(t, url); break; }
        }
        Tab *tab = tm->first;
        while (tab && tab->id != id) tab = tab->next;
        if (tab) {
            char *json = tab_to_json(tab);
            ok_json(sock, json);
            free(json);
            save_state();
        } else
            RESP("HTTP/1.1 404 Not Found\r\n\r\n");
    }

    else
        RESP("HTTP/1.1 404 Not Found\r\n\r\n");
}

int main(void) {
    tm = tm_init();
    load_state(); 
    
    
    if (!tm->first) {
        tm_new_tab(tm);
    }
    
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY};
    bind(server, (struct sockaddr*)&addr, sizeof addr);
    listen(server, 10);
    printf("Listening on http://localhost:%d\n", PORT);
    while (1) {
        int sock = accept(server, NULL, NULL);
        if (sock >= 0) { handle_request(sock); close(sock); }
    }
}