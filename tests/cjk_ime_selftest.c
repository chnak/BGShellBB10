/*
 * Host side test for the pinyin engine that BGShellBB10 uses (cjk_ime.c).
 *
 * The engine is plain C with no Qt dependency, so it can be built and checked
 * on the build host - the ARM build only proves it compiles:
 *
 *     tools/test-cjk-ime.sh
 *
 * It exits non zero when a behaviour the terminal relies on is broken.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cjk_ime.h"

static int failures;
static int checks;

static void ok(int cond, const char *what, const char *detail)
{
	++checks;
	if (cond) {
		printf("  ok   %s\n", what);
	} else {
		printf("  FAIL %s\n", what);
		if (detail && detail[0])
			printf("       got \"%s\"\n", detail);
		++failures;
	}
}

static void feed(cjk_ime_t *ime, const char *keys, char *out, size_t cap,
                 size_t *out_len)
{
	if (out_len)
		*out_len = 0;
	for (; *keys; ++keys) {
		int r = cjk_ime_handle_key(ime, (unsigned char)*keys, 0, 1,
		                           out, cap, out_len);
		if (r == 2)
			return;
	}
}

static void reset(cjk_ime_t *ime)
{
	cjk_ime_init(ime);
	cjk_ime_set_enabled(ime, 1);
}

static void commit_word(cjk_ime_t *ime, const char *letters, char *out, size_t cap,
                        size_t *out_len)
{
	reset(ime);
	feed(ime, letters, out, cap, out_len);
	if (cjk_ime_handle_key(ime, ' ', 0, 1, out, cap, out_len) != 2)
		*out_len = 0;
}

static void test_words(void)
{
	cjk_ime_t ime;
	char out[192];
	size_t n;

	printf("words\n");
	commit_word(&ime, "nihao", out, sizeof(out), &n);
	ok(n > 0 && strcmp(out, "你好") == 0, "nihao -> 你好", out);

	commit_word(&ime, "zhongwen", out, sizeof(out), &n);
	ok(n > 0 && strcmp(out, "中文") == 0, "zhongwen -> 中文", out);

	commit_word(&ime, "woaini", out, sizeof(out), &n);
	ok(n > 0 && strcmp(out, "我爱你") == 0, "woaini -> 我爱你 (segmented)", out);

	commit_word(&ime, "xianzai", out, sizeof(out), &n);
	ok(n > 0 && strcmp(out, "现在") == 0, "xianzai -> 现在", out);
}

static void test_order(void)
{
	cjk_ime_t ime;

	printf("candidate order\n");
	reset(&ime);
	feed(&ime, "shi", NULL, 0, NULL);
	ok(cjk_ime_candidate_count(&ime) > 0 &&
	   strcmp(cjk_ime_candidate(&ime, 0), "是") == 0, "shi starts with 是",
	   cjk_ime_candidate_count(&ime) > 0 ? cjk_ime_candidate(&ime, 0) : "(none)");

	reset(&ime);
	feed(&ime, "ni", NULL, 0, NULL);
	ok(cjk_ime_candidate_count(&ime) > 0 &&
	   strcmp(cjk_ime_candidate(&ime, 0), "你") == 0, "ni starts with 你",
	   cjk_ime_candidate_count(&ime) > 0 ? cjk_ime_candidate(&ime, 0) : "(none)");

	reset(&ime);
	feed(&ime, "de", NULL, 0, NULL);
	ok(cjk_ime_candidate_count(&ime) > 0 &&
	   strcmp(cjk_ime_candidate(&ime, 0), "的") == 0, "de starts with 的",
	   cjk_ime_candidate_count(&ime) > 0 ? cjk_ime_candidate(&ime, 0) : "(none)");
}

static void test_paging(void)
{
	cjk_ime_t ime;
	char out[192];
	size_t n = 0;
	int pages;

	printf("paging\n");
	reset(&ime);
	feed(&ime, "shi", out, sizeof(out), &n);
	pages = cjk_ime_candidate_page_count(&ime);
	ok(pages > 1, "shi needs several pages", NULL);

	cjk_ime_handle_key(&ime, '=', 0, 1, out, sizeof(out), &n);
	ok(cjk_ime_candidate_page(&ime) == 1, "'=' turns the page", NULL);
	cjk_ime_handle_key(&ime, '-', 0, 1, out, sizeof(out), &n);
	ok(cjk_ime_candidate_page(&ime) == 0, "'-' turns it back", NULL);
}

static void test_commit(void)
{
	cjk_ime_t ime;
	char out[192];
	size_t n = 0;

	printf("commit\n");
	reset(&ime);
	feed(&ime, "shi", out, sizeof(out), &n);
	ok(cjk_ime_handle_key(&ime, '1', 0, 1, out, sizeof(out), &n) == 2 &&
	   strcmp(out, "是") == 0, "'1' commits the first candidate", out);
	ok(cjk_ime_preedit(&ime)[0] == '\0', "composition is cleared after commit", NULL);

	reset(&ime);
	feed(&ime, "ni", out, sizeof(out), &n);
	ok(cjk_ime_handle_key(&ime, ' ', 0, 1, out, sizeof(out), &n) == 2 &&
	   strcmp(out, "你") == 0, "space commits the first candidate", out);
}

static void test_keys_that_must_pass_through(void)
{
	cjk_ime_t ime;
	char out[192];
	size_t n = 0;

	printf("keys the terminal must keep\n");
	reset(&ime);
	ok(cjk_ime_handle_key(&ime, '\r', 0, 1, out, sizeof(out), &n) == 0,
	   "Enter with an empty buffer goes to the shell", NULL);
	ok(cjk_ime_handle_key(&ime, '1', 0, 1, out, sizeof(out), &n) == 0,
	   "digits with an empty buffer go to the shell", NULL);
	ok(cjk_ime_handle_key(&ime, 0x1b, 0, 1, out, sizeof(out), &n) == 0,
	   "Esc with an empty buffer goes to the shell", NULL);
	ok(cjk_ime_handle_key(&ime, ' ', 0, 1, out, sizeof(out), &n) == 0,
	   "space with an empty buffer goes to the shell", NULL);

	reset(&ime);
	feed(&ime, "zhong", out, sizeof(out), &n);
	ok(cjk_ime_handle_key(&ime, 0x1b, 0, 1, out, sizeof(out), &n) == 1 &&
	   cjk_ime_preedit(&ime)[0] == '\0', "Esc drops a composition", NULL);
	feed(&ime, "zhong", out, sizeof(out), &n);
	ok(cjk_ime_handle_key(&ime, '\r', 0, 1, out, sizeof(out), &n) == 1 &&
	   cjk_ime_preedit(&ime)[0] == '\0', "Enter drops a composition, no newline", NULL);
}

static void test_learning(void)
{
	cjk_ime_t ime;
	char out[192];
	char picked[8] = {0};
	size_t n = 0;

	printf("learning\n");
	reset(&ime);
	feed(&ime, "hao", out, sizeof(out), &n);
	ok(cjk_ime_handle_key(&ime, '3', 0, 1, out, sizeof(out), &n) == 2,
	   "third candidate commits", out);
	if (n > 0 && n < sizeof(picked))
		memcpy(picked, out, n);

	reset(&ime);
	feed(&ime, "hao", out, sizeof(out), &n);
	ok(cjk_ime_candidate_count(&ime) > 0 && strcmp(cjk_ime_candidate(&ime, 0), picked) == 0,
	   "the picked character moves to the front", cjk_ime_candidate(&ime, 0));
}

int main(void)
{
	cjk_ime_set_freq_path("/tmp/cjk_ime_selftest_freq");
	cjk_ime_clear_freq();
	remove("/tmp/cjk_ime_selftest_freq");

	test_words();
	test_order();
	test_paging();
	test_commit();
	test_keys_that_must_pass_through();
	test_learning();

	remove("/tmp/cjk_ime_selftest_freq");

	printf("\n%d checks, %d failed\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
