


static DWORD MemoryProtection_CMemoryProtector_tlsSlotForInstance = 0;



/*
   CMemoryProtection::CMemoryProtector对象结构
*/
//struct CMemoryProtection::CMemoryProtector
//{
//    struct protection_item * p_protection_item_buffer;  //保存protection_item数组
//    DWORD item_all_size;                                //数组中所有内存块大小
//    DWORD item_number;                                  //数组中内存快item数量
//    DWORD item_buffer_space;                            //数组空间大小
//    DWORD is_sort;                                      //是否排序，0为没排序
//    DWORD undefine;
//    DWORD init_stack_page_high_address;                 //初始化时栈页的高地址
//    DWORD undefine;
//};



/*
   protection_item 结构
*/
//struct protection_item
//{
//    VOID *    p_free_memory;    //要释放的内存指针，低二位表示heap_type
//    DWORD free_memory_size;     //要释放内存块的大小
//};



struct _SBlockDescriptorItem
{
    void * p_free_memory;
    int free_memory_size;
}SBlockDescriptorItem;



class MemoryProtection::SBlockDescriptorArray
{
//public:
    struct SBlockDescriptorItem* p_item_array;
    int item_all_size;
    int item_number;
    int item_buffer_space;

//public:
    /*
       释放p_item_array数组中所有的item对象
    */
    void HeapFreeAllBlocks()
    {
        if(this->item_number>0)
        {
            int index = 0;
            do
            {
                HADNLE hHandle = NULL;
                struct SBlockDescriptorItem * item = this->p_item_array[index];
                if(item.p_free_memory&2)
                    hHandle = _g_hIsolatedHeap;
                else
                    hHandle = _g_hProcessHeap;
                HeapFree(hHandle, 0, item.p_free_memory & 0xfffffffc);
            } while(index < this->item_number)
        }
    }



    /*
       添加一个延迟释放地址到列表中
    */
    int AddBlockDescriptor(void * p_free_memory,bool heap_type,int *p_ret_value)
    {
        if(!this->item_buffer_space)
        {
            this->p_item_array = HeapAlloc(_g_hProcessHeap,0,2000);
        }
        else if(this->item_number == this->item_buffer_space)
        {
            this->item_buffer_space *= 2;
            this->p_item_array = HeapReAlloc(_g_hProcessHeap,0,this->p_item_array);
        }
        if(this->p_item_array)
        {
            HANDLE hHadle = _g_hIsolatedHeap;
            if(!heap_type)
                hHadle = _g_hProcessHeap;
            int free_memory_size = Heapsize(hHadle,p_free_memory);
            this->item_all_size += free_memory_size;
            *p_ret_value = free_memory_size;
            this->item_number += 1;
            this->p_item_array[this->item_number].p_free_memory = p_free_memory;
            this->p_item_array[this->item_number].free_memory_size = free_memory_size;
            return True
        }
        return False
    }
}



class CMemoryProtection::CMemoryProtector(MemoryProtection::SBlockDescriptorArray)
{
//public:
    //数组内的元素是否排序，0为没排序
    bool is_sort;
    DWORD member_1;
    //初始化时栈页的高地址
    DWORD init_stack_high_address;
    DWORD member_2;


    /*
       初始化MemoryProtection机制
    */
    static int DllNotification(int flage)
    {
        if(flage == 0)
        {
            MemoryProtection::CMemoryProtector::UnprotectProcess(flage==0);
        }
        else if(flage == 1)
        {
            MemoryProtection_CMemoryProtector_tlsSlotForInstance = TlsAlloc();
        }
        else if(flage == 3)
        {
            MemoryProtection::CMemoryProtector::UnprotectCurrentThread()
        }
    }



    /*
        释放当前进程的内存保护
    */
    static void CMemoryProtection::CMemoryProtector::UnprotectProcess(bool is_release)
    {
        if(is_release)
        {
            if(MemoryProtection_CMemoryProtector_tlsSlotForInstance != 0xffffffff)
            {
                MemoryProtection::CMemoryProtector::UnprotectCurrentThread();
                TlsFree(MemoryProtection_CMemoryProtector_tlsSlotForInstance);
                MemoryProtection_CMemoryProtector_tlsSlotForInstance |= 0xffffffff;
            }
        }
    }



    /*
       释放当前线程保护
    */
    static void MemoryProtection::CMemoryProtector::UnprotectCurrentThread()
    {
        if(MemoryProtection_CMemoryProtector_tlsSlotForInstance != 0xffffffff)
        {
            CMemoryProtection::CMemoryProtector p_memory_proetction = (CMemoryProtection::CMemoryProtector)TlsGetValue(MemoryProtection_CMemoryProtector_tlsSlotForInstance);
            TlsSetValue(MemoryProtection_CMemoryProtector_tlsSlotForInstance,0);
            if(p_memory_proetction)
            {
                p_memory_proetction->HeapFreeAllBlocks()
                HeapFree(p_memory_proetction);
            }
        }
    }




    /*
       当前线程延迟保护初始化
    */
    static void MemoryProtection::CMemoryProtector::ProtectCurrentThread()
    {
        if(MemoryProtection_CMemoryProtector_tlsSlotForInstance != 0xffffffff)
        {
            MemoryProtection::CMemoryProtector p_memory_proetction = TlsGetValue(MemoryProtection_CMemoryProtector_tlsSlotForInstance);
            if(!p_memory_proetction)
            {
                void * object = NULL;
                if(CFeatureControlKey::CoInternetFeatureValueInternal(&object) && object)
                {
                    p_memory_proetction = HeapAlloc(_g_hProcessHeap,0,0x20); //new
                    if(!p_memory_proetction)
                        p_memory_proetction = NULL
                    else
                        memeset(p_memory_proetction,0,0,0x20);
                    if(p_memory_proetction && TlsSetValue(MemoryProtection_CMemoryProtector_tlsSlotForInstance,p_memory_proetction))
                        p_memory_proetction->CaptureStackHighAddress()
                    else
                        RaiseFailFastException(0,0,0);
                }
            }
            else
            {
                p_memory_proetction->ReclaimMemoryWithoutProtection()
            }
        }
    }


    /*
       捕获堆栈页的高地址
    */
    bool MemoryProtection::CMemoryProtector::CaptureStackHighAddress()
    {
        struct _MEMORY_BASIC_INFORMATION buffer;
        DWORD stack_address;
        stack_address = &stack_address;
        if(VirtualQuery(&stack_address,&buffer,sizeof(buffer)) == 0x1c)
        {
            this->init_stack_high_address = buffer.RegionSize+Buffer.BaseAddress;
            return TRUE
        }
        return FALSE
    }



    /*
        释放当前数组中的所有元素
    */
    int MemoryProtection::CMemoryProtector::ReclaimMemoryWithoutProtection()
    {
        if(this->item_number)
        {
            for(int i=0; i < this->item_number; i++)
            {
                struct SBlockDescriptorItem p_block = this->p_item_array[i];
                HANDLE hHeap = _g_hIsolatedHeap;
                if(!(p_block->p_free_memory & 2))
                    hHeap = _g_hProcessHeap;
                HeapFree(hHeap,0,p_block->p_free_memory);
            }
            this->item_all_size = 0;
            this->item_number = 0;
        }
    }



    /*
       安全的释放函数

       先将所有要释放的对象全部都放入延迟列表中
       如果列表中堆块的总大小超过100000时，则对其进行标记与释放。
    */
    static int MemoryProtection::CMemoryProtector::ProtectedFree(HANDLE hHeap, LPVOID p_free_memory)
    {
        bool heap_type = 0
        if(p_free_memory!=NULL)
        {
            if(MemoryProtection_CMemoryProtector_tlsSlotForInstance == 0xffffffff)
            {
                HeapFree(hHeap,p_free_memory);
                return;
            }
            CMemoryProtection::CMemoryProtector p_memory_proetction = TlsGetValue(MemoryProtection_CMemoryProtector_tlsSlotForInstance);
            if(p_memory_proetction->item_number != 0)
            {
                //数组中所有元素大小总和大于100000时则进行标记与释放
                if(p_memory_proetction->item_all_size >= 100000) || (p_memory_proetction->init_stack_high_address != NULL)
                {
                    int stack_address;
                    p_memory_proetction->MarkBlocks(&stack_address);
                    p_memory_proetction->ReclaimUnmarkedBlocks();
                }           
            }
            if(hHeap==_g_hIsolatedHeap)
                heap_type = TRUE;

            //申请或扩展数组缓冲区
            if(p_memory_proetction->p_item_array == NULL)
            {
                p_memory_proetction->p_item_array = HeapAlloc(_g_hProcessHeap,0,0x2000);
            }

            int new_item_buffer_space = p_memory_proetction->item_buffer_space;

            if(p_memory_proetction->item_number == p_memory_proetction->item_buffer_space)
            {
                new_item_buffer_space = new_item_buffer_space * 2;
                p_memory_proetction->p_item_array = HeapReAlloc(_g_hProcessHeap,0,p_memory_proetction->p_item_array,new_item_buffer_space*8);
            }

            if(p_memory_proetction->p_item_array == NULL)
                RaiseFailFastException(0,0,0);
            else
            {
                //将释放堆块加入到数组元素中
                p_memory_proetction->item_buffer_space = new_item_buffer_space;
                HADNLE heap;
                if(heap_type)
                    heap = _g_hIsolatedHeap;
                else
                    heap = _g_hProcessHeap;
                int free_memory_size = HeapSize(heap,p_free_memory);
                p_memory_proetction->item_all_size += free_memory_size;
                p_memory_proetction->p_item_array[p_memory_proetction->item_number].p_free_memory = p_free_memory;
                p_memory_proetction->p_item_array[p_memory_proetction->item_number].free_memory_size = free_memory_size;
                p_memory_proetction->item_number += 1;
                if(is_IsolatedHeap)
                    p_memory_proetction->p_item_array[p_memory_proetction->item_number].p_free_memory |= 2;
                memset(p_free_memory,0,free_memory_size);
            }

        }
    }



    /*
       标记 p_CMemoryProtector_this->p_item_array 中的元素
       MemoryProtection::CMemoryProtector *p_CMemoryProtector_this

       0x100A6F40
       主要功能为遍历
    */
    int MemoryProtection::CMemoryProtector::MarkBlocks(DWORD current_stack_address)
    {
        int stack_length = this->init_stack_high_address - current_stack_address;
        if(stack_length != 0)
        {
            Template_pxxx()
        }
        if(this->item_number != 0)
        {
            if(!this->is_sort)
            {
                qsort_s(this->p_item_array,this->item_number,sizeof(SBlockDescriptorItem),MemoryProtection::SBlockDescriptor::BaseAddressCompare,0);
                this->is_sort = TRUE;
            }
        }
        void * first_memory = this->p_item_array[0].p_free_memory & 0xfffffffc;
        DWORD last_item_free_memory_end = this->p_item_array[this->item_number-1].p_free_memory+this->p_item_array[this->item_number-1].free_memory_size;
        if(stack_length > 4)
        {
            int stack_item_number = stack_length/4;
            int i = 0;
            do
            {
                if((current_stack_address[i] > first_memory) && (current_stack_address[i] < last_item_free_memory_end))
                {
                    if(this->item_number)
                    {
                        if(!this->is_sort)
                        {
                            qsort_s(this->p_item_array,this->item_number,sizeof(SBlockDescriptorItem),MemoryProtection::SBlockDescriptor::BaseAddressCompare,0);
                            this->isis_sort = TRUE;
                        }
                       struct SBlockDescriptorItem * p_block =  bsearch_s(current_stack_address[i],this->p_item_array,this->item_number,sizeof(SBlockDescriptorItem),MemoryProtection::SBlockDescriptor::AddressInBlockRange,0);
                       if(p_block)
                       {
                            if(!(p_block&1))
                                p_block->p_free_memory |= 1;
                       }
                       i++;
                       stack_item_number--;
                    }
                }
                
            } while(stack_item_number)
        }
    }



    /*
       回收没有标记的内存块
    */
    int MemoryProtection::CMemoryProtector::ReclaimUnmarkedBlocks()
    {
        if(this->item_number != 0)
        {
            int i = 0;
            do
            {
                struct SBlockDescriptorItem * p_block = this->p_item_array[i];
                if(p_block->p_free_memory & 1)
                    p_block->p_free_memory &= 0xfffffffe;
                else
                {
                    HANDLE hHeap = NULL;
                    if(p_block->p_free_memory & 2)
                        hHeap = _g_hIsolatedHeap;
                    else
                        hHeap = _g_hProcessHeap;
                    HeapFree(hHeap,0,p_block->p_free_memory);
                    this->item_number -= 1;
                    this->item_all_size -= p_block->free_memory_size;
                    i -= 1;
                    p_block->p_free_memory = this->p_item_array[this->item_number-1].p_free_memory;
                    p_block->free_memory_size = this->p_item_array[this->item_number-1].free_memory_size;
                }
                i += 1;
            }while (i<this->item_number)
        }
    }



    /*
       清理内存
    */
    void MemoryProtection::CMemoryProtector::ReclaimMemory(current_stack_address)
    {
        if(this->item_number)
        {
            if(this->item_all_size >= 0x100000)
            {
                this->MarkBlocks(current_stack_address);
                this->ReclaimUnmarkedBlocks();
            }
        }
    }
}










