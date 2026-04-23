#include <stdio.h>
#include <stdbool.h>

#define MAX_PEOPLE 50

typedef struct {
    int id;
} People;

typedef struct {
    People data[MAX_PEOPLE];
    int head;
    int tail;
    int count;
} Queue;

void in(Queue *q, int id) {
    if (q->count == MAX_PEOPLE) return;
    q->data[q->tail].id = id;
    q->tail = (q->tail + 1) % MAX_PEOPLE;
    q->count++;
}

int out(Queue *q) {
    if (q->count == 0) return -1;
    int id = q->data[q->head].id;
    q->head = (q->head + 1) % MAX_PEOPLE;
    q->count--;
    return id;
}

int main() {
    Queue q;
    int total_people=50;
    int report_interval=5;

    // init
    q.head = 0;
    q.tail = 0;
    q.count = 0;

    for(int i = 0; i < MAX_PEOPLE; i++) {
        in(&q, i+1);
    }

    while(total_people > 1) {
        int id = -1;
        for(int i = 0; i < report_interval - 1; i++) {
            id = out(&q);
            if(id != -1) in(&q, id);
        }
        id = out(&q);
        printf("淘汰: %d\n", id);

        total_people--;
    }

    printf("最后剩下的人是: %d\n", q.data[q.head].id);

    return 0;
}
