#ifndef OXY_FUNCTIONS_H_
#define OXY_FUNCTIONS_H_

int create_table(int, char **, struct tm *);
int insert_into(int, char **, struct tm *);
int count_player_X_place(int, char **, struct tm *);
int count_top5s(int, char **, struct tm *);
int count_top10s(int, char **, struct tm *);
int download_level_scores(int, char**, struct tm*);
int download_metanet_scores(int, char **, struct tm *);
int generate_level_history(int, char **, struct tm *);
int generate_all_levels_history(int, char **, struct tm *);
int update_charts(int, char **, struct tm *);
int generate_diff_level_history(int, char **, struct tm *);
int set_tied_scores(int, char **, struct tm *);
int show_older_0ths(int, char **, struct tm *);
int show_total_score(int, char **, struct tm *);
int generate_histogram(int, char **, struct tm *);
int generate_changes_list(int, char **, struct tm *);
int generate_heatmap(int, char **, struct tm *);
int export_level_data(int, char **, struct tm *);
int export_demo_data(int, char **, struct tm *);
int show_episode_ownages(int, char**, struct tm *);
int produce_compressed_demo(int, char **, struct tm *);
int generate_latest_0ths(int, char **, struct tm *);
int get_leaderboard(int, char **, struct tm *);
int get_maxed_levels(int, char **, struct tm *);
int get_filled_levels(int, char **, struct tm *);
int show_greatest_board_diff(int, char **, struct tm *);
int generate_activity(int, char**, struct tm*);

#endif // OXY_FUNCTIONS_H_
