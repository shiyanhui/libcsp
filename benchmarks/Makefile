CFLAGS := -Wall -O3
WORKING_DIR := build

TARGETS := benchmark_sum benchmark_sum_go

.PHONY: benchmark
benchmark: clean $(TARGETS)

benchmark_sum: sum.c
	@cspcli init --working-dir=$(WORKING_DIR)
	@$(CC) $(CFLAGS) -o $@.o -c $^ -fplugin=libcsp -fplugin-arg-libcsp-working-dir=$(WORKING_DIR)
	@cspcli analyze --working-dir=$(WORKING_DIR) --cpu-cores=$(CPU_CORES)
	@$(CC) $(CFLAGS) -o $@ $@.o $(WORKING_DIR)/config.c -lcsp -pthread
	@cspcli clean --working-dir=$(WORKING_DIR)
	@./$@

benchmark_sum_go:
	@go build -o $@ sum.go
	@./$@

.PHONY: clean
clean:
	@rm -rf $(TARGETS)
