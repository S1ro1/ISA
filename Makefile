CXX = g++
CXXFLAGS = -g -std=c++17
LDFLAGS = -pthread

# locations
UTILS_DIR = utils
BIN_DIR = bin
OBJ_DIR = obj

# sources
UTILS_SOURCES = $(wildcard $(UTILS_DIR)/*.cpp)
SERVER_SOURCES = server.cpp $(UTILS_SOURCES)
CLIENT_SOURCES = client.cpp $(UTILS_SOURCES)

# objects
SERVER_OBJECTS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SERVER_SOURCES)))
CLIENT_OBJECTS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(CLIENT_SOURCES)))

# executables
SERVER_EXEC = $(BIN_DIR)/tftp-server
CLIENT_EXEC = $(BIN_DIR)/tftp-client

all: $(BIN_DIR) $(OBJ_DIR) $(SERVER_EXEC) $(CLIENT_EXEC)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(SERVER_EXEC): $(SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_EXEC): $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(UTILS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# cleanup
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(SERVER_EXEC) $(CLIENT_EXEC) $(BIN_DIR) $(OBJ_DIR)
