.PHONY: clean debug

FLAGS = --std=c++11

BINS = bin/testRM.o bin/testIM.o bin/testSM.o bin/recman.o bin/recPacker.o \
	   bin/fs.o bin/bplus.o bin/IndexHandle.o bin/IndexManager.o \
	   bin/queryman.o bin/miscs.o bin/sql.tab.o bin/sql.lex.o \
	   bin/CompOp.o bin/SystemManager.o bin/indexOps.o create drop

all: main

main: $(BINS)
	g++ bin/*.o -o main $(FLAGS)

bin/main.o: main.cpp test/test.h
	g++ -c $< -o $@ $(FLAGS)

bin/testRM.o: test/testRM.cpp test/test.h recman/RecordManager.h recman/RID.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/testIM.o: test/testIM.cpp test/test.h index/IndexHandle.h index/IndexManager.h index/bplus.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/testSM.o: test/testSM.cpp test/test.h sysman/sysman.h recman/RecordManager.h index/IndexManager.h def.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/recman.o: recman/recman.cpp recman/RecordManager.h recman/RID.h filesystem/bufmanager/BufPageManager.h recman/recMemAllocStrat.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/queryman.o: queryman/queryman.cpp queryman/queryman.h queryman/RelAttr.h queryman/ValueHolder.h queryman/Condition.h
	g++ -c $< -o $@ $(FLAGS)

bin/recPacker.o: recman/recPacker.cpp recman/recPacker.h recman/RID.h utils/type.h
	g++ -c $< -o $@ $(FLAGS)

bin/SystemManager.o: sysman/SystemManager.cpp sysman/sysman.h recman/recPacker.h index/IndexManager.h recman/RecordManager.h sysman/sysman.h CompOp.h queryman/queryman.h
	g++ -c $< -o $@ $(FLAGS)

bin/indexOps.o: queryman/indexOps.cpp queryman/indexOps.h recman/RID.h errors.h def.h index/IndexHandle.h recman/RecordManager.h sysman/sysman.h
	g++ -c $< -o $@ $(FLAGS)

bin/fs.o: filesystem/fs.cpp filesystem/utils/MyBitMap.h
	g++ -c $< -o $@ $(FLAGS)

bin/bplus.o: index/bplus.cpp index/bplus.h index/IndexHandle.h recman/RID.h def.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/IndexHandle.o: index/IndexHandle.cpp index/IndexHandle.h recman/RID.h CompOp.h
	g++ -c $< -o $@ $(FLAGS)

bin/IndexManager.o: index/IndexManager.cpp index/IndexManager.h recman/RID.h errors.h filesystem/bufmanager/BufPageManager.h
	g++ -c $< -o $@ $(FLAGS)

bin/sql.tab.cpp: parse/sql.y queryman/ValueHolder.h 
	# if [ ! -d "bin/src" ] ; then mkdir bin/src; fi
	bison -o $@ $< --defines

bin/sql.lex.yy.cpp: parse/sql.l bin/sql.tab.hpp
	# if [ ! -d "bin/src" ] ; then mkdir bin/src; fi
	flex -o bin/lex.yy.c $< 
	mv bin/lex.yy.c $@

bin/sql.tab.o: bin/sql.tab.cpp sysman/sysman.h def.h utils/type.h parse/parse.h
	g++ -c $< -o $@ $(FLAGS)

bin/sql.lex.o: bin/sql.lex.yy.cpp bin/sql.tab.hpp
	g++ -c $< -o $@ $(FLAGS)

bin/CompOp.o: CompOp.cpp CompOp.h def.h utils/type.h recman/RecordManager.h
	g++ -c $< -o $@ $(FLAGS)

bin/miscs.o: utils/miscs.cpp def.h utils/type.h recman/RID.h
	g++ -c $< -o $@ $(FLAGS)

create: dbman/create.cpp
	g++ $< -o $@ $(FLAGS)

drop: dbman/drop.cpp
	g++ $< -o $@ $(FLAGS)

clean:
	rm bin/*; rm main; rm create; rm drop

debug:
	make FLAGS='$(FLAGS) -g'