#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opt.h"
#include "util.h"
#include "functions.h"
#include "usage.h"

struct options {
	const char *opt_string;
	opt_callback_func* opt_callback;
    int args_cnt;
    void (*opt_callback_usage)(void);
};

#define OPTIONS \
	X(OPTION_ACTIVITY, "--act", generate_activity, 0) \
    X(OPTION_BOARD_DIFF, "--board-diff", show_greatest_board_diff, 0) \
    X(OPTION_CHANGES, "--chg", generate_changes_list, 1) \
    X(OPTION_GET_DEMO, "--demo", export_demo_data, 2) \
	X(OPTION_GEN_DEMO, "--demo-gen", produce_compressed_demo, 0) \
	X(OPTION_DOWNLOAD_HS, "--dm", download_metanet_scores, 0) \
    X(OPTION_FILLED, "--filled", get_filled_levels, 1) \
    X(OPTION_GEN_HEATMAP, "--heat", generate_heatmap, 1) \
	X(OPTION_GEN_HISTOGRAM, "--hist", generate_histogram, 1) \
	X(OPTION_GEN_LATEST, "--latest", generate_latest_0ths, 0) \
	X(OPTION_TABLE, "--lead", get_leaderboard, 1) \
	X(OPTION_MAXED, "--maxed", get_maxed_levels, 0) \
    X(OPTION_OLDER_THAN, "--older-than", show_older_0ths, 1) \
	X(OPTION_TENS, "--top10", count_top10s, 1)  \
	X(OPTION_FIVES, "--top5", count_top5s, 1)  \
	X(OPTION_TOTAL_SCORE, "--ts", show_total_score, 1) \
	X(OPTION_VERSION, "--version", get_fw_version, 0) \
	X(OPTION_ZEROTHS, "-0", count_player_X_place, 2) \
	X(OPTION_CREATE_DB, "-c", create_table, 1) \
	X(OPTIOIN_DOWNLOAD_LEVEL_HS, "-d", download_level_scores, 1) \
	X(OPTION_F, "-f", download_metanet_scores, 1) \
	X(OPTION_INSERT, "-i", insert_into, 1) \
	X(OPTION_L, "-l", generate_level_history, 1) \
	X(OPTION_GET_MAP_DATA, "-m", export_level_data, 1) \
    X(OPTION_OWNAGE, "-o", show_episode_ownages, 0) \
	X(OPTION_P, "-p", generate_all_levels_history, 0) \
	X(OPTION_ADD_TIE, "-t", set_tied_scores, 0)       \
	X(OPTION_VERSION_2, "-v", get_fw_version, 0) \
    X(OPTION_WORK, "-w", update_charts, 1)

enum nv2stats_options {
#define X(opt, b, c, d) opt ,
	OPTIONS
#undef X
	OPTIONS_LEN
};

struct options options[OPTIONS_LEN] = {
#define X(opt, opt_str, opt_cb, opt_args_cnt) [opt] = {.opt_string = opt_str, .opt_callback = opt_cb, .args_cnt = opt_args_cnt, .opt_callback_usage = CONCAT(opt_cb, _usage)},
	OPTIONS
#undef X
};

static int cmp_opt(const void *k, const void *o)
{
	const char *key = k;
	const struct options *opt = o;
	return strcmp(key, opt->opt_string);
}

void optget(int *argc, char ***argv, opt_callback_func **cb)
{
	*cb = 0;

	if (*argc < 2) {
		usage();
		return;
	}

	struct options *rc = bsearch((*argv)[1], options, OPTIONS_LEN, sizeof *options, cmp_opt);
	if (rc == NULL) {
		usage();
		return;
	}

	*argc -= 2;
	*argv += 2;

    if (*argc < rc->args_cnt) {
        rc->opt_callback_usage();
        return;
    }

	*cb = rc->opt_callback;
}

// Another implementatition of option parsing
#if 0
void optget(int *argc, char ***argv, int (**cb)(int, char **))
{
	if (*argc < 2)
		goto out;

	int idx;
	char *opt_arg = (*argv)[1];
	if (*opt_arg++ == '-') {
		if (*opt_arg++ == '-') {
			//TODO: check long arguments, standard way, with strcmp
		}
		else {
			switch (*--opt_arg) {
				case 'c': idx = OPTION_CREATE_DB; break;
				case 'd': idx = OPTION_DOWNLOAD_HS; break;
				case 'i': idx = OPTION_INSERT; break;
				case 't': idx = OPTION_ADD_TIE; break;
				case 'm': idx = OPTION_GET_MAP_DATA; break;
				case 'l': idx = OPTION_L; break;
				case 'p': idx = OPTION_P; break;
				case 'f': idx = OPTION_F; break;
				case 'w': idx = OPTION_WORK; break;
				case '0': idx = OPTION_ZEROTHS; break;
				default: goto out;
			}
		}
	}
	else goto out;

	*argc -= 2;
	*argv += 2;
	*cb = options[idx].opt_callback;
	return;

out:	*cb = usage;
	return;
}
#endif

