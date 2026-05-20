# AI usage for the "city manager" project
## PHASE 1
I used Gemini 3.1 Pro for this project.
The first prompt that I wrote was:
"You are an expert engineer and programmer in the C99 language and operating systems. I want you to write a function with the following form:
int parse_condition(const char *input, char*field, char *op, char*value);
This function will break the input char (that is formatted like filed:op:value) into those 3 parts and return them using the char * from the function declaration."

It returned a pretty good response, but I found some things that could have been better, therefore I used the following prompt:
"modify the function to check against a MAX_FILTER_FIELD_LENGTH constant and return -1 if the input is not correct. Also, modify it so it handles escaped ':' (so 20\:30\:10:==:time will take the last 2 ':', not the first 2).
Lastly, make sure that the former modification does not cause errors when using '\' (so they also need to be escaped like '\\')."

I was not specific enough and it failed to check all of the fields' length. I clarified with:
"Check the MAX_LENGTH against the op and val lengths alongside the field length. Also, the length should be strictly smaller than the max, so there is an extra space for the null termiantor in char *field, op and val."

The functions were good enough, so I went on to the next function with this prompt:
"Next we will implement the following function:
int match_condition(REPORT_DATA *r, const char*field, const char *op, const char*value);
which returns 1 if the record satisfies the condition and 0 otherwise.

Supported fields: severity, category, inspector, timestamp.
Supported operators: ==, !=, <, <=, >, >=

Keep in mind that value is a char*, therefore you will need to convert it to the needed datatype and check if the conversion worked.
Severity can be 1,2 or 3.
category can be anything.
inspector can also be anything (a username, mostly)
timestamp is of type time_t in my implementation."

The result was already good, so I went with it after carefully reviewing it.

## PAHSE 2
Not a lot of AI usage in this phase, I think I used it to correct a few bugs in the code, but not major ones.

## PHASE 3
In this phase I used Gemini to brainstorm ideas about the calculate_scores. I first wanted to use a shell script and pipes to get the Inspector name and severity of the report and then use that to calculate the score, but I found out that it was not that good of an idea due to the difficulty/having to also use a .sh file in an only C project. I then asked Gemini for a better solution and it suggested calculating everything in C, which is what I ended up doing. I also used Gemini to help me with the implementation, it came up with the idea of using lists to store the inspectors and their scores. it also suggested using 64 pair of pipes to calculate the scores in parallel. It was able to give me a working implementation, modifying my one pair of pipes implementation to the 64 pair of pipes one.
It also generated the CLI (meaning the actual printf("[HUB_MON]") and so on.)
