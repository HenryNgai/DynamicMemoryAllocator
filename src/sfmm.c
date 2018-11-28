/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"
#include "helper.h"

void *sf_malloc(size_t size) {
    //Setting up prologue
    sf_prologue prologue;
    prologue.padding = 0;
    prologue.header.info.allocated=1;
    prologue.header.info.prev_allocated=0;
    prologue.header.info.two_zeroes=0;
    prologue.header.info.block_size=0;
    prologue.header.info.requested_size=0;
    prologue.header.links.next =0;
    prologue.header.links.prev =0;

    //Not sure if payload needs to be set
    //Not sure if links need to be set to 0


    prologue.footer.info.allocated=1;
    prologue.footer.info.prev_allocated=0;
    prologue.footer.info.two_zeroes=0;
    prologue.footer.info.block_size=0;
    prologue.footer.info.requested_size=0;

    //setting up epilogue
    sf_epilogue epilogue;
    epilogue.footer.info.allocated=1;
    epilogue.footer.info.prev_allocated=0;
    epilogue.footer.info.two_zeroes=0;
    epilogue.footer.info.block_size=0;
    epilogue.footer.info.requested_size=0;

    if (size == 0){
        return  NULL;
    }

    if(sf_mem_start() == sf_mem_end()){
        sf_mem_grow();
        *(sf_prologue*)sf_mem_start() = prologue;
        *(((sf_epilogue*)sf_mem_end())-1) = epilogue;

        size_t leftOverPage = 4096 - sizeof(prologue) - sizeof(epilogue);

        sf_header firstHeader;
        setHeader(leftOverPage,&firstHeader);
        firstHeader.info.prev_allocated =1;
        sf_footer firstFooter;
        setFooter(leftOverPage,&firstFooter);
        firstFooter.info.prev_allocated =1;


        sf_header *posHeader = ((sf_header*)((sf_prologue*)(sf_mem_start())+1));
        sf_footer *temp = (sf_footer*)(((char*)posHeader)+leftOverPage-sizeof(firstFooter));
        *temp = firstFooter;
        *posHeader = firstHeader;

        storeHeader(posHeader,findNode(leftOverPage));

        // posHeader->links.next = &(newlyAddedNode->head);
        // posHeader->links.prev = &(newlyAddedNode->head);
        // (newlyAddedNode->head).links.next = posHeader;
        // (newlyAddedNode->head).links.prev = posHeader;
    }




    //calculate required size + padding if needed
    size_t requiredSize;
    if ( (8 + size) %16 == 0){
        requiredSize = 8 + size;
    }
    else{
        requiredSize = 8 + size + (16-(size+8)%16);
    }

    if(requiredSize < 32){
        requiredSize = 32;
    }

    sf_header* validHeader = doesNodeandHeaderExist(requiredSize);

    if (validHeader == NULL){
        while(validHeader == NULL){
            //set new page up as a seperate block
            debug("grow heap");
            sf_header *newBlockHeader = (sf_header*)((char*)(sf_mem_end() - sizeof(sf_epilogue)));
            if(sf_mem_grow() == NULL){
                sf_errno = ENOMEM;
                return NULL;
            }
            *(((sf_epilogue*)sf_mem_end())-1) = epilogue;
            size_t newPage= 4096;
            sf_header newHeader;
            setHeader(newPage,&newHeader);
            sf_footer newFooter;
            setFooter(newPage,&newFooter);

            sf_footer *newBlockFooter = (sf_footer*)(((char*)newBlockHeader)+newPage-sizeof(newFooter));
            *newBlockFooter = newFooter;
            *newBlockHeader = newHeader;

            if (((sf_footer*)((char*)newBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                newBlockHeader ->info.prev_allocated = 1;
                newBlockFooter ->info.prev_allocated =1;
            }

            storeHeader(newBlockHeader,findNode(newBlockHeader->info.block_size<<4));
/***********************************************************************************************************************/
            //coalese

            sf_footer *prevNodeFooter = (sf_footer*)(newBlockHeader)-1;
            sf_header *prevNodeHeader = (sf_header*)((char*)(prevNodeFooter)-(prevNodeFooter->info.block_size <<4) + sizeof(sf_footer));

            //Should I Coalese
            if(prevNodeFooter->info.allocated == 0){
                debug("coalese");

                //remove the previous block's links from list.
                sf_header* next = prevNodeHeader->links.next;
                sf_header* prev = prevNodeHeader->links.prev;
                prev -> links.next = next;
                next ->links.prev = prev;

                prevNodeHeader ->links.next =NULL;
                prevNodeHeader ->links.prev =NULL;

                //remove the new block's links from list;
                next =  newBlockHeader->links.next;
                prev = newBlockHeader->links.prev;
                prev -> links.next = next;
                next->links.prev=prev;

                newBlockHeader ->links.next=NULL;
                newBlockHeader ->links.prev=NULL;

                 //creates new headers for big block and sets them up. Then puts them in the correct place.
                sf_header bigHeader;
                setHeader((newBlockHeader->info.block_size<<4)+(prevNodeFooter->info.block_size <<4),&bigHeader);
                sf_footer bigFooter;
                setFooter((newBlockHeader->info.block_size<<4)+(prevNodeFooter->info.block_size <<4),&bigFooter);

                *prevNodeHeader = bigHeader;
                *newBlockFooter = bigFooter;

                if (((sf_footer*)((char*)newBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                    prevNodeHeader ->info.prev_allocated = 1;
                    newBlockFooter ->info.prev_allocated =1;
                }

                storeHeader(prevNodeHeader,findNode(prevNodeHeader->info.block_size<<4));
                validHeader = doesNodeandHeaderExist(requiredSize);
             }
         }
    }
    debug("mem grow and coalese successully (if needed)");

/*****************************************************************************************/
    debug("check to see if we need to split");
        if(((validHeader->info.block_size<<4) - requiredSize)>= 32){
                size_t temp = (validHeader->info.block_size<<4) - requiredSize;
                debug("Split needed");
                sf_header* requiredBlockHeader = validHeader;
                sf_footer* requiredBlockFooter = (sf_footer*)(((char*)requiredBlockHeader)+requiredSize-sizeof(sf_footer));

                sf_header* leftOverBlockHeader = (sf_header*)(requiredBlockFooter+1);
                sf_footer* leftOverBlockFooter = (sf_footer*)(((char*)leftOverBlockHeader)+temp-sizeof(sf_footer));


                sf_header* next = validHeader->links.next;
                sf_header* prev = validHeader->links.prev;
                prev -> links.next = next;
                next ->links.prev = prev;

                validHeader ->links.next =NULL;
                validHeader ->links.prev =NULL;


                sf_header firstBlockHeader;
                setHeader(requiredSize,&firstBlockHeader);
                firstBlockHeader.info.allocated = 1;
                firstBlockHeader.info.requested_size = requiredSize;

                sf_footer firstBlockFooter;
                setFooter(requiredSize,&firstBlockFooter);
                firstBlockFooter.info.allocated = 1;
                firstBlockFooter.info.prev_allocated = firstBlockHeader.info.prev_allocated;
                firstBlockFooter.info.requested_size=requiredSize;

                sf_header secondBlockHeader;
                setHeader(temp,&secondBlockHeader);
                secondBlockHeader.info.prev_allocated = 1;

                sf_footer secondBlockFooter;
                setFooter(temp,&secondBlockFooter);
                secondBlockFooter.info.prev_allocated = secondBlockHeader.info.prev_allocated;
                secondBlockFooter.info.prev_allocated = 1;

                *requiredBlockHeader = firstBlockHeader;
                if (((sf_footer*)((char*)requiredBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                    requiredBlockHeader->info.prev_allocated = 1;
                    //requiredBlockHeader->info.prev_allocated =1;
                }

                *requiredBlockFooter = firstBlockFooter;
                requiredBlockFooter->info.prev_allocated = requiredBlockHeader->info.prev_allocated;

                *leftOverBlockHeader = secondBlockHeader;
                if (((sf_footer*)((char*)leftOverBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                    leftOverBlockHeader->info.prev_allocated = 1;
                    //leftOverBlockHeader->info.prev_allocated =1;
                }

                *leftOverBlockFooter = secondBlockFooter;
                leftOverBlockFooter->info.prev_allocated = leftOverBlockFooter->info.prev_allocated;



                storeHeader(leftOverBlockHeader,findNode(temp));
                findNode(requiredSize);
                requiredBlockHeader->info.requested_size = size;
                return (&(requiredBlockHeader->payload)); // Not sure if this is right

            }
            else{
                debug("found the correct header and returning the address of that header");
                //remove head from sublist. Return address of header of sublist.
                sf_header* next = validHeader->links.next;
                sf_header* prev = validHeader->links.prev;
                prev -> links.next = next;
                next ->links.prev = prev;

                validHeader ->links.next =NULL;
                validHeader ->links.prev =NULL;
                validHeader->info.allocated =1;
                //sf_show_heap();
                validHeader->info.allocated=1;
                ((sf_footer*)((char*)(validHeader) + (validHeader->info.block_size << 4) - sizeof(sf_footer)))->info.allocated=1;
                validHeader->info.requested_size = size;
                return(&(validHeader->payload)); //Not sure if this is right.


            }


    //sf_show_heap();
    sf_errno = ENOMEM;
    return NULL;
}

void sf_free(void *pp) {
    sf_header* checkHeader = (sf_header*)((char*)pp - 8);
    if (checkHeader == NULL){
        debug("pointerHeader is null");
        abort();
    }

    if((char*)checkHeader < (char*)((sf_prologue*)sf_mem_start()+1) ||
        (char*)(checkHeader) > ((char*)((sf_epilogue*)sf_mem_end()-1))){
        debug("pointerHeader is before end of prologue or after start of epilogue");
        abort();
    }
    if(checkHeader->info.allocated == 0){ // Did not check footer because we assume it will be tehe same
        debug("pointerHeader allocated bit is 0");
        abort();
    }
    if((checkHeader->info.block_size <<4) % 16 != 0 || (checkHeader->info.block_size <<4) < 32){
        debug("pointerHeader block size is not a multiple of 16 or it is less than 32");
        abort();
    }
    if(checkHeader->info.requested_size + 8 > checkHeader->info.block_size << 4){
        debug("pointerHeader requested_Size + header Size is greater than block size");
        abort();
    }
    if(checkHeader->info.prev_allocated ==0){
        if(((sf_footer*)(checkHeader))->info.allocated == 1){ // Did not check header because we assume it will be the same as the footer.
            debug("previous footer allocated bit is 1");
            abort();
        }
    }


    /* Valid Pointer was Given***************************************/

    //Free Block
    sf_header * pointerAddress = (sf_header*) (((char*)pp) - 8);
    size_t headerBlockSize = pointerAddress->info.block_size<<4 ;


    if (((sf_footer*)((char*)pointerAddress - sizeof(sf_footer)))->info.allocated == 1){
        pointerAddress->info.prev_allocated = 1;
    }
    //says this block is free
    pointerAddress->info.allocated = 0;


    sf_footer newFooter;
    sf_footer* pointerAddressFooter =(sf_footer*)((char*)(pointerAddress) + headerBlockSize - sizeof(sf_footer));
    *pointerAddressFooter = newFooter;
    setFooter(headerBlockSize,pointerAddressFooter);

    pointerAddressFooter->info.prev_allocated = pointerAddress ->info.prev_allocated;
    pointerAddressFooter->info.requested_size = pointerAddress->info.requested_size;
    pointerAddressFooter->info.allocated = pointerAddress->info.allocated;




    storeHeader(pointerAddress,findNode(pointerAddress->info.block_size<<4));

    //Change the next node prev allocated bit to 0;
    sf_header* nextBlockHeader =  (sf_header*)((char*)pointerAddressFooter+sizeof(sf_footer));
    nextBlockHeader ->info.prev_allocated=0;
    sf_footer* nextBlockFooter = (sf_footer*)((char*)(nextBlockHeader)+(nextBlockHeader->info.block_size<<4)-sizeof(sf_footer));
    nextBlockFooter->info.prev_allocated=0;

    debug("Freed Node. Now checking coalesing");

    //sf_show_heap();

    //sf_footer* prevAdjacentBlockFooter = (sf_footer*)(checkHeader)-1;
    if(checkHeader->info.prev_allocated == 0){
        debug("coalesceWithPrevBlock");
        coalesceWithPrevBlock(checkHeader);

    }


    sf_header* nextAdjacentBlockheader = (sf_header*)((char*)(checkHeader)+(checkHeader->info.block_size<<4));
    if(nextAdjacentBlockheader->info.allocated == 0){
        debug("coalesceWithNextBlock");
        coalesceWithNextBlock(checkHeader);
    }






}


void *sf_realloc(void *pp, size_t rsize) {
sf_header* checkHeader = (sf_header*)((char*)pp - 8);
    if (checkHeader == NULL){
        debug("pointerHeader is null");
        abort();
    }

    if((char*)checkHeader < (char*)((sf_prologue*)sf_mem_start()+1) ||
        (char*)(checkHeader) > ((char*)((sf_epilogue*)sf_mem_end()-1))){
        debug("pointerHeader is before end of prologue or after start of epilogue");
        abort();
    }
    if(checkHeader->info.allocated == 0){ // Did not check footer because we assume it will be tehe same
        debug("pointerHeader allocated bit is 0");
        abort();
    }
    if((checkHeader->info.block_size <<4) % 16 != 0 || (checkHeader->info.block_size <<4) < 32){
        debug("pointerHeader block size is not a multiple of 16 or it is less than 32");
        abort();
    }
    if(checkHeader->info.requested_size + 8 > checkHeader->info.block_size << 4){
        debug("pointerHeader requested_Size + header Size is greater than block size");
        abort();
    }
    if(checkHeader->info.prev_allocated ==0){
        if(((sf_footer*)(checkHeader)-1)->info.allocated == 1){ // Did not check header because we assume it will be the same as the footer.
            debug("previous footer allocated bit is 1");
            abort();
        }
    }

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }


    if(rsize > checkHeader->info.block_size<<4){
        void* temp = sf_malloc(rsize);
        memcpy (temp,pp,(checkHeader->info.block_size <<4)-8);
        sf_free(pp);
        return temp;
    }

    if(rsize<= checkHeader->info.block_size<<4){
        //calculate the size of block i need.
        size_t requiredSize;
        if ( (8 + rsize) %16 == 0){
            requiredSize = 8 + rsize;
        }
        else{
            requiredSize = 8 + rsize + (16-(rsize+8)%16);
        }

        if(requiredSize < 32){
            requiredSize = 32;
        }
        debug("%lu\n",requiredSize);

        ///////////////////////////////////////////////////////////

        if((checkHeader->info.block_size <<4) -requiredSize < 32){
            //splinter will happen
        }

        else{
            //split
            sf_header* allocatedBlockHeaderPos = checkHeader;
            sf_header* freeBlockHeaderPos = (sf_header*)((char*)(allocatedBlockHeaderPos)+requiredSize);
           // sf_footer* freeBlockFooterPos = (sf_footer*)(((char*)(allocatedBlockHeaderPos)
             //   + (allocatedBlockHeaderPos->info.block_size<<4))-sizeof(sf_footer));

            sf_header allocatedBlockHeader;
            setHeader(requiredSize,&allocatedBlockHeader);
            allocatedBlockHeader.info.allocated=1;
            allocatedBlockHeader.info.requested_size=rsize;

            sf_header freeBlockHeader = {0};
            setHeader((allocatedBlockHeaderPos->info.block_size<<4)-requiredSize,&freeBlockHeader);

            // sf_footer freeBlockFooter;
            // setFooter((allocatedBlockHeaderPos->info.block_size<<4)-requiredSize,&freeBlockFooter);
            // freeBlockFooter.info.prev_allocated=1;

            *allocatedBlockHeaderPos = allocatedBlockHeader;

            freeBlockHeader.info.prev_allocated=1;
            freeBlockHeader.info.allocated=1;
            *freeBlockHeaderPos = freeBlockHeader;
            // *freeBlockFooterPos = freeBlockFooter;
            sf_free((char*)freeBlockHeaderPos+8);
        }

        checkHeader->info.requested_size = rsize;
        return (pp);




    }


    return NULL;
}























//Helper Functions

//This function works perfectly
sf_free_list_node *insertNewNode(size_t size){
    sf_free_list_node *node = &sf_free_list_head;
    while(node->next != &sf_free_list_head){
        node = (node->next);
        if (node->size >= size){
            return sf_add_free_list(size,node);
        }
    }
    return sf_add_free_list(size,&sf_free_list_head);
}


sf_free_list_node *findNode(size_t size){
    sf_free_list_node *node = &sf_free_list_head;
    while(node->next != &sf_free_list_head){
        node = (node->next);
        if (node->size ==size){
            return node;
            }
        }
    debug("didn't find node. Making new node");
    return (insertNewNode(size));
}



sf_header* doesNodeandHeaderExist(size_t requiredSize){
    sf_free_list_node *node = &sf_free_list_head;
    while(node->next != &sf_free_list_head){
        node = (node->next);
        if (node->size >= requiredSize && node->head.links.next != &(node->head)){
            return (node->head.links.next);
            }
        }
    return NULL;
}


void storeHeader(sf_header *header,sf_free_list_node *node ){
    ((node->head).links.next)->links.prev = header;
    header->links.next = ((node->head).links.next);
    node->head.links.next = header;
    header->links.prev = &(node->head);
    debug("stored header properly");
}


void setHeader (size_t leftOverPage, sf_header* newHeader){
    (*newHeader).info.allocated = 0;
    (*newHeader).info.prev_allocated = 0;
    (*newHeader).info.two_zeroes = 00;
    (*newHeader).info.requested_size = 0;
    (*newHeader).info.block_size = leftOverPage>>4;


}

void setFooter(size_t leftOverPage, sf_footer* newFooter){
    (*newFooter).info.allocated = 0;
    (*newFooter).info.prev_allocated = 0;
    (*newFooter).info.two_zeroes = 00;
    (*newFooter).info.requested_size = 0;
    (*newFooter).info.block_size = leftOverPage>>4;
}


void coalesceWithPrevBlock(sf_header* originalBlockHeader){
    sf_footer *adjacentBlockFooter= (sf_footer*)(originalBlockHeader)-1;
    sf_header *adjacentBlockHeader = (sf_header*)((char*)(adjacentBlockFooter)-(adjacentBlockFooter->info.block_size <<4) + sizeof(sf_footer));
    sf_footer* originalBlockFooter = (sf_footer*)((char*)(originalBlockHeader)+(originalBlockHeader->info.block_size <<4) - sizeof(sf_footer));
        //Should I Coalese
        if(originalBlockHeader->info.prev_allocated == 0){
            debug("Add with Prev Block");

            //remove the adjacent block's links from list.
            sf_header* next = adjacentBlockHeader->links.next;
            sf_header* prev = adjacentBlockHeader->links.prev;
            prev -> links.next = next;
            next ->links.prev = prev;

            adjacentBlockHeader ->links.next =NULL;
            adjacentBlockHeader ->links.prev =NULL;

            //remove the original block's links from list;
            next =  originalBlockHeader->links.next;
            prev = originalBlockHeader->links.prev;
            prev -> links.next = next;
            next->links.prev=prev;

            originalBlockHeader ->links.next=NULL;
            originalBlockHeader ->links.prev=NULL;

             //creates new headers for big block and sets them up. Then puts them in the correct place.
            sf_header bigHeader;
            setHeader((originalBlockHeader->info.block_size<<4)+(adjacentBlockFooter->info.block_size <<4),&bigHeader);
            sf_footer bigFooter;
            setFooter((originalBlockHeader->info.block_size<<4)+(adjacentBlockFooter->info.block_size <<4),&bigFooter);

            *adjacentBlockHeader = bigHeader;
            *originalBlockFooter = bigFooter;

            if (((sf_footer*)((char*)adjacentBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                adjacentBlockHeader ->info.prev_allocated = 1;
                originalBlockFooter ->info.prev_allocated =1;
            }

            storeHeader(adjacentBlockHeader,findNode(adjacentBlockHeader->info.block_size<<4));
         }
    debug("coalesed block successfully with previous block");
}

void coalesceWithNextBlock(sf_header* originalBlockHeader){
    //sf_footer* originalBlockFooter = (sf_footer*)((char*)originalBlockHeader+(originalBlockHeader->info.block_size<<4)-sizeof(sf_footer));
    sf_header* nextBlockHeader = (sf_header*)((char*)originalBlockHeader + (originalBlockHeader->info.block_size<<4));
    sf_footer* nextBlockFooter = (sf_footer*)((char*)nextBlockHeader + (nextBlockHeader->info.block_size<<4)-sizeof(sf_footer));

    if(nextBlockHeader->info.allocated==0){
        debug("Add with Next Block");

        //remove the next Block's Links from list.
        sf_header* next = nextBlockHeader->links.next;
        sf_header* prev = nextBlockHeader->links.prev;
        prev -> links.next = next;
        next ->links.prev = prev;

        nextBlockHeader ->links.next =NULL;
        nextBlockHeader ->links.prev =NULL;

        //remove the original block's links from list;
        next =  originalBlockHeader->links.next;
        prev = originalBlockHeader->links.prev;
        prev -> links.next = next;
        next->links.prev=prev;

        originalBlockHeader ->links.next=NULL;
        originalBlockHeader ->links.prev=NULL;

        sf_header bigHeader;
        setHeader((originalBlockHeader->info.block_size<<4)+(nextBlockHeader->info.block_size<<4),&bigHeader);
        sf_footer bigFooter;
        setFooter((originalBlockHeader->info.block_size<<4)+(nextBlockHeader->info.block_size<<4),&bigFooter);

        *originalBlockHeader = bigHeader;
        *nextBlockFooter = bigFooter;

        if (((sf_footer*)((char*)originalBlockHeader - sizeof(sf_footer)))->info.allocated == 1){
                    originalBlockHeader ->info.prev_allocated = 1;
                    nextBlockFooter ->info.prev_allocated =1;
        }

        storeHeader(originalBlockHeader,findNode(originalBlockHeader->info.block_size<<4));

    }
    debug("coalesced block sucessfully with next block");

}




