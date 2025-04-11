// src/group.c
#include "group.h"
#include <stdlib.h>

Group* group_split(const int *array, int N, int k, int *total_groups) {
    int pairs = N / 2;
    *total_groups = (pairs + k - 1) / k; // Arrotondamento per eccesso
    Group *groups = malloc(*total_groups * sizeof(Group));
    if (!groups) return NULL;

    for (int i = 0; i < *total_groups; i++) {
        groups[i].array = (int*)array;
        groups[i].start = 2 * i * k;
        groups[i].end = groups[i].start + 2 * k;
        if (groups[i].end > N) groups[i].end = N;
    }
    return groups;
}

int group_sum(const Group *group) {
    int sum = 0;
    for (int i = group->start; i < group->end; i++) {
        sum += group->array[i];
    }
    return sum;
}