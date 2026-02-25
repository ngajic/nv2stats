#ifndef OXY_SQLCMDS_H_
#define OXY_SQLCMDS_H_

static const char *create = { "CREATE TABLE %s(day DATE, level INT(3) NOT NULL, nick VARCHAR(255) NOT NULL, place INT(1) NOT NULL, score INT(10) NOT NULL, flag_tied INT(1) NOT NULL, PRIMARY KEY(day, level, place)) WITHOUT ROWID;" };
static const char *insert = { "INSERT INTO %s VALUES('%04d-%02d-%02d','%d','%s','%d','%d'%s);" };
static const char *select = { "SELECT * FROM %s WHERE nick = '%-14s' AND place = %d ORDER BY score DESC;" };
static const char *drop = { "DROP TABLE IF EXISTS %s;" };
static const char *countX = { "SELECT count(*) FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000) AS sub WHERE sub.nick='%s' AND sub.place=%d;" };
static const char *check_score = { "SELECT count(*) FROM %s WHERE level=%d AND place=%d AND nick='%s' AND score=%d;" };
static const char *count_distinc_days = { "SELECT count(DISTINCT day) FROM scores;" };
static const char *count_0th_changes = { "SELECT count(%snick) FROM scores2 WHERE level=%d AND place=0;" };
static const char *empty = { "" };
static const char *gen_history_cmd = { "SELECT day, nick, score FROM scores2 WHERE level=%d AND place=%d;" };
static const char *select_distinct_nick = { "SELECT %snick FROM scores2 WHERE level=%d and place=%d;" };
static const char *select_distinct_score = { "SELECT day, score FROM scores2 WHERE level=%d AND place=%d;" };
static const char *select_day = { "SELECT day FROM scores WHERE day>'%s' LIMIT 1;" };
static const char *select_score = { "SELECT score FROM scores WHERE day='%s' AND level=%d AND place=%d;" };
static const char *update_flag = { "UPDATE scores SET flag_tied=1 WHERE day='%s' AND level=%d AND place=%d;" };
static const char *pick_older_0ths = { "SELECT level/5 as ep, level%%5 as lvl FROM scores2 GROUP BY level HAVING count(CASE WHEN day>='%s' and place=0 THEN 1 ELSE NULL END)=0;" };
static const char *count_player_top10s = { "SELECT count(sub.place) FROM (SELECT nick, place FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s'" };
static const char *player_spots = { "SELECT sub.level, sub.place FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s' ORDER BY sub.level" };
static const char *get_player_id = { "SELECT player_id FROM players WHERE nick='%s';" };
static const char *community_total_score = { "SELECT sum(score) FROM tmp WHERE place=0;" };
static const char *maxed_levels_query_cmd = { "SELECT tmp.level/5, tmp.level%5 FROM (SELECT * FROM scores ORDER BY DAY DESC LIMIT 6000) AS tmp GROUP BY tmp.level HAVING count(DISTINCT tmp.score)=1;" };

#define SQL_CMD_BUF_LEN 512
static char sql_cmd[SQL_CMD_BUF_LEN];

#endif // OXY_SQLCMDS_H_
