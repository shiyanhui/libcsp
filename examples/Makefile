CFLAGS := -Wall -O3
WORKING_DIR := build

TARGETS := async chan infinity long_params mutex netpoll producer_consumer \
	select sleep sync timer

EXAMPLE ?= sync

$(EXAMPLE): clean
	@cspcli init --working-dir=$(WORKING_DIR)
	@$(CC) $(CFLAGS) -o $(EXAMPLE).o -c $(EXAMPLE).c -fplugin=libcsp -fplugin-arg-libcsp-working-dir=$(WORKING_DIR)
	@cspcli analyze --working-dir=$(WORKING_DIR)
	@$(CC) $(CFLAGS) -o $(EXAMPLE) $(EXAMPLE).o $(WORKING_DIR)/config.c -lcsp -pthread
	@cspcli clean --working-dir=$(WORKING_DIR)
	./$(EXAMPLE)

.PHONY: clean
clean:
	@rm -rf $(TARGETS) || true
