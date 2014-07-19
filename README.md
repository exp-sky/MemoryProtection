    本月微软对IE的布丁中新增了一个保护机制，我们先将其称之为MemoryProtection。本文主要分析其提供保护的机制。
    此机制主要的目的是，保护函数调用栈内的局部变量中存储的对象指针。因为特定情况下如果此对象被释放，而此处的指针没有被清空，则有可能导致UAF漏洞。并将所有被释放的对象都延迟释放，只有当列表中的数据块总大小超过100000时，才对列表中的元素进行标记并释放。
    此保护机制的原理为，首先保存初始化时的栈页基址，而后在每次释放对象时都将要释放的对象指针，保存到其维护的一个数组列表中。
    接着如果数组列表中的元素总大小超过100000时，则通过一个专用函数对数组中的所有对象进行扫描，判断其是否存在于初始化栈地址到当前栈地址之中。如果存在则将其标记，并在后面的释放函数中不释放被标记的对象。
    这样就解决了局部变量没对引用计数进行维护，而导致异常情况下对象被释放后的UAF漏洞了。
    最后这个保护机制只能保护从开启此机制起到有对象释放时，并且是栈上存在释放对象指针的情况，而不能保护对象中存在释放对象指针的情况。

相关函数如下：
MemoryProtection::CMemoryProtector::CaptureStackHighAddress
MemoryProtection::CMemoryProtector::MarkBlocks
MemoryProtection::CMemoryProtector::ProtectCurrentThread
MemoryProtection::CMemoryProtector::ProtectedFree
MemoryProtection::CMemoryProtector::ReclaimMemory
MemoryProtection::CMemoryProtector::ReclaimMemoryWithoutProtection
MemoryProtection::CMemoryProtector::ReclaimUnmarkedBlocks
MemoryProtection::CMemoryProtector::UnprotectCurrentThread
MemoryProtection::CMemoryProtector::UnprotectProcess
MemoryProtection::DllNotification
MemoryProtection::SBlockDescriptorArray::AddBlockDescriptor
MemoryProtection::SBlockDescriptorArray::HeapFreeAllBlocks
MemoryProtection::SBlockDescriptor::AddressInBlockRange
MemoryProtection::SBlockDescriptor::BaseAddressCompare

    最后详细代码参考MemoryProtection.cpp文件。
    此保护机制有一个不明显的副作用，就是其首先不管被释放的对象是否存在于栈中都将其放入缓冲区中。只有缓冲区内对象大小的总和超过0x100000时，才会将不存在栈中的对象释放掉。这就导致一些不再其保护范围内的漏洞也没办法触发，需要先将待释放缓冲区内的对象大小达到0x100000时，才可触发漏洞。

