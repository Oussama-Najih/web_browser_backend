#ifndef SERIALIZE_H
#define SERIALIZE_H
#include "../browser/browser.h"
#include <json-c/json.h>


json_object *historynode_to_json_obj(const HistoryNode *n);

json_object *tab_to_json_obj(const Tab *t);

json_object *tm_to_json_obj(const TabManager *tm);


char *tab_to_json_str(const Tab *t);

char *tm_to_json_str(const TabManager *tm);

#endif