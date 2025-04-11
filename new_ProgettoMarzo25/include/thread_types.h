#ifndef GROUP_H
#define GROUP_H

/**
 * @brief Struttura per un gruppo di elementi
 */
typedef struct {
    int *elements;  ///< Copia degli elementi
    int size;       ///< Dimensione del gruppo
} Group;

Group* group_split(const int *array, int N, int k, int *total_groups);
void group_free(Group *groups, int total_groups);
int group_sum(const Group *group);

#endif