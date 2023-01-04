#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "gmap.h"
#include "entry.h"
#include "string_key.h"

/** 
 * Creates a struct that keeps track of each players results
 * 
 * @param id a string of the player's id
 * @param wins an integer that keeps track of the wins
 * @param score an integer that keeps track of the score per match
 * @param overall_score an integer that keeps track of the overall score
 * @param games an integer that keeps track of the games played
 */
typedef struct _result
{
    char *id;
    double wins;
    double score;
    double overall_score;
    double games;
} result;

//max_id of characters
#define MAX_ID 32

//function for handling commmand line argument errors
int handle_errors(FILE* location_file, int argc, char *argv[]);

//run a blotto game based on the wins
void play_blotto(gmap *all_players, FILE* matchup_file, int battlefields, char *argv[]);

//functions for qsort comparison
int cmpfunc_win(const void *key1, const void *key2);
int cmpfunc_score(const void *key1, const void *key2);

//functions for freeing gmaps
void free_fnc(gmap *all_players);
void free_fnc2(gmap *point_map);

int main(int argc, char *argv[])
{
    //checks if file is present
    if (argv[1] == NULL)
    {
        fprintf(stderr, "Blotto: missing filename\n");
        exit(1);
    }

    FILE *matchup_file;
    matchup_file = fopen(argv[1], "r");

    //handles command line errors
    if (handle_errors(matchup_file, argc, argv) == 1) 
    {
        exit(1);
    }

    //for-loop that checks the command line distribution value are greater than 0
    for (size_t i = 3; i < argc; i++)
    {
        if (atoi(argv[i]) <= 0)
        {
            fclose(matchup_file);
            fprintf(stderr, "Blotto: distribution needs to be postive integers\n");
            exit(1);
        }
    }

    //variable to keep track of the number of games
    int battlefields = argc - 3;

    gmap *all_players = gmap_create(duplicate, compare_keys, hash29, free);

    //reads in the values from standard input
    entry player = entry_read(stdin, MAX_ID, battlefields);
    while (player.id != NULL && strcmp(player.id, "") != 0)
    {

        if (gmap_contains_key(all_players, player.id))
        {
            free(player.id);
            free(player.distribution);
            //entry_destroy(&player);

            free_fnc(all_players);
            fclose(matchup_file);

            fprintf(stderr, "Blotto: Duplicate Player\n");
            exit(1);
        }

        gmap_put(all_players, player.id, player.distribution);
        free(player.id);
        player = entry_read(stdin, MAX_ID, battlefields);    
    }

    if (player.id != NULL)
    {
        free(player.id);
    }

    //check if plyer.id is NULL
    if (player.id == NULL)
    {
        free_fnc(all_players);
        fclose(matchup_file);

        fprintf(stderr, "Blotto: Invalid Distribution\n");
        exit(1);
    }

    if (gmap_size(all_players) == 0)
    {
        gmap_destroy(all_players);
        fclose(matchup_file);

        fprintf(stderr, "Blotto: Empty Distribution File\n");
        exit(1);
    }

    //function to run blotto game
    play_blotto(all_players, matchup_file, battlefields, argv);

    free_fnc(all_players);

    fclose(matchup_file);
}

int handle_errors(FILE* matchup_file, int argc, char *argv[])
{
    //checks if file opens
    if (!matchup_file || matchup_file == NULL)
    {
        fprintf(stderr, "Blotto: could not open %s\n", argv[1]);
        return 1;
    }

    //check if second argument is win or score
    else if (argv[2] == NULL || (strcmp(argv[2], "win") != 0 && strcmp(argv[2], "score") != 0))
    {
        fclose(matchup_file);
        fprintf(stderr, "Blotto: missing 'win' or 'score'\n");
        return 1;
    }

    //check is distribution present
    else if (argv[3] == NULL)
    {
        fclose(matchup_file);
        fprintf(stderr, "Blotto: missing distribution\n");
        return 1;
    }

    return 0;
}

void play_blotto(gmap *all_players, FILE* matchup_file, int battlefields, char *argv[])
{
    //gmap for ids and result structs
    gmap *point_map = gmap_create(duplicate, compare_keys, hash29, free);

    //strings to store ids fread form matchup file
    char id1[MAX_ID];
    char id2[MAX_ID];

    //num for fscanf
    int num;

    //arrays of integeres for distrbution of two competitors
    int *arr1;
    int *arr2;

    //game structs
    result *game1;
    result *game2;

    //variable for fgetc()
    int ch;

    //check whether there is a blank space or empty line in the beginning of the file
    if ((ch = fgetc(matchup_file)) == 32 || ch == 10)
    {
        free_fnc(all_players);
        gmap_destroy(point_map);
        fclose(matchup_file);

        fprintf(stderr, "Blotto: Invalid Matchup File\n");
        exit(1);
    }

    else
    {
        ungetc(ch, matchup_file);
    }

    //cloop through matchup file
    while ((num = fscanf(matchup_file, "%s %s", id1, id2)) == 2)
    {
        //checks if matchup file starts with an empty line or a space
        if ((ch = fgetc(matchup_file)) != 10 && ch != EOF)
        {
            free_fnc(all_players);
            gmap_destroy(point_map);
            fclose(matchup_file);

            fprintf(stderr, "Blotto: Wrong Matchup File\n");
            exit(1);    
        } 

        //checks whether ids have a distribtuion
        if (gmap_contains_key(all_players, id1) && gmap_contains_key(all_players, id2))
        {
            //if id doesn't have an entry yet, create one
            if (gmap_contains_key(point_map, id1) == false)
            {
                //struct for initialization
                result *stats = malloc(sizeof(result));
                stats->games = 0.0;
                stats->score = 0.0;
                stats->overall_score = 0.0;
                stats->wins = 0.0;
                stats->id = malloc(sizeof(char) * MAX_ID);
                strcpy(stats->id, id1);
                gmap_put(point_map, id1, stats);  
            }

            if (gmap_contains_key(point_map, id2) == false)
            {
                //struct for initialization
                result *stats = malloc(sizeof(result));
                stats->games = 0.0;
                stats->score = 0.0;
                stats->overall_score = 0.0;
                stats->wins = 0.0;
                stats->id = malloc(sizeof(char) * MAX_ID);
                strcpy(stats->id, id2);
                gmap_put(point_map, id2, stats);  
            }

            //assign distribution stored in all_players for the respective id to array
            arr1 = gmap_get(all_players, id1);
            arr2 = gmap_get(all_players, id2);

            for (int i = 0; i < battlefields; i++)
            {
                if (arr1[i] > arr2[i])
                {
                    game1 = gmap_get(point_map, id1);
                    game1->score += atof(argv[i + 3]);
                }

                if (arr1[i] == arr2[i])
                {
                    game1 = gmap_get(point_map, id1);
                    game2 = gmap_get(point_map, id2);
                    game1->score += (atof(argv[i + 3])/2);
                    game2->score += (atof(argv[i + 3])/2);
                }

                if (arr1[i] < arr2[i])
                {
                    game2 = gmap_get(point_map, id2);
                    game2->score += atof(argv[i + 3]);
                }
            }

            game1 = gmap_get(point_map, id1);
            game2 = gmap_get(point_map, id2);

            //set the scores to the overall score
            game1->overall_score += game1->score;
            game2->overall_score += game2->score;

            //updates result structs for both ids
            if (game1->score > game2->score)
            {
                game1->wins++;
                game1->games++;
                game2->games++;
                game1->score = 0;
                game2->score = 0;
            }

            else if (game1->score == game2->score)
            {
                game1->wins += 0.5;
                game2->wins += 0.5;
                game1->games++;
                game2->games++;
                game1->score = 0;
                game2->score = 0;
            }

            else
            {
                game2->wins++;
                game1->games++;
                game2->games++;
                game1->score = 0;
                game2->score = 0;
            }
        }

        else
        {
            free_fnc(all_players);
            free_fnc2(point_map);
            fclose(matchup_file);

            fprintf(stderr, "Blotto: Invalid Player\n");
            exit(1);
        }
    }

    //if num doesn't return -1 (EOF) something is wrong with the format of the file
    if (num != -1)
    {
        free_fnc(all_players);
        free_fnc2(point_map);
        fclose(matchup_file);

        fprintf(stderr, "Blotto: Issue with Matchup File\n");
        exit(1);
    }

    //if matchup file is empty
    if (gmap_size(point_map) == 0)
    {
        free_fnc(all_players);
        gmap_destroy(point_map);
        fclose(matchup_file);

        fprintf(stderr, "Blotto: Empty Matchup File\n");
        exit(1);
    }

    //create array of all result structs in point_map
    result *stats_arr = malloc(sizeof(result) * gmap_size(point_map));
    const char **key_arr = (const char**) gmap_keys(point_map);

    for (size_t i = 0; i < gmap_size(point_map); i++)
    {
        stats_arr[i] = *(result*) gmap_get(point_map, key_arr[i]);
        //printf("%s %lf", stats_arr[i]->id, stats_arr[i]->wins);
    }

    //in case of win
    if (strcmp(argv[2], "win") == 0)
    {
        qsort(stats_arr, gmap_size(point_map), sizeof(result), cmpfunc_win);

        for (int i = 0; i < gmap_size(point_map); i++)
        {
            printf("%7.3f %s\n", (stats_arr[i].wins/stats_arr[i].games), stats_arr[i].id); 

        }
    } 

    //in case of score
    else if (strcmp(argv[2], "score") == 0)
    {
        qsort(stats_arr, gmap_size(point_map), sizeof(result), cmpfunc_score);

        for (int i = 0; i < gmap_size(point_map); i++)
        { 
            printf("%7.3f %s\n", (stats_arr[i].overall_score/stats_arr[i].games), stats_arr[i].id); 
        }
    }

    free(stats_arr);

    for (size_t i = 0; i < gmap_size(point_map); i++)
    {
        free(((result *) gmap_get(point_map, key_arr[i]))->id);
        free(gmap_get(point_map, key_arr[i]));
    }

    free(key_arr);
    gmap_destroy(point_map);
}

int cmpfunc_win(const void *key1, const void *key2)
{
    //declares const void as result*
    result *r1 = (result*)key1;
    result *r2 = (result*)key2;

    if ((r1->wins/r1->games) > (r2->wins/r2->games))
    {
        return -1;
    }

    else if ((r1->wins/r1->games) < (r2->wins/r2->games))
    {
        return 1;
    }

    else
    {
        return strcmp(r1->id, r2->id);
    }
}

int cmpfunc_score(const void *key1, const void *key2)
{
    //declares const void as result*
    result *r1 = (result*)key1;
    result *r2 = (result*)key2;

    if ((r1->overall_score/r1->games) > (r2->overall_score/r2->games))
    {
        return -1;
    }

    else if ((r1->overall_score/r1->games) < (r2->overall_score/r2->games))
    {
        return 1;
    }

    else
    {
        return strcmp(r1->id, r2->id);
    }
}

void free_fnc(gmap *all_players)
{
    //function for freeing first gmap
    const char **key_arr = (const char**) gmap_keys(all_players);

    for (size_t i = 0; i < gmap_size(all_players); i++)
    {
        free(gmap_get(all_players, key_arr[i]));
    }

    free(key_arr);
    gmap_destroy(all_players);
}

void free_fnc2(gmap *point_map)
{
    //function for freeing second gmap
    const char **key_arr = (const char**) gmap_keys(point_map);

    for (size_t i = 0; i < gmap_size(point_map); i++)
    {
        free(((result *) gmap_get(point_map, key_arr[i]))->id);
        free(gmap_get(point_map, key_arr[i]));
    }

    free(key_arr);
    gmap_destroy(point_map);
}