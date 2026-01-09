CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra
TARGET = trip_analyzer
SOURCE = trip_analyzer.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: all clean

