CXX = g++
CXXFLAGS = -std=c++20 -O3 -pthread -Wall

TARGET = raytracer
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.ppm