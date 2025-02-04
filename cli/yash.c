/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file CLI.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include <histedit.h>	/* libedit */

#define CONFIG_PROG		"yash"
#define CONFIG_HISTORY_SIZE	128
#define CONFIG_EDITOR		"emacs"
#define CONFIG_PROMPT		"CLI: > "

static char *
prompt(EditLine *el)
{
#if 0
	char *pr = getenv("PS1");

	return pr ? pr : CONFIG_PROMPT;
#else
	return CONFIG_PROMPT;
#endif
}

int
start_yash(int argc, char **argv)
{
	EditLine	*el;
	History		*hi;
	HistEvent	 ev;
	Tokenizer	*tk;
	const char	*ln;
	int		 nu;

	setlocale(LC_CTYPE, "");

	hi = history_init();
	history(hi, &ev, H_SETSIZE, CONFIG_HISTORY_SIZE);

	tk = tok_init(NULL);

	el = el_init(CONFIG_PROG, stdin, stdout, stderr);
	/* personal taste */
	el_set(el, EL_PROMPT_ESC, prompt, '\1');
	el_set(el, EL_EDITOR, CONFIG_EDITOR);
	el_set(el, EL_HIST, history, hi);
	el_source(el, NULL);

	while((ln = el_gets(el, &nu)) != NULL && nu != 0) {
		const LineInfo	 *li;
		int		  ac, cc, co, er, i;
		const char	**av;

		li = el_line(el);
		er = tok_line(tk, li, &ac, &av, &cc, &co);

		if (er < 0) {
			fprintf(stderr, " Internal error\n");
			continue;
		}

		for (i = 0; i < ac; i++) {
			(void) fprintf(stderr, "%s\n", av[i]);
		}

		tok_reset(tk);
	}

	el_end(el);
	tok_end(tk);
	history_end(hi);

	return 0;
}
