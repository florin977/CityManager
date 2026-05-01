#include "../../include/filter_utils.h"

/**
 * Helper function to copy characters while resolving escape sequences.
 * Converts "\:" to ":" and "\\" to "\".
 */
void copy_unescaped(char *dest, const char *start, size_t raw_len) {
  size_t i = 0;
  size_t j = 0;

  while (i < raw_len) {
    if (start[i] == '\\' && i + 1 < raw_len &&
        (start[i + 1] == ':' || start[i + 1] == '\\')) {
      dest[j++] = start[i + 1];
      i += 2;
    } else {
      dest[j++] = start[i++];
    }
  }
  dest[j] = '\0'; // Null-terminate the destination
}

/**
 * Parses a string formatted as "field:op:value" into its three components.
 * Checks all lengths against MAX limits, ensuring space for null terminators.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
  if (!input || !field || !op || !value) {
    return -1;
  }

  const char *p = input;
  const char *colon1 = NULL;
  const char *colon2 = NULL;

  // 1. Scan for unescaped colons
  while (*p) {
    if (*p == '\\') {
      if (*(p + 1) == ':' || *(p + 1) == '\\') {
        p += 2;
        continue;
      }
    } else if (*p == ':') {
      if (!colon1) {
        colon1 = p;
      } else if (!colon2) {
        colon2 = p;
        break;
      }
    }
    p++;
  }

  if (!colon1 || !colon2) {
    return -1;
  }

  // 2. Calculate raw lengths
  size_t raw_field_len = colon1 - input;
  size_t raw_op_len = colon2 - (colon1 + 1);
  size_t raw_val_len = strlen(colon2 + 1);

  // 3. Strict length checks
  // The raw length must be strictly less than the max size to leave
  // room for the null terminator.
  if (raw_field_len >= MAX_FIELD_LENGTH || raw_op_len >= MAX_OP_LENGTH ||
      raw_val_len >= MAX_VAL_LENGTH) {
    return -1;
  }

  // 4. Extract and unescape the components
  copy_unescaped(field, input, raw_field_len);
  copy_unescaped(op, colon1 + 1, raw_op_len);
  copy_unescaped(value, colon2 + 1, raw_val_len);

  return 0; // Success
}

/**
 * Compares two long long integers based on a string operator.
 */
int compare_numeric(long long left, long long right, const char *op) {
  if (strcmp(op, "==") == 0)
    return left == right;
  if (strcmp(op, "!=") == 0)
    return left != right;
  if (strcmp(op, "<") == 0)
    return left < right;
  if (strcmp(op, "<=") == 0)
    return left <= right;
  if (strcmp(op, ">") == 0)
    return left > right;
  if (strcmp(op, ">=") == 0)
    return left >= right;
  return 0; // Unknown operator
}

/**
 * Compares two strings using strcmp based on a string operator.
 */
int compare_string(const char *left, const char *right, const char *op) {
  int cmp = strcmp(left, right);
  if (strcmp(op, "==") == 0)
    return cmp == 0;
  if (strcmp(op, "!=") == 0)
    return cmp != 0;
  if (strcmp(op, "<") == 0)
    return cmp < 0;
  if (strcmp(op, "<=") == 0)
    return cmp <= 0;
  if (strcmp(op, ">") == 0)
    return cmp > 0;
  if (strcmp(op, ">=") == 0)
    return cmp >= 0;
  return 0; // Unknown operator
}

/**
 * Parses a string into a time_t. Supports ISO 8601 (YYYY-MM-DDTHH:MM:SS)
 * or raw Unix epoch integers.
 */
time_t parse_time(const char *value) {
  struct tm t = {0};

  // Try to parse as ISO 8601 Date/Time string
  if (sscanf(value, "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday,
             &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {

    t.tm_year -= 1900; // tm_year is years since 1900
    t.tm_mon -= 1;     // tm_mon is 0-11
    t.tm_isdst = -1;   // Let mktime determine Daylight Saving Time

    time_t parsed_time = mktime(&t);
    if (parsed_time != (time_t)-1) {
      return parsed_time;
    }
  }

  // Fallback: Try parsing as a raw integer (Unix Epoch)
  char *endptr;
  long long epoch = strtoll(value, &endptr, 10);
  if (*value != '\0' && *endptr == '\0') {
    return (time_t)epoch;
  }

  return (time_t)-1; // Parsing completely failed
}

// --- Main Function ---

/**
 * Evaluates whether a REPORT_DATA record satisfies a given condition.
 * @return 1 if condition is met, 0 if not met or on error (invalid
 * field/conversion).
 */
int match_condition(REPORT_DATA *r, const char *field, const char *op,
                    const char *value) {
  if (!r || !field || !op || !value) {
    return 0;
  }

  // 1. Handle "severity" (Integer)
  if (strcmp(field, "severity") == 0) {
    char *endptr;
    long sev_val = strtol(value, &endptr, 10);

    // Check if conversion succeeded entirely (no trailing characters)
    if (*value == '\0' || *endptr != '\0') {
      return 0;
    }
    return compare_numeric((long long)r->severity_level, (long long)sev_val,
                           op);
  }

  // 2. Handle "timestamp" (time_t)
  else if (strcmp(field, "timestamp") == 0) {
    time_t time_val = parse_time(value);
    if (time_val == (time_t)-1) {
      return 0; // Conversion failed
    }
    return compare_numeric((long long)r->timestamp, (long long)time_val, op);
  }

  // 3. Handle "category" (String)
  else if (strcmp(field, "category") == 0) {
    return compare_string(r->issue_category, value, op);
  }

  // 4. Handle "inspector" (String)
  else if (strcmp(field, "inspector") == 0) {
    return compare_string(r->username, value, op);
  }

  // Unknown field
  return 0;
}
