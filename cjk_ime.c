#include "cjk_ime.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cjk_ime_dict.inc"

enum {
	CJK_IME_SYL_MAX = 6,   /* zhuang 等最长音节 */
	CJK_IME_SEG_MAX = 16,
	CJK_IME_FREQ_MAX = 512
};

typedef struct {
	unsigned count;
	char py[CJK_IME_PREEDIT_MAX];
	char zh[CJK_IME_COMMIT_MAX];
} cjk_freq_ent_t;

static cjk_freq_ent_t g_freq[CJK_IME_FREQ_MAX];
static int g_freq_n;
static int g_freq_loaded;
static char g_freq_path_override[512];
static char g_freq_path_resolved[512];

static size_t dict_len(void) {
	return sizeof(CJK_IME_DICT) / sizeof(CJK_IME_DICT[0]);
}

static void freq_resolve_path(char *buf, size_t cap) {
	const char *home;
	if (g_freq_path_override[0] != '\0') {
		snprintf(buf, cap, "%s", g_freq_path_override);
		return;
	}
	home = getenv("HOME");
	if (home && home[0]) {
		snprintf(buf, cap, "%s/.term/cjk_ime_freq", home);
		return;
	}
	snprintf(buf, cap, "cjk_ime_freq");
}

static void freq_ensure_parent_dir(const char *path) {
	char dir[512];
	char *slash;
	size_t n;
	n = strlen(path);
	if (n == 0 || n >= sizeof(dir)) return;
	memcpy(dir, path, n + 1);
	slash = strrchr(dir, '/');
	if (!slash || slash == dir) return;
	*slash = '\0';
	if (mkdir(dir, 0700) != 0 && errno != EEXIST) {
		/* 再试一层：HOME/.term */
		slash = strrchr(dir, '/');
		if (slash && slash != dir) {
			*slash = '\0';
			(void)mkdir(dir, 0700);
			*slash = '/';
			(void)mkdir(dir, 0700);
		}
	}
}

static void freq_load(void) {
	FILE *fp;
	char line[256];
	if (g_freq_loaded) return;
	g_freq_loaded = 1;
	g_freq_n = 0;
	freq_resolve_path(g_freq_path_resolved, sizeof(g_freq_path_resolved));
	fp = fopen(g_freq_path_resolved, "r");
	if (!fp) return;
	while (fgets(line, sizeof(line), fp) && g_freq_n < CJK_IME_FREQ_MAX) {
		unsigned count = 0;
		char *p, *tab1, *tab2;
		size_t pyl, zhl;
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;
		p = line;
		while (*p && *p != '\t') {
			if (*p < '0' || *p > '9') { p = NULL; break; }
			count = count * 10u + (unsigned)(*p - '0');
			++p;
		}
		if (!p || *p != '\t' || count == 0) continue;
		tab1 = p;
		tab2 = strchr(tab1 + 1, '\t');
		if (!tab2) continue;
		*tab1 = '\0';
		*tab2 = '\0';
		pyl = (size_t)(tab2 - (tab1 + 1));
		zhl = strlen(tab2 + 1);
		while (zhl > 0 && (tab2[1 + zhl - 1] == '\n' || tab2[1 + zhl - 1] == '\r'))
			--zhl;
		if (pyl == 0 || pyl >= CJK_IME_PREEDIT_MAX || zhl == 0 ||
		    zhl >= CJK_IME_COMMIT_MAX)
			continue;
		memcpy(g_freq[g_freq_n].py, tab1 + 1, pyl);
		g_freq[g_freq_n].py[pyl] = '\0';
		memcpy(g_freq[g_freq_n].zh, tab2 + 1, zhl);
		g_freq[g_freq_n].zh[zhl] = '\0';
		g_freq[g_freq_n].count = count;
		++g_freq_n;
	}
	fclose(fp);
}

static void freq_save(void) {
	FILE *fp;
	int i;
	freq_load();
	freq_ensure_parent_dir(g_freq_path_resolved);
	fp = fopen(g_freq_path_resolved, "w");
	if (!fp) return;
	fputs("# cjk_ime freq v1\n", fp);
	for (i = 0; i < g_freq_n; ++i) {
		fprintf(fp, "%u\t%s\t%s\n", g_freq[i].count, g_freq[i].py,
		        g_freq[i].zh);
	}
	fclose(fp);
}

static unsigned freq_get(const char *py, const char *zh) {
	int i;
	freq_load();
	if (!py || !zh) return 0;
	for (i = 0; i < g_freq_n; ++i) {
		if (strcmp(g_freq[i].py, py) == 0 && strcmp(g_freq[i].zh, zh) == 0)
			return g_freq[i].count;
	}
	return 0;
}

static void freq_note(const char *py, const char *zh) {
	int i, min_i;
	unsigned min_c;
	size_t pyl, zhl;
	freq_load();
	if (!py || !zh || !py[0] || !zh[0]) return;
	pyl = strlen(py);
	zhl = strlen(zh);
	if (pyl >= CJK_IME_PREEDIT_MAX || zhl >= CJK_IME_COMMIT_MAX) return;
	for (i = 0; i < g_freq_n; ++i) {
		if (strcmp(g_freq[i].py, py) == 0 && strcmp(g_freq[i].zh, zh) == 0) {
			if (g_freq[i].count < 1000000u) g_freq[i].count++;
			freq_save();
			return;
		}
	}
	if (g_freq_n < CJK_IME_FREQ_MAX) {
		g_freq[g_freq_n].count = 1;
		memcpy(g_freq[g_freq_n].py, py, pyl + 1);
		memcpy(g_freq[g_freq_n].zh, zh, zhl + 1);
		++g_freq_n;
		freq_save();
		return;
	}
	/* 表满：替换计数最低的一条 */
	min_i = 0;
	min_c = g_freq[0].count;
	for (i = 1; i < g_freq_n; ++i) {
		if (g_freq[i].count < min_c) {
			min_c = g_freq[i].count;
			min_i = i;
		}
	}
	g_freq[min_i].count = 1;
	memcpy(g_freq[min_i].py, py, pyl + 1);
	memcpy(g_freq[min_i].zh, zh, zhl + 1);
	freq_save();
}

void cjk_ime_set_freq_path(const char *path) {
	if (!path || !path[0]) {
		g_freq_path_override[0] = '\0';
	} else {
		snprintf(g_freq_path_override, sizeof(g_freq_path_override), "%s", path);
	}
	g_freq_loaded = 0;
	g_freq_n = 0;
	g_freq_path_resolved[0] = '\0';
}

void cjk_ime_clear_freq(void) {
	g_freq_n = 0;
	g_freq_loaded = 1; /* 阻止立刻从旧文件再载入；测试可先 set path 再 clear */
}

static int page_count_of(const cjk_ime_t *ime) {
	if (ime->pool_count <= 0) return 1;
	return (ime->pool_count + CJK_IME_PAGE_SIZE - 1) / CJK_IME_PAGE_SIZE;
}

static void apply_page(cjk_ime_t *ime) {
	int pages = page_count_of(ime);
	int start, i;
	if (ime->page >= pages) ime->page = pages - 1;
	if (ime->page < 0) ime->page = 0;
	start = ime->page * CJK_IME_PAGE_SIZE;
	ime->cand_count = 0;
	for (i = 0; i < CJK_IME_PAGE_SIZE && start + i < ime->pool_count; ++i) {
		ime->cands[ime->cand_count++] = ime->pool[start + i];
	}
}

static int py_exact_exists(const char *py, size_t len) {
	char buf[CJK_IME_SYL_MAX + 1];
	size_t i, n;
	int fi;
	if (len == 0 || len > CJK_IME_SYL_MAX) return 0;
	memcpy(buf, py, len);
	buf[len] = '\0';
	freq_load();
	for (fi = 0; fi < g_freq_n; ++fi) {
		if (strcmp(g_freq[fi].py, buf) == 0) return 1;
	}
	n = dict_len();
	for (i = 0; i < n; ++i) {
		if (strcmp(CJK_IME_DICT[i].py, buf) == 0) return 1;
	}
	return 0;
}

/* 从左贪心最长音节；成功时 offs[0..nseg)=各音节起点，offs[nseg]=总长。 */
static int segment_greedy(const char *s, int len, int *offs, int *nseg_out) {
	int pos = 0;
	int nseg = 0;
	if (len <= 0) return 0;
	while (pos < len) {
		int L, found = 0;
		if (nseg >= CJK_IME_SEG_MAX) return 0;
		for (L = CJK_IME_SYL_MAX; L >= 1; --L) {
			if (pos + L > len) continue;
			if (py_exact_exists(s + pos, (size_t)L)) {
				offs[nseg++] = pos;
				pos += L;
				found = 1;
				break;
			}
		}
		if (!found) return 0;
	}
	offs[nseg] = len;
	*nseg_out = nseg;
	return 1;
}

/* 收集某音节的精确汉字候选到 out[]，返回个数（≤ max）。 */
static int collect_exact_zh(const char *py, size_t len,
                            const char **out, int max) {
	char buf[CJK_IME_PREEDIT_MAX];
	size_t i, n;
	int count = 0;
	int fi;
	if (len == 0 || len >= sizeof(buf) || max <= 0) return 0;
	memcpy(buf, py, len);
	buf[len] = '\0';
	freq_load();
	for (fi = 0; fi < g_freq_n && count < max; ++fi) {
		if (strcmp(g_freq[fi].py, buf) == 0)
			out[count++] = g_freq[fi].zh;
	}
	n = dict_len();
	for (i = 0; i < n && count < max; ++i) {
		if (strcmp(CJK_IME_DICT[i].py, buf) == 0)
			out[count++] = CJK_IME_DICT[i].zh;
	}
	return count;
}

static int utf8_append(char *dst, size_t cap, size_t *off, const char *zh) {
	size_t n;
	if (!zh) return 0;
	n = strlen(zh);
	if (n == 0 || *off + n >= cap) return 0;
	memcpy(dst + *off, zh, n);
	*off += n;
	dst[*off] = '\0';
	return 1;
}

static int pool_add(cjk_ime_t *ime, const char *zh) {
	int i;
	if (!zh || zh[0] == '\0' || ime->pool_count >= CJK_IME_POOL_MAX) return 0;
	/* 去重：同屏池内相同字面量只留一次 */
	for (i = 0; i < ime->pool_count; ++i) {
		if (strcmp(ime->pool[i], zh) == 0) return 0;
	}
	ime->pool[ime->pool_count++] = zh;
	return 1;
}

static void sort_pool_by_freq(cjk_ime_t *ime) {
	int i, j;
	const char *py = ime->preedit;
	for (i = 1; i < ime->pool_count; ++i) {
		const char *key = ime->pool[i];
		unsigned kc = freq_get(py, key);
		j = i;
		while (j > 0) {
			unsigned jc = freq_get(py, ime->pool[j - 1]);
			if (jc >= kc) break;
			ime->pool[j] = ime->pool[j - 1];
			--j;
		}
		ime->pool[j] = key;
	}
}

/* 整串可切分为音节时，用各音节首选字组合，并变化首音节的其它读法。 */
static void refresh_segmented(cjk_ime_t *ime) {
	int offs[CJK_IME_SEG_MAX + 1];
	int nseg = 0;
	const char *cols[CJK_IME_SEG_MAX][CJK_IME_PAGE_SIZE];
	int col_n[CJK_IME_SEG_MAX];
	int s, v;
	if (!segment_greedy(ime->preedit, ime->preedit_len, offs, &nseg))
		return;
	if (nseg < 2) return; /* 单音节已由精确/前缀路径覆盖 */
	for (s = 0; s < nseg; ++s) {
		size_t slen = (size_t)(offs[s + 1] - offs[s]);
		col_n[s] = collect_exact_zh(ime->preedit + offs[s], slen,
		                           cols[s], CJK_IME_PAGE_SIZE);
		if (col_n[s] <= 0) return;
	}
	for (v = 0; v < col_n[0] && ime->pool_count < CJK_IME_POOL_MAX; ++v) {
		size_t off = 0;
		char *slot = ime->cand_storage[ime->pool_count];
		slot[0] = '\0';
		if (!utf8_append(slot, CJK_IME_COMMIT_MAX, &off, cols[0][v]))
			continue;
		for (s = 1; s < nseg; ++s) {
			if (!utf8_append(slot, CJK_IME_COMMIT_MAX, &off, cols[s][0])) {
				off = 0;
				break;
			}
		}
		if (off == 0) continue;
		pool_add(ime, slot);
	}
}

static void refresh_cands(cjk_ime_t *ime) {
	size_t i, n;
	int fi;
	ime->pool_count = 0;
	ime->page = 0;
	ime->cand_count = 0;
	if (ime->preedit_len <= 0) return;
	freq_load();
	/* 用户学过的整词/单字：精确匹配优先进入池 */
	for (fi = 0; fi < g_freq_n && ime->pool_count < CJK_IME_POOL_MAX; ++fi) {
		if (strcmp(g_freq[fi].py, ime->preedit) == 0)
			pool_add(ime, g_freq[fi].zh);
	}
	n = dict_len();
	for (i = 0; i < n && ime->pool_count < CJK_IME_POOL_MAX; ++i) {
		if (strcmp(CJK_IME_DICT[i].py, ime->preedit) == 0)
			pool_add(ime, CJK_IME_DICT[i].zh);
	}
	/* 无精确匹配时，收集前缀匹配（短词/整词优先已在表序中体现）。 */
	if (ime->pool_count == 0) {
		size_t plen = (size_t)ime->preedit_len;
		for (fi = 0; fi < g_freq_n && ime->pool_count < CJK_IME_POOL_MAX; ++fi) {
			if (strncmp(g_freq[fi].py, ime->preedit, plen) == 0)
				pool_add(ime, g_freq[fi].zh);
		}
		for (i = 0; i < n && ime->pool_count < CJK_IME_POOL_MAX; ++i) {
			if (strncmp(CJK_IME_DICT[i].py, ime->preedit, plen) == 0)
				pool_add(ime, CJK_IME_DICT[i].zh);
		}
	}
	/* 仍无候选：多音节组合（如 woaini → 我爱你）。 */
	if (ime->pool_count == 0)
		refresh_segmented(ime);
	sort_pool_by_freq(ime);
	apply_page(ime);
}

void cjk_ime_init(cjk_ime_t *ime) {
	memset(ime, 0, sizeof(*ime));
	freq_load();
}

void cjk_ime_reset(cjk_ime_t *ime) {
	ime->preedit_len = 0;
	ime->preedit[0] = '\0';
	ime->pool_count = 0;
	ime->page = 0;
	ime->cand_count = 0;
}

int cjk_ime_enabled(const cjk_ime_t *ime) {
	return ime && ime->enabled;
}

void cjk_ime_set_enabled(cjk_ime_t *ime, int on) {
	if (!ime) return;
	ime->enabled = on ? 1 : 0;
	if (!ime->enabled) cjk_ime_reset(ime);
}

const char *cjk_ime_preedit(const cjk_ime_t *ime) {
	return ime ? ime->preedit : "";
}

int cjk_ime_candidate_count(const cjk_ime_t *ime) {
	return ime ? ime->cand_count : 0;
}

const char *cjk_ime_candidate(const cjk_ime_t *ime, int index) {
	if (!ime || index < 0 || index >= ime->cand_count) return NULL;
	return ime->cands[index];
}

int cjk_ime_candidate_page(const cjk_ime_t *ime) {
	return ime ? ime->page : 0;
}

int cjk_ime_candidate_page_count(const cjk_ime_t *ime) {
	return ime ? page_count_of(ime) : 1;
}

static int commit_index(cjk_ime_t *ime, int index, char *out, size_t out_cap, size_t *out_len) {
	const char *zh = cjk_ime_candidate(ime, index);
	char py_saved[CJK_IME_PREEDIT_MAX];
	size_t n, i;
	if (!zh || !out || out_cap == 0 || !out_len) return 1;
	n = strlen(zh);
	if (n == 0 || n >= out_cap) return 1;
	for (i = 0; i < n; ++i) {
		unsigned char c = (unsigned char)zh[i];
		if (c < 0x20 || c == 0x7f) return 1;
	}
	memcpy(py_saved, ime->preedit, (size_t)ime->preedit_len + 1);
	memcpy(out, zh, n);
	out[n] = '\0';
	*out_len = n;
	freq_note(py_saved, zh);
	cjk_ime_reset(ime);
	return 2;
}

static int page_delta(cjk_ime_t *ime, int delta) {
	int pages = page_count_of(ime);
	if (ime->preedit_len == 0) return 0;
	if (pages <= 1) return 1; /* 组词中吞掉翻页键，避免泄漏到 PTY */
	ime->page += delta;
	if (ime->page < 0) ime->page = pages - 1;
	if (ime->page >= pages) ime->page = 0;
	apply_page(ime);
	return 1;
}

int cjk_ime_handle_key(cjk_ime_t *ime, int sym, int mods, int pressed,
                       char *out, size_t out_cap, size_t *out_len) {
	(void)mods;
	if (out_len) *out_len = 0;
	if (!ime || !pressed || !ime->enabled) return 0;

	/* Esc：取消组词 */
	if (sym == 0x1b) {
		if (ime->preedit_len == 0) return 0;
		cjk_ime_reset(ime);
		return 1;
	}

	/* Backspace */
	if (sym == 0x08 || sym == 0x7f) {
		if (ime->preedit_len == 0) return 0;
		ime->preedit[--ime->preedit_len] = '\0';
		refresh_cands(ime);
		return 1;
	}

	/* 翻页：- / , / [ 上一页；= / . / ] 下一页（仅组词中） */
	if (sym == '-' || sym == ',' || sym == '[') {
		return page_delta(ime, -1);
	}
	if (sym == '=' || sym == '.' || sym == ']') {
		return page_delta(ime, +1);
	}

	/* 数字选词 1..cand_count；无候选时不吞数字 */
	if (sym >= '1' && sym <= '9') {
		int idx = sym - '1';
		if (ime->preedit_len == 0) return 0;
		if (idx >= ime->cand_count) return 1; /* 吞掉无效选择，保持组词 */
		return commit_index(ime, idx, out, out_cap, out_len);
	}

	/* Space：选第一候选 */
	if (sym == ' ') {
		if (ime->preedit_len == 0) return 0;
		if (ime->cand_count <= 0) return 1;
		return commit_index(ime, 0, out, out_cap, out_len);
	}

	/* 回车不由引擎提交执行：未组词时交给终端；组词中则取消组词不提交 */
	if (sym == '\n' || sym == '\r') {
		if (ime->preedit_len == 0) return 0;
		cjk_ime_reset(ime);
		return 1;
	}

	/* a-z 累积拼音 */
	if (sym >= 'a' && sym <= 'z') {
		if (ime->preedit_len + 1 >= CJK_IME_PREEDIT_MAX) return 1;
		ime->preedit[ime->preedit_len++] = (char)sym;
		ime->preedit[ime->preedit_len] = '\0';
		refresh_cands(ime);
		return 1;
	}
	if (sym >= 'A' && sym <= 'Z') {
		char lower = (char)tolower((unsigned char)sym);
		if (ime->preedit_len + 1 >= CJK_IME_PREEDIT_MAX) return 1;
		ime->preedit[ime->preedit_len++] = lower;
		ime->preedit[ime->preedit_len] = '\0';
		refresh_cands(ime);
		return 1;
	}

	return 0;
}
