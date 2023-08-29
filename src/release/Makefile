all: brie utest

CXXFLAGS += -g -O2

../briebase.yy.cpp: ../briebase.l
	flex -t ../briebase.l > $@

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS)  $< -o $@ -MMD

brie: $(OBJS) briebase.yy.o $(APP_MAIN)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(APP_LIBS) && cp $@ $(PROJECT_HOME)/bin/.

utest: $(UTEST_MAIN) $(TEST_OBJS) $(OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(TEST_LIBS)

.PHONY: run
run: brie
	./brie

.PHONY: test
test: utest
	./utest

clean:
	rm -rf *.o
	rm -rf *.d
	rm -rf *.a
	rm -rf utest
	rm -rf brie

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

