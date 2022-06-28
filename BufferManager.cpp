// lyx
#include "BufferManager.h"
#include "SecondFileKernel.h"
#include<string.h>
#include<iostream>
using namespace std;
BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

void BufferManager::Initialize()
{
	cout<<"Initalize..."<<endl;
	int i;
	Buf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	// this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	for(i = 0; i < NBUF; i++)
	{
		// 控制的
		bp = &(this->m_Buf[i]);
		// bp->b_dev = -1;
		// 存的
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		// 链表中插入
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		pthread_mutex_init(&bp->buf_lock, NULL);
		pthread_mutex_lock(&bp->buf_lock);
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);

	}
	// this->m_DeviceManager = &Kernel::Instance().GetDeviceManager();
	return;
}


Buf* BufferManager::GetBlk(int blkno){
	
	Buf*headbp=&this->bFreeList; //取得自有缓存队列的队首地址
	Buf*bp; //返回的bp 
	// 查看bFreeList中是否已经有该块的缓存, 有就返回
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		//cout<<"block_no"<<bp->b_blkno<<endl;
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_BUSY;
		pthread_mutex_lock(&bp->buf_lock);
		//cout << "在缓存队列中找到对应的缓存，置为busy，GetBlk返回 blkno=" <<blkno<< endl;
		return bp;
	}
	// 没有 到队头找
	bp = this->bFreeList.b_forw;
	if (bp->b_flags&Buf::B_DELWRI)
	{
		// 如果有多西了，先写回
		//	cout << "分配到有延迟写标记的缓存，将执行Bwrite" << endl;
		//注：有延迟写标志，在这里直接写，不做异步IO的标志
		this->Bwrite(bp);
	}
	// Busy（锁
	//注：这里清空了其他所有的标志，只置为busy
	bp->b_flags = Buf::B_BUSY;
	pthread_mutex_lock(&bp->buf_lock);
	//注：我这里的操作是将头节点变成尾节点
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;

	bp->b_back = this->bFreeList.b_back->b_forw;
	this->bFreeList.b_back->b_forw = bp;
	bp->b_forw = &this->bFreeList;
	this->bFreeList.b_back = bp;

	bp->b_blkno = blkno;
//	cout << "成功分配到可用的缓存，getBlk将成功返回" << endl;
	return bp;
}
// 这是干啥的
// 可能是释放的
void BufferManager::Brelse(Buf* bp)
{
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息 
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	pthread_mutex_unlock(&bp->buf_lock);
	return;
}


Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;
	// 拷贝到内存
	memcpy(bp->b_addr,&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno],BufferManager::BUFFER_SIZE);
	/* 
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */
	// this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	/* 同步读，等待I/O操作结束 */
	// this->IOWait(bp);
	return bp;
}



void BufferManager::Bwrite(Buf *bp)
{
	// unsigned int flags;
	// flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	memcpy(&this->p[BufferManager::BUFFER_SIZE * bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
	this->Brelse(bp);

	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for(unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}
/* 队列中延迟写的缓存全部输出到磁盘 */
void BufferManager::Bflush()
{
	cout<<"Bflush"<<endl;
	Buf* bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */
// loop:
//	X86Assembly::CLI();
	for(bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if( (bp->b_flags & Buf::B_DELWRI)) //&& (dev == DeviceManager::NODEV || dev == bp->b_dev) )
		{
			// 把当前的buf从队列里拿出来（修改前面和后面buf的指针
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;
			// buf后向指针指向头的前一个？？？为啥
			bp->b_back = this->bFreeList.b_back->b_forw;
			// 头的后一个buf的前一个指向buf
			this->bFreeList.b_back->b_forw = bp;
			// buf的前向指向头
			bp->b_forw = &this->bFreeList;
			// 头的后向是buf
			this->bFreeList.b_back = bp;
			// 我们这里没有异步
			// bp->b_flags |= Buf::B_ASYNC;
			// this->NotAvail(bp);
			this->Bwrite(bp);
			// goto loop;
		}
	}
	// X86Assembly::STI();
	return;
}



void BufferManager::GetError(Buf* bp)
{
	User& u = SecondFileKernel::Instance().GetUser();

	if (bp->b_flags & Buf::B_ERROR)
	{
		u.u_error = EIO;
	}
	return;
}

// void BufferManager::NotAvail(Buf *bp)
// {
// 	X86Assembly::CLI();		/* spl6();  UNIX V6的做法 */
// 	/* 从自由队列中取出 */
// 	bp->av_back->av_forw = bp->av_forw;
// 	bp->av_forw->av_back = bp->av_back;
// 	/* 设置B_BUSY标志 */
// 	bp->b_flags |= Buf::B_BUSY;
// 	X86Assembly::STI();
// 	return;
// }

Buf* BufferManager::InCore( int blkno)
{
	cout<<"Incore"<<endl;
	Buf* bp;
	// Devtab* dp;
	// short major = Utility::GetMajor(adev);
	Buf*dp=  &this->bFreeList;
	// dp = this->m_DeviceManager->GetBlockDevice(major).d_tab;
	for(bp = dp->b_forw; bp != (Buf *)dp; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno)// && bp->b_dev == adev)
			return bp;
	}
	return NULL;
}

// Buf& BufferManager::GetSwapBuf()
// {
// 	return this->SwBuf;
// }

Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

