#include <stdio.h>

#include "prompt.h"
#include "parse.h"
#include "envp.h"

void print_prompt(void) {
	char *prompt = shell_get_env("PS1");
	if (!prompt) {
		printf("> ");
		return;
	}

	arg_t final = {0};
	if (parse_arg(&final, make_arg(prompt), true) != 0) return;

	printf("%.*s", (int)final.len, final.ptr);
}
