################################################################################
#                                                                               
#                                                                               
#                                                                               
#  Author :  Karhik M                                                           
#  Date   :  20-Nov-2015
#                                                                               
#  History                                                                      
#  ---------------------------------------------------------------------------- 
#  15-Dec-2015   karthik M             Adding Distclean Recipe
#                                                                               
############################################################################### 


# Use 'make V=1' to see the full commands                                       
# Use 'make help' to see Usage                                                  
ifeq ($(V), 1)                                                                  
	S=                                                                           
else                                                                            
	S=@                                                                          
endif 


.PHONY: clean

TARGET_APP= FileSystem

OBJDIR= Objects/
OBJ_NAMES= FileSystem.o
VPATH=Source/  Objects/ Include/

OBJECTS=$(addprefix $(OBJDIR),$(OBJ_NAMES)) 


INCLUDE = -I Include/



all: $(TARGET_APP)



$(TARGET_APP):$(OBJECTS)
	$(S) $(CC)  $^ -o $(TARGET_APP)
	$(S) echo "Compiled successfully"


#For this kind of .o.c rule VPATH need to be there
${OBJDIR}%.o:%.c  FileSystem.h
	$(S) $(CC)  $(INCLUDE) $(CFLAGS) -c $< -o ${OBJDIR}$*.o 
	$(S) echo "GEN   $(PWD)/$@"



clean:
	rm -rf $(OBJECTS) $(TARGET_APP)

distclean:
	rm -f `find . -iname 'cscope.*' -o -iname 'tags' -type f`
