#ifndef PLAYER_H_
#define PLAYER_H_

typedef struct {
    char* username;
    char* uuid;
    double x;
    double y;
    double z;
    int is_adventure_mode; // Using int as boolean (0 for false, 1 for true)
} Player;

#endif /* PLAYER_H_ */
