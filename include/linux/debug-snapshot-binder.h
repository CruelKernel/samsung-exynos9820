#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
#ifndef DEBUG_SNAPSHOT_BINDER_H
#define DEBUG_SNAPSHOT_BINDER_H

#define TASK_COMM_LEN 16

enum binder_trace_type {
	TRANSACTION = 1,
	TRANSACTION_DONE,
	TRANSACTION_ERROR,
};
struct trace_binder_transaction_base {
	int trace_type;
	int transaction_id;
	int from_pid;
	int from_tid;
	int to_pid;
	int to_tid;
	char from_pid_comm[TASK_COMM_LEN];
	char from_tid_comm[TASK_COMM_LEN];
	char to_pid_comm[TASK_COMM_LEN];
	char to_tid_comm[TASK_COMM_LEN];
};
struct trace_binder_transaction {
	int to_node_id;
	int reply;
	unsigned int flags;
	unsigned int code;
};
struct trace_binder_transaction_error {
	unsigned int return_error;
	unsigned int return_error_param;
	unsigned int return_error_line;
};
#endif /* DEBUG_SNAPSHOT_BINDER_H */
#endif /* CONFIG_DEBUG_SNAPSHOT_BINDER */
