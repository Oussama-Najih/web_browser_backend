#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "serialize/json-serialize.h"
#include "state/state.h"
#include "browser/browser.h"

#define PORT 3001
#define RESP(buf) send(sock, buf, strlen(buf), 0)

TabManager *tm;

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

void respond_tab(int sock, int id) {
    Tab *tab = find_tab(tm,id);
    if (tab) {
        char *json = tab_to_json_str(tab);
        ok_json(sock, json);
        free(json);
        save_state(tm);
    } else {
        RESP("HTTP/1.1 404 Not Found\r\n\r\n");
    }
}

void respond_all_tabs(int sock) {
    char *json = tm_to_json_str(tm);
    ok_json(sock, json);
    free(json);
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
            Tab *tab = find_tab(tm,id);
            if (tab) {
                if      (strcmp(direction, "back")    == 0) tab_go_back(tab);
                else if (strcmp(direction, "forward") == 0) tab_go_forward(tab);
            }
        }
        respond_tab(sock, id);
        return;
    }

    
    
    if (strncmp(buf, "DELETE /tab/", 12) == 0) {
        int id; sscanf(buf, "DELETE /tab/%d", &id);
        tm_close_tab(tm, id);
        respond_all_tabs(sock);
        save_state(tm);
        return;
    }

    if (strncmp(buf, "POST /tabs", 10) == 0) {
        tm_new_tab(tm);
        respond_all_tabs(sock);
        save_state(tm);
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
            Tab *tab = find_tab(tm,id);
            if (tab) tab_visit(tab, url, 0);
        }
        respond_tab(sock, id);
        return;
    }

    RESP("HTTP/1.1 404 Not Found\r\n\r\n");
}


int main(void) {
    tm = tm_init();
    load_state(tm);
    if (!tm->first) tm_new_tab(tm);

    int server = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server, 10) < 0) {
        perror("listen");
        return 1;
    }

    printf("Listening on http://localhost:%d\n", PORT);

    while (1) {
        int sock = accept(server, NULL, NULL);
        if (sock >= 0) {
            handle_request(sock);
            close(sock);
        }
    }
}