/* 终端内拼音组词：应用掌握提交边界，不依赖系统 IMF。 */
#ifndef CJK_IME_H
#define CJK_IME_H

#include <stddef.h>

/* BGShellBB10 drives this engine from C++ (TerminalDisplay.cpp); Term50 builds
 * it as plain C, so the guard is a no-op there. */
#ifdef __cplusplus
extern "C" {
#endif

enum {
	CJK_IME_PREEDIT_MAX = 64,
	CJK_IME_PAGE_SIZE = 8,
	CJK_IME_CAND_MAX = CJK_IME_PAGE_SIZE, /* 每屏可见候选数（数字 1..8） */
	CJK_IME_POOL_MAX = 64,                /* 组词池上限，可翻页 */
	CJK_IME_COMMIT_MAX = 128
};

typedef struct cjk_ime {
	int enabled;
	char preedit[CJK_IME_PREEDIT_MAX];
	int preedit_len;
	/* 全量候选池 + 当前页窗口 */
	int pool_count;
	const char *pool[CJK_IME_POOL_MAX];
	int page; /* 0-based */
	int cand_count;
	const char *cands[CJK_IME_CAND_MAX];
	/* 音节组合出的候选落在此缓冲；词典原文仍直接指向 .inc 字面量。 */
	char cand_storage[CJK_IME_POOL_MAX][CJK_IME_COMMIT_MAX];
} cjk_ime_t;

void cjk_ime_init(cjk_ime_t *ime);
void cjk_ime_reset(cjk_ime_t *ime);
int cjk_ime_enabled(const cjk_ime_t *ime);
void cjk_ime_set_enabled(cjk_ime_t *ime, int on);

/*
 * 返回：0=未消费；1=已消费（组词中）；2=产生提交（写入 out / *out_len）。
 * pressed==0 时不处理。out 仅为 UTF-8，不含 CR/LF。
 */
int cjk_ime_handle_key(cjk_ime_t *ime, int sym, int mods, int pressed,
                       char *out, size_t out_cap, size_t *out_len);

const char *cjk_ime_preedit(const cjk_ime_t *ime);
int cjk_ime_candidate_count(const cjk_ime_t *ime);
const char *cjk_ime_candidate(const cjk_ime_t *ime, int index);
/* 翻页信息：page 从 0 起；page_count 至少为 1（无候选时为 1）。 */
int cjk_ime_candidate_page(const cjk_ime_t *ime);
int cjk_ime_candidate_page_count(const cjk_ime_t *ime);

/*
 * 词频文件路径（选中计数持久化）。默认：$HOME/.term/cjk_ime_freq
 * path 非空时覆盖；传 NULL 恢复默认。测试可指向临时文件。
 */
void cjk_ime_set_freq_path(const char *path);
/* 清空内存词频表（不删文件）。 */
void cjk_ime_clear_freq(void);

#ifdef __cplusplus
}
#endif

#endif
