/**
  PROJECT - 1

AUTHOR  : Sumit Jain
UTD-ID  : 2021288044
NET-ID  : spj150230@utdallas.edu
SUBJECT : Distributed Computing
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_VAL		(1000)
#define MAX_NODES	(100)

int n_of_procs;
int root_proc;
int ids[MAX_VAL];
int edges[MAX_NODES][MAX_NODES];
int map[MAX_NODES];
sem_t sem_A[MAX_NODES], sem_B[MAX_NODES];	// Semaphore to synchronize execution of rounds
sem_t shared_sem;

// Since its globally declared, no explicit message passing scheme between the threads
// needs to be deployed
int dist[MAX_NODES][2];		// MAX_NODES nodes storing [parent_id][total_weight_to_reach_i0]
int convergecast_msg[MAX_NODES][MAX_NODES][2];	// Stores dist[0](parent) received from child
int bfs_ack_nack[MAX_NODES][MAX_NODES];	// Msg sent to parent to let him know about the children in bfs
int terminate[MAX_NODES][MAX_NODES];	// terminate msg send by the root to the nodes along the tree
int live_node[MAX_NODES];		// Helps in determining if a node is live


initialize_data_structs() {
    memset((void *) map, -1, sizeof(int) * MAX_NODES);

    // Bellman relevant initialization
    memset((void *) edges, -1, sizeof(int) * MAX_NODES * MAX_NODES);
    memset((void *) dist, -1, sizeof(int) * MAX_NODES * 2);
    memset((void *) convergecast_msg, -1, sizeof(int) * MAX_NODES * MAX_NODES * 2);
    memset((void *) bfs_ack_nack, 0, sizeof(int) * MAX_NODES * MAX_NODES);
    memset((void *) terminate, 0, sizeof(int) * MAX_NODES * MAX_NODES);
    memset((void *) live_node, 0, sizeof(int) * MAX_NODES); // Initially all nodes are alive
}

initialize_semaphores() {
    int i;

    for(i = 0; i < n_of_procs; i++) {
        sem_init(&sem_A[i], 0, 0); // Initialize semaphores
        sem_init(&sem_B[i], 0, 0); // Initialize semaphores
    }

    sem_init(&shared_sem, 0, 1); // Initialize semaphores
}

file_parser(FILE *fd) {
    int i, j, k = 0, l = 0;
    char line[MAX_VAL], character[10];

    fgets(line, sizeof(line), fd);
    while(line[k] == '#' || line[k] == '\n') {
        fgets(line, sizeof(line), fd);
    }

    n_of_procs = atoi(line);

    fgets(line, sizeof(line), fd);
    while(line[k] == '#' || line[k] == '\n') {
        fgets(line, sizeof(line), fd);
    }

    for(i = 0; i < n_of_procs; i++) {
        while(line[k] != ' ' && line[k] != '\n')
            character[l++] = line[k++];
        character[l] = '\0';
        ids[i] = i;

        // Create a mapping of node ID's
        map[i] = atoi(character);

        l = 0; k++;
    }

    k = 0;
    // Get who is the root
    fgets(line, sizeof(line), fd);
    while(line[k] == '#' || line[k] == '\n') {
        fgets(line, sizeof(line), fd);
    }
    root_proc = atoi(line);

    i = 0;
    while(map[i] != root_proc) {i++;}
    root_proc = i;

    k = 0;
    for(i = 0; i < n_of_procs; i++) {
        fgets(line, sizeof(line), fd);
        while(line[k] == '#' || line[k] == '\n') {
            fgets(line, sizeof(line), fd);
        }
        for(j = 0; j < n_of_procs; j++) {
            while(line[k] == ' ')
                k++;
            while(line[k] != ' ' && line[k] != '\n')
                character[l++] = line[k++];
            character[l] = '\0';
            if(strcmp(character, "-1"))
                edges[i][j] = atoi(character);
            else
                edges[i][j] = -1; // negative weight denotes no edge
            l = 0; k++;
        }
        k = 0;
    }

    // Dump edges matrix (DEBUG Purpose)
    if(0) {
        for(i = 0; i < n_of_procs; i++) {
            for(j = 0; j < n_of_procs; j++) {
                printf("%d ", edges[i][j]);
            }
            printf("\n");
        }
    }
}

// Scan my neighbors
void scan_my_neighbors(int id, int *neighbors) {
    int i, j = 0;

    for(i = 0; i < n_of_procs; i++) {

        if(i != id) {
            if(-1 != edges[id][i])
                neighbors[j++] = i;
        }
    }

    // Dump my neigbhors (DEBUG Purpose)
    if(0) {	
        printf("ID = %d -  ", id);
        for(i = 0; neighbors[i] != -1; i++)
            printf(" %d ", neighbors[i]);
        printf("\n");
    }
}


// Common thread function
void* thread_node(void *arg)
{
    bool root = false, leaf = true, msg = false, convergecast_done = false;
    bool wait = false, wait_for_terminate = false;
    int id, i, j, value, k;
    int wait_val_0, wait_val_1, wait_val_2, wait_val_3, wait_val_4, wait_val_5;
    int round_num = 0, retval, neighbor_dist, neighbor;
    int neighbors[MAX_NODES], path[MAX_NODES];

    id = (int)*(int *)arg;

    memset((void *) neighbors, -1, sizeof(int) * MAX_NODES);

    scan_my_neighbors(id, neighbors);

    if(id == root_proc) {
        dist[id][0] = -2;	// -2 here indicates that I am the root and hence I have no parent
        dist[id][1] = 0; 	// The distance to reach myself is 0
        for(i = 0; -1 != neighbors[i]; i++) {
            bfs_ack_nack[neighbors[i]][id] = -1;
        }
    }

    // Send nack to yourself as a part of initialization
    bfs_ack_nack[id][id] = -1;

    while(true) {

        // Wait for the signal from the master thread (in order to begin the round)
        sem_wait(&sem_A[id]);

        round_num++;

        if(-1 == neighbors[k])
            k = 0;

        // Access one of my neighbor's dist
        neighbor = neighbors[k];
        neighbor_dist = dist[neighbor][1];

        if(0 == (wait_val_1 + 1)) {
            // Get the random number of rounds to wait before sending any packet on the link
            wait_val_1 = rand() % 18 + 1;    // Random number in the range 1 to 18
        }

        // Don't access any neighbor node's data until the random number of rounds expire
        if(0 == wait_val_1) {

            // If my neighbor has distance to i0 then...
            if(-1 != neighbor_dist && id != root_proc) {

                // If I already have a dist to i0 then compare which one is shorter
                if(-1 != dist[id][1]) {
                    if((neighbor_dist + 1) < dist[id][1]) {
                        dist[id][1] = neighbor_dist + 1;

                        // Tell my existing parent that you cannot be my parent anymore since
                        // I have found a new parent with a shorter route to the root node
                        bfs_ack_nack[dist[id][0]][id] = -1;

                        dist[id][0] = neighbor;
                    } else if ((neighbor_dist + 1) >= dist[id][1] && neighbor != dist[id][0]) {
                        // If the update wasn't successful then send the nack
                        bfs_ack_nack[neighbor][id] = -1;
                    }
                } else {
                    // If this is the first time, then just make an update
                    dist[id][1] = neighbor_dist + 1;
                    dist[id][0] = neighbor;
                }
            }
            k++;
        }

        if(!wait_for_terminate) {
            // Check if I have received ACK/NACK's from all my neighbors
            for(i = 0; -1 != neighbors[i]; i++) {
                if(0 == bfs_ack_nack[id][neighbors[i]]) {
                    // wait since I havent yet heard from them all
                    wait = true;
                    break;
                }
                wait = false;
            }
        }

        // Processing for all nodes != root
        if(id != root_proc) {

            if(0 == (wait_val_2 + 1)) {
                // Get the random number of rounds to wait before sending any packet on the link
                wait_val_2 = rand() % 18 + 1;    // Random number in the range 1 to 18
            }

            // Don't access any neighbor node's data until the random number of rounds expire
            if(0 == wait_val_2) {
                // Send ACK (telling that he is my parent) only if I have received ACK/NACK's
                // from all my neighbors
                if(!wait && !wait_for_terminate) {
                    bfs_ack_nack[dist[id][0]][id] = 1;      // dist[id][0] is my new parent. Tell him this.
                    wait_for_terminate = true;
                }
            }

            // Check if I am the leaf since the convergecast procedure always begins at the leaf nodes
            // if all bfs_ack_nack is -1 then I am leaf
            for(i = 0; -1 != neighbors[i]; i++) {
                if(-1 != bfs_ack_nack[id][neighbors[i]]) {
                    leaf = false;
                    break;
                }
                leaf = true;
            }

            if(0 == (wait_val_3 + 1)) {
                // Get the random number of rounds to wait before sending any packet on the link
                wait_val_3 = rand() % 18 + 1;    // Random number in the range 1 to 18
            }

            // Don't access any neighbor node's data until the random number of rounds expire
            if(0 == wait_val_3) {
                // if leaf==true and I have received a terminate msg then hence start converge-casting
                if(leaf && terminate[id][dist[id][0]]) {
                    convergecast_msg[dist[id][0]][id][0] = dist[id][0];
                    convergecast_msg[dist[id][0]][id][1] = dist[id][1];
                    sem_post(&sem_B[id]);
                    live_node[id] = -1;
                    pthread_exit(&retval);
                }
            }

            if(0 == (wait_val_0 + 1)) {
                // Get the random number of rounds to wait before sending any packet on the link
                wait_val_0 = rand() % 18 + 1;    // Random number in the range 1 to 18
            }

            if(0 == wait_val_0) {
                // Check if I recieved terminate msg from my parent
                // If I did receive then send terminate to my children
                if(terminate[id][dist[id][0]] && -1 != dist[id][0]) {
                    for(i = 0; -1 != neighbors[i]; i++) {
                        if(1 == bfs_ack_nack[id][neighbors[i]]) {
                            terminate[neighbors[i]][id] = 1;
                        }
                    }
                }
            }

            if(!leaf) {
                msg = false;
                // If I am not the leaf then check if I received a convergecast msg from my child/'ren'
                for(i = 0; -1 != neighbors[i]; i++) {
                    if(1 == bfs_ack_nack[id][neighbors[i]]) {
                        if(-1 != convergecast_msg[id][neighbors[i]][0]) {
                            msg = true;
                        } else {
                            msg = false;
                            break;
                        }
                    }
                }

                if(0 == (wait_val_4 + 1)) {
                    // Get the random number of rounds to wait before sending any packet on the link
                    wait_val_4 = rand() % 18 + 1;    // Random number in the range 1 to 18
                }

                // Don't access any neighbor node's data until the random number of rounds expire
                if(0 == wait_val_4) {
                    // If I have received convergecast msg from all my children
                    if(msg) {
                        for(i = 0; i < MAX_NODES; i++) {
                            if(-1 != convergecast_msg[id][i][0]) {
                                // sending msges that I received to my parent
                                convergecast_msg[dist[id][0]][i][0] = convergecast_msg[id][i][0];
                                convergecast_msg[dist[id][0]][i][1] = convergecast_msg[id][i][1];
                            }
                        }
                        // And finally sending my own msg
                        convergecast_msg[dist[id][0]][id][0] = dist[id][0];
                        convergecast_msg[dist[id][0]][id][1] = dist[id][1];

                        sem_post(&sem_B[id]);
                        live_node[id] = -1;
                        pthread_exit(&retval);
                    }
                }
            }
        } else { // I am root and I will do following processing


            if(0 == (wait_val_5 + 1)) {
                // Get the random number of rounds to wait before sending any packet on the link
                wait_val_5 = rand() % 18 + 1;    // Random number in the range 1 to 18
            }

            // Don't access any neighbor node's data until the random number of rounds expire
            if(0 == wait_val_5) {
                // I have received ACK's and NACK's from all my children
                if(!wait) {
                    // Send terminate to my children
                    for(i = 0; -1 != neighbors[i]; i++) {
                        if(1 == bfs_ack_nack[id][neighbors[i]]) {
                            terminate[neighbors[i]][id] = 1;
                        }
                    }
                }
            }

            // Check if I recieved msg from all the nodes in the tree
            convergecast_done = false;
            for(i = 0; -1 != neighbors[i]; i++) {
                if(1 == bfs_ack_nack[id][neighbors[i]]) {
                    if(-1 == convergecast_msg[id][neighbors[i]][0]) {
                        convergecast_done = false;
                        break;
                    } else {
                        convergecast_done = true;
                    }
                }
            }

            if(convergecast_done) {
                printf("\n---Printing adjacency list for [ROOT NODE = %d] ---\n\n", map[id]);
                for(i = 0; i < MAX_NODES; i++) {
                    j = 0;
                    if(-1 != convergecast_msg[id][i][0])
                    {
                        // I can reach this node directly in 1 hop
                        if(root_proc == convergecast_msg[id][i][0])
                            printf("Node: %d Parent: %d Distance_from_root: %d\n", 
                                    map[i], convergecast_msg[id][i][0], convergecast_msg[id][i][1]);
                        else {
                            printf("Node: %d Parent: %d Distance_from_root: %d\n",   
                                    map[i], convergecast_msg[id][i][0], convergecast_msg[id][i][1]);
                            value = convergecast_msg[id][i][0];
                            path[j++] = i;
                            path[j++] = value;
                            while(convergecast_msg[id][value][0] != root_proc) {
                                value = convergecast_msg[id][value][0];
                                path[j++] = value;
                            }

                            while(j-- > 0);
                        }
                    }
                }

                sem_post(&sem_B[id]);
                live_node[id] = -1;
                pthread_exit(&retval);
            }
        }

        // Decrement round counters with each round
        wait_val_0--, wait_val_1--, wait_val_2--, wait_val_3--, wait_val_4--, wait_val_5--;

        // Done with the round. Signal master thread that I am done
        sem_post(&sem_B[id]);
    }
}

int main(int argc, char *argv[]) {
    int i = 0, id[MAX_NODES];
    pthread_t master_t_id;
    pthread_t t_id[MAX_VAL];
    bool  exit_p;
    FILE *config_fd;

    initialize_data_structs();

    config_fd = fopen(argv[1], "r");

    if(NULL == config_fd)
        perror("Cannot open config.txt file\n");

    file_parser(config_fd);
    initialize_semaphores();
    fclose(config_fd);

    while(i < n_of_procs) {
        id[i] = i;
        /* Create seperate threads */
        if(pthread_create(&t_id[i], NULL, &thread_node, &id[i]) != 0)
            perror("Couldn't create the thread\n");
        i++;
    }


    memset((void *) live_node, 0, sizeof(int) * MAX_NODES); // Initially all nodes are alive

    // Round synchronization for Bellman
    while(true) {

        exit_p = true;

        // Signal all the processes to begin the round
        for(i = 0; i < n_of_procs; i++) {
            if(0 == live_node[i]) {
                sem_post(&sem_A[i]);
                exit_p = false;
            }
        }

        if(exit_p)
            exit(0);

        // Wait for all the processes to finish the round(x) before we start the next one(x+1)
        for(i = 0; i < n_of_procs; i++) {
            if(0 == live_node[i]) {
                sem_wait(&sem_B[i]);
            }
        }
    }

    return 0;
}
