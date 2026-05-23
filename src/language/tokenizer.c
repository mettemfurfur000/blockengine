#include "include/tokenizer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *token_str(token_type t)
{
#define CASE(x)                                                                                                        \
	case x:                                                                                                            \
		return #x;
	switch (t)
	{
		CASE(TOK_ERROR)
		CASE(TOK_EOF)
		CASE(TOK_LABEL)
		CASE(TOK_OPCODE)
		CASE(TOK_EXT_OPCODE)
		CASE(TOK_TARGET)
		CASE(TOK_NUMBER)
		CASE(TOK_FLOAT)
		CASE(TOK_COMMA)
		CASE(TOK_DOT)
		CASE(TOK_COLON)
		CASE(TOK_SEMICOLON)
		CASE(TOK_CHAR_LITERAL)
		CASE(TOK_STRING)
		CASE(TOK_BRACKET_LEFT)
		CASE(TOK_BRACKET_RIGHT)
		CASE(TOK_SQUARE_BRACKET_LEFT)
		CASE(TOK_SQUARE_BRACKET_RIGHT)
		CASE(TOK_CURLY_BRACKET_LEFT)
		CASE(TOK_CURLY_BRACKET_RIGHT)
		CASE(TOK_PLUS)
		CASE(TOK_MINUS)
		CASE(TOK_ASTERISK)
		CASE(TOK_FORWARDSLASH)
		CASE(TOK_EXCLAMATION_MARK)
		CASE(TOK_AT)
		CASE(TOK_HASHTAG)
		CASE(TOK_DOLLARSIGN)
		CASE(TOK_PERCENT)
		CASE(TOK_CARET)
		CASE(TOK_AMPERSAND)
		CASE(TOK_QUESTION_MARK)
		CASE(TOK_TILDA)
		CASE(TOK_LESSER)
		CASE(TOK_GREATER)
		CASE(TOK_LESSER_OR_EQUAL)
		CASE(TOK_GREATER_OR_EQUAL)
		CASE(TOK_EQUAL)
		CASE(TOK_NOT_EQUAL)
		CASE(TOK_PLUS_EQUAL)
		CASE(TOK_MINUS_EQUAL)
		CASE(TOK_ASTERISK_EQUAL)
		CASE(TOK_FORWARDSLASH_EQUAL)
		CASE(TOK_COMMENT)
	default:
		return "Unknown token";
	}
#undef CASE
}

char handle_escape_sequence(char *s)
{
	if (*s == '\\') // escape sequence
	{
		s++; // char after backslash
		switch (*s)
		{
		case 'a':
			return 0x07;
		case 'b':
			return 0x08;
		case 'e':
			return 0x1b;
		case 'f':
			return 0x0c;
		case 'n':
			return 0x0a;
		case 'r':
			return 0x0d;
		case 't':
			return 0x09;
		case 'v':
			return 0x0B;
		case '\\':
			return '\\';
		case '\'':
			return '\'';
		case '\"':
			return '\"';
		case '\?':
			return '\?';
		default:
			return -1; // unknown escape sequence
		}
	}
	else // a character
		return *s;
}

// Advances the source pointer and returns the next token
token token_next(const char **src, i32 *lines_ret)
{
	assert(src);
	i32 __ = 0;
	if (!lines_ret)
		lines_ret = &__;

	token tok = {0};
	const char *s = *src;

	// Skip whitespace and newlines
	while (*s == ' ' || *s == '\t' || *s == '\f' || *s == '\v' || *s == '\n' || *s == '\r')
	{
		*lines_ret += (*s == '\n' || *s == '\r') ? 1 : 0;
		s++;
	}

	if (*s == '\0')
	{
		tok.type = TOK_EOF;
		*src = s;
		return tok;
	}

	if (*s == '/' && *(s + 1) == '/') // C-style comments.
	{
		s += 2;
		tok.type = TOK_COMMENT;
		const char *start = s;
		while (*s != '\n' && *s != '\0')
			s++;
		u64 len = s - start;
		if (len >= sizeof(tok.text))
		{
			// fprintf(stderr, "Warning: token is too long: %s\n", tok.text);
			// len = sizeof(tok.text) - 1;

			token errtok = {0};
			errtok.type = TOK_ERROR;
			snprintf(errtok.text, sizeof(errtok.text), "Token is too long at position %d: %.32s\n", (i32)(s - *src),
					 tok.text);
			return errtok;
		}
		strncpy(tok.text, start, len);
		tok.text[len] = '\0';
		tok.text_length = len;
		*src = s;
		return tok;
	}

	// Handle labels and opcodes
	if ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || *s == '_')
	{
		const char *start = s;
		while ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_')
			s++;
		u64 len = s - start;
		if (len >= sizeof(tok.text))
		{
			token errtok = {0};
			errtok.type = TOK_ERROR;
			snprintf(errtok.text, sizeof(errtok.text), "Token is too long at position %d: %.32s\n", (i32)(s - *src),
					 tok.text);
			return errtok;
		}

		strncpy(tok.text, start, len);
		tok.text[len] = '\0';
		tok.text_length = len;
		tok.type = TOK_LABEL;

		*src = s;
		return tok;
	}

	// Handle numbers (decimal or hex)

#define HEX_DIGIT_TO_I64_OP(s)                                                                                         \
	((*s >= '0' && *s <= '9') ? *s - '0' : ((*s >= 'a' && *s <= 'f') ? *s - 'a' + 10 : *s - 'A' + 10))

	if (*s >= '0' && *s <= '9')
	{
		const char *start = s;
		
		// Check if this is a floating point number by scanning ahead
		const char *scan = s;
		bool is_float = false;
		
		// Skip initial digits
		while (*scan >= '0' && *scan <= '9')
			scan++;
		
		// Check for dot followed by digits
		if (*scan == '.' && (*(scan + 1) >= '0' && *(scan + 1) <= '9'))
		{
			is_float = true;
			scan++; // skip dot
			while (*scan >= '0' && *scan <= '9')
				scan++;
		}
		
		// Parse as float if it contains a decimal point
		if (is_float)
		{
			u64 len = scan - start;
			if (len >= sizeof(tok.text))
			{
				token errtok = {0};
				errtok.type = TOK_ERROR;
				snprintf(errtok.text, sizeof(errtok.text), "Float literal too long at position %d\n",
						 (i32)(start - *src));
				return errtok;
			}
			strncpy(tok.text, start, len);
			tok.text[len] = '\0';
			tok.text_length = len;
			tok.type = TOK_FLOAT;
			*src = scan;
			return tok;
		}
		
		// Parse as integer
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		{
			s += 2; // consume '0x'
			while ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))
			{
				if (HEX_DIGIT_TO_I64_OP(s) > 0 && tok.value > (INT64_MAX - HEX_DIGIT_TO_I64_OP(s)) / 16)
				{
					token errtok = {0};
					errtok.type = TOK_ERROR;
					snprintf(errtok.text, sizeof(errtok.text), "Integer literal overflow at position %d\n",
							 (i32)(s - *src));
					return errtok;
				}
				tok.value = tok.value * 16 + HEX_DIGIT_TO_I64_OP(s);
				s++;
			}
		}
		else
		{
			while (*s >= '0' && *s <= '9')
			{
				if (tok.value > (INT64_MAX - (*s - '0')) / 10)
				{
					token errtok = {0};
					errtok.type = TOK_ERROR;
					snprintf(errtok.text, sizeof(errtok.text), "Integer literal overflow at position %d\n",
							 (i32)(s - *src));
					return errtok;
				}

				tok.value = tok.value * 10 + (*s - '0');
				s++;
			}
		}
		u64 len = s - start;
		strncpy(tok.text, start, len);
		tok.text[len] = '\0';
		tok.text_length = len;
		tok.type = TOK_NUMBER;
		*src = s;
		return tok;
	}

#define HANDLE_SINGLE(symbol, tok_type)                                                                                \
	if (*(s) == symbol)                                                                                                \
	{                                                                                                                  \
		tok.type = tok_type;                                                                                           \
		s++;                                                                                                           \
		*src = s;                                                                                                      \
		tok.text[0] = symbol;                                                                                          \
		tok.text[1] = '\0';                                                                                            \
		tok.text_length = 1;                                                                                           \
		return tok;                                                                                                    \
	}

#define HANDLE_DOUBLE(symbol1, symbol2, tok_type)                                                                      \
	if (*(s) == symbol1 && *(s + 1) == symbol2)                                                                        \
	{                                                                                                                  \
		tok.type = tok_type;                                                                                           \
		s += 2;                                                                                                        \
		*src = s;                                                                                                      \
		tok.text[0] = symbol1;                                                                                         \
		tok.text[1] = symbol2;                                                                                         \
		tok.text[2] = '\0';                                                                                            \
		tok.text_length = 2;                                                                                           \
		return tok;                                                                                                    \
	}

	HANDLE_SINGLE('.', TOK_DOT);
	HANDLE_SINGLE(',', TOK_COMMA);
	HANDLE_SINGLE(':', TOK_COLON);
	HANDLE_SINGLE(';', TOK_SEMICOLON);

	HANDLE_SINGLE('(', TOK_BRACKET_LEFT);
	HANDLE_SINGLE(')', TOK_BRACKET_RIGHT);
	HANDLE_SINGLE('[', TOK_SQUARE_BRACKET_LEFT);
	HANDLE_SINGLE(']', TOK_SQUARE_BRACKET_RIGHT);
	HANDLE_SINGLE('{', TOK_CURLY_BRACKET_LEFT);
	HANDLE_SINGLE('}', TOK_CURLY_BRACKET_RIGHT);

	HANDLE_SINGLE('+', TOK_PLUS);
	HANDLE_SINGLE('-', TOK_MINUS);
	HANDLE_SINGLE('*', TOK_ASTERISK);
	HANDLE_SINGLE('/', TOK_FORWARDSLASH);
	HANDLE_DOUBLE('+', '=', TOK_PLUS_EQUAL);
	HANDLE_DOUBLE('-', '=', TOK_MINUS_EQUAL);
	HANDLE_DOUBLE('*', '=', TOK_ASTERISK_EQUAL);
	HANDLE_DOUBLE('/', '=', TOK_FORWARDSLASH_EQUAL);

	HANDLE_SINGLE('!', TOK_EXCLAMATION_MARK);
	HANDLE_SINGLE('@', TOK_AT);
	HANDLE_SINGLE('#', TOK_HASHTAG);

	HANDLE_SINGLE('$', TOK_DOLLARSIGN);
	HANDLE_SINGLE('%', TOK_PERCENT);
	HANDLE_SINGLE('^', TOK_CARET);
	HANDLE_SINGLE('&', TOK_AMPERSAND);

	HANDLE_SINGLE('?', TOK_QUESTION_MARK);
	HANDLE_SINGLE('~', TOK_TILDA);

	HANDLE_SINGLE('<', TOK_LESSER);
	HANDLE_SINGLE('>', TOK_GREATER);
	HANDLE_DOUBLE('<', '=', TOK_LESSER_OR_EQUAL);
	HANDLE_DOUBLE('>', '=', TOK_GREATER_OR_EQUAL);

	HANDLE_SINGLE('=', TOK_EQUAL);
	HANDLE_DOUBLE('!', '=', TOK_NOT_EQUAL);

#undef HANDLE_SINGLE
#undef HANDLE_DOUBLE

	// Handle character literals

	if (*s == '\'')
	{
		tok.type = TOK_CHAR_LITERAL;
		s++; // first literal

		char c = handle_escape_sequence((char *)s);
		if (c == -1)
		{
			token errtok = {0};
			errtok.type = TOK_ERROR;
			snprintf(errtok.text, sizeof(errtok.text), "Unknown escape sequence in character literal at position %d\n",
					 (i32)(s - *src));
			return errtok;
		}

		tok.text[0] = c;
		tok.text[1] = '\0';
		tok.text_length = 1;

		s++; // single quote?
		if (*s != '\'')
		{
			token errtok = {0};
			errtok.type = TOK_ERROR;
			snprintf(errtok.text, sizeof(errtok.text), "Missing terminating character: \"\'\" at position %d\n",
					 (i32)(s - *src));
			return errtok;
		}
		s++; // next token

		*src = s;
		return tok;
	}

	// Strings

	if (*s == '\"')
	{
		s++;
		tok.type = TOK_STRING;
		const char *start = s;
		while (*s != '\"' && *s != '\0')
			s++;
		u64 len = s - start;
		if (len >= sizeof(tok.text))
		{
			token errtok = {0};
			errtok.type = TOK_ERROR;
			snprintf(errtok.text, sizeof(errtok.text), "Token is too long at position %d: %.32s\n", (i32)(s - *src),
					 tok.text);
			return errtok;
		}

		// todo: extract into another function
		u64 out_index = 0;
		for (u64 i = 0; i < len; i++)
		{
			if (start[i] == '\\')
			{
				char esc = handle_escape_sequence((char *)&start[i]);
				if (esc == -1)
				{
					token errtok = {0};
					errtok.type = TOK_ERROR;
					snprintf(errtok.text, sizeof(errtok.text),
							 "Unknown escape sequence in string literal at position %d\n", (i32)(s - *src));
					return errtok;
				}
				tok.text[out_index++] = esc;
				i++; // skip next character as it's part of the escape sequence
			}
			else
			{
				tok.text[out_index++] = start[i];
			}
		}

		tok.text[len] = '\0';
		tok.text_length = len;
		s++;

		*src = s;
		return tok;
	}

	// Unknown character

	tok.type = TOK_ERROR;
	snprintf(tok.text, sizeof(tok.text), "Unknown character: \"%c\" at position %d\n", *s, (i32)(s - *src));

	return tok;
}

void token_debug_all(const char *src)
{
	const char *s = src;

	i32 line = 1;

	while (true)
	{
		token tok = token_next(&s, &line);
		if (tok.type == TOK_ERROR)
		{
			printf("Error token at line %d: %s\n", line, tok.text);
			break;
		}

		if (tok.type == TOK_EOF)
			break;

		printf("%d:\t\"%s\"\t\"%s\"\n", line, token_str(tok.type), tok.text);
	}
}