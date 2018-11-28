void setHeader(size_t leftOverPage,sf_header* head);
void setFooter (size_t leftOverPage, sf_footer* foot);
void storeHeader(sf_header* header, sf_free_list_node* node);
sf_header* doesNodeandHeaderExist(size_t requiredSize);
sf_free_list_node *insertNewNode(size_t size);
void split (size_t requiredSize, sf_header* header);
sf_free_list_node *findNode(size_t size);
void coalesceWithPrevBlock(sf_header* header);
void coalesceWithNextBlock(sf_header* originalBlockHeader);