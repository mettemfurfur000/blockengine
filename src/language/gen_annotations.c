#include "include/gen_annotations.h"
#include "include/tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  helpers                                                            */
/* ------------------------------------------------------------------ */

static const char *skip_leading(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static const char *skip_trailing(const char *s, const char *end)
{
    while (end > s && (*(end - 1) == ' ' || *(end - 1) == '\t'))
        end--;
    return end;
}

static void trim_str(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t')
        p++;
    if (p != s)
        memmove(s, p, strlen(p) + 1);
    char *end = s + strlen(s);
    while (end > s && (*(end - 1) == ' ' || *(end - 1) == '\t'))
        end--;
    *end = 0;
}

static char *split_by(char *str, const char *delim, char **save)
{
    if (str)
        *save = str;
    if (!*save || **save == 0)
        return NULL;
    char *start = *save;
    char *p = strstr(start, delim);
    if (p)
    {
        *p = 0;
        *save = p + strlen(delim);
    }
    else
    {
        *save = start + strlen(start);
    }
    return start;
}

static void parse_param(const char *spec, char *name_out, u32 name_sz, char *type_out, u32 type_sz)
{
    const char *colon = strchr(spec, ':');
    if (!colon)
    {
        strncpy(name_out, spec, name_sz - 1);
        name_out[name_sz - 1] = 0;
        type_out[0] = 0;
        return;
    }
    const char *name_start = spec;
    const char *name_end   = skip_trailing(spec, colon);
    u32        nlen        = (u32)(name_end - name_start);
    if (nlen >= name_sz)
        nlen = name_sz - 1;
    strncpy(name_out, name_start, nlen);
    name_out[nlen] = 0;

    const char *type_start = skip_leading(colon + 1);
    strncpy(type_out, type_start, type_sz - 1);
    type_out[type_sz - 1] = 0;
}

/* ------------------------------------------------------------------ */
/*  read whole file into heap-allocated buffer                         */
/* ------------------------------------------------------------------ */

static char *read_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = 0;
    if (out_size)
        *out_size = got;
    return buf;
}

/* ------------------------------------------------------------------ */
/*  annotation list helpers                                           */
/* ------------------------------------------------------------------ */

static void annot_add_func(annotation *a, const char *name, bool has_sig, const char *params, const char *returns)
{
    if (a->func_count >= a->func_cap)
    {
        u32 new_cap = a->func_cap ? a->func_cap * 2 : 16;
        func_entry *tmp = (func_entry *)realloc(a->funcs, new_cap * sizeof(func_entry));
        if (!tmp)
            return;
        a->funcs      = tmp;
        a->func_cap   = new_cap;
    }
    func_entry *fe = &a->funcs[a->func_count++];
    memset(fe, 0, sizeof(*fe));
    strncpy(fe->name, name, MAX_LUA_NAME - 1);
    fe->has_sig = has_sig;
    if (has_sig)
    {
        strncpy(fe->params, params, MAX_SIG_LEN - 1);
        strncpy(fe->returns, returns, MAX_SIG_LEN - 1);
    }
}

static void annot_init(annotation *a, annotation_type type, const char *lua_name)
{
    memset(a, 0, sizeof(*a));
    a->type = type;
    strncpy(a->lua_name, lua_name, MAX_LUA_NAME - 1);
}

static void annot_destroy(annotation *a)
{
    free(a->funcs);
    a->funcs      = NULL;
    a->func_count = 0;
    a->func_cap   = 0;
}

/* ------------------------------------------------------------------ */
/*  parse the @lua marker comment                                      */
/*  " @lua class: Layer"  or  " @lua lib: blockengine"                */
/* ------------------------------------------------------------------ */

static bool parse_marker(const char *text, annotation_type *type, char *lua_name, u32 lua_sz)
{
    text = skip_leading(text);
    if (strncmp(text, "@lua", 4) != 0)
        return false;
    text = skip_leading(text + 4);

    if (strncmp(text, "class", 5) == 0)
        *type = ANNOT_CLASS;
    else if (strncmp(text, "lib", 3) == 0)
        *type = ANNOT_LIB;
    else
        return false;

    text = skip_leading(text + (*type == ANNOT_CLASS ? 5 : 3));
    if (*text == ':')
        text = skip_leading(text + 1);
    text = skip_leading(text);

    const char *end = text;
    while (*end && *end != ' ' && *end != '\t')
        end++;
    u32 len = (u32)(end - text);
    if (len >= lua_sz)
        len = lua_sz - 1;
    strncpy(lua_name, text, len);
    lua_name[len] = 0;
    return true;
}

/* ------------------------------------------------------------------ */
/*  parse a function signature comment                                 */
/*  "(x:integer, y:integer) → boolean"  or  "→ boolean"              */
/* ------------------------------------------------------------------ */

static bool parse_signature(const char *text, char *params_out, u32 params_sz, char *returns_out, u32 returns_sz)
{
    text = skip_leading(text);
    params_out[0]  = 0;
    returns_out[0] = 0;

    const char *arrow = NULL;
    {
        const unsigned char *u = (const unsigned char *)text;
        while (*u)
        {
            if (u[0] == 0xE2 && u[1] == 0x86 && u[2] == 0x92)
            {
                arrow = (const char *)u;
                break;
            }
            u++;
        }
    }
    if (!arrow)
        arrow = strstr(text, "->");

    if (*text == '(')
    {
        text++;
        const char *paren_end = strchr(text, ')');
        if (!paren_end)
            return false;

        u32 plen = (u32)(paren_end - text);
        if (plen >= params_sz)
            plen = params_sz - 1;
        strncpy(params_out, text, plen);
        params_out[plen] = 0;

        text = skip_leading(paren_end + 1);
        if (arrow && arrow >= paren_end)
        {
            const char *ret = skip_leading(arrow + (*(const unsigned char *)arrow == 0xE2 ? 3 : 2));
            strncpy(returns_out, ret, returns_sz - 1);
            returns_out[returns_sz - 1] = 0;
        }
    }
    else if (arrow)
    {
        const char *ret = skip_leading(arrow + (*(const unsigned char *)arrow == 0xE2 ? 3 : 2));
        strncpy(returns_out, ret, returns_sz - 1);
        returns_out[returns_sz - 1] = 0;
    }
    else
        return false;

    trim_str(params_out);
    trim_str(returns_out);
    return true;
}

/* ------------------------------------------------------------------ */
/*  iterate comma-separated params with a callback                     */
/* ------------------------------------------------------------------ */

typedef void (*param_walk_fn)(void *ctx, const char *name, const char *type);

static void walk_params(const char *params_str, void *ctx, param_walk_fn fn)
{
    if (!params_str || !params_str[0])
        return;
    char buf[MAX_SIG_LEN];
    strncpy(buf, params_str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *save = NULL;
    char *part = split_by(buf, ",", &save);
    while (part)
    {
        trim_str(part);
        if (part[0])
        {
            char pname[MAX_LUA_NAME];
            char ptype[MAX_SIG_LEN];
            parse_param(part, pname, sizeof(pname), ptype, sizeof(ptype));
            fn(ctx, pname, ptype);
        }
        part = split_by(NULL, ",", &save);
    }
}

/* ------------------------------------------------------------------ */
/*  write helpers                                                      */
/* ------------------------------------------------------------------ */

static void write_param_line(void *ctx, const char *name, const char *type)
{
    FILE *f = (FILE *)ctx;
    if (type[0])
        fprintf(f, "---@param %s %s\n", name, type);
    else
        fprintf(f, "---@param %s\n", name);
}

typedef struct
{
    FILE       *f;
    bool        need_comma;
} write_state;

static void write_param_call(void *ctx, const char *name, const char *type)
{
    (void)type;
    write_state *ws = (write_state *)ctx;
    if (ws->need_comma)
        fputc(',', ws->f);
    fprintf(ws->f, "%s", name);
    ws->need_comma = true;
}

/* ------------------------------------------------------------------ */
/*  scan file for @lua markers and luaL_Reg tables — line-based        */
/* ------------------------------------------------------------------ */

/* find matching closing brace, handling nesting */
static const char *find_closing_brace(const char *open, const char *end)
{
    u32 depth = 1;
    const char *p = open + 1;
    bool in_string = false;
    bool in_char   = false;
    while (p < end)
    {
        if (in_string)
        {
            if (*p == '"' && *(p - 1) != '\\')
                in_string = false;
        }
        else if (in_char)
        {
            if (*p == '\'' && *(p - 1) != '\\')
                in_char = false;
        }
        else
        {
            if (*p == '"')
                in_string = true;
            else if (*p == '\'')
                in_char = true;
            else if (*p == '{')
                depth++;
            else if (*p == '}')
            {
                depth--;
                if (depth == 0)
                    return p;
            }
        }
        p++;
    }
    return NULL;
}

/* tokenize just the table body (inside { ... }) */
static int parse_table_body(const char *body_start, u32 body_len, annotation *annot)
{
    /* create a null-terminated copy for safe tokenization */
    char *copy = (char *)malloc(body_len + 1);
    if (!copy)
        return FAIL;
    memcpy(copy, body_start, body_len);
    copy[body_len] = 0;

    const char *cursor = copy;
    i32        line    = 0;

    bool sig_for_next  = false;
    char sig_params[MAX_SIG_LEN];
    char sig_returns[MAX_SIG_LEN];

    u32  entry_depth     = 0; /* 0=between entries, 1=inside entry, not counting table's own { } */
    bool expect_lua_name = false;

    while (true)
    {
        token tok = token_next(&cursor, &line);
        if (tok.type == TOK_ERROR)
        {
            /* skip past problematic character (use byte copy) */
            cursor++;
            continue;
        }
        if (tok.type == TOK_EOF)
            break;

        if (tok.type == TOK_COMMENT)
        {
            if (entry_depth == 0)
            {
                if (parse_signature(skip_leading(tok.text),
                                    sig_params, sizeof(sig_params),
                                    sig_returns, sizeof(sig_returns)))
                    sig_for_next = true;
            }
            continue;
        }

        if (tok.type == TOK_CURLY_BRACKET_LEFT)
        {
            entry_depth++;
            if (entry_depth == 1)
                expect_lua_name = true;
        }
        else if (tok.type == TOK_CURLY_BRACKET_RIGHT)
        {
            if (entry_depth == 0)
                break; /* table closing } (shouldn't happen — not in body) */
            entry_depth--;
            expect_lua_name = false;
        }
        else if (tok.type == TOK_STRING && entry_depth == 1 && expect_lua_name)
        {
            expect_lua_name = false;
            if (strcmp(tok.text, "NULL") == 0)
                continue;

            annot_add_func(annot, tok.text,
                           sig_for_next,
                           sig_for_next ? sig_params : "",
                           sig_for_next ? sig_returns : "");
            sig_for_next = false;
        }
        else if (tok.type == TOK_LABEL && entry_depth == 1)
        {
            /* C function name — skip */
        }
        else if (tok.type == TOK_COMMA)
        {
            /* separator — no-op */
        }
    }

    free(copy);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  process a single source file                                       */
/* ------------------------------------------------------------------ */

static int process_file(const char *path, annotation **out, u32 *out_count, u32 *out_cap)
{
    size_t src_size;
    char  *src = read_file(path, &src_size);
    if (!src)
        return FAIL;

    const char *end = src + src_size;
    const char *p   = src;

    while (p < end)
    {
        /* look for "// @lua"  */
        const char *marker = strstr(p, "// @lua");
        if (!marker)
            break;

        /* parse the @lua marker */
        const char *comment_start = marker + 2; /* skip // */
        const char *comment_end   = comment_start;
        while (comment_end < end && *comment_end != '\n')
            comment_end++;

        /* copy comment text */
        char comment_text[512];
        u32  ct_len = (u32)(comment_end - comment_start);
        if (ct_len >= sizeof(comment_text))
            ct_len = sizeof(comment_text) - 1;
        strncpy(comment_text, comment_start, ct_len);
        comment_text[ct_len] = 0;

        annotation_type at;
        char            lua_name[MAX_LUA_NAME];
        if (!parse_marker(comment_text, &at, lua_name, sizeof(lua_name)))
        {
            p = comment_end;
            continue;
        }

        /* advance past the marker line */
        p = comment_end;

        /* scan forward for "luaL_Reg" label, skipping strings/comments */
        bool found_reg = false;
        const char *table_start = NULL;
        const char *scan = p;

        while (scan < end)
        {
            /* skip string literals */
            if (*scan == '"')
            {
                scan++;
                while (scan < end && *scan != '"')
                {
                    if (*scan == '\\')
                        scan++;
                    scan++;
                }
                if (scan < end)
                    scan++;
                continue;
            }
            /* skip char literals */
            if (*scan == '\'')
            {
                scan++;
                if (scan < end && *scan == '\\')
                    scan++;
                if (scan < end)
                    scan++;
                if (scan < end && *scan == '\'')
                    scan++;
                continue;
            }
            /* skip line comments */
            if (*scan == '/' && *(scan + 1) == '/')
            {
                scan += 2;
                while (scan < end && *scan != '\n')
                    scan++;
                continue;
            }
            /* skip block comments */
            if (*scan == '/' && *(scan + 1) == '*')
            {
                scan += 2;
                while (scan < end && !(*scan == '*' && *(scan + 1) == '/'))
                    scan++;
                if (scan < end)
                    scan += 2;
                continue;
            }

            /* look for luaL_Reg */
            if (strncmp(scan, "luaL_Reg", 8) == 0)
            {
                const char *after = scan + 8;
                /* verify it's followed by space, tab, newline, or bracket */
                if (!isalnum((unsigned char)*after) && *after != '_')
                {
                    found_reg = true;
                    /* skip past the variable name and [ ] = */
                    const char *eq = strstr(after, "=");
                    if (!eq || eq >= end)
                        break;
                    eq = skip_leading(eq + 1);
                    if (*eq == '{')
                    {
                        table_start = eq;
                        break;
                    }
                }
            }

            scan++;
        }

        if (!found_reg || !table_start)
            continue;

        /* find matching closing brace */
        const char *closing = find_closing_brace(table_start, end);
        if (!closing)
            continue;

        /* extract table body (between { and }) */
        const char *body_start = table_start + 1;
        u32         body_len   = (u32)(closing - body_start);

        /* parse the table body */
        annotation current;
        annot_init(&current, at, lua_name);
        parse_table_body(body_start, body_len, &current);

        if (current.func_count > 0)
        {
            if (*out_count >= *out_cap)
            {
                u32 new_cap = *out_cap ? *out_cap * 2 : 8;
                annotation *tmp = (annotation *)realloc(*out, new_cap * sizeof(annotation));
                if (!tmp)
                {
                    annot_destroy(&current);
                    free(src);
                    return FAIL;
                }
                *out     = tmp;
                *out_cap = new_cap;
            }
            (*out)[(*out_count)++] = current;
        }
        else
            annot_destroy(&current);

        p = closing + 1;
    }

    free(src);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  write one annotation to a file                                     */
/* ------------------------------------------------------------------ */

static void write_lib_annotation(FILE *f, const annotation *a)
{
    for (u32 i = 0; i < a->func_count; i++)
    {
        func_entry *fe = &a->funcs[i];
        fprintf(f, "\n");

        if (fe->has_sig && fe->params[0])
            walk_params(fe->params, f, write_param_line);

        if (fe->has_sig && fe->returns[0])
            fprintf(f, "---@return %s\n", fe->returns);

        fprintf(f, "function m.%s(", fe->name);

        write_state ws = {f, false};
        if (fe->has_sig && fe->params[0])
            walk_params(fe->params, &ws, write_param_call);

        fprintf(f, ") return %s.%s(", a->lua_name, fe->name);

        ws.need_comma = false;
        if (fe->has_sig && fe->params[0])
            walk_params(fe->params, &ws, write_param_call);

        fprintf(f, ") end\n");
    }
}

static void write_class_annotation(FILE *f, const annotation *a)
{
    fprintf(f, "\n---@class %s\n", a->lua_name);
    for (u32 i = 0; i < a->func_count; i++)
    {
        func_entry *fe = &a->funcs[i];
        fprintf(f, "---@field %s fun(self:%s", fe->name, a->lua_name);

        if (fe->has_sig && fe->params[0])
        {
            char buf2[MAX_SIG_LEN];
            strncpy(buf2, fe->params, sizeof(buf2) - 1);
            buf2[sizeof(buf2) - 1] = 0;

            char *save2 = NULL;
            char *part2 = split_by(buf2, ",", &save2);
            bool  skip_self = true;
            while (part2)
            {
                trim_str(part2);
                if (part2[0])
                {
                    if (skip_self && strncmp(part2, "self", 4) == 0)
                    {
                        skip_self = false;
                        part2 = split_by(NULL, ",", &save2);
                        continue;
                    }
                    skip_self = false;

                    char pname[MAX_LUA_NAME];
                    char ptype[MAX_SIG_LEN];
                    parse_param(part2, pname, sizeof(pname), ptype, sizeof(ptype));
                    fprintf(f, ", %s:%s", pname, ptype[0] ? ptype : "any");
                }
                part2 = split_by(NULL, ",", &save2);
            }
        }

        fputc(')', f);

        if (fe->has_sig && fe->returns[0])
        {
            fputc(':', f);
            if (strcmp(fe->returns, "nil") == 0)
                fputs("nil", f);
            else
                fputs(fe->returns, f);
        }

        fputs("\n", f);
    }
}

static int write_output(const char *path, const annotation *annotations, u32 count)
{
    FILE *f = fopen(path, "w");
    if (!f)
    {
        fprintf(stderr, "error: cannot write '%s'\n", path);
        return FAIL;
    }

    fprintf(f, "---@meta\n");

    bool any_lib = false;
    for (u32 i = 0; i < count; i++)
    {
        const annotation *a = &annotations[i];
        if (a->type == ANNOT_CLASS)
            write_class_annotation(f, a);
        else
        {
            if (!any_lib)
            {
                any_lib = true;
                fprintf(f, "\nlocal m = {}\n");
            }
            write_lib_annotation(f, a);
        }
    }

    if (any_lib)
        fprintf(f, "\nreturn m\n");

    fclose(f);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  public API                                                         */
/* ------------------------------------------------------------------ */

int gen_annotations(const char **src_files, u32 file_count, const char *output_path)
{
    annotation *annotations = NULL;
    u32         count       = 0;
    u32         cap         = 0;

    for (u32 i = 0; i < file_count; i++)
    {
        if (process_file(src_files[i], &annotations, &count, &cap) != SUCCESS)
            fprintf(stderr, "warning: failed to process '%s'\n", src_files[i]);
    }

    int ret = SUCCESS;
    if (count > 0)
    {
        if (write_output(output_path, annotations, count) != SUCCESS)
        {
            fprintf(stderr, "error: failed to write '%s'\n", output_path);
            ret = FAIL;
        }
    }
    else
    {
        FILE *f = fopen(output_path, "w");
        if (f)
        {
            fprintf(f, "---@meta\n");
            fclose(f);
        }
    }

    for (u32 i = 0; i < count; i++)
        annot_destroy(&annotations[i]);
    free(annotations);

    return ret;
}
