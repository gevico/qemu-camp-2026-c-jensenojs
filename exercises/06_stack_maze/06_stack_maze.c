#include <stdio.h>

#define MAX_ROW 5
#define MAX_COL 5

int maze[MAX_ROW][MAX_COL] = {
	0, 1, 0, 0, 0,
	0, 1, 0, 1, 0,
	0, 0, 0, 0, 0,
	0, 1, 1, 1, 0,
	0, 0, 0, 1, 0,
};

typedef struct point {
    int row;
    int col;
} point;

point trace[MAX_ROW * MAX_COL];
int visited[MAX_ROW][MAX_COL] = {0};
int found = 0;

/* 按题目期望的路径顺序：下、上、右、左 */
int dx[4] = {1, -1, 0, 0};
int dy[4] = {0, 0, 1, -1};

void dfs(point current, int count) {
    if (found) {
        return;
    }
    if (current.row < 0 || current.row >= MAX_ROW || current.col < 0 || current.col >= MAX_COL) {
        return;
    }
    if (maze[current.row][current.col] == 1) {
        return;
    }
    if (visited[current.row][current.col]) {
        return;
    }

    /* push current into trace */
    trace[count] = current;

    /* reached goal: print path (include current) */
    if (current.row == MAX_ROW - 1 && current.col == MAX_COL - 1) {
        for (int i = count; i >= 0; i--) {
            printf("(%d, %d)\n", trace[i].row, trace[i].col);
        }
        found = 1;
        return;
    }

    visited[current.row][current.col] = 1;
    for (int i = 0; i < 4; i++) {
        point next = {current.row + dx[i], current.col + dy[i]};
        dfs(next, count + 1);
        if (found) {
            break;
        }
    }
    visited[current.row][current.col] = 0; /* backtrack */
}


int main(void)
{
    point start = {0, 0};
    dfs(start, 0);
    return 0;
}
