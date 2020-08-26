CFLAGS := -Wall -O3
WORKING_DIR := build

TARGETS := benchmark_sum_libcsp benchmark_sum_go benchmark_sum_thread

.PHONY: benchmark
benchmark: clean $(TARGETS)

benchmark_sum_libcsp: sum.c
	@cspcli init --working-dir=$(WORKING_DIR)
	@$(CC) $(CFLAGS) -o $@.o -c $^ -fplugin=libcsp -fplugin-arg-libcsp-working-dir=$(WORKING_DIR)
	@cspcli analyze --working-dir=$(WORKING_DIR) --cpu-cores=$(CPU_CORES)
	@$(CC) $(CFLAGS) -o $@ $@.o $(WORKING_DIR)/config.c -lcsp -pthread
	@cspcli clean --working-dir=$(WORKING_DIR)
	@./$@

benchmark_sum_go:
	@go build -o $@ sum.go
	@./$@

benchmark_sum_thread: sum_thread.c
	@$(CC) $(CFLAGS) -o $@ $^ -pthread
	@./$@

.PHONY: clean
clean:
	@rm -rf $(TARGETS)
