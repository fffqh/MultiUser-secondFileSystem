//kyf
#include "OpenFileManager.h"
#include "SecondFileKernel.h"
#include "INode.h"
#include <chrono>  
#include <time.h>

/*==============================class OpenFileTable===================================*/
/* 系统全局打开文件表对象实例的定义 */
OpenFileTable g_OpenFileTable;

OpenFileTable::OpenFileTable()
{
	//nothing to do here
}

OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}

/*作用：进程打开文件描述符表中找的空闲项  之 下标  写入 u_ar0[EAX]*/
File* OpenFileTable::FAlloc()
{
	int fd;
	User& u = SecondFileKernel::Instance().GetUser();

	/* 在进程打开文件描述符表中获取一个空闲项 */
	fd = u.u_ofiles.AllocFreeSlot();

	if(fd < 0)	/* 如果寻找空闲项失败 */
	{
		return NULL;
	}

	for(int i = 0; i < OpenFileTable::NFILE; i++)
	{
		/* f_count==0表示该项空闲 */
		if(this->m_File[i].f_count == 0)
		{
			/* 建立描述符和File结构的勾连关系 */
			u.u_ofiles.SetF(fd, &this->m_File[i]);
			/* 增加对file结构的引用计数 */
			this->m_File[i].f_count++;
			/* 清空文件读、写位置 */
			this->m_File[i].f_offset = 0;
			return (&this->m_File[i]);
		}
	}

	printf("[error]No Free File Struct\n");
	u.u_error = ENFILE;
	return NULL;
}

void OpenFileTable::CloseF(File *pFile)
{
	//Inode* pNode;
	//ProcessManager& procMgr = SecondFileKernel::Instance().GetProcessManager();

	// /* 管道类型 */
	// if(pFile->f_flag & File::FPIPE)
	// {
	// 	pNode = pFile->f_inode;
	// 	pNode->i_mode &= ~(Inode::IREAD | Inode::IWRITE);
	// 	//procMgr.WakeUpAll((unsigned long)(pNode + 1));
	// 	//procMgr.WakeUpAll((unsigned long)(pNode + 2));
	// }

	if(pFile->f_count <= 1)
	{
		/* 
		 * 如果当前进程是最后一个引用该文件的进程，
		 * 对特殊块设备、字符设备文件调用相应的关闭函数
		 */
		//pFile->f_inode->CloseI(pFile->f_flag & File::FWRITE);
		g_InodeTable.IPut(pFile->f_inode);
	}

	/* 引用当前File的进程数减1 */
	pFile->f_count--;
}

/*==============================class InodeTable===================================*/
/*  定义内存Inode表的实例 */
InodeTable g_InodeTable;

InodeTable::InodeTable()
{
	//nothing to do here
}

InodeTable::~InodeTable()
{
	//nothing to do here
}

void InodeTable::Initialize()
{
	/* 获取对g_FileSystem的引用 */
	this->m_FileSystem = &SecondFileKernel::Instance().GetFileSystem();
}

Inode* InodeTable::IGet(int inumber)
{
	Inode* pInode;
	User& u = SecondFileKernel::Instance().GetUser();

	while(true)
	{
		/* 检查指定设备dev中编号为inumber的外存Inode是否有内存拷贝 */
		int index = this->IsLoaded(inumber);
		if(index >= 0)	/* 找到内存拷贝 */
		{
			pInode = &(this->m_Inode[index]);
			/* 如果该内存Inode被上锁 */
			//printf("[IGET]上锁pInode: index=%d i_number=%d\n",index, pInode->i_number);
			pthread_mutex_lock(&pInode->mutex);
			//if( pInode->i_flag & Inode::ILOCK )
			//{
				/* 增设IWANT标志，然后睡眠 */
				//pInode->i_flag |= Inode::IWANT;
				
				//u.u_procp->Sleep((unsigned long)&pInode, ProcessManager::PINOD);
				
				/* 回到while循环，需要重新搜索，因为该内存Inode可能已经失效 */
				//continue;
			//}

			/* 如果该内存Inode用于连接子文件系统，查找该Inode对应的Mount装配块 */
			// if( pInode->i_flag & Inode::IMOUNT )
			// {
			// 	Mount* pMount = this->m_FileSystem->GetMount(pInode);
			// 	if(NULL == pMount)
			// 	{
			// 		/* 没有找到 */
			// 		Utility::Panic("No Mount Tab...");
			// 	}
			// 	else
			// 	{
			// 		/* 将参数设为子文件系统设备号、根目录Inode编号 */
			// 		dev = pMount->m_dev;
			// 		inumber = FileSystem::ROOTINO;
			// 		/* 回到while循环，以新dev，inumber值重新搜索 */
			// 		continue;
			// 	}
			// }

			/* 
			 * 程序执行到这里表示：内存Inode高速缓存中找到相应内存Inode，
			 * 增加其引用计数，增设ILOCK标志并返回之
			 */
			pInode->i_count++;
			pInode->i_flag |= Inode::ILOCK;
			return pInode;
		}
		else	/* 没有Inode的内存拷贝，则分配一个空闲内存Inode */
		{
			pInode = this->GetFreeInode();
			/* 若内存Inode表已满，分配空闲Inode失败 */
			if(NULL == pInode)
			{
				printf("[error]Inode Table Overflow !\n");
				u.u_error = ENFILE;
				return NULL;
			}
			else	/* 分配空闲Inode成功，将外存Inode读入新分配的内存Inode */
			{
				/* 设置新的设备号、外存Inode编号，增加引用计数，对索引节点上锁 */
				// pInode->i_dev = dev;
				pInode->i_number = inumber;
				pInode->i_flag = Inode::ILOCK;
				pInode->NFlock();
				pInode->i_count++;
				pInode->i_lastr = -1;

				BufferManager& bm = SecondFileKernel::Instance().GetBufferManager();
				/* 将该外存Inode读入缓冲区 */
				Buf* pBuf = bm.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR );

				/* 如果发生I/O错误 */
				if(pBuf->b_flags & Buf::B_ERROR)
				{
					/* 释放缓存 */
					bm.Brelse(pBuf);
					/* 释放占据的内存Inode */
					this->IPut(pInode);
					return NULL;
				}

				/* 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中 */
				pInode->ICopy(pBuf, inumber);
				/* 释放缓存 */
				bm.Brelse(pBuf);
				return pInode;
			}
		}
	}
	return NULL;	/* GCC likes it! */
}

/* close文件时会调用Iput
 *      主要做的操作：内存i节点计数 i_count--；若为0，释放内存 i节点、若有改动写回磁盘
 * 搜索文件途径的所有目录文件，搜索经过后都会Iput其内存i节点。路径名的倒数第2个路径分量一定是个
 *   目录文件，如果是在其中创建新文件、或是删除一个已有文件；再如果是在其中创建删除子目录。那么
 *   	必须将这个目录文件所对应的内存 i节点写回磁盘。
 *   	这个目录文件无论是否经历过更改，我们必须将它的i节点写回磁盘。
 * */
void InodeTable::IPut(Inode *pNode)

{

	/* 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode */
	if(pNode->i_count == 1)
	{
		/* 
		 * 上锁，因为在整个释放过程中可能因为磁盘操作而使得该进程睡眠，
		 * 此时有可能另一个进程会对该内存Inode进行操作，这将有可能导致错误。
		 */
		pNode->i_flag |= Inode::ILOCK;

		/* 该文件已经没有目录路径指向它 */
		if(pNode->i_nlink <= 0)
		{
			/* 释放该文件占据的数据盘块 */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* 释放对应的外存Inode */
			this->m_FileSystem->IFree(pNode->i_number);
		}

		/* 更新外存Inode信息 */
		//pNode->IUpdate(Time::time);
		time_t timestamp;
		pNode->IUpdate(time(&timestamp));
		
		/* 解锁内存Inode，并且唤醒等待进程 */
		pNode->NFrele();
		/* 清除内存Inode的所有标志位 */
		pNode->i_flag = 0;
		/* 这是内存inode空闲的标志之一，另一个是i_count == 0 */
		pNode->i_number = -1;
	}

	/* 减少内存Inode的引用计数，唤醒等待进程 */
	pNode->i_count--;
	pNode->NFrele();
}

void InodeTable::UpdateInodeTable()
{
	for(int i = 0; i < InodeTable::NINODE; i++)
	{
		/* 
		 * 如果Inode对象没有被上锁，即当前未被其它进程使用，可以同步到外存Inode；
		 * 并且count不等于0，count == 0意味着该内存Inode未被任何打开文件引用，无需同步。
		 */
		if( (this->m_Inode[i].i_flag & Inode::ILOCK) == 0 && this->m_Inode[i].i_count != 0 )
		{
			/* 将内存Inode上锁后同步到外存Inode */
			this->m_Inode[i].i_flag |= Inode::ILOCK;
			time_t timestamp;
			this->m_Inode[i].IUpdate(time(&timestamp));

			/* 对内存Inode解锁 */
			this->m_Inode[i].NFrele();
		}
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* 寻找指定外存Inode的内存拷贝 */
	for(int i = 0; i < InodeTable::NINODE; i++)
	{
		if(this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0 )
		{
			return i;	
		}
	}
	return -1;
}

Inode* InodeTable::GetFreeInode()
{
	for(int i = 0; i < InodeTable::NINODE; i++)
	{
		/* 如果该内存Inode引用计数为零，则该Inode表示空闲 */
		if(this->m_Inode[i].i_count == 0)
		{
			return &(this->m_Inode[i]);
		}
	}
	return NULL;	/* 寻找失败 */
}
