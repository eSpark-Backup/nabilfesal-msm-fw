# GenerateBuildInfo.cmake

# --- Git Info ---
execute_process(
    COMMAND git log -1 --pretty=format:%ae
    OUTPUT_VARIABLE GIT_AUTHOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git log -1 --pretty=format:%H
    OUTPUT_VARIABLE GIT_HASH_FULL
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# --- Date/Time ---
string(TIMESTAMP BUILD_DATE "%b %d %Y")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")


# --- Write buildinfo.h ---
file(WRITE ${OUTPUT_FILE} "
#pragma once

#define BUILD_DATE \"${BUILD_DATE}\"
#define BUILD_TIME \"${BUILD_TIME}\"
#define BUILD_AUTHOR \"${GIT_AUTHOR}\"
#define GIT_BRANCH \"${GIT_BRANCH}\"
#define GIT_COMMIT_HASH \"${GIT_HASH_FULL}\"
")
