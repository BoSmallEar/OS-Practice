CC=g++ -g -Wall

# List of source files for your pager
PAGER_SOURCES=vm_pager.cpp

# Generate the names of the pager's object files
PAGER_OBJS=${PAGER_SOURCES:.cpp=.o}

all: pager app

# Compile the pager and tag this compilation
pager: ${PAGER_OBJS} libvm_pager.o
	./autotag.sh
	${CC} -o $@ $^

# Compile an application program
app: test9.4.cpp libvm_app.o
	${CC} -o $@ $^ -ldl

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${PAGER_OBJS} pager app
