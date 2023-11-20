CXX = g++
CXXFLAGS = -g -std=c++17 -I$(UTILS_DIR) -I$(TFTP_DIR)
LDFLAGS = -pthread

# locations
SRC_DIR = src
UTILS_DIR = $(SRC_DIR)/utils
TFTP_DIR = $(SRC_DIR)/tftp
BIN_DIR = .
OBJ_DIR = obj
OBJ_UTILS_DIR = $(OBJ_DIR)/utils
OBJ_TFTP_DIR = $(OBJ_DIR)/tftp

# sources
UTILS_SOURCES = $(wildcard $(UTILS_DIR)/*.cpp)
TFTP_SOURCES = $(wildcard $(TFTP_DIR)/*.cpp)
SERVER_SOURCES = $(SRC_DIR)/tftp-server.cpp $(UTILS_SOURCES) $(TFTP_SOURCES)
CLIENT_SOURCES = $(SRC_DIR)/tftp-client.cpp $(UTILS_SOURCES) $(TFTP_SOURCES)

# objects
SERVER_OBJECTS = $(SERVER_SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# executables
SERVER_EXEC = $(BIN_DIR)/tftp-server
CLIENT_EXEC = $(BIN_DIR)/tftp-client

all: $(BIN_DIR) $(OBJ_DIR) $(OBJ_UTILS_DIR) $(OBJ_TFTP_DIR) $(SERVER_EXEC) $(CLIENT_EXEC)

$(BIN_DIR) $(OBJ_DIR) $(OBJ_UTILS_DIR) $(OBJ_TFTP_DIR):
	mkdir -p $@

$(SERVER_EXEC): $(SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_EXEC): $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(UTILS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TFTP_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# cleanup
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(SERVER_EXEC) $(CLIENT_EXEC) $(OBJ_DIR)
