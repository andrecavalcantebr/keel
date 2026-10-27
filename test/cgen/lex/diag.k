/* lexer diagnostics (lexer-design §4, §6, §8); expected stderr in diag.stderr */
char *s = "x
y";
char c = 'z
;
#define keel_x 1
#define KEEL_CHECKS 0
#undef constexpr
#  undef /* c */ match
#define ma\
tch 1
#define F(x) match(x)
#define keelx 1
#define KEEL 1
#if defined(keel_x)
#endif
