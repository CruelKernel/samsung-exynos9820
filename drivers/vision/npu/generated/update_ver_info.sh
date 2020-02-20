#!/bin/sh

BASE_PATH=drivers/vision/npu
TARGET_FILE=$BASE_PATH/generated/npu-ver-info~
NPU_GIT_LOG=`git log --oneline -5 ${BASE_PATH}|cut --bytes=-60|sed 's/[\"%]/\\\\&/g'|awk '{print "\""$0"\\\n\""}'`
NPU_GIT_HASH=`git log --oneline -5 --format=format:%H ${BASE_PATH}|cut --bytes=-60|sed 's/[\"%]/\\\\&/g'|awk '{print "\""$0"\\\n\""}'`
NPU_GIT_LOCAL_CHANGE="$(git diff --shortstat)"
if [ -z "$NPU_GIT_LOCAL_CHANGE" ]
then
	NPU_GIT_LOCAL_CHANGE="No local change"
fi
STASH_DEPTH=`git stash list | wc -l`
USER_INFO=whoami|sed 's/\\/\-/g'

# Error checking
if [ ( -z $NPU_GIT_LOG ) -o ( -z $NPU_GIT_HASH ) -o ( -z $USER_INFO ) -o ( -z $NPU_GIT_LOCAL_CHANGE ) -o ( -z $STASH_DEPTH ) ]
then
	echo "An error occured during build info gathering." >&2
	exit 16
fi

BUILD_INFO="$(USER_INFO)@$(hostname) / Build on $(date --rfc-3339='seconds')"

cat > $TARGET_FILE << ENDL
const char *npu_git_log_str =
${NPU_GIT_LOG}
"${NPU_GIT_LOCAL_CHANGE} / Stash depth=${STASH_DEPTH}";
const char *npu_git_hash_str =
${NPU_GIT_HASH}
"${NPU_GIT_LOCAL_CHANGE} / Stash depth=${STASH_DEPTH}";
const char *npu_build_info =
"${BUILD_INFO}";
ENDL
