TARGET:=ut_qspiram.exe
SUBTARGETS=ut_qspiram.o ../green-thread.o

CXXFLAGS:= -I . -I ..

all: $(TARGET)

$(TARGET): $(SUBTARGETS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(SUBTARGETS)