#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}
int global_array[4]; //Πίνακας που στην πρώτη θέση κρατάει το global_depth και στην δεύτερη το size του πίνακα κατακερματισμού
index_values indexVal[MAX_OPEN_FILES];

int insert_rec_onBlock(Record record,void* bucket_pointer){ //Συνάρτηση που εισάγει μια εγγραφή σε ένα μπλόκ.
  MetaData *meta = bucket_pointer;
  
  int i = meta[63].rec_num;
  if(i<8){
    Record* rec = bucket_pointer;
    Record *array = malloc(sizeof(Record)*8);
    
    
    memset(array,0,sizeof(Record)*8);
    for(int j =0;j<i;j++){
      strcpy(array[j].city,rec[j].city);
      strcpy(array[j].name,rec[j].name);
      strcpy(array[j].surname,rec[j].surname);
      array[j].id = rec[j].id;
    }
    
    strcpy(array[i].city,record.city);
    strcpy(array[i].name,record.name);
    strcpy(array[i].surname,record.surname);
    array[i].id = record.id;
   
    memcpy(rec,array,sizeof(Record)*8);
    meta[63].rec_num++;
    free(array);
    
    return 1;
  }else return 0;
}





int square(int num,int j){ //Συνάρτηση για ύψωση σε δύναμη.
    int sum = 2;
    for(int i=1;i<j;i++){
        sum = sum*num;
    }
    return sum;
}

char* check_hash(char* hashing,char* value,int global_depth){//Συνάρτηση που ελέγχει αν δύο strings κάνουν hash.
  int check = 0;
  char* error = "ERROR";
  for(int j = 0; j<global_depth;j++){
    if(hashing[j] != value[j]){
      check = 1;
      break;
    }
  }        
  if(check == 0){
    return hashing;
  }else return error;    
}

void insert_metadata(void* data){//Συνάρτηση που εισάγει μεταδεδομένα σε ένα bucket. 
  MetaData *metadata;
  metadata = data;
  metadata[63].local_depth = 1;
  metadata[63].rec_num = 0;
}

int check_if_buddies(char* string1,char* string2,int depth){//Συνάρτηση που ελ΄έγχει άν δύο values είναι φιλαράκια.
  for(int i=0;i<depth;i++){
    if(string1[i] != string2[i]){
      return 1;
    }
  }
  return 0;
}

hash_values ** find_buddies(char* string,int local_depth,int global_depth,int indexDesc,hash_values **hash){//Συνάρτηση που επιστρέφει τα φιλάρακια του string.
  int size = square(2,global_depth);
 
    
    char *string2;
    char* empty = "EMPTY";


      hash_values *extend_array,*hash_of_pos;


      int i=1;
      int counter_buddies=0;
      int block_number_start = (i/30); //30 είναι ο αριθμός των hash_values που βάζουμε σε ένα μπλόκ του πίνακα κατακερματισμού.
      int counter =0;
      while(counter<indexVal[indexDesc].number_of_extend_array_blocks){
        void* data1 = BF_Block_GetData(indexVal[indexDesc].blocks[counter]);
        extend_array = data1;
        for(int i=1;i<=30;i++){
          if(extend_array[i].pointer!=0){
            string2 = extend_array[i].value;
            if(check_if_buddies(string,string2,1) == 0){
              strcpy(hash[counter_buddies]->value,string2);
              hash[counter_buddies]->pointer = extend_array[i].pointer;
              counter_buddies++;
            }
          }
        }
        counter++;
      }
    return hash;
}

int find_where_is_buddie(char* buddie_value,int indexDesc){//Συνάρτηση που βρίσκει το bucket και αλλάζει τους pointers του μετά απο το split.
  hash_values *hash;
  int found=0;
  int blocks_num;
  BF_GetBlockCounter(indexVal[indexDesc].filedesc,&blocks_num);
  for(int i=0;i<indexVal[indexDesc].number_of_extend_array_blocks;i++){
    void* data = BF_Block_GetData(indexVal[indexDesc].blocks[i]);
    hash = data;
    for(int j=1;j<=30;j++){
      if(hash[j].pointer!=0){
        if(strcmp(buddie_value,hash[j].value)==0){
          hash[j].pointer = blocks_num -1;
          found=1;
          BF_Block_SetDirty(indexVal[indexDesc].blocks[i]);
          return found;
        }
      }
    }
  }
  return found;
}





int get_index_val_i(int filedesc){//Συνάρτηση που με δεδομένο το filedesc επιστρέφει τον αριθμό του ανοιχτού αρχείου που το περιέχει.
  for(int i=0;i<20;i++){
    if(indexVal[i].filedesc == filedesc){
      return i;
    }
  }
}

BF_Block* Create_Block(int filedesc){//Συνάρτηση που δημιουργεί δεσμεύει και επιστρέφει ένα μπλόκ για δεδομένο αρχείο.
  BF_Block *block;
  BF_Block_Init(&block);
  if(BF_AllocateBlock(filedesc,block) == BF_OK){
    int blocks_num;
    BF_GetBlockCounter(filedesc,&blocks_num);
    return block;
  }
}


char* intToBinaryDepthBits(int num,int depth) {//Συνάρτηση που δέχεται έναν ακέραιου και επιστρέφει το binary representation του σε depth bits.
  static char binary[12];
  for (int i = depth -1; i >= 0; i--) {
    binary[i] = (num & 1) + '0'; 
    num >>= 1; 
  }
  return binary;
}



results add_block_on_double_array(int indexdesc,int new_depth,hash_values *array,int i,int k,int blocks_num,hash_values *hash,int* hash_before){
  int l =1;
  int max = BF_BLOCK_SIZE/sizeof(hash_values);
  memset(array,0,sizeof(hash_values)*indexVal[indexdesc].extend_array_size);
  while(l<max-1 && i<indexVal[indexdesc].extend_array_size){
    strcpy(array[l].value,intToBinaryDepthBits(i,new_depth));
    if(i%2==0){
      array[l].pointer = hash_before[k];
    }
    else{
      array[l].pointer = hash_before[k];
      k++;
    }
    
    l++;
    i++;
  }
  array[max-1].pointer = blocks_num;
  char *string = "EMPTY";
  strcpy(array[max-1].value,string);

  results res;
  res.k = k;
  res.times = i;

  if(i<indexVal[indexdesc].extend_array_size){
    memcpy(hash,array,sizeof(hash_values)*max);
  
    return res;
  }
  else{
    array[max-1].pointer = -1;

    memcpy(hash,array,sizeof(hash_values)*max);
    return res;
  }
}



void double_array(int indexdesc,int size){//Συνάρτηση που διπλασιάζει τον πίνακα κατακερματισμού.
  int max = BF_BLOCK_SIZE/sizeof(hash_values);
  if(size*2<=max-2){
    BF_Block* extend_array;
    extend_array = indexVal[indexdesc].blocks[0];
    void *data1 = BF_Block_GetData(extend_array);
    hash_values* hash1;
    hash1 = data1;
    int size = indexVal[indexdesc].extend_array_size;
    int depth = indexVal[indexdesc].global_depth;
    int new_depth = indexVal[indexdesc].global_depth+1;
    int new_size = size*2;
    indexVal[indexdesc].extend_array_size = new_size;
    indexVal[indexdesc].global_depth = new_depth;

    
    hash_values* array = malloc(sizeof(hash_values)*max);
    memset(array,0,sizeof(hash_values)*max);
    int k =1;
    int l =0;
    for(int i=1;i<=new_size;i++){
      strcpy(array[i].value,intToBinaryDepthBits(l,new_depth));
      if(i%2==1){
        array[i].pointer = hash1[k].pointer;
      }
      else{
        array[i].pointer = hash1[k].pointer;
        k++;
      }
      l++;
    }
    char *string = "EMPTY";
    strcpy(array[max-1].value,string);
    array[max-1].pointer = -1;
    memcpy(hash1,array,sizeof(hash_values)*max);
    free(array);

    BF_Block_SetDirty(indexVal[indexdesc].blocks[0]);
  }
else{

    void *data1;
    hash_values *hash;



    
    int size = indexVal[indexdesc].extend_array_size;
    int depth = indexVal[indexdesc].global_depth;

    int new_depth = depth+1;
    int new_size = size*2;

    int *hash_before = malloc(sizeof(int)*size);
    
   
    hash_values* array = malloc(sizeof(hash_values)*new_size);
    memset(array,0,sizeof(hash_values)*new_size);

    int k =0;
    int i = 0;
    indexVal[indexdesc].global_depth=new_depth;
    indexVal[indexdesc].extend_array_size = new_size;
    

    int number_extend_before=indexVal[indexdesc].number_of_extend_array_blocks;
    results result;
    result.times =0;
    result.k =0;
    int counter = 0;
    int blocks_num;
    BF_GetBlockCounter(indexVal[indexdesc].filedesc,&blocks_num);
    int count=0;
   


    int hash_count=0;
    int j=0;
    int p;
    while(hash_count<indexVal[indexdesc].number_of_extend_array_blocks){
      
      data1 = BF_Block_GetData(indexVal[indexdesc].blocks[hash_count]);
      hash = data1;
      p=1;
      while(p<=30){
        if(hash[p].pointer){
          hash_before[j] = hash[p].pointer;
          p++;
          j++;
        }else break;
      }
      hash_count++;
    }

    int hash_before_count =0;
    while(count<indexVal[indexdesc].number_of_extend_array_blocks){
      
      data1 = BF_Block_GetData(indexVal[indexdesc].blocks[count]);
      hash = data1;

      
      
      if(hash[31].pointer!=-1){
        blocks_num = hash[31].pointer;
      }
      else{
        BF_GetBlockCounter(indexVal[indexdesc].filedesc,&blocks_num);
      }
      

      result = add_block_on_double_array(indexdesc,new_depth,array,result.times,result.k,blocks_num,hash,hash_before);
      BF_Block_SetDirty(indexVal[indexdesc].blocks[count]);
      
     
      
     
      count++;
    }

    int n = count;

    
    
    
    char *string = "EMPTY";
    
    

    while(result.times < new_size){
      BF_Block *block = Create_Block(indexVal[indexdesc].filedesc);
      indexVal[indexdesc].number_of_extend_array_blocks++;
      
      BF_GetBlockCounter(indexVal[indexdesc].filedesc,&blocks_num);
      indexVal[indexdesc].blocks[count] = block;
      void *data = BF_Block_GetData(block);

     
      hash_values *hash2 = data;


        result = add_block_on_double_array(indexdesc,new_depth,array,result.times,result.k,blocks_num,hash2,hash_before);
        BF_Block_SetDirty(block);
        BF_Block_SetDirty(indexVal[indexdesc].blocks[count]);
     
     
      
    

        if(result.times >= new_size){
          break;
        }

      count++;
      
      
      
      
    }

    free(array);
  free(hash_before);
 
  }
}

char* intToBinary8Bits(int num) {//Συνάρτηση που επιστρέφει το σε string[8] την δυαδική μουρφή ενός αριθμού.
  static char binary[12]; 
  for (int i = 0; i <= 11; i++) {
    binary[i] = (num & 1) + '0'; 
    num >>= 1; 
  }
  return binary;
}


char* hash_function(int num){//Συνάρτηση κατακερματισμού.
  if (num > 4095) {
    int x = num-4096;
    hash_function(x);
  }else{    
    static char binaryString[12]; 
    for (int i = 11; i >= 0; i--) {
      if ((num >> i) & 1) {
        binaryString[11 - i] = '1';
      }else{
        binaryString[11 - i] = '0';
      }
    }
    return binaryString;
  }
}



int get_filedesc(int indexDesc){
  for(int i=0;i<20;i++){
    if(indexDesc == i){
      return indexVal[i].filedesc;
    }
  }
}




HT_ErrorCode HT_Init() {
  global_array[2] = 0; //Πόσα αρχεία είναι ανοιχτά
  for(int i=0;i<MAX_OPEN_FILES;i++){
    indexVal[i].blocks_num = 0; //Πόσα blocks έχει το αρχείο.
    indexVal[i].filedesc = -1;  //Filedesc του αρχείου.
    indexVal[i].extend_array_size = 0;  //Μ΄έγεθος πίνακα κατακερματισμού.
    indexVal[i].number_of_extend_array_blocks=0; //Μπλόκς του πίνακα κατακερματισμού.
  }
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  global_array[0] =  depth;
  global_array[1] = square(2,depth);
  if(BF_CreateFile(filename) == BF_OK){
    int filedesc;
    if(BF_OpenFile(filename,&filedesc) == BF_OK){
      int size = square(2,depth);
      int max= (BF_BLOCK_SIZE)/sizeof(hash_values);
      if(size<=max - 2){
        BF_Block* extend_array= Create_Block(filedesc);

        void *data1 = BF_Block_GetData(extend_array);
        hash_values *hash1 = data1;
        hash_values *array = malloc(sizeof(hash_values)*max);
        int k =0;
        MetaData *meta = malloc(512);
        
        memset(array,0,sizeof(hash_values)*max);
        int block_num;
        for(int i=1;i<=size;i++){
          strcpy(array[i].value,intToBinaryDepthBits(k,depth));
          BF_Block* block = Create_Block(filedesc);
          void *data2 = BF_Block_GetData(block);
          BF_GetBlockCounter(filedesc,&block_num);

          array[i].pointer = block_num-1;

          insert_metadata(data2);

          BF_Block_SetDirty(block);
          BF_UnpinBlock(block);
          BF_Block_Destroy(&block);
          k++;
        }
        char *string = "EMPTY";
        strcpy(array[max - 1].value,string);
        array[max-1].pointer = -1;
        
        memcpy(hash1,array,sizeof(hash_values)*max);
        
      
        free(array);



        BF_Block_SetDirty(extend_array);
        BF_UnpinBlock(extend_array);
        BF_Block_Destroy(&extend_array);
      }
      else{
        BF_Block *block_need[14];


        int size_of_blocks = 0;
        void* next_block = NULL;
        int max = (BF_BLOCK_SIZE)/sizeof(hash_values);
        int k = 0;
        int extend_blocks = 0;
        void *data1[14];
        hash_values *hash1[14];
   
        while(size>=(max-2)*size_of_blocks){
          BF_Block* block1= Create_Block(filedesc);
          block_need[extend_blocks] = block1;
          data1[extend_blocks] = BF_Block_GetData(block1);
          hash1[extend_blocks] = data1[extend_blocks];
          
          
          

          
          hash_values *array = malloc(sizeof(hash_values)*max);
        
          memset(array,0,sizeof(hash_values)*max);
         
          MetaData *array1 = malloc(sizeof(MetaData)*64);
          if(size - (max-2)*size_of_blocks>=30){
            int block_num;
            for(int i=1;i<max-1;i++){
             
              BF_Block* block = Create_Block(filedesc);
              void *data3 = BF_Block_GetData(block);
              BF_GetBlockCounter(filedesc,&block_num);

              array[i].pointer = block_num-1;
              MetaData *meta = data3;
              memset(array1,0,sizeof(MetaData)*64);
              memcpy(meta,array1,sizeof(MetaData)*64);
              strcpy(array[i].value,intToBinaryDepthBits(k,depth));
              
              insert_metadata(data3);

              BF_Block_SetDirty(block);
              BF_UnpinBlock(block);
              BF_Block_Destroy(&block);
              k++;
            }
            BF_GetBlockCounter(filedesc,&block_num);
            char* string = "EMPTY";
            strcpy(array[max-1].value,string);
            array[max-1].pointer = block_num;

            memcpy(hash1[extend_blocks],array,sizeof(hash_values)*max);
          }
          else{
            int block_num;
            for(int i=1;i<=size-(max-2)*size_of_blocks;i++){
              BF_Block* block = Create_Block(filedesc);
              void *data3 = BF_Block_GetData(block);
              BF_GetBlockCounter(filedesc,&block_num);
              memset(data3,0,sizeof(block));
              array[i].pointer = block_num-1;
              strcpy(array[i].value,intToBinaryDepthBits(k,depth));
              
              memset(array1,0,sizeof(MetaData)*64);
              MetaData *meta = data3;
              memcpy(meta,array1,sizeof(MetaData)*64);
              insert_metadata(data3);
              BF_Block_SetDirty(block);
              BF_UnpinBlock(block);
              BF_Block_Destroy(&block);
              k++;
            }
            char* string = "EMPTY";
            strcpy(array[max-1].value,string);
            array[max-1].pointer = -1;
            
            memcpy(hash1[extend_blocks],array,sizeof(hash_values)*max);

          }

          
           
          
          size_of_blocks++;
          free(array);
          free(array1);
          BF_Block_SetDirty(block_need[extend_blocks]);
          BF_UnpinBlock(block_need[extend_blocks]);
          BF_Block_Destroy(&block_need[extend_blocks]);
          extend_blocks++;
        
        }
      }
      BF_CloseFile(filedesc);
      return HT_OK;
    }
  }
  return HT_ERROR;
}


HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  if(global_array[2] < 20){
    BF_Block *block;
    BF_Block_Init(&block);
    int filedesc;
    int blocks_num;
    if(BF_OpenFile(fileName,&filedesc)==BF_OK){
      if(BF_GetBlockCounter(filedesc,&blocks_num) == BF_OK){
        if(BF_GetBlock(filedesc,0,block)==BF_OK){
          for(int i = 0; i < 20;i++){
            if(indexVal[i].filedesc == -1){
              indexVal[i].blocks_num = blocks_num;
              indexVal[i].filedesc = filedesc;
              int size = 0;
              char string[12] ="EMPTY";             
           
              int block_of_extend_array = 0;
              int extend_blocks = 0;
              indexVal[i].blocks[extend_blocks]= block;
              void* data = BF_Block_GetData(block);
              int extend_blocks_check=0;
              int j=0;

            
              while(j<blocks_num-extend_blocks_check){
                BF_Block *checkBlock;
                BF_Block_Init(&checkBlock);
                if(BF_GetBlock(filedesc,j,checkBlock)==BF_OK){
                  indexVal[i].blocks[extend_blocks_check] = checkBlock;
                  extend_blocks_check++;
                }else{
                  BF_Block_Destroy(&checkBlock);
                  break;
                }
                hash_values * hash_check_block;
                void *data = BF_Block_GetData(checkBlock);
                hash_check_block = data;
                j = hash_check_block[31].pointer;
                if(j == -1){
                  break;
                }
              }

              BF_Block_Destroy(&block);

              
             

              indexVal[i].extend_array_size = blocks_num - extend_blocks_check;
              indexVal[i].number_of_extend_array_blocks = extend_blocks_check;
              size =  indexVal[i].extend_array_size;
              while(size!=1){
                size /=2;
                indexVal[i].global_depth++;
              }
              *indexDesc = i;
              global_array[2]++;
              return HT_OK;
            }
          }
        }
      }  
    }
    BF_Block_Destroy(&block);
  }
  return HT_ERROR;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    int filedesc=-1;
    for(int i=0;i<20;i++){
      if(i == indexDesc){
        filedesc = indexVal[i].filedesc;
        indexVal[i].blocks_num = 0;
        indexVal[i].filedesc = -1;
        if(indexVal[i].blocks[0] != NULL){
          int blocks_num;
          BF_GetBlockCounter(filedesc,&blocks_num);
          BF_Block *checkBlock;
          BF_Block_Init(&checkBlock);
          for(int j=0;j<blocks_num;j++){
            BF_GetBlock(filedesc,j,checkBlock);
            BF_Block_SetDirty(checkBlock);
            BF_UnpinBlock(checkBlock);
          }
          BF_Block_Destroy(&checkBlock);


          int j=0;
          while(indexVal[i].blocks[j]!=NULL){
            BF_Block_SetDirty(indexVal[i].blocks[j]);
            BF_UnpinBlock(indexVal[i].blocks[j]);
            BF_Block_Destroy(&indexVal[i].blocks[j]);
            indexVal[i].blocks[j]= NULL;
            j++;
          }
        }  
        break;
      }
    }
    if(filedesc != -1){
      if(BF_CloseFile(filedesc)== BF_OK){
        global_array[2]--;
        return HT_OK;
      }
    }
  return HT_ERROR;
}

int poses = 0;
HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  char* hashing = hash_function(record.id);
  
  int count=0;
  int found=0;
  int i;
  void* data;
  hash_values *hash;

  void* bucket_data;
  MetaData *metaBucket;
  Record *bucket_rec;


  while(count<indexVal[indexDesc].number_of_extend_array_blocks){
    data = BF_Block_GetData(indexVal[indexDesc].blocks[count]);
    hash = data;
    for(i=1;i<=30;i++){
     if(hash[i].pointer!=0){
      if(strcmp(check_hash(hashing,hash[i].value,indexVal[indexDesc].global_depth),"ERROR")!=0){
          found=1;
          break;
        }   
      }
      
    }
    if(found==1){
      break;
    }
    count++;
  }

  BF_Block *bucket;
  BF_Block_Init(&bucket);
  
  BF_GetBlock(indexVal[indexDesc].filedesc,hash[i].pointer,bucket);
  bucket_data = BF_Block_GetData(bucket);
  metaBucket = bucket_data;
  bucket_rec = bucket_data;
  

  if(insert_rec_onBlock(record,bucket_data)==1){
    poses++;
  
    BF_Block_SetDirty(bucket);
    BF_UnpinBlock(bucket);
    BF_Block_Destroy(&bucket);
    return HT_OK;
  }else{
    int rows = square(2,indexVal[indexDesc].global_depth); // Number of rows
    int cols = 8; // Number of columns
    int size = square(2,indexVal[indexDesc].global_depth);

    // Allocate memory for the 2D array of structs
    hash_values **hash_friends = (hash_values **)malloc(rows * sizeof(hash_values*));
    for (int j = 0; j < rows; j++) {
        hash_friends[j] = (hash_values *)malloc(cols * sizeof(hash_values));
    }

    for (int j = 0; j < rows; j++) {
        hash_friends[j]->pointer = -1;
        strcpy( hash_friends[j]->value," ");
    }

    hash_values **friends = find_buddies(hash[i].value,metaBucket[63].local_depth,indexVal[indexDesc].global_depth,indexDesc,hash_friends);
    char buddie_value[12];
    int check_buddies=0;
    for(int j=0;j<rows;j++){
      if(friends[j]->pointer!=0){
        if(hash[i].pointer == friends[j]->pointer && strcmp(hash[i].value,friends[j]->value)!=0){
          strcpy(buddie_value,friends[j]->value);
          check_buddies = 1;   
        }
      }
      if(check_buddies==1){
        break;
      }
    }

   


    if(check_buddies == 0){
      double_array(indexDesc,indexVal[indexDesc].extend_array_size);
      for(int j=0;j<rows;j++){
        free(hash_friends[j]);
      }
      free(hash_friends);
      
      
      if(HT_InsertEntry(indexDesc,record) == HT_OK){
        BF_Block_SetDirty(bucket);
        BF_UnpinBlock(bucket);
        BF_Block_Destroy(&bucket);
        return HT_OK;
      }
    }else{
      BF_Block *new_bucket = Create_Block(indexVal[indexDesc].filedesc);
      void* new_bucket_data = BF_Block_GetData(new_bucket);
      insert_metadata(new_bucket_data);
      MetaData *new_bucket_metadata = new_bucket_data;

      if(find_where_is_buddie(buddie_value,indexDesc)==1){
        Record records[8];
        for(int j=0;j<metaBucket[63].rec_num;j++){
          records[j] = bucket_rec[j];
        }
        int old_rec = metaBucket[63].rec_num;
        metaBucket[63].rec_num = 0;
        metaBucket[63].local_depth++;



        
        for(int j=0;j<old_rec;j++){
          HT_InsertEntry(indexDesc,records[j]);
        }
        new_bucket_metadata[63].local_depth = metaBucket[63].local_depth;
        if(HT_InsertEntry(indexDesc,record)==HT_OK){
          BF_Block_SetDirty(new_bucket);
          BF_UnpinBlock(new_bucket);
          BF_Block_Destroy(&new_bucket);
          for(int j=0;j<rows;j++){
            free(hash_friends[j]);
          }
          free(hash_friends);
          BF_Block_SetDirty(bucket);
          BF_UnpinBlock(bucket);
          BF_Block_Destroy(&bucket);
          return HT_OK;
        }     
      }


      BF_Block_SetDirty(new_bucket);
      BF_UnpinBlock(new_bucket);
      BF_Block_Destroy(&new_bucket);
      

      
    }


   
    for(int j=0;j<rows;j++){
      free(hash_friends[j]);
    }
    free(hash_friends);
  


  }



  BF_Block_SetDirty(bucket);
  BF_UnpinBlock(bucket);
  BF_Block_Destroy(&bucket);
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int* id) {
  int counter=0;
  int blocks_num;
  BF_GetBlockCounter(indexVal[indexDesc].filedesc,&blocks_num);
  int check=0;

  if(*id==-1){
    check =1;
  }
  
  for(int i=0;i<indexVal[indexDesc].number_of_extend_array_blocks;i++){
    void* data = BF_Block_GetData(indexVal[indexDesc].blocks[i]);
    hash_values *hash = data;
    for(int j=1;j<=30;j++){
      if(hash[j].pointer != 0){
        BF_Block *block;
        BF_Block_Init(&block);
        if(BF_GetBlock(indexVal[indexDesc].filedesc,hash[j].pointer,block) == BF_OK){
          void* metadata = BF_Block_GetData(block);
          MetaData *meta = metadata;  
          Record *rec = metadata; 
          for(int k=0;k<8;k++){
            if(rec[k].id!=0){
              counter++;
            }
            if(check==1 && rec[k].id!=0 ){
              printf("NAME: %s\n",rec[k].name);
              printf("ID: %d\n",rec[k].id);
            }else if(rec[k].id==*id){
              printf("NAME: %s\n",rec[k].name);
              printf("ID: %d\n",rec[k].id);
              BF_UnpinBlock(block); 
              BF_Block_Destroy(&block);
              return HT_OK;
            }
          }
        }
        BF_UnpinBlock(block); 
        BF_Block_Destroy(&block);
      }

    }
  }
  
  *id = 5;
  return HT_OK;
}

HT_ErrorCode HashStatistics(const char* fileName){
  int filedesc;
  int blocks_num;
  int max=0;
  int min=9;
  int mid;
  double sum=0;
  double bucket_counter=0;
  if(BF_OpenFile(fileName,&filedesc)==BF_OK){
    if(BF_GetBlockCounter(filedesc,&blocks_num)==BF_OK){
      for(int i = 0; i < 20;i++){
        if(indexVal[i].filedesc != -1){
          int counter=0;
          hash_values *hash;
          while(counter<indexVal[i].number_of_extend_array_blocks){
            void* block_data = BF_Block_GetData(indexVal[i].blocks[counter]);
            hash = block_data;
            for(int j=1;j<=30;j++){
              if(hash[j].pointer!=0){
                BF_Block *bucket;
                BF_Block_Init(&bucket);
                if(BF_GetBlock(indexVal[i].filedesc,hash[j].pointer,bucket) == BF_OK){
                  void* bucket_data = BF_Block_GetData(bucket);
                  MetaData *bucket_meta = bucket_data;
                  if(bucket_meta[63].rec_num<min){
                    min = bucket_meta[63].rec_num;
                  }else if(bucket_meta[63].rec_num>max){
                    max = bucket_meta[63].rec_num;
                  }
                  sum+=bucket_meta[63].rec_num;
                  bucket_counter++;
                }
                BF_Block_Destroy(&bucket);
              }
            }
            counter++; 
          }
        }
      }
    }
  }
  if(sum>0){
    printf("BLOCKS NUM:%d\n",blocks_num);
    printf("MAX:%d\n",max);
    printf("MIN:%d\n",min);
    printf("MID:%f\n",sum/bucket_counter);
    BF_CloseFile(filedesc);
    return HT_OK;
  }else{
    BF_CloseFile(filedesc);
    return HT_ERROR;
  } 
}

