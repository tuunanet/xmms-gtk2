#include <glib.h>
#include "xentry.h"

static void test_forward_word_position(void)
{
	g_assert_cmpint(xmms_entry_word_position("hello, world", 0, TRUE), ==, 0);
	g_assert_cmpint(xmms_entry_word_position("hello, world", 5, TRUE), ==, 7);
	g_assert_cmpint(xmms_entry_word_position("hello, world", 12, TRUE), ==, 12);
}

static void test_backward_word_position(void)
{
	g_assert_cmpint(xmms_entry_word_position("hello, world", 12, FALSE), ==, 7);
	g_assert_cmpint(xmms_entry_word_position("hello, world", 6, FALSE), ==, 0);
	g_assert_cmpint(xmms_entry_word_position(" -- ", 4, FALSE), ==, 0);
}

static void test_utf8_word_position(void)
{
	g_assert_cmpint(xmms_entry_word_position("\303\251x caf\303\251", 2, TRUE), ==, 3);
	g_assert_cmpint(xmms_entry_word_position("\303\251x caf\303\251", 7, FALSE), ==, 3);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/xentry/forward-word-position", test_forward_word_position);
	g_test_add_func("/xentry/backward-word-position", test_backward_word_position);
	g_test_add_func("/xentry/utf8-word-position", test_utf8_word_position);

	return g_test_run();
}
