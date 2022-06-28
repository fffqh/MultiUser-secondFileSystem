//fqh
#include <string.h>
#include "time.h"
#include "User.h"
#include "FileSystem.h"
#include "SecondFileKernel.h"

/*==============================class SuperBlock===================================*/
/* 系统全局超级块SuperBlock对象 */
SuperBlock g_spb;

SuperBlock::SuperBlock()
{
	pthread_mutex_init(&s_ilock,NULL);
	pthread_mutex_init(&s_flock,NULL);
	//nothing to do here
}

SuperBlock::~SuperBlock()
{
	pthread_mutex_destroy(&s_ilock);
	pthread_mutex_destroy(&s_flock);
	//nothing to do here
}

/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	//nothing to do here
}

FileSystem::~FileSystem()
{
	//nothing to do here
}

void FileSystem::Initialize()
{
	this->m_BufferManager = &SecondFileKernel::Instance().GetBufferManager();
	this->updlock = 0;
}

void FileSystem::LoadSuperBlock()
{
	User& u = SecondFileKernel::Instance().GetUser();
	BufferManager& bufMgr = SecondFileKernel::Instance().GetBufferManager();
	Buf* pBuf;

	// 读入磁盘上的两个扇区
	for (int i = 0; i < 2; i++)
	{
		int* p = (int *)&g_spb + i * 128;
		pBuf = bufMgr.Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);

		memcpy(p, pBuf->b_addr, BufferManager::BUFFER_SIZE);

		bufMgr.Brelse(pBuf);
	}
	if (NOERROR != u.u_error)
	{
		printf("[error] Load SuperBlock Error....!\n");
	}

	// g_spb.s_flock = 0;
	// g_spb.s_ilock = 0;
	g_spb.s_ronly = 0;
	time_t cur_time;
	time(&cur_time);
	g_spb.s_time = cur_time;
	pthread_mutex_init(&(g_spb.s_flock),NULL);
	pthread_mutex_init(&(g_spb.s_ilock),NULL);

}

SuperBlock* FileSystem::GetFS()
{
	return &g_spb;
}

/*
 * @comment 将SuperBlock对象的内存副本更新到
 * 存储设备的SuperBlock中去
 */
void FileSystem::Update()
{
	//int i;
	SuperBlock* sb = &g_spb;
	Buf* pBuf;


	// 对 Update 函数进行上锁

	/* 同步SuperBlock到磁盘 */

	
	/* 如果该SuperBlock内存副本没有被修改，直接管理inode和空闲盘块被上锁或该文件系统是只读文件系统 */
	if(sb->s_fmod == 0 || sb->s_ronly != 0)//sb->s_ilock != 0 || sb->s_flock != 0 ||
	{
		printf("[info] FileSystem::Update 提前结束，无需Update\n");
		return;
	}

	pthread_mutex_lock(&sb->s_ilock);
	pthread_mutex_lock(&sb->s_flock);
	cout << "[Update] sb->s_flock 上锁" <<endl;
	/* 清SuperBlock修改标志 */
	sb->s_fmod = 0;
	/* 写入SuperBlock最后存访时间 */
	time_t cur_time;
	time(&cur_time);
	sb->s_time = cur_time;
	/* 
	* 为将要写回到磁盘上去的SuperBlock申请一块缓存，由于缓存块大小为512字节，
	* SuperBlock大小为1024字节，占据2个连续的扇区，所以需要2次写入操作。
	*/
	for(int j = 0; j < 2; j++)
	{
		/* 第一次p指向SuperBlock的第0字节，第二次p指向第512字节 */
		int* p = (int *)sb + j * 128;

		/* 将要写入到设备dev上的SUPER_BLOCK_SECTOR_NfUMBER + j扇区中去 */
		pBuf = this->m_BufferManager->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

		/* 将SuperBlock中第0 - 511字节写入缓存区 */
		memcpy(pBuf->b_addr, p, 512);

		/* 将缓冲区中的数据写到磁盘上 */
		this->m_BufferManager->Bwrite(pBuf);
	}
	//kyf 仅判断无锁，不加锁，也就是unix在刚上完锁时解锁，此处改进
	pthread_mutex_unlock(&sb->s_ilock);
	pthread_mutex_unlock(&sb->s_flock);
	cout << "[Update] sb->s_flock解锁!" <<endl;
	/* 同步修改过的内存Inode到对应外存Inode */
	g_InodeTable.UpdateInodeTable();

	/* 清除Update()函数锁 */
	this->updlock = 0;

	/* 将延迟写的缓存块写到磁盘上 */
	this->m_BufferManager->Bflush();
}

/*
 * @comment  在存储设备dev上分配一个空闲
 * 外存INode，一般用于创建新的文件。
 */
Inode* FileSystem::IAlloc()
{
	SuperBlock* sb;
	Buf* pBuf;
	Inode* pNode;
	User& u = SecondFileKernel::Instance().GetUser();
	int ino;	/* 分配到的空闲外存Inode编号 */

	/* 获取相应设备的SuperBlock内存副本 */
	sb = this->GetFS();


	/* 如果SuperBlock空闲Inode表被上锁，则睡眠等待至解锁 */
	// while(sb->s_ilock)
	// {
	// 	u.u_procp->Sleep((unsigned long)&sb->s_ilock, ProcessManager::PINOD);
	// }

	/* 
	 * SuperBlock直接管理的空闲Inode索引表已空，
	 * 必须到磁盘上搜索空闲Inode。先对inode列表上锁，
	 * 因为在以下程序中会进行读盘操作可能会导致进程切换，
	 * 其他进程有可能访问该索引表，将会导致不一致性。
	 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	if(sb->s_ninode <= 0)
	{
		/* 空闲Inode索引表上锁 */
		//sb->s_ilock++;
		pthread_mutex_lock(&sb->s_ilock);
		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		ino = -1;

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for(int i = 0; i < sb->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int* p = (int *)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用 */
			for(int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *( p + j * sizeof(DiskInode)/sizeof(int) );

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if(mode != 0)
				{
					continue;
				}

				/* 
				 * 如果外存inode的i_mode==0，此时并不能确定
				 * 该inode是空闲的，因为有可能是内存inode没有写到
				 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
				 */
				if( g_InodeTable.IsLoaded(ino) == -1 )
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					sb->s_inode[sb->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if(sb->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			this->m_BufferManager->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if(sb->s_ninode >= 100)
			{
				break;
			}
		}
		/* 解除对空闲外存Inode索引表的锁，唤醒因为等待锁而睡眠的进程 */
		pthread_mutex_unlock(&sb->s_ilock);
		// sb->s_ilock = 0;
		// Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_ilock);
		
		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if(sb->s_ninode <= 0)
		{
			printf("[error] cannot find aviliable free diskInode\n");
			u.u_error = ENOSPC;
			return NULL;
		}
	}

	/* 
	 * 上面部分已经保证，除非系统中没有可用外存Inode，
	 * 否则空闲Inode索引表中必定会记录可用外存Inode的编号。
	 */
	while(true)
	{
		/* 从索引表“栈顶”获取空闲外存Inode编号 */
		ino = sb->s_inode[--sb->s_ninode];

		/* 将空闲Inode读入内存 */
		pNode = g_InodeTable.IGet(ino);
		/* 未能分配到内存inode */
		if(NULL == pNode)
		{
			return NULL;
		}

		/* 如果该Inode空闲,清空Inode中的数据 */
		if(0 == pNode->i_mode)
		{
			pNode->Clean();
			/* 设置SuperBlock被修改标志 */
			sb->s_fmod = 1;
			return pNode;
		}
		else	/* 如果该Inode已被占用 */
		{
			g_InodeTable.IPut(pNode);
			continue;	/* while循环 */
		}
	}
	return NULL;	/* GCC likes it! */
}

/*
 * @comment  释放存储设备dev上编号为number
 * 的外存INode，一般用于删除文件。
 */
void FileSystem::IFree(int number)
{
	SuperBlock* sb;

	sb = this->GetFS();	/* 获取相应设备的SuperBlock内存副本 */
	
	/* 
	 * 如果超级块直接管理的空闲Inode表上锁，
	 * 则释放的外存Inode散落在磁盘Inode区中。
	 */
	// if(sb->s_ilock)
	// {
	// 	return;
	// }

	/* 
	 * 如果超级块直接管理的空闲外存Inode超过100个，
	 * 同样让释放的外存Inode散落在磁盘Inode区中。
	 */
	if(sb->s_ninode >= 100)
	{
		return;
	}

	sb->s_inode[sb->s_ninode++] = number;

	/* 设置SuperBlock被修改标志 */
	sb->s_fmod = 1;
}

/*
 * @comment 在存储设备dev上分配空闲磁盘块
 */
Buf* FileSystem::Alloc()
{
	int blkno;	/* 分配到的空闲磁盘块编号 */
	SuperBlock* sb;
	Buf* pBuf;
	User& u = SecondFileKernel::Instance().GetUser();
	//pthread_mutex_t p_mutex;
	//pthread_mutex_init(&p_mutex, NULL);
	/* 获取SuperBlock对象的内存副本 */
	sb = this->GetFS();

	/* 
	 * 如果空闲磁盘块索引表正在被上锁，表明有其它进程
	 * 正在操作空闲磁盘块索引表，因而对其上锁。这通常
	 * 是由于其余进程调用Free()或Alloc()造成的。
	 */
	//pthread_mutex_lock(&p_mutex);
	pthread_mutex_lock(&sb->s_flock);//s_mfree--的写操作需要保护
	cout << "[Alloc] sb->s_flock上锁" <<endl;
	// while(sb->s_flock)
	// {
	// 	/* 进入睡眠直到获得该锁才继续 */
	// 	u.u_procp->Sleep((unsigned long)&sb->s_flock, ProcessManager::PINOD);
	// }

	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = sb->s_free[--sb->s_nfree];

	/* 
	 * 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	 * 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	 * 都意味着分配空闲磁盘块操作失败。
	 */
	if(0 == blkno )
	{
		sb->s_nfree = 0;
		printf("[error] No Space on device\n");
		//Diagnose::Write("No Space On %d !\n", dev);
		u.u_error = ENOSPC;
		return NULL;
	}
	if( this->BadBlock(sb, blkno) )
	{
		printf("[error] BadBlock, Alloc Failed\n");
		return NULL;
	}

	/* 
	 * 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	 * 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	 */
	if(sb->s_nfree <= 0)
	{
		/* 
		 * 此处加锁，因为以下要进行读盘操作，有可能发生进程切换，
		 * 新上台的进程可能对SuperBlock的空闲盘块索引表访问，会导致不一致性。
		 */
		//sb->s_flock++;

		/* 读入该空闲磁盘块 */
		pBuf = this->m_BufferManager->Bread(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int *)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		sb->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		memcpy(sb->s_free, p, 400);

		/* 缓存使用完毕，释放以便被其它进程使用 */
		this->m_BufferManager->Brelse(pBuf);

		/* 解除对空闲磁盘块索引表的锁，唤醒因为等待锁而睡眠的进程 */
		// sb->s_flock = 0;
	}
	pthread_mutex_unlock(&sb->s_flock);
	cout << "[Alloc] sb->s_flock解锁!" << endl;
	/* 普通情况下成功分配到一空闲磁盘块 */
	pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为该磁盘块申请缓存 */
	this->m_BufferManager->ClrBuf(pBuf);	/* 清空缓存中的数据 */
	sb->s_fmod = 1;	/* 设置SuperBlock被修改标志 */

	return pBuf;
}

/*
 * @comment 释放存储设备dev上编号为blkno的磁盘块
 */
void FileSystem::Free(int blkno)
{
	SuperBlock* sb;
	Buf* pBuf;
	//User& u = SecondFileKernel::Instance().GetUser();

	sb = this->GetFS();
	sb->s_fmod = 1;
	/* 
	 * 尽早设置SuperBlock被修改标志，以防止在释放
	 * 磁盘块Free()执行过程中，对SuperBlock内存副本
	 * 的修改仅进行了一半，就更新到磁盘SuperBlock去
	 */
	pthread_mutex_lock(&sb->s_flock);
	cout << "[Free] sb->s_flock上锁" <<endl;
	/* 如果空闲磁盘块索引表被上锁，则睡眠等待解锁 */
	// while(sb->s_flock)
	// {
	// 	u.u_procp->Sleep((unsigned long)&sb->s_flock, ProcessManager::PINOD);
	// }

	/* 检查释放磁盘块的合法性 */
	if(this->BadBlock(sb, blkno))
	{
		return;
	}

	/* 
	 * 如果先前系统中已经没有空闲盘块，
	 * 现在释放的是系统中第1块空闲盘块
	 */
	if(sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* 使用0标记空闲盘块链结束标志 */
	}

	/* SuperBlock中直接管理空闲磁盘块号的栈已满 */
	if(sb->s_nfree >= 100)
	{
		//sb->s_flock++;

		/* 
		 * 使用当前Free()函数正要释放的磁盘块，存放前一组100个空闲
		 * 磁盘块的索引表
		 */
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为当前正要释放的磁盘块分配缓存 */

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int *)pBuf->b_addr;
		
		/* 首先写入空闲盘块数，除了第一组为99块，后续每组都是100块 */
		*p++ = sb->s_nfree;
		/* 将SuperBlock的空闲盘块索引表s_free[100]写入缓存中后续位置 */
		memcpy(p, sb->s_free, 400);

		sb->s_nfree = 0;
		/* 将存放空闲盘块索引表的“当前释放盘块”写入磁盘，即实现了空闲盘块记录空闲盘块号的目标 */
		this->m_BufferManager->Bwrite(pBuf);
		// 解锁
		// sb->s_flock = 0;
	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock中记录下当前释放盘块号 */
	sb->s_fmod = 1;
	pthread_mutex_unlock(&sb->s_flock);
	cout << "[Free] sb->s_flock解锁!" << endl;
}

// ???????? 这是在干嘛
bool FileSystem::BadBlock(SuperBlock *spb, int blkno)
{
	return 0;
}
