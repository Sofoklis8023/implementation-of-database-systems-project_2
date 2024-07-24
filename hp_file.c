#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

//Συνάρτηση που επιστρέφει το HP_INFO με τα μεταδεδομένα του αρχείου.
HP_info* get_first_block(int file_desc){
  BF_Block * block;
  BF_Block_Init(&block);
  if(BF_GetBlock(file_desc,0,block) == BF_OK){
    void * data;
    data = BF_Block_GetData(block);
    HP_info *info;
    info = data;
    return info;
  }
  BF_Block_Destroy(&block);
}


//Συνάρτηση που δημιουργεί ένα νέο μπλόκ και προσθέτει μια εγγραφή.
int Create_Block_and_insert(int file_desc,Record record,int blocks_num){
  BF_Block *block;
  BF_Block_Init(&block);
  if(BF_AllocateBlock(file_desc, block) == BF_OK){
    void* data;
    data = BF_Block_GetData(block);
    Record* rec = data; 
    memcpy((char*)data,&record,sizeof(Record));
    HP_block_info *block_info;
    block_info = data;
    block_info->block_id = blocks_num + 1;
    block_info->num_records = 1;
    memcpy((char*)data +63* sizeof(block_info),&block_info,sizeof(block_info));
    HP_info *info = get_first_block(file_desc);
    info->id++;
    BF_Block_SetDirty(block);
    if(BF_GetBlock(file_desc,0,block) == BF_OK){
      BF_Block_Destroy(&block);
    }
  }
 
}

//Συνάρτηση που προσθέτει μια εγγραφή σε ένα ήδη υπάρχων μπλόκ.
void Insert_on_excisted_block(int file_desc,Record record,BF_Block *block,HP_block_info *info){
  void* data;
  data = BF_Block_GetData(block);
  Record* rec = data;
  memcpy((char*)data + info->num_records * sizeof(Record),&record,sizeof(Record));
  info->num_records ++;
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block); 
}



int HP_CreateFile(char *fileName){  
  if(BF_CreateFile(fileName) == BF_OK){
    int file_desc;
    if(BF_OpenFile(fileName, &file_desc) == BF_OK){
      int  blocks_num;
      if(BF_GetBlockCounter(file_desc, &blocks_num) == BF_OK){
        BF_Block *block;
        BF_Block_Init(&block);
        if(BF_AllocateBlock(file_desc, block) == BF_OK){
          void *data = BF_Block_GetData(block);
          HP_info *info;
          info = data;
          info->id = 0;
          info->rec_num = BF_BLOCK_SIZE/REC_SIZE;
          info[0] = *info;
        }
        BF_Block_Destroy(&block); 
      }
    } 
    return 0;
  } else return -1;
  

}

HP_info* HP_OpenFile(char *fileName,int *file_desc){
  if(BF_OpenFile(fileName,file_desc) == BF_OK){
    return get_first_block(*file_desc);
  }
  return NULL ;
}


int HP_CloseFile(int file_desc, HP_info* header_info){
  BF_Block *block;
  BF_Block_Init(&block);
  for(int i=0;i<header_info->id;i++){
    BF_GetBlock(file_desc,i,block);
    BF_UnpinBlock(block);
  }
  
  BF_Block_Destroy(&block);

  if(BF_CloseFile(file_desc)==BF_OK){  
    return 0;
  }
  return -1;
}

int HP_InsertEntry(int file_desc, HP_info* header_info, Record record){
    if(header_info->id == 0){ //Περίπτωση που στο αρχείο υπάρχει μόνο το π΄ρωτο μπλόκ που περιέχει μεταδεδομένα.
        Create_Block_and_insert(file_desc,record,header_info->id);
        return header_info->id;
    }else{
        BF_Block * block;
        BF_Block_Init(&block);
        if(BF_GetBlock(file_desc,header_info->id,block) == BF_OK){
          void* data;
          data = BF_Block_GetData(block);
          HP_block_info *info1;
          info1 = data;
          if(info1->num_records >= header_info->rec_num){//Περίπτωση που η εγγραφή δεν χωράει στο μπλόκ (δημιουργία νέου και εισαγωγή σε αυτο).
            BF_UnpinBlock(block);
            Create_Block_and_insert(file_desc,record,header_info->id);
            BF_Block_Destroy(&block); 
            return header_info->id;
          }else{//Περίπτωση που η εγγραφή χωράει στο μπλόκ και απλα εισάγεται σε αυτο.
            Insert_on_excisted_block(file_desc,record,block,info1);
            BF_Block_Destroy(&block); 
            return header_info->id;
          }
        }
              
    }   
  
  return -1;
} 


    


int HP_GetAllEntries(int file_desc, HP_info* header_info, int value){
  int blocks_num;
  BF_Block * block;
  BF_Block_Init(&block);
  if(BF_GetBlockCounter(file_desc,&blocks_num) == BF_OK){
    for(int i=1;i<blocks_num;i++){//Προσπέλαση όλων των μπλόκ μέχρι να βρεθεί αυτό που ψάχνουμε, τότε επιστρέφουμε τον αριθμό των μπλόκ που διαβάστηκαν.
      BF_GetBlock(file_desc,i,block);
      void *data = BF_Block_GetData(block);
      Record* rec = data;
      HP_block_info* info1 = data;
      for(int j=0;j<6;j++){
        if(rec[j].id == value){
          printRecord(rec[j]);
          return i;
        }
      }
      BF_UnpinBlock(block);
    }
  }
  BF_Block_Destroy(&block);
  return -1;
}