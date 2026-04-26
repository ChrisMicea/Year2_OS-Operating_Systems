The tool used was Claude.
I really liked how it used a lot of helper functions - this is something that I kind of learned and that I hope to apply in the future.
I also like how it splits long lines of code into more shorter lines. In different cases, the syntax for doing this is different, so I learned things like that too.
Lastly, I used AI for consulting me on things like proper file and folder structure and that helps a lot.

I really don't like how it formats code sometimes, so, for example, I changed unclear variable names like 'r' to report.
However, I preserved most of its logic intact to keep the spirit of the experiment. Besides, most of the work it has done is writing a lot of tedious code with a lot of if...elses to check for possible errors

My first prompt:

Okay, here are my current working files for the project:
#ifndef TEST_H
#define TEST_H
#include <time.h>
#define NAME_LEN 20
#define ISSUE_CATEGORY_LEN 20
#define DESCRIPTION_LEN 200
#define RELATIVE_FILEPATH "./districts/" 
typedef struct {
int reportID;
char inspectorName[NAME_LEN];
struct {
float x, y;
    } coords;
char issueCategory[ISSUE_CATEGORY_LEN];
int severityLevel; // 1 = minor, 2 = moderate, 3 = critical
time_t timestamp;
char description[DESCRIPTION_LEN];
} report_t;
typedef enum {
    MANAGER, INSPECTOR
} role_t;
typedef enum {
    OK, 
    ERROR_USER_WRONG_COMMAND,
    ERROR_HANDLE_USER_COMMAND,
    ERROR_USER_ARGUMENTS_FOR_ADD,
    ERROR
} returnType_t;
#endif // TEST_H
. We're going to be tackling the filter function. First, implement thisfunction, please
int parse_condition(const char *input, char *field, char *op, char *value);
      which splits a field:operator:value string into its three parts.

The AI response:

Looking at your code and the project structure, I'll implement parse_condition that splits a condition string like severity:>:2 or category:==:flooding into its three parts.Here's the parse_condition function. I'll also show a quick diagram of how it works, then explain the design decisions.

Here's the implementation, designed to fit naturally into your existing codebase:

/*
 * parse_condition - splits "field:op:value" into three separate buffers.
 *
 * Supported operators: ==, !=, >, <, >=, <=
 * Example inputs:
 *   "severity:>:2"
 *   "category:==:flooding"
 *   "inspector:==:alice"
 *
 * Returns 0 on success, -1 on malformed input.
 *
 * Caller must ensure field/op/value buffers are large enough.
 * Recommended: field[32], op[4], value[DESCRIPTION_LEN]
 */
int parse_condition(const char *input, char *field, char *op, char *value)
{
    if (!input || !field || !op || !value) {
        fprintf(stderr, "parse_condition: NULL argument\n");
        return -1;
    }

    /* Find the first colon — separates field from operator */
    const char *first_colon = strchr(input, ':');
    if (!first_colon) {
        fprintf(stderr, "parse_condition: missing first ':' in \"%s\"\n", input);
        return -1;
    }

    /* Find the second colon — separates operator from value */
    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) {
        fprintf(stderr, "parse_condition: missing second ':' in \"%s\"\n", input);
        return -1;
    }

    /* Extract field */
    size_t field_len = first_colon - input;
    if (field_len == 0) {
        fprintf(stderr, "parse_condition: empty field in \"%s\"\n", input);
        return -1;
    }
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    /* Extract operator */
    size_t op_len = second_colon - (first_colon + 1);
    if (op_len == 0 || op_len > 2) {   /* longest valid op is ">=", 2 chars */
        fprintf(stderr, "parse_condition: bad operator length in \"%s\"\n", input);
        return -1;
    }
    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    /* Validate operator is one of the recognised tokens */
    if (strcmp(op, "==") != 0 && strcmp(op, "!=") != 0 &&
        strcmp(op, ">")  != 0 && strcmp(op, "<")  != 0 &&
        strcmp(op, ">=") != 0 && strcmp(op, "<=") != 0) {
        fprintf(stderr, "parse_condition: unrecognised operator \"%s\"\n", op);
        return -1;
    }

    /* Extract value — everything after the second colon */
    const char *val_start = second_colon + 1;
    if (*val_start == '\0') {
        fprintf(stderr, "parse_condition: empty value in \"%s\"\n", input);
        return -1;
    }
    strcpy(value, val_start);   /* safe: caller owns a buffer sized to the input */

    return 0;   /* OK */
}

A few design notes:
strchr is called twice on the same string rather than using strtok, because strtok is destructive (it writes \0 into the input) and not re-entrant. Since input is declared const char *, it's the right choice here.
The operator length is capped at 2 and then validated against the exact set of allowed tokens — this means garbage like "severity:??:3" is rejected explicitly, giving a clear error rather than silently producing nonsense in the filter logic later.
The value gets a plain strcpy rather than strncpy because the caller controls the buffer. I'd suggest declaring callers like this:

char field[32], op[4], value[DESCRIPTION_LEN];
if (parse_condition(argv[2], field, op, value) != 0) {
    fprintf(stderr, "Invalid filter condition.\n");
    return -1;
}

When you're ready, the next step is the filter function itself — which will call parse_condition, then iterate reports.dat comparing each report_t field against the parsed condition. Let me know if you want to tackle that next or adjust anything here first.


Second prompt:

ok perfect, and as for the second function,
int match_condition(Report *r, const char *field, const char *op, const char *value);
      which returns 1 if the record satisfies the condition and 0 otherwise.
let's implement this too. Be mindful of the field types.
typedef struct {
    int reportID;
    char inspectorName[NAME_LEN];
    struct {
        float x, y;
    } coords;
    char issueCategory[ISSUE_CATEGORY_LEN];
    int severityLevel; // 1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char description[DESCRIPTION_LEN];
} report_t;

AI response:

/*
 * match_condition - returns 1 if report r satisfies field op value, 0 otherwise.
 *
 * Numeric fields  (reportID, severityLevel, timestamp, coords.x, coords.y):
 *   all six comparison operators apply: ==  !=  <  >  <=  >=
 *
 * String fields   (inspectorName, issueCategory, description):
 *   only == and != are meaningful; </>/<=/>=  are rejected and return 0.
 *
 * Field name aliases accepted:
 *   "id"          → reportID
 *   "inspector"   → inspectorName
 *   "x" / "lat"  → coords.x
 *   "y" / "lon"  → coords.y
 *   "category"   → issueCategory
 *   "severity"   → severityLevel
 *   "timestamp"  → timestamp
 *   "description"→ description
 */

/* ── internal helpers ─────────────────────────────────────────────────── */

/* apply op to two longs; all six operators */
static int cmp_long(long lhs, const char *op, long rhs)
{
    if (strcmp(op, "==") == 0) return lhs == rhs;
    if (strcmp(op, "!=") == 0) return lhs != rhs;
    if (strcmp(op, "<")  == 0) return lhs <  rhs;
    if (strcmp(op, ">")  == 0) return lhs >  rhs;
    if (strcmp(op, "<=") == 0) return lhs <= rhs;
    if (strcmp(op, ">=") == 0) return lhs >= rhs;
    return 0;
}

/* apply op to two doubles */
static int cmp_double(double lhs, const char *op, double rhs)
{
    if (strcmp(op, "==") == 0) return lhs == rhs;   /* exact float compare —
                                                        fine for filter use */
    if (strcmp(op, "!=") == 0) return lhs != rhs;
    if (strcmp(op, "<")  == 0) return lhs <  rhs;
    if (strcmp(op, ">")  == 0) return lhs >  rhs;
    if (strcmp(op, "<=") == 0) return lhs <= rhs;
    if (strcmp(op, ">=") == 0) return lhs >= rhs;
    return 0;
}

/* apply op to two strings; only == and != are permitted */
static int cmp_string(const char *lhs, const char *op, const char *rhs)
{
    if (strcmp(op, "==") == 0) return strcmp(lhs, rhs) == 0;
    if (strcmp(op, "!=") == 0) return strcmp(lhs, rhs) != 0;

    fprintf(stderr,
            "match_condition: operator \"%s\" is not valid for string fields\n",
            op);
    return 0;
}

/* ── public function ──────────────────────────────────────────────────── */

int match_condition(const report_t *r, const char *field,
                    const char *op,    const char *value)
{
    if (!r || !field || !op || !value) {
        fprintf(stderr, "match_condition: NULL argument\n");
        return 0;
    }

    /* ── numeric fields ─────────────────────────────────────────────── */

    if (strcmp(field, "id") == 0) {
        long rhs = atol(value);
        return cmp_long((long)r->reportID, op, rhs);
    }

    if (strcmp(field, "severity") == 0) {
        long rhs = atol(value);
        if (rhs < 1 || rhs > 3) {
            fprintf(stderr,
                    "match_condition: severity value must be 1, 2 or 3 (got \"%s\")\n",
                    value);
            return 0;
        }
        return cmp_long((long)r->severityLevel, op, rhs);
    }

    if (strcmp(field, "timestamp") == 0) {
        long rhs = atol(value);
        return cmp_long((long)r->timestamp, op, rhs);
    }

    if (strcmp(field, "x") == 0 || strcmp(field, "lat") == 0) {
        double rhs = atof(value);
        return cmp_double((double)r->coords.x, op, rhs);
    }

    if (strcmp(field, "y") == 0 || strcmp(field, "lon") == 0) {
        double rhs = atof(value);
        return cmp_double((double)r->coords.y, op, rhs);
    }

    /* ── string fields ──────────────────────────────────────────────── */

    if (strcmp(field, "inspector") == 0)
        return cmp_string(r->inspectorName, op, value);

    if (strcmp(field, "category") == 0)
        return cmp_string(r->issueCategory, op, value);

    if (strcmp(field, "description") == 0)
        return cmp_string(r->description, op, value);

    /* ── unknown field ──────────────────────────────────────────────── */

    fprintf(stderr, "match_condition: unknown field \"%s\"\n", field);
    return 0;
}
A few things worth calling out:
Type dispatch is explicit, not generic. Rather than casting everything to a string and comparing, each field is matched to its actual C type — long for integer fields, double for the float coordinates, const char * for the character arrays. This avoids the classic pitfall where "9" > "10" lexicographically but 9 < 10 numerically.
String fields reject ordering operators. Allowing description:>:foo would silently use strcmp ordering, which is almost never what a user of this tool intends. cmp_string logs a diagnostic and returns 0 instead.
atol/atof are acceptable here because parse_condition already validated that value is non-empty and the operator is known. If you want stricter rejection of non-numeric input for numeric fields (e.g. severity:==:abc), you can swap them out for strtol/strtod and check errno or the endptr.
timestamp takes a Unix epoch integer — so a user would filter with something like timestamp:>:1700000000. If you want human-readable date strings later, that's a small extension to parse_condition + match_condition (parse the string with strptime, convert to time_t, then compare).
Convenient aliases (lat/lon, id, inspector, category, severity) match the shorthand a user would naturally type in the CLI, while the underlying struct field names stay internal.
When you're ready for the filter command itself, it'll just be: open reports.dat, read each report_t, call parse_condition once, then loop calling match_condition on each record and print matches.