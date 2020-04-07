TARGETS := test_chan test_corepool test_mem test_proc test_rand test_rbq \
	test_rbtree test_runq test_timer

SRC := ../src

define test_module
	@$(CC) -o $@ $^ -pthread -fomit-frame-pointer
	@./$@
	@echo $< passed!
endef

.PHONY: test
test: clean $(TARGETS)

test_chan: chan.c $(SRC)/chan.h
	$(test_module)

test_corepool: corepool.c
	$(test_module)

test_mem: mem.c $(SRC)/rand.c
	$(test_module)

test_proc: proc.c
	$(test_module)

test_rand: rand.c $(SRC)/rand.h $(SRC)/rand.c
	$(test_module)

test_rbq: rbq.c $(SRC)/rbq.h
	$(test_module)

test_rbtree: rbtree.c $(SRC)/rbtree.h
	$(test_module)

test_runq: runq.c
	$(test_module)

test_timer: timer.c $(SRC)/timer.h
	$(test_module)

clean:
	@rm -rf $(TARGETS)
