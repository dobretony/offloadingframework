CC=gcc
TARGET=offload-server
COMM_DIR=./comm

SOURCE += ./main.c ./server.c
SOURCE += $(COMM_DIR)/bluetooth.c

## End sources definition
INCLUDE = -I$(COMM_DIR) 
#INCLUDE = -I.$(OTHERDIR)/../inc
## end more includes

LIBS = -lbluetooth

VPATH=$(COMM_DIR)
OBJ=$(join $(addsuffix bin/, $(dir $(SOURCE))), $(notdir $(SOURCE:.c=.o))) 

## Fix dependency destination to be ../.dep relative to the src dir
DEPENDS=$(join $(addsuffix .dep/, $(dir $(SOURCE))), $(notdir $(SOURCE:.c=.d)))

## Default rule executed
all: $(TARGET)
	@true

## Clean Rule
clean:
	@-rm -f $(TARGET) $(OBJ) $(DEPENDS)


## Rule for making the actual target
$(TARGET): $(OBJ)
	@echo "============="
	@echo "Linking the target $@"
	@echo "============="
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo -- Link finished --

## Generic compilation rule
%.o : %.c
	@mkdir -p $(dir $@)
	@echo "============="
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@


## Rules for object files from cpp files
## Object file for each file is put in obj directory
## one level up from the actual source directory.
./bin/%.o : %.c
	@mkdir -p $(dir $@)
	@echo "============="
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Rule for "other directory"  You will need one per "other" dir
$(COMM_DIR)/bin/%.o : %.c
	@mkdir -p $(dir $@)
	@echo "============="
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

## Make dependancy rules
.dep/%.d: %.c
	@mkdir -p $(dir $@)
	@echo "============="
	@echo Building dependencies file for $*.o
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< | sed "s^$*.o^./bin/$*.o^" > $@'

## Dependency rule for "other" directory
$(COMM_DIR)/.dep/%.d: %.c
	@mkdir -p $(dir $@)
	@echo "============="
	@echo Building dependencies file for $*.o
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< | sed "s^$*.o^$(COMM_DIR)/bin/$*.o^" > $@'

## Include the dependency files
-include $(DEPENDS)
