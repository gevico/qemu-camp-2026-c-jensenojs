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

int visited[MAX_ROW][MAX_COL] = {0};
point prev[MAX_ROW][MAX_COL];
point queue[MAX_ROW * MAX_COL];

int dx[4] = {1, 0, -1, 0};
int dy[4] = {0, 1, 0, -1};

int main(void)
{
	point start = {0, 0};
	point goal = {MAX_ROW - 1, MAX_COL - 1};
	int head = 0;
	int tail = 0;
	int found = 0;
	point path[MAX_ROW * MAX_COL];
	int path_len = 0;

	if (maze[start.row][start.col] == 1 || maze[goal.row][goal.col] == 1) {
		printf("No path!\n");
		return 0;
	}

	queue[tail++] = start;
	visited[start.row][start.col] = 1;
	prev[start.row][start.col].row = -1;
	prev[start.row][start.col].col = -1;

	while (head < tail && !found) {
		point current = queue[head++];

		if (current.row == goal.row && current.col == goal.col) {
			found = 1;
			break;
		}

		for (int i = 0; i < 4; i++) {
			point next = {current.row + dx[i], current.col + dy[i]};

			if (next.row < 0 || next.row >= MAX_ROW || next.col < 0 || next.col >= MAX_COL) {
				continue;
			}
			if (maze[next.row][next.col] == 1 || visited[next.row][next.col]) {
				continue;
			}

			visited[next.row][next.col] = 1;
			prev[next.row][next.col] = current;
			queue[tail++] = next;

			if (next.row == goal.row && next.col == goal.col) {
				found = 1;
				break;
			}
		}
	}

	if (!found) {
		printf("No path!\n");
		return 0;
	}

	for (point cur = goal; cur.row != -1 && cur.col != -1; cur = prev[cur.row][cur.col]) {
		path[path_len++] = cur;
	}

	for (int i = 0; i < path_len; i++) {
		printf("(%d, %d)\n", path[i].row, path[i].col);
	}

	return 0;
}
