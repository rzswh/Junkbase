.PHONY: clean debug

FLAGS = --std=c++11

BINS = bin/main.o bin/testRM.o bin/testIM.o bin/recman.o bin/fs.o bin/bplus.o bin/IndexHandle.o bin/IndexManager.o create drop

all: main

main: $(BINS)
	g++ bin/*.o -o main $(FLAGS)

bin/main.o: main.cpp test/test.h
	g++ -c $< -o $@ $(FLAGS)

bin/testRM.o: test/testRM.cpp test/test.h recman/RecordManager.h recman/RID.h
	g++ -c $< -o $@ $(FLAGS)

bin/testIM.o: test/testIM.cpp test/test.h index/IndexHandle.h index/IndexManager.h index/bplus.h
	g++ -c $< -o $@ $(FLAGS)

bin/recman.o: recman/recman.cpp recman/RecordManager.h recman/RID.h filesystem/bufmanager/BufPageManager.h
	g++ -c $< -o $@ $(FLAGS)

bin/fs.o: filesystem/fs.cpp filesystem/utils/MyBitMap.h
	g++ -c $< -o $@ $(FLAGS)

bin/bplus.o: index/bplus.cpp index/bplus.h index/IndexHandle.h recman/RID.h def.h
	g++ -c $< -o $@ $(FLAGS)

bin/IndexHandle.o: index/IndexHandle.cpp index/IndexHandle.h recman/RID.h
	g++ -c $< -o $@ $(FLAGS)

bin/IndexManager.o: index/IndexManager.cpp index/IndexManager.h recman/RID.h errors.h filesystem/bufmanager/BufPageManager.h
	g++ -c $< -o $@ $(FLAGS)

create: dbman/create.cpp
	g++ $< -o $@ $(FLAGS)

drop: dbman/drop.cpp
	g++ $< -o $@ $(FLAGS)

clean:
	rm bin/*.o main create drop

debug:
	make FLAGS='$(FLAGS) -g'