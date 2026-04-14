//
// Created by gn203426m on 3/7/2022.
//
#include <stdio.h>

static void count_player_X_place_usage(void)
{
    puts("Usage: nv2stats.exe -0 <place> <player>");
    puts("---------------------------------------");
    puts("<place>       - Spot on the leaderboards interested in");
    puts("<player>      - Player interested in");
    puts("Description: gets the number of X-th spots for a player.\n");
}

static void count_top10s_usage(void)
{
    puts("Usage: nv2stats.exe --top10 <player>");
    puts("---------------------------------------");
    puts("<player>      - Player interested in");
    puts("Description: gets the number of top10 spots for a player.\n");
}

static void count_top5s_usage(void)
{
    puts("Usage: nv2stats.exe --top5 <player>");
    puts("---------------------------------------");
    puts("<player>      - Player interested in");
    puts("Description: gets the number of top5 spots for a player.\n");
}

static void create_table_usage(void)
{
    puts("Usage: nv2stats.exe -c <name>");
    puts("------------------------------------------");
    puts("<name>        - Name for the table to create.");
    puts("Description: Creates table with appropriate columns for keeping Nv2 highscoring data.\n");
}

static void download_level_scores_usage(void)
{
    puts("Usage: nv2stats.exe -d <level>");
    puts("-----------------------------------");
    puts("<level>        - Level for which highscore leaderboard is requested. Can be user level or metanet level.");
    puts("Description: downloads highscoring data from desired level and presents top10 leaderboard in output.\n");
}

static void download_metanet_scores_usage(void)
{
    puts("Usage: nv2stats.exe --dm");
    puts("-----------------------------------");
    puts("Description: downloads highscoring data from metanet servers and stores it under .hs2 file named after the current date.\n");
}

static void export_demo_data_usage(void)
{
    puts("Usage: nv2stats.exe --demo <level> <player>");
    puts("-----------------------------------");
    puts("<level>        - Level for which demo data is asked for. Can be user level or metanet level.");
    puts("<player>       - Player nick you are interested in. This basically can be anyone who played the level, not just the ones seen on the leaderboard.");
    puts("Description: Outputs demo data on a given level for a given player to the console. Frame actions are listed in separate lines.\n");
}

static void export_level_data_usage(void)
{
    puts("Usage: nv2stats.exe -m <level_id>");
    puts("-----------------------------------");
    puts("<level_id>        - ID of the user level.");
    puts("Description: Outputs level data to the console for given level id. Valid user level ID's start with 1001 onward.\n");
}

static void show_episode_ownages_usage(void)
{
    puts("Usage: nv2stats.exe -o");
    puts("-----------------------------------");
    puts("Description: Lists episode ownages on metanet leadeboards.\n");
}

static void generate_changes_list_usage(void)
{
    puts("Usage: nv2stats.exe --chg <starting_date> [<ending_date>]");
    puts("-----------------------------------");
    puts("<starting_date>       - Date in format YYYY-MM-DD used as starting date.");
    puts("<ending_date>         - Date in format YYYY-MM-DD used as ending date. Optional.");
    puts("Description: Shows all changes on metanet leadeboards between two dates, or after <starting_date> if <ending_date> is ommited.\n");
}

static void get_filled_levels_usage(void)
{
    puts("Usage: nv2stats.exe --filled <filled_spots>");
    puts("-----------------------------------");
    puts("<filled_spots>        - Number of spots on leaderboard that have highest score");
    puts("Description: Shows levels with almost maxed boards. Number of filled spots should be between 1 and 9.\n");
}

static void generate_heatmap_usage(void)
{
    puts("Usage: nv2stats.exe --heat <player>");
    puts("-----------------------------------");
    puts("<player>        - Player interested in generating heatmap for.");
    puts("Description: Shows metanet levels placement distribution for a player on a map, with heat colors indicating better or worse placements.\n");

}

static void usage_usage(void)
{
    puts(__func__);
}

static void generate_histogram_usage(void)
{
    puts("Usage: nv2stats.exe --hist <player>");
    puts("---------------------------------------");
    puts("<player>        - Player interested in.");
    puts("Description: draws histogram for players metanet scores.\n");
}

static void generate_latest_0ths_usage(void)
{
    puts("Usage: nv2stats.exe --latest");
    puts("---------------------------------------");
    puts("Description: generates index.html page with latest (up to 7 days in past) 0ths.\n");
}

static void get_leaderboard_usage(void)
{
    puts("Usage: nv2stats.exe --lead <level>");
    puts("---------------------------------------");
    puts("<level>         - Level interested in.");
    puts("Description: Outputs leaderbaord for selected level. Leaderboard is up to date as of last score download.\n");
}

static void generate_activity_usage(void)
{
    puts("Usage: nv2stats.exe --act");
    puts("---------------------------------------");
    puts("Description: generates .htm page presenting number of new 0ths each day in a timeline.\n");
}

static void show_greatest_board_diff_usage(void)
{
    puts("Usage: nv2stats.exe --board-diff");
    puts("---------------------------------------");
    puts("Description: Outputs top 20 levels with greatest difference between 0th owner and 1st place owner.\n");
}

static void get_maxed_levels_usage(void)
{
    puts("Usage: nv2stats.exe --maxed");
    puts("---------------------------------------");
    puts("Description: Outputs maxed levels, ie. levels with filled out leaderabord with supposed max scores.\n");
}

static void generate_level_history_usage(void)
{
    puts("Usage: nv2stats.exe -l <level>");
    puts("---------------------------------------");
    puts("<level>       - Level interested in. Should be entered with normal naming, eg 42-1.");
    puts("Description: generates .htm page presenting 0th timeline chart for particula metanet level.\n");
}

static void generate_all_levels_history_usage(void)
{
    puts("Usage: nv2stats.exe -p");
    puts("---------------------------------------");
    puts("Description: generates .htm page with 0th timeline charts for all metanet levels.\n");
}

static void get_fw_version_usage(void)
{
    puts(__func__);
}

static void insert_into_usage(void)
{
    puts(__func__);
}

static void produce_compressed_demo_usage(void)
{
    puts(__func__);
}

static void set_tied_scores_usage(void)
{
    puts(__func__);
}

static void show_older_0ths_usage(void)
{
    puts(__func__);
}

static void show_total_score_usage(void)
{
    puts(__func__);
}

static void update_charts_usage(void)
{
    puts("Usage: nv2stats.exe -w <timline_pick_flag>");
    puts("---------------------------------------");
    puts("<timline_pick_flag> - hexadecimal, decimal or octal number, in which SET bits select what timeline chart to generate.");
    puts("    bit0 set: generate pure 0ths rankings timeline chart");
    puts("    bit1 set: generate tied 0ths rankings timeline chart");
    puts("    bit2 set: generate top10s rankings timeline chart");
    puts("    bit3 set: generate top5s rankings timeline chart");
    puts("    bit4 set: generate points rankings timeline chart");
    puts("    bit5 set: generate average rankings timeline chart");
    puts("Description: generate timeline and current rankings chart\n");
}

static void usage(void)
{
    fputs(""
        "Usage: nv2stats.exe [OPTION] [ARGS]"
        , stdout);

    fputs(""
        "\nProduce various statistical information about Nv2 game highscoring competition\n\n"
        , stdout);

    fputs(""
        "\nGATHERING STATISTICAL INFORMATION\n"
        LINEBREAK
        "   --dm                download highscore file (saved with .hs2 extension)\n"
        "   -i                  insert data from .hs2 (provide name with extension) files into database\n"
        "   -c                  create table (when starting anew)\n"
        "   -t                  update table to introduce tied_flag column\n\n"
        , stdout);

    fputs(""
        "\nSHOW STATISTICAL INFORMATION ABOUT NV2.0 HIGHSCORING\n"
        LINEBREAK
        "   -0                  show number of positions on a leaderboard for a player\n"
        "   -d                  show level's leaderboard\n"
        "   -o                  show episode ownages\n"
        "   --maxed             show maxed levels\n"
        "   --filled            show almost maxed levels\n"
        "   --top10             count number of top10s for a player\n"
        "   --top5              count number of top5s for a player\n"
        "   --ts                count total score for a player (compared against community total score)\n"
        "   --chg               show changes on leaderboards between two dates\n"
        "   --hist              generate histogram for a player\n"
        "   --older-than        show 0th scores older than provided date\n"
        "   --lead              show leaderboard of a level\n"
        "   --board-diff        Shows top20 board differences between top and bottom spot\n\n"
        , stdout);

    fputs(""
        "\nGAMEPLAY INFORMATION\n"
        LINEBREAK
        "   -m                  export level data\n"
        "   --demo              show readable demo on a level for particular player\n"
        "   --demo-gen          generate compressed demo from readable input\n\n"
        , stdout);

#define OXY_PRODUCE_HTML_OPTS
#if defined OXY_PRODUCE_HTML_OPTS
    fputs(""
        "\nPRODUCING HTML FILES\n"
        LINEBREAK
        "   -w                  generate timeline charts and latest rankings\n"
        "   -p                  generate 0ths owner timeline changes for a level (used in generating all levels history)\n"
        "   -l                  generate 0th owner timeline changes for a level\n"
        "   -f                  generate 0ths vs 9ths timeline chart for a level\n"
        "   --heat              generate heatmaps and histogram charts for a player\n"
        "   --latest            generate index page with latest 0ths (up to 7 days in past)\n\n"
        , stdout);
#endif

    fputs(""
        "\nINFO ABOUT PROGRAM\n"
        LINEBREAK
        "   -v,--version           program version\n\n"
        , stdout);

    fputs(""
        "EXAMPLES:\n"
        LINEBREAK
        "   nv2stats.exe -dm\n"
        "   nv2stats.exe -i one_file other_file"
        , stdout);
}
