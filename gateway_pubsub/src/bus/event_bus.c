#include <core/connection.h>
#include <core/event_loop.h>
#include <stdlib.h>
#include <string.h>

typedef struct subscriber {
  connection_t *conn;
  struct subscriber *next;
} subscriber_t;

typedef struct topic {
  char name[64];
  subscriber_t *subs;
  struct topic *next;
} topic_t;

static topic_t *topics = NULL;

void event_bus_init() { topics = NULL; }

static topic_t *find_or_create_topic(const char *name) {
  topic_t *t = topics;
  while (t) {
    if (strcmp(t->name, name) == 0)
      return t;
    t = t->next;
  }

  t = calloc(1, sizeof(topic_t));
  strncpy(t->name, name, sizeof(t->name) - 1);
  t->next = topics;
  topics = t;
  return t;
}

void event_subscribe(const char *topic, connection_t *conn) {
  topic_t *t = find_or_create_topic(topic);

  subscriber_t *s = calloc(1, sizeof(subscriber_t));
  s->conn = conn;
  s->next = t->subs;
  t->subs = s;
}

void event_unsubscribe_all(connection_t *conn) {
  topic_t *t = topics;
  while (t) {
    subscriber_t **pp = &t->subs;
    while (*pp) {
      if ((*pp)->conn == conn) {
        subscriber_t *tmp = *pp;
        *pp = (*pp)->next;
        free(tmp);
      } else {
        pp = &(*pp)->next;
      }
    }
    t = t->next;
  }
}

void event_publish(const char *topic, const char *data, int len) {
  topic_t *t = topics;
  while (t) {
    if (strcmp(t->name, topic) == 0) {
      subscriber_t *s = t->subs;
      while (s) {
        subscriber_t *next = s->next;
        connection_append_out(s->conn, data, len);
        s = s->next;
      }
      return;
    }
    t = t->next;
  }
}
