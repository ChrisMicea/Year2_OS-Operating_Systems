#ifndef REPORT_H

#define REPORT_H

#include "commons.h"
#include "utils.h"

int add_report(char* districtID);
int write_report(char* reportFilepath);
int get_next_report_id(int fd);
int list_reports(char* districtID);
int view_specific_report(char* districtID, char* reportID);
int remove_report(char* districtID, char* reportID);

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
int parse_condition(const char *input, char *field, char *op, char *value);

/*
 * match_condition - returns 1 if report 'report' satisfies field op value, 0 otherwise.
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
int match_condition(const report_t *report, const char *field, const char *op, const char *value);

int filter_reports(char *districtID, char **conditions, int num_conditions);

#endif // REPORT_H