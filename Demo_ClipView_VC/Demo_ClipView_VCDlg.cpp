//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
// Demo_ClipView_VCDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "Demo_ClipView_VC.h"
#include "Demo_ClipView_VCDlg.h"
#include "afxdialogex.h"
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PI 3.141592653

// CDemo_ClipView_VCDlg 对话框

#define CANVAS_WIDTH	800
#define CANVAS_HEIGHT	600
#define INFO_HEIGHT		50
#define TESTDATA_XML1  "TestData1.xml"
#define TESTDATA_XML2  "TestData2.xml"

//如果为0，将不载入也不处理，以方便测试性能
#define TEST_LINES 1
#define TEST_CIRCLES 1

//是否绘出初始数据和裁剪结果，使用0来方便测试
#define TEST_DRAW_INITIAL 1
#define TEST_DRAW_ANSWER 1

#define MAX_THREAD_NUMBER 16

//裁剪算法相关定义
const int INTIALIZE=0;
const double ESP=1e-5;


struct ThreadTime
{
	int i;
	double time;
};

//记录每个线程所花的时间
vector<ThreadTime> thread_use_time;

//开始裁剪时的clock值
double startclock;

/////////////////////////////////////
// main part begins

struct  _param   {  
	vector<Line> thread_lines;
	vector<Circle> thread_circles;
	Boundary boundary;
	//CDemo_ClipView_VCDlg* dlg;
	int i;
	//CClientDC dc(this);
};

struct  _arc2draw   {  //这个结构是用于保存一个需要画的弧线
	CRect rect;
	CPoint start_point;
	CPoint end_point;
};

vector<Line> lines;
vector<Circle> circles;
Boundary boundary;

//用于存储需要画的线的全局变量的容器
vector<Line> lines_to_draw;   //用于存储所有要画的线
vector<_arc2draw> circles_to_draw;  //用于存储所有要画的弧线
vector<Line> sbLines;

CRITICAL_SECTION g_cs;  //声明保护临界区

void CDemo_ClipView_VCDlg::OnBnClickedBtnClip()
{
	//boundary里面可能会有连续两点相同的异常情况，先进行处理清除此异常情况
	vector<CPoint>::iterator pos;
	for (pos = boundary.vertexs.begin()+1; pos != boundary.vertexs.end(); pos++)
	{
		if ((*pos).x ==(*(pos-1)).x&&(*pos).y ==(*(pos-1)).y)
		{
			pos = boundary.vertexs.erase(pos);
			pos--;
		}
	}

	//先保留足够的空间，以免之后空间不够时发生内存拷贝
	lines_to_draw.reserve(2*lines.size());
	circles_to_draw.reserve(2*circles.size());

	//根据CPU数量确定线程数
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	int THREAD_NUMBER=info.dwNumberOfProcessors;
	if (THREAD_NUMBER>MAX_THREAD_NUMBER)
		THREAD_NUMBER=MAX_THREAD_NUMBER;
	//int THREAD_NUMBER=1;

	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));

	COLORREF clrBoundary = RGB(255,0,0);
	COLORREF clrLine = RGB(0,255,0);
	COLORREF clrCircle = RGB(0,0,255);
	CPen penLine,penCircle;
	penLine.CreatePen(PS_SOLID, 1, clrLine);
	penCircle.CreatePen(PS_SOLID, 1, clrCircle);

	DrawBoundary(boundary, clrBoundary);


	HANDLE hThead[MAX_THREAD_NUMBER];     //用于存储线程句柄
	DWORD  dwThreadID[MAX_THREAD_NUMBER]; //用于存储线程的ID
	_param* Info = new _param[MAX_THREAD_NUMBER]; //传递给线程处理函数的参数
	int nThreadLines=(int)(lines.size()/THREAD_NUMBER);
	int nThreadCircles = (int)(circles.size()/THREAD_NUMBER);

	InitializeCriticalSection(&g_cs);//初始化临界区变量

	for(int i=0;i<THREAD_NUMBER;i++){
		int upperNumber=(i+1)*nThreadLines;
		if (i==THREAD_NUMBER-1)
			upperNumber=lines.size();
		for (int j = i*nThreadLines; j < upperNumber; j++)
			Info[i].thread_lines.push_back(lines[j]);

		upperNumber=(i+1)*nThreadCircles;
		if (i==THREAD_NUMBER-1)
			upperNumber=circles.size();
		for (int j = i*nThreadCircles; j < upperNumber; j++)
			Info[i].thread_circles.push_back(circles[j]);

		//Info[i].dlg = this;	
		Info[i].i = i;
		Info[i].boundary = boundary;
	}

	//Info[THREAD_NUMBER-1].dlg = this;

	BeginTimeAndMemoryMonitor();

	startclock=clock();

	for (int i = 0;i<THREAD_NUMBER;i++)
	{
		hThead[i] = CreateThread(NULL,0,ThreadProc2,&Info[i],0,&(dwThreadID[i]));
		SetThreadAffinityMask(hThead[i],i+1);
	}

	//for (int i = 0; i < THREAD_NUMBER; i++)
	//	WaitForSingleObject(hThead[i],INFINITE);

	//等待线程全部返回
	WaitForMultipleObjects(THREAD_NUMBER,hThead,true,INFINITE);	

	if (TEST_DRAW_ANSWER)
	{
		//开始画线的代码
		dc.SelectObject(&penLine);
		for (vector<Line>::iterator iter=lines_to_draw.begin();iter!=lines_to_draw.end(); iter++)
		{
			dc.MoveTo((*iter).startpoint);
			dc.LineTo((*iter).endpoint);
		}

		//开始画圆的代码
		dc.SelectObject(&penCircle);
		for (vector<_arc2draw>::iterator iter=circles_to_draw.begin();iter!=circles_to_draw.end(); iter++)
			dc.Arc(&((*iter).rect),(*iter).start_point,(*iter).end_point);
	}

	EndTimeAndMemoryMonitor();

	DeleteCriticalSection(&g_cs); //程序完了  释放临界区变量

	lines_to_draw.clear();
	circles_to_draw.clear();
	penLine.DeleteObject();
	penCircle.DeleteObject();
}


DWORD WINAPI CDemo_ClipView_VCDlg::ThreadProc2(LPVOID lpParam)  
{  
	_param  * Info = ( _param *)lpParam; 

	double starttime=clock();
	ThreadTime a;
	a.i=Info->i;
	a.time=1.0*(starttime-startclock)/CLOCKS_PER_SEC;
	thread_use_time.push_back(a);

	if (TEST_LINES)
	{
		if (Info->boundary.isConvex)
			dealConvex(Info->thread_lines,Info->boundary);
		else
			dealConcave(Info->thread_lines,Info->boundary);
	}
	if (TEST_CIRCLES)
		forCircleRun(Info->thread_circles,Info->boundary);

	//thread_event[Info->i].SetEvent();

	double endtime=clock();
	a.i=Info->i;
	a.time=1.0*(endtime-startclock)/CLOCKS_PER_SEC;
	thread_use_time.push_back(a);
	//thread_use_time.push_back(1.0*(endtime-starttime)/CLOCKS_PER_SEC);

	return 0;  
}  
// main part ends
/////////////////////////////////////////



//////////////////////////////////
// 凸多边形线裁剪算法 开始

/*
*功能：判断线段是否在包围盒里，若在包围盒里则返回true，否则返回false
*参数：boundary为裁剪边界  line为所判断的线段
*思路：遍历多边形的顶点，以最小的x,y和最大的x,y值分别为包围盒的对角线端点
*/
bool InBox(Line& line){
	bool isVisible=true;
	int xl=INTIALIZE,xr=-INTIALIZE,yt=-INTIALIZE,yb=INTIALIZE; //对包围盒的边界值进行初始化
	int n=boundary.vertexs.size();
	for(int i=0;i<n-1;i++){
		if(boundary.vertexs[i].x<=xl) xl=boundary.vertexs[i].x;
		if(boundary.vertexs[i].x>=xr) xr=boundary.vertexs[i].x;
		if(boundary.vertexs[i].y>=yt) yt=boundary.vertexs[i].y;
		if(boundary.vertexs[i].y<=yb) yb=boundary.vertexs[i].y;
	}
	if(line.startpoint.x<xl&&line.endpoint.x<xl) return false;
	else if((line.startpoint.x>xr)&&(line.endpoint.x>xr)) return false;
	else if(line.startpoint.y>yt&&line.endpoint.y>yt) return false;
	else if(line.startpoint.y<yb&&line.endpoint.y<yb) return false;
	else return true;
}

/*
*功能：过点(x1,y1)(x2,y2)一般式直线方程为(y2-y1)x+(x1-x2)y+x2y1-x1y2=0，根据上述多项式判断(x,y)与(x1,y1)(x,2,y2)线段的位置,返回多项式值
*参数：(x1,y1),(x2,y2)为该线段两端端点的坐标值  (x,y）为所判断的点的坐标值
*思路：将被判断的点的坐标代入多项式中，得知其正负号
*/
int Multinomial(int x1,int y1,int x2,int y2,int x,int y){
	int result=(y2-y1)*x+(x1-x2)*y+x2*y1-x1*y2;
	return result;
}

/*
*功能：返回line1,line2两条直线的交点
*思路：根据直线的一般式方程，求直线直接的交点
*/
CPoint CrossPoint(Line& line1,Line& line2){
	CPoint CrossP;
	int a1 = line1.endpoint.y-line1.startpoint.y;
	int b1 = line1.startpoint.x-line1.endpoint.x;
	int c1 = line1.endpoint.x*line1.startpoint.y-line1.startpoint.x*line1.endpoint.y;

	int a2 = line2.endpoint.y-line2.startpoint.y;
	int b2 = line2.startpoint.x-line2.endpoint.x;
	int c2 = line2.endpoint.x*line2.startpoint.y-line2.startpoint.x*line2.endpoint.y;

	if((b1*a2-a1*b2)!=0){
		double tmp_y = (double)(a1*c2-c1*a2)/(double)(b1*a2-a1*b2);
		CrossP.y = (long) tmp_y;

		if(a1 != 0) 
		{
			double tmp_x = (-c1-b1* tmp_y )/a1;
			CrossP.x = (long) tmp_x;
		}
		else if(a2 !=0)
		{
			double tmp_x = (-c2-b2* tmp_y )/a2;
			CrossP.x = (long) tmp_x;
		}
	}

	return CrossP;	
}

/*
*功能：判断点pt是否在线line上，若在线上则返回true，否则返回false
*思路：将该直线的两个端点和被判断的端点传入Multinomial()函数中，若此多项式值为0则说明点在线段上
*/
bool IsOnline(CPoint& pt,Line& line){
	if(Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt.x,pt.y)==0)
		return 1;
	else 
		return 0;
}

/*
*功能：判断由pt1,pt2两个端点所在线段与线段line是否相交，若相交则返回true，否则返回false
*思路：分别将pt1,line的两个端点和pt2，line的两个端点传入Multinomial()函数中，若返回的两个多项式值异号则说明pt1,pt2所在线段和line相交
*/
bool Intersect(CPoint& pt1,CPoint& pt2,Line& line){
	int a1=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt1.x,pt1.y);
	int a2=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt2.x,pt2.y);
	if((a1<0&&a2>0)||(a1>0&&a2<0))
		return 1;
	else
		return 0;
}

/*
*功能：传入裁剪边界boundary和被裁剪线段line.返回裁剪后所显示线段
*思路：根据点与多边形的位置，计算出需要显示线段的端点，得出显示线段
*/
Line result(Line& line){
	Line line_result;
	line_result.startpoint.x=line_result.startpoint.y=INTIALIZE;
	line_result.endpoint.x=line_result.endpoint.y=INTIALIZE;

	bool bspt=isPointInBoundary(line.startpoint);//startpoint是否在多边形内，若是返回值为true

	bool bept=isPointInBoundary(line.endpoint);  //endpoint是否在多边形内，若是返回值为true
	//若两点都在多边形内则绘制该直线
	if(bspt&&bept)
		return line; 
	//若startpoint在多边形内，但endpoint在多边形外
	else if((bspt&&(!bept))||((!bspt)&&bept)){
		if(bspt&&(!bept))
			line_result.startpoint=line.startpoint;
		else if((!bspt)&&bept)
			line_result.startpoint=line.endpoint;

		//求交点
		int n=boundary.vertexs.size();
		int i=0;
		while(i<n-1){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			//过点(x1,y1)(x2,y2)一般式直线方程 (y2-y1)x+(x1-x2)y+x2y1-x1y2=0
			if(Intersect(side.startpoint,side.endpoint,line)){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=CrossPoint(line,side);
						break;
				}
				else i++;
			}
			else i++;
		}
		return line_result;
	}
	//startpoint和endpoint均在多边形外
	else if((!bspt)&&(!bept)){
		int n=boundary.vertexs.size();
		int i=0,nCrossPoint=0;
		while(i<(n-1)&&nCrossPoint!=2){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			bool isIntersect=Intersect(side.startpoint,side.endpoint,line);
			if(isIntersect&&line_result.startpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.startpoint=p;
						nCrossPoint++;
						i++;
				}
				else i++;
			}
			else if(isIntersect&&line_result.endpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=p;
						nCrossPoint++;
						i++;
				}
				else i++;
			}
			else i++;
		}
		return line_result;
	}
}


void dealConvex(vector<Line>& lines,Boundary& boundary)
{
	//此处处理的是凸多边形窗口
	for(int i=0;i<lines.size();i++){
		//先判断线段是否在包围盒内，若明显不在则不显示该线段，否则继续以下步骤
		if(InBox(lines[i])){
			Line line=result(lines[i]);
			//lines_to_draw.push_back(line);
			if(line.startpoint.x!=INTIALIZE &&line.startpoint.y!=INTIALIZE && line.endpoint.x!=INTIALIZE && line.endpoint.y!=INTIALIZE)
				lines_to_draw.push_back(line);
		}
	}
	//ClearTestLines();
}

// 凸多边形线裁剪算法 结束
//////////////////////////////////















//////////////////////////////////
// 任意多边形线裁剪算法 开始

/*
*功能：计算两个向量的叉积
*参数：a1,a2为第一个向量的起点与终点，b1,b2为第二个向量的起点与终点
*/
int CrossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2)
{
	int ux=a2.x-a1.x;
	int uy=a2.y-a1.y;
	int vx=b2.x-b1.x;
	int vy=b2.y-b1.y;
	return ux*vy-uy*vx;
}

/*
*功能：用于交点排序的比较函数（按t值得从小到大排序）
*参数：a,b分别为两个交点
*/
bool Compare(IntersectPoint a,IntersectPoint b)
{
	if (a.t<b.t)
		return true;
	else
		return false;
}

/*
*功能：判断多边形每个点是凹点还是凸点
*思路：求相邻两边的向量的叉积，已知凸点的值与凹点的值正负性不同，并且x坐标最大的点一定是凸点。
*	   先计算相邻两边的向量的叉积并保存，同时找出x最大的点。再通过与x最大的点比较正负性得出点的凹凸性。
*/
void JudgeConvexPoint(vector<BOOL>& convexPoint)
{
	int dotnum=boundary.vertexs.size()-1;//多边形顶点数
	vector<int> z;
	int max=-1,maxnum=-1;
	for (int i=0;i<dotnum;i++)
	{
		//当前点为p2,其前一点为p1,后一点为p3
		CPoint* p1;
		if (i==0)
			p1=&boundary.vertexs[dotnum-1];
		else
			p1=&boundary.vertexs[i-1];
		CPoint* p2=&boundary.vertexs[i];
		CPoint* p3=&boundary.vertexs[i+1];
		int crossmulti=CrossMulti(*p1,*p2,*p2,*p3);//计算相邻两条边的叉积（即p1p2与p2p3）

		z.push_back(crossmulti);
		if (p2->x>max)//找出x最大的点，记录
		{
			max=p2->x;
			maxnum=i;
		}
	}

	//通过与x最大的点比较正负性得出点的凹凸性。
	for (int i=0;i<dotnum;i++)  
		if ((z[maxnum]>0 && z[i]>0)||(z[maxnum]<0 && z[i]<0))
			convexPoint.push_back(true);
		else
			convexPoint.push_back(false);
	convexPoint.push_back(convexPoint[0]);
}

/*
*功能：计算多边形的边的内法向量
*思路：绕多边形的边界依次计算。对于某一条边，先随意构造出一个与当前边垂直的法向量。
*	   若该法向量与前一条边的向量的点积为正，则将法向量反向。
*	   如果这两条边的公共顶点为凹点，则将该法向量再反向。
*/
void ComputeNormalVector(vector<Vector>& normalVector,vector<BOOL>& convexPoint)
{
	int edgenum=boundary.vertexs.size()-1;//多边形边数

	for (int i=0;i<edgenum;i++)
	{
		CPoint p1=boundary.vertexs[i];
		CPoint p2=boundary.vertexs[i+1];//p1p2为多边形的边

		//求内法向量
		CPoint p0;//p0为p1前面一个顶点
		if (i==0)
			p0=boundary.vertexs[edgenum-1];
		else
			p0=boundary.vertexs[i-1];
		double n1,n2;
		//先任意算出一个法向量
		if (p2.y-p1.y==0)
		{
			n2=1;
			n1=0;
		}
		else
		{
			n1=1;
			n2=-(double)(p2.x-p1.x)/(p2.y-p1.y);
		}
		if ((p1.x-p0.x)*n1+(p1.y-p0.y)*n2>0)//与前一条边点乘，如果方向错了，使法向量反向
		{
			n1=-n1;
			n2=-n2;
		}
		if (!convexPoint[i])//如果是凹点，法向量再反向
		{
			n1=-n1;
			n2=-n2;
		}

		Vector nv;
		nv.x=n1;
		nv.y=n2;
		normalVector.push_back(nv);
	}
}


/*
*功能：处理凹多边形的裁剪
*思路：先预处理判断出每个点的凹凸性，算出所有多边形的边的外矩形框。
*	   然后枚举每条线段与每条多边形的边，先通过快速排除法排除明显不可能相交的边，再通过跨立试验进一步判断是否有交点。
*	   接着用改进的cyrus-beck算法算出交点的值，以及交点的性质（出点还是入点），并将线段的起点作为入点，终点作为出点。
*	   然后处理当线段与多边形的顶点相交的特殊情况。
*	   最后寻找所有符合“入点-出点”的线段作为显示的线段。
*/
void dealConcave(vector<Line>& lines,Boundary& boundary)
{
	//TODO 在此处完成裁剪算法和裁剪后显示程序
	COLORREF clrLine = RGB(0,255,0);
	vector<BOOL> convexPoint;//多边形的点的凹凸性，true为凸点
	vector<RECT> edgeRect;//多边形的边的外矩形框
	vector<Vector> normalVector;//多边形的边的内法向量
	vector<IntersectPoint> intersectPoint;//线段的所有交点（也包括线段的起点与终点）
	JudgeConvexPoint(convexPoint);
	ComputeNormalVector(normalVector,convexPoint);

	RECT whole;
	whole.left=100000;
	whole.right=-1;
	whole.top=-1;
	whole.bottom=10000;
	//先算出所有多边形的边的外矩形框
	int edgenum=boundary.vertexs.size()-1;
	for (int i=0;i<edgenum;i++)
	{
		RECT r;
		r.bottom=min(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.top=max(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.left=min(boundary.vertexs[i].x,boundary.vertexs[i+1].x);
		r.right=max(boundary.vertexs[i].x,boundary.vertexs[i+1].x);

		if (r.bottom<whole.bottom) whole.bottom=r.bottom;
		if (r.top>whole.top) whole.top=r.top;
		if (r.left<whole.left) whole.left=r.left;
		if (r.right>whole.right) whole.right=r.right;
		edgeRect.push_back(r);
	}


	int linenum=lines.size();
	RECT r;
	for (int i=0;i<linenum;i++)//枚举每条线段
	{
		r.bottom=min(lines[i].startpoint.y,lines[i].endpoint.y);
		r.top=max(lines[i].startpoint.y,lines[i].endpoint.y);
		r.left=min(lines[i].startpoint.x,lines[i].endpoint.x);
		r.right=max(lines[i].startpoint.x,lines[i].endpoint.x);

		//将线段起点当作入点放在首位
		intersectPoint.clear();
		IntersectPoint ip1;
		ip1.isIntoPoly=true;
		ip1.t=0;
		ip1.point=lines[i].startpoint;
		intersectPoint.push_back(ip1);

		BOOL haveIntersect=false;
		for (int j=0;j<edgenum;j++)//枚举每条多边形的边
		{
			//快速排除法
			if (r.left>edgeRect[j].right || r.right<edgeRect[j].left ||
				r.top<edgeRect[j].bottom || r.bottom>edgeRect[j].top)
				continue;

			//跨立试验
			CPoint p1=boundary.vertexs[j];
			CPoint p2=boundary.vertexs[j+1];//p1p2为多边形的边
			CPoint q1=lines[i].startpoint;
			CPoint q2=lines[i].endpoint;//q1q2为线段


			long long a= ((long long)CrossMulti(p1,q1,p1,p2)) * (CrossMulti(p1,p2,p1,q2));
			long long b= ((long long)CrossMulti(q1,p1,q1,q2)) * (CrossMulti(q1,q2,q1,p2));
			if (!(a>=0 && b>=0))//不相交就跳过这条边
				continue;

			//取出内法向量
			double n1=normalVector[j].x;
			double n2=normalVector[j].y;

			//cyrus-beck算法 算出参数t
			double t1=n1*(q2.x-q1.x)+n2*(q2.y-q1.y);
			double t2=n1*(q1.x-p1.x)+n2*(q1.y-p1.y);
			double t=-t2/t1;
			CPoint pt;
			pt.x=(LONG)((q2.x-q1.x)*t+q1.x);
			pt.y=(LONG)((q2.y-q1.y)*t+q1.y);
			if (t>=0 && t<=1) //如果算出的参数t满足 t>=0 && t<=1, 说明交点在线段上，将它加入交点集
			{
				IntersectPoint ip;
				if (t1>0)
					ip.isIntoPoly=true;
				else
					ip.isIntoPoly=false;
				ip.t=t;
				ip.point=pt;
				ip.contexPoint=j;
				intersectPoint.push_back(ip);

				haveIntersect=true;
			}
			else
			{
				//error
				int a=0;
			}
		}


		//没有交点
		if (!haveIntersect)
		{
			if (isPointInBoundary(lines[i].startpoint))//如果在多边形内
				//dlg->DrawLine(lines[i],clrLine);
			{
				EnterCriticalSection(&g_cs); 
				lines_to_draw.push_back(lines[i]);
				LeaveCriticalSection(&g_cs); 
			}
			continue;
		}

		//将线段终点当作出点放在末位
		IntersectPoint ip2;
		ip2.isIntoPoly=false;
		ip2.t=1;
		ip2.point=lines[i].endpoint;
		intersectPoint.push_back(ip2);


		std::sort(intersectPoint.begin()+1,intersectPoint.end()-1,Compare);//按t从小到大排序

		//如果有两点t值相同（即线段与多边形的顶点相交的特殊情况），并且一个是入点，一个是出点，则根据顶点的凹凸性重新安排入、出（不能动第一个点和最后的点，即线段的端点）
		vector<IntersectPoint>::iterator iter = intersectPoint.begin();
		iter++;
		for (;iter!= intersectPoint.end()&&(iter+1)!= intersectPoint.end()&&(iter+2)!= intersectPoint.end();iter++ )
			if ( ((*iter).isIntoPoly ^ (*(iter+1)).isIntoPoly )
				&&abs((*iter).t-(*(iter+1)).t)<1e-9)  
			{
				//得到该顶点序号
				int contexPointNumber=max((*iter).contexPoint,(*(iter+1)).contexPoint);
				if (((*iter).contexPoint==edgenum-1 && (*(iter+1)).contexPoint==0)||((*(iter+1)).contexPoint==edgenum-1 && (*iter).contexPoint==0))
					contexPointNumber=0;

				if (convexPoint[contexPointNumber]&& !(*iter).isIntoPoly)//如果是凸点，安排为先进后出
				{
					(*iter).isIntoPoly=true;
					(*(iter+1)).isIntoPoly=false;
				}
				else if (!convexPoint[contexPointNumber] && (*iter).isIntoPoly)//如果是凹点，安排为先出后进
				{
					(*iter).isIntoPoly=false;
					(*(iter+1)).isIntoPoly=true;
				}

			}


		//保存交点
		int size=(int)intersectPoint.size()-1;
		for (int j=0;j<size;j++)
		{
			if (intersectPoint[j].isIntoPoly && !intersectPoint[j+1].isIntoPoly)
			{
				Line l;
				l.startpoint=intersectPoint[j].point;
				l.endpoint=intersectPoint[j+1].point;
				/*if (!isPointInBoundary(l.startpoint) || !isPointInBoundary(l.endpoint))
				{
					EnterCriticalSection(&g_cs); 
					sbLines.push_back(l);
					LeaveCriticalSection(&g_cs); 
					continue;
				}*/
				//if (l.startpoint.x<whole.left || l.startpoint.x>whole.right || l.startpoint.y<whole.bottom || l.startpoint.y>whole.top
				//	||l.endpoint.x<whole.left || l.endpoint.x>whole.right || l.endpoint.y<whole.bottom || l.endpoint.y>whole.top)
				//{
				//	EnterCriticalSection(&g_cs); 
				//	sbLines.push_back(lines[i]);
				//	LeaveCriticalSection(&g_cs); 
				//	continue;
				//}

				EnterCriticalSection(&g_cs); 
				lines_to_draw.push_back(l);
				LeaveCriticalSection(&g_cs); 
				/*dlg->DrawLine(l,clrLine);*/
				j++;
			}
		}
		//abcdef
	}

}

// 任意多边形线裁剪算法 结束
/////////////////////////////////












//////////////////////////////////
// 任意多边形圆裁剪算法 开始

void  forCircleRun(vector<Circle>& circles,Boundary& boundary)
{

	for(unsigned int i = 0;i<circles.size();i++){  //再次开始遍历每一个圆
		vector<r_lineNum> point_Array;   //用于存储每个圆与多边形窗口的交点
		getInterpointArray(point_Array,i,circles);  //获得存储交点的容器

		if(point_Array.size()==0||point_Array.size()==1) //将完全跟多边形不相交以及相切的情况的圆排除，
		{
			bool inBoundary = isPointInBoundary(circles[i].center); //圆心若在多边形内
			if (inBoundary==true)
			{
				long _x = boundary.vertexs[0].x-circles[i].center.x;
				long _y = boundary.vertexs[0].y-circles[i].center.y;
				bool t = (_x*_x+_y*_y)>(circles[i].radius*circles[i].radius);
				if (t==true)   //若圆的半径比较小
				{
					//画整个圆
					CRect rect(circles[i].center.x - circles[i].radius,circles[i].center.y - circles[i].radius,circles[i].center.x + circles[i].radius,circles[i].center.y + circles[i].radius);
					CPoint start_point,end_point;
					start_point.x = 0; start_point.y = 0;
					end_point.x = 0; end_point.y = 0;
					/*CClientDC dc(dlg);
					dc.Arc(&rect, start_point, end_point);*/
					struct _arc2draw arc2draw ;
					arc2draw.rect = rect;
					arc2draw.start_point = start_point;
					arc2draw.end_point = end_point;
					EnterCriticalSection(&g_cs);  
					circles_to_draw.push_back(arc2draw);
					LeaveCriticalSection(&g_cs); 
				}
			}
		}
		//没有交点以及相切的情况讨论完毕
		//开始讨论一般的情况
		else
		{

			point_Array.push_back(point_Array[0]); //为了便于讨论和计算，在整个交点容器的最后加入第一个交点
			//当圆刚好过多边形的某一点时，会出现该点被记录两次的情况，故需要进行排除
			for (int j = 0; j < point_Array.size()-1; j++)
			{
				if (point_Array[j].point.x ==point_Array[j+1].point.x&&point_Array[j].point.y ==point_Array[j+1].point.y)
				{
					point_Array.erase(point_Array.begin() + j);
					j--;
				}
			}
			//开始对交点容器中的每一个交点进行遍历
			for (unsigned int j = 0; j < point_Array.size()-1; j++)
			{
				CPoint mid_point = getMiddlePoint(point_Array,i,j,circles); //获得两交点的在圆上的中间点
				bool mid_in_bound = isPointInBoundary(mid_point);  //判断中间点是否在多边形窗口内
				if(mid_in_bound == true) //在多边形窗口内，则画整个圆弧 
				{				
					int start_x = circles[i].center.x-circles[i].radius;
					int start_y = circles[i].center.y-circles[i].radius;
					int end_x =  circles[i].center.x+circles[i].radius;
					int end_y = circles[i].center.y+circles[i].radius;

					CRect rect(start_x,start_y,end_x,end_y);
					//dc.Arc(&rect,point_Array[j].point,point_Array[j+1].point);
					struct _arc2draw arc2draw ;
					arc2draw.rect = rect;
					arc2draw.start_point = point_Array[j].point;
					arc2draw.end_point = point_Array[j+1].point;
					EnterCriticalSection(&g_cs);  
					circles_to_draw.push_back(arc2draw);
					LeaveCriticalSection(&g_cs); 

				}
			}
		}

	}
}
/*
*功能：给出圆的圆点的坐标和圆上某点的坐标以及半径的长度，通过圆的参数方程，获得该点的角度
*思路：通过三角函数即可求得。
*/
double getAngle(long x1,long x2,long y1,long y2,double r)
{
	double cos = (double)(x1-x2)/(double)r;
	double sin = (double)(y1-y2)/(double)r;
	if (sin>0)
	{
		double a = acos(cos);
		return a;
	}
	else
	{
		double a = PI+PI- acos(cos);
		return a;
	}
}
/*
*功能：获得多边形窗口与圆的相邻的两个交点在圆上的的中间点。
*思路：通过圆的参数方程，将两交点的坐标代入方程中，求得两交点各自的角度，（使用getAngle函数）
*	   然后通过三角计算，获得中间点的角度，将此角度代入圆的参数方程，就可以求得中间点的坐标
*/
CPoint getMiddlePoint(vector<r_lineNum>& point_Array,int i,int j,vector<Circle>& circles2)
{
	double a1 = getAngle(point_Array[j].point.x,circles2[i].center.x,point_Array[j].point.y,circles2[i].center.y,circles2[i].radius);
	double a2 = getAngle(point_Array[j+1].point.x,circles2[i].center.x,point_Array[j+1].point.y,circles2[i].center.y,circles2[i].radius);
	double mid_a = 0;

	if(a1<PI&&a2<PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}

	}
	else if (a1<PI&&a2>PI)
	{
		if(a1+a2>2*PI)
		{
			mid_a = (a1+a2)/2-PI;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}

	}
	else if (a2<PI&&a1>PI)
	{

		mid_a = (a1+a2)/2;
	}
	else if (a2>PI&&a1>PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2-PI;
		}

	}
	double x = circles2[i].center.x+circles2[i].radius*cos(mid_a);
	double y = circles2[i].center.y+circles2[i].radius*sin(mid_a);
	CPoint point ;
	point.x = x;
	point.y = y;
	return point;
}
/*
*功能：判断点是否在多边形窗口内
*思路：根据交点法求得某点是否在多边形的窗口内部。通过该点取平行于y轴的直线l，
*	   先判断直线是否穿过多边形窗口的点。若穿过，判断与该点相邻的两边是否在l的两边，
*	   若在，则交点数加一，若在同一边，则交点数加二。
*	   然后判断多边形窗口的每一条边是否与直线l有交点，
*	   若有，则交点数加一。最后若交点数为偶数，则该点在多边形窗口外部，若为奇数，则该点在多边形窗口内部。
*/
bool isPointInBoundary(CPoint& point)
{
	int interNum = 0;
	long x1 = point.x;
	long y1 = point.y;
	long x2 = x1;
	long y2 = 0;
	for (unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		bool tmp = (boundary.vertexs[i].x==x1)&&(boundary.vertexs[i].y<y1);
		if ((boundary.vertexs[i].x==x1)&&(boundary.vertexs[i].y<y1)) //对边界点在线上和连续两边界点在线上的情况进行讨论
		{
			if (i==(boundary.vertexs.size()-1))
			{
				long between1 =boundary.vertexs[i-1].x-x1;
				long between2 =boundary.vertexs[1].x-x1;
				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
				else if(between1==0)
				{
					long between3 =boundary.vertexs[i-2].x-x1;
					if ((between3>0&&between2<0)||(between3<0&&between2>0))
					{
						interNum++;
					}
				}
				/*else if(between2==0)
				{
					long between3 =boundary.vertexs[2].x-x1;
					if ((between3>0&&between1<0)||(between3<0&&between1>0))
					{
						interNum++;
					}
				}*/

			}
			else if (i==(boundary.vertexs.size()-2))
			{
				long between1 =boundary.vertexs[i-1].x-x1;
				long between2 =boundary.vertexs[i+1].x-x1;
				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
				else if(between1==0)
				{
					long between3 =boundary.vertexs[i-2].x-x1;
					if ((between3>0&&between2<0)||(between3<0&&between2>0))
					{
						interNum++;
					}
				}
				/*else if(between2==0)
				{
					long between3 =boundary.vertexs[1].x-x1;
					if ((between3>0&&between1<0)||(between3<0&&between1>0))
					{
						interNum++;
					}
				}*/

			}
			else if (i==1)
			{
				long between1 =boundary.vertexs[2].x-x1;
				long between2 =boundary.vertexs[0].x-x1;
				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
				/*else if(between1==0)
				{
					long between3 =boundary.vertexs[3].x-x1;
					if ((between3>0&&between2<0)||(between3<0&&between2>0))
					{
						interNum++;
					}
				}*/
				else if(between2==0)
				{
					long between3 =boundary.vertexs[boundary.vertexs.size()-2].x-x1;
					if ((between3>0&&between1<0)||(between3<0&&between1>0))
					{
						interNum++;
					}
				}

			}
			else
			{
				long between1 =boundary.vertexs[i-1].x-x1;
				long between2 =boundary.vertexs[i+1].x-x1;
				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
				else if(between1==0)
				{
					long between3 =boundary.vertexs[i-2].x-x1;
					if ((between3>0&&between2<0)||(between3<0&&between2>0))
					{
						interNum++;
					}
				}
				/*else if(between2==0)
				{
					long between3 =boundary.vertexs[i+2].x-x1;
					if ((between3>0&&between1<0)||(between3<0&&between1>0))
					{
						interNum++;
					}
				}*/
			}
		}
	}

	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		long between1 =boundary.vertexs[i].x-x1;
		long between2 =boundary.vertexs[i-1].x-x1;
		if((between1>0&&between2<0)||(between1<0&&between2>0))
		{
			long x3 = boundary.vertexs[i].x;
		    long x4 = boundary.vertexs[i-1].x;
			long y3 = boundary.vertexs[i].y;
			long y4 = boundary.vertexs[i-1].y;
			long a = y4-y3;
			long b = x3-x4;
			long c = x4*y3-x3*y4;
			long long isInter1 = (long long)(a*x1+b*y1+c);
			long long isInter2 = (long long)(a*x2+b*y2+c);
			if((isInter1<=0&&isInter2>=0)||(isInter1>=0&&isInter2<=0))
			{
				interNum++;
			}

		}
	}
	if(interNum%2!=0)
	{
		return true;
	}
	else
	{
		return false;
	}

}

/*
*功能：通过直线参数方程的参数，直线的两交点的坐标，获得该直线上的一个点。
*思路：通过直线的参数方程可以求得。
*/
struct r_lineNum getInterpoint(double t,int x1,int x2,int y1,int y2,int i)
{
	long x = x1+(x2-x1)*t;
	long y = y1+(y2-y1)*t;
	struct r_lineNum pit;
	pit.point.x = x;
	pit.point.y = y;
	pit.num_line = i;
	return pit;
}

/*
*功能：获得圆与多边形的交点的集合，且交点顺序按照逆时针的方向存入point_Array的容器中
*思路：逆时针遍历多边形的每一条边，根据直线到圆心的距离判断是否有交点，若有交点，将直线
*	   上的点的参数方程的形式代入圆的表达式，化简得到A*t^2+B*t+C=0的形式（t是直线参数方程中的参数，介于0-1）
*	   求出A,B,C的值，若 B^2-4*A*C=0,则只有一个解，且解为(-B)/(2*A)，
*	   若B^2-4*A*C>0有两个解,一个解为((-B)+sqrt(B^2-4*A*C))/(2*A),另一个解为((-B)-sqrt(B^2-4*A*C))/(2*A)
*	   然后，对解的值进行判断，若解大于等于0，解小于等于1，则是交点，否则舍去。将解值代入参数方程即可求得交点。
*	   最后，对交点的顺序进行判断，解值小的值先存入point_Array容器。
*/
void getInterpointArray(vector<r_lineNum>& point_Array,int circle_num,vector<Circle>& circle2)
{
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		long x1 = boundary.vertexs[i].x;
		long x2 = boundary.vertexs[i-1].x;
		long y1 = boundary.vertexs[i].y;
		long y2 = boundary.vertexs[i-1].y;
		long x0 = circle2[circle_num].center.x;
		long y0 = circle2[circle_num].center.y;
		long a = y2-y1;
		long b = x1-x2;
		long c = x2*y1-x1*y2;
		double _abs = abs((double)(a*x0+b*y0+c));
		double _sqrt = sqrt((double)(a*a+b*b));
		double d = _abs/_sqrt;
		if (d>circle2[circle_num].radius)
		{
			continue;
		}
		long long A = x1*x1+x2*x2+y1*y1+y2*y2-2*x1*x2-2*y1*y2;
		long long B = 2*(x0*x1+x1*x2+y0*y1+y1*y2-x0*x2-y0*y2-x1*x1-y1*y1);
		long long C = x0*x0+x1*x1+y0*y0+y1*y1-2*x0*x1-2*y0*y1-circle2[circle_num].radius*circle2[circle_num].radius;
		long long key = B*B-4*A*C;
		if(key == 0)
		{
			double t = ((double)(-B)/(double)(2*A));
			if (t>=0&&t<=1)
			{
				struct r_lineNum pit = getInterpoint(t,x1,x2,y1,y2,i);
				point_Array.push_back(pit);
			}
			else
			{
				continue;
			}
		}
		else if(key > 0)
		{
			double t1 = (double)((-B+sqrt(key))/(2*A));
			double t2 = (double)((-B-sqrt(key))/(2*A));
			if (t1>t2)
			{
				double tmp = t2;
				t2 = t1;
				t1 =tmp;
			}
			if (t1>=0&&t1<=1)
			{				
				struct r_lineNum pit = getInterpoint(t1,x1,x2,y1,y2,i);
				point_Array.push_back(pit);
				if (t2>=0&&t2<=1)
				{
					struct r_lineNum pit = getInterpoint(t2,x1,x2,y1,y2,i);
					point_Array.push_back(pit);
				}

			}
			else
			{
				if (t2>=0&&t2<=1)
				{
					struct r_lineNum pit = getInterpoint(t2,x1,x2,y1,y2,i);
					point_Array.push_back(pit);
				}
				else
				{
					continue;
				}
			}
		}
	}
	if (point_Array.size()>2)
	{
		
		//以下代码用于将所有的交点按照顺时针排序
		vector<CPoint> y_up_0;
		vector<CPoint> y_down_0;
		for (int i = 0; i < point_Array.size(); i++)
		{
			if (point_Array[i].point.y <= circle2[circle_num].center.y)
			{
				y_down_0.push_back(point_Array[i].point);
			}
			else
			{
				y_up_0.push_back(point_Array[i].point);
			}
		}
		if (y_down_0.size()>1)
		{
			for (int i = 0; i < y_down_0.size()-1; i++)
			{
				for (int j = 0; j < y_down_0.size()-1-i; j++)
				{
					if (y_down_0[j].x < y_down_0[j+1].x)
					{
						CPoint tmp;
						tmp.x = y_down_0[j].x;
						tmp.y = y_down_0[j].y;
						y_down_0[j] = y_down_0[j+1];
						y_down_0[j+1] = tmp;
					}
					else if(y_down_0[j].x == y_down_0[j+1].x) //这个可能是由于计算精度损失导致同x不同y的情况
					{
						if (y_down_0[j].x < circle2[circle_num].center.x)
						{
							if (y_down_0[j].y > y_down_0[j+1].y)
							{
								CPoint tmp;
								tmp.x = y_down_0[j].x;
								tmp.y = y_down_0[j].y;
								y_down_0[j] = y_down_0[j+1];
								y_down_0[j+1] = tmp;
							}
						}
						else
						{
							if (y_down_0[j].y < y_down_0[j+1].y)
							{
								CPoint tmp;
								tmp.x = y_down_0[j].x;
								tmp.y = y_down_0[j].y;
								y_down_0[j] = y_down_0[j+1];
								y_down_0[j+1] = tmp;
							}

						}
					}
				}
			}
		}
		if (y_up_0.size()>1)
		{
			for (int i = 0; i < y_up_0.size()-1; i++)
			{
				for (int j = 0; j < y_up_0.size()-1-i; j++)
				{
					if (y_up_0[j].x > y_up_0[j+1].x)
					{
						CPoint tmp;
						tmp.x = y_up_0[j].x;
						tmp.y = y_up_0[j].y;
						y_up_0[j] = y_up_0[j+1];
						y_up_0[j+1] = tmp;
					}
				}
			}
		}
		if (y_down_0.size()>0)
		{
			for (int i = 0; i < y_down_0.size(); i++)
			{
				struct r_lineNum tmp;
				tmp.point = y_down_0[i];
				tmp.num_line = 0;
				point_Array[i] = tmp;
			}
		}
		if (y_up_0.size()>0)
		{
			for (int i = 0; i < y_up_0.size(); i++)
			{
				struct r_lineNum tmp;
				tmp.point = y_up_0[i];
				tmp.num_line = 0;
				point_Array[y_down_0.size()+i] = tmp;
			}
		}
		

	
	//以上代码用于将所有的交点按照顺时针排序

	}
}


// 任意多边形圆裁剪算法 结束
/////////////////////////////////














CDemo_ClipView_VCDlg::CDemo_ClipView_VCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDemo_ClipView_VCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemo_ClipView_VCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLIP, m_btn_clip);
	DDX_Control(pDX, IDC_STATIC_DRAWING, m_stc_drawing);
	DDX_Control(pDX, IDC_STATIC_INFO_1, m_stc_info1);
	DDX_Control(pDX, IDC_STATIC_INFO_2, m_stc_info2);
}

BEGIN_MESSAGE_MAP(CDemo_ClipView_VCDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLIP, &CDemo_ClipView_VCDlg::OnBnClickedBtnClip)
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()


BOOL CDemo_ClipView_VCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	CRect clientRect;
	GetClientRect(&clientRect);
	CRect windowRect;
	GetWindowRect(&windowRect);
	int width = CANVAS_WIDTH + windowRect.Width() - clientRect.Width();
	int height = CANVAS_HEIGHT + INFO_HEIGHT + windowRect.Height() - clientRect.Height();
	SetWindowPos(NULL, 0, 0, width, height, SWP_NOMOVE|SWP_NOZORDER);

	m_btn_clip.SetWindowPos(NULL, 50, CANVAS_HEIGHT + 10, 80, 30, SWP_NOZORDER);
	m_stc_drawing.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 15, 200, 20, SWP_NOZORDER);
	m_stc_info1.SetWindowPos(NULL, 350, CANVAS_HEIGHT + 5, 450, 20, SWP_NOZORDER);
	m_stc_info2.SetWindowPos(NULL, 350, CANVAS_HEIGHT + 25, 450, 20, SWP_NOZORDER);

	hasOutCanvasData = FALSE;
	GetModuleFileName(NULL,curPath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
	curPath.ReleaseBuffer();
	curPath = curPath.Left(curPath.ReverseFind('\\') + 1);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDemo_ClipView_VCDlg::OnPaint()
{

	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CDemo_ClipView_VCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CDemo_ClipView_VCDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg=LOWORD(wParam);
	switch(uMsg)
	{
	case ID_TESTCASE1:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "1");
		}
		break;
	case ID_TESTCASE2:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "2");
		}
		break;
	case ID_TESTCASE3:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "3");
		}
		break;
	case ID_TESTCASE4:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "4");
		}
		break;
	case ID_TESTCASE5:
		{
			CString xmlPath = curPath + TESTDATA_XML2;
			DrawTestCase(xmlPath, "5");
		}
		break;
	default:
		break;
	}
	return CDialogEx::OnCommand(wParam, lParam);
}

void CDemo_ClipView_VCDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (HTCAPTION == nHitTest) {
		return;
	}
	CDialogEx::OnNcLButtonDown(nHitTest, point);
}



BOOL CDemo_ClipView_VCDlg::XmlNodeToPoint(pugi::xml_node node, CPoint& piont)
{
	pugi::xml_node pointNode = node.first_child();
	CString strValue(pointNode.value());
	int curPos = 0;
	int count = 0;
	while(TRUE)
	{
		CString resToken = strValue.Tokenize(",", curPos);
		if (resToken.IsEmpty())
			break;
		int coord = _ttoi(resToken);
		count++;
		if (count == 1)
		{
			piont.x = coord;
		}
		else if (count == 2)
		{
			piont.y = CANVAS_HEIGHT - coord; //调整CAD与窗口显示的坐标系一致
		}
	}
	if (count != 2)
	{
		return FALSE;
	}
	return TRUE;
}


int  CDemo_ClipView_VCDlg::SplitCStringToArray(CString str,CStringArray& strArray, CString split)
{
	int curPos = 0;
	while(TRUE)
	{
		CString resToken = str.Tokenize(split, curPos);
		if (resToken.IsEmpty())
			break;
		strArray.Add(resToken);
	}
	return strArray.GetSize();
}
BOOL CDemo_ClipView_VCDlg::IsPointOutCanvas(CPoint point)
{
	if (point.x < 0 || point.y < 0 || point.x > CANVAS_WIDTH || point.y > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsLineOutCanvas(Line line)
{
	if (IsPointOutCanvas(line.startpoint) || IsPointOutCanvas(line.endpoint))
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsCircleOutCanvas(Circle circle)
{
	if (IsPointOutCanvas(circle.center))
	{
		return TRUE;
	}
	if ((circle.center.x - circle.radius) < 0 || (circle.center.x + circle.radius) > CANVAS_WIDTH
		|| (circle.center.y - circle.radius) < 0 || (circle.center.y + circle.radius) > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsBoundaryOutCanvas(Boundary boundary)
{
	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	for (;iter != boundary.vertexs.end(); iter++)
	{
		if (IsPointOutCanvas(*iter))
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CDemo_ClipView_VCDlg::LoadTestCaseData(CString xmlPath, CString caseID)
{
	pugi::xml_document doc;
	if (!doc.load_file(xmlPath)) return FALSE;
	pugi::xml_node root = doc.child("TestRoot");
	pugi::xml_node testCase = root.find_child_by_attribute("ID",caseID);
	if (!testCase) return FALSE;
	pugi::xml_attribute complex = testCase.attribute("complex");
	if (!complex) return FALSE;
	CString strComplex = complex.value();
	if (SplitCStringToArray(strComplex, complexArray, ",") != 6)
	{
		complexArray.RemoveAll();
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
	}
	for (pugi::xml_node entity = testCase.first_child(); entity; entity = entity.next_sibling())
	{
		pugi::xml_attribute type = entity.attribute("Type");
		CString typeValue(type.value());
		if (TEST_LINES && typeValue.CompareNoCase("Line") == 0) //如果TEST_LINES == 0，不执行载入操作
		{
			Line line;
			pugi::xml_node startNode = entity.child("StartPoint");
			if(!XmlNodeToPoint(startNode, line.startpoint)) return FALSE;
			pugi::xml_node endNode = entity.child("EndPoint");
			if(!XmlNodeToPoint(endNode, line.endpoint)) return FALSE;
			if (IsLineOutCanvas(line))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			lines.push_back(line);
		} 
		else if(TEST_CIRCLES && typeValue.CompareNoCase("Circle") == 0) //如果TEST_CIRCLES == 0，不执行载入操作
		{
			Circle circle;
			pugi::xml_node centerNode = entity.child("CenterPoint");
			if(!XmlNodeToPoint(centerNode, circle.center)) return FALSE;
			pugi::xml_node radiusNode = entity.child("Radius");
			pugi::xml_node pointNode = radiusNode.first_child();
			circle.radius = _ttoi(pointNode.value());
			if (IsCircleOutCanvas(circle))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			circles.push_back(circle);
		}
		else if (typeValue.CompareNoCase("Convex Polygon") == 0)
		{
			boundary.isConvex = true;
			for (pugi::xml_node vertex = entity.first_child(); vertex; vertex = vertex.next_sibling())
			{
				CPoint point;
				if(!XmlNodeToPoint(vertex, point)) return FALSE;
				boundary.vertexs.push_back(point);
			}
			if (IsBoundaryOutCanvas(boundary))
			{
				hasOutCanvasData = TRUE;
				boundary.vertexs.clear();
			}
		}
		else if (typeValue.CompareNoCase("Concave Polygon") == 0)
		{
			boundary.isConvex = false;
			for (pugi::xml_node vertex = entity.first_child(); vertex; vertex  = vertex.next_sibling())
			{
				CPoint point;
				if(!XmlNodeToPoint(vertex, point)) return FALSE;
				boundary.vertexs.push_back(point);
			}
			if (IsBoundaryOutCanvas(boundary))
			{
				hasOutCanvasData = TRUE;
				boundary.vertexs.clear();
			}
		}
	}
	return TRUE;
}
void CDemo_ClipView_VCDlg::ClearTestCaseData()
{
	hasOutCanvasData = FALSE;
	complexArray.RemoveAll();
	boundary.vertexs.clear();
	circles.clear();
	lines.clear();

	//added
	//convexPoint.clear();
	//edgeRect.clear();
	//intersectPoint.clear();

	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}
double CDemo_ClipView_VCDlg::maxMemory = 0;
bool CDemo_ClipView_VCDlg::m_bStopTimer = false;
int CDemo_ClipView_VCDlg::timerId = 0;
void CALLBACK TimerProc(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime);
void CALLBACK TimerProc(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime)
{
	HANDLE handle=GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;    
	GetProcessMemoryInfo(handle,&pmc,sizeof(pmc));
	double curMemory = pmc.PeakPagefileUsage;
	if (curMemory > CDemo_ClipView_VCDlg::maxMemory)
	{
		CDemo_ClipView_VCDlg::maxMemory = curMemory;
	}
}
UINT ThreadProc(LPVOID lParam);
UINT ThreadProc(LPVOID lParam)
{
	SetTimer(NULL, ++CDemo_ClipView_VCDlg::timerId, /*频率*/10/*毫秒*/, TimerProc);
	int itemp;
	MSG msg;
	while((itemp=GetMessage(&msg, NULL, NULL, NULL)) && (-1 != itemp))
	{
		if (CDemo_ClipView_VCDlg::m_bStopTimer)
		{
			KillTimer(NULL, CDemo_ClipView_VCDlg::timerId);
			return 0;
		}
		if (msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

void CDemo_ClipView_VCDlg::BeginTimeAndMemoryMonitor()
{
	m_btn_clip.EnableWindow(FALSE);
	m_stc_drawing.SetWindowText("裁剪中...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	HANDLE handle=GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;    
	GetProcessMemoryInfo(handle,&pmc,sizeof(pmc));
	startMemory = pmc.PagefileUsage;
	maxMemory = 0;
	AfxBeginThread(ThreadProc, NULL);
	startTime = clock();
}
void CDemo_ClipView_VCDlg::EndTimeAndMemoryMonitor()
{
	endTime = clock();
	m_bStopTimer = true;
	double useTimeS = (endTime - startTime)/1000;
	double useMemoryM = (maxMemory - startMemory)/1024/1024;
	if (useMemoryM < 0)
	{
		useMemoryM = 0.0;
	}
	m_stc_drawing.SetWindowText("裁剪完毕！");
	m_stc_info1.ShowWindow(SW_SHOW);
	CString strTime;
	strTime.Format("耗时:  %lfs", useTimeS);
	m_stc_info1.SetWindowText(strTime);
	m_stc_info2.ShowWindow(SW_SHOW);
	CString strMemory;
	strMemory.Format("占用内存:  %lfM", useMemoryM);
	m_stc_info2.SetWindowText(strMemory);
}
void CDemo_ClipView_VCDlg::DrawTestCase(CString xmlPath, CString caseID)
{
	m_stc_drawing.ShowWindow(SW_SHOW);
	m_stc_drawing.SetWindowText("图形绘制中...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	ClearTestCaseData();
	if (!LoadTestCaseData(xmlPath, caseID))
	{
		MessageBox("读取数据失败!","Demo_CliView_VC",MB_OK);
		m_stc_drawing.SetWindowText("");
		return;
	}

	if (TEST_DRAW_INITIAL)
	{
		COLORREF clrLine = RGB(0,255,0);
		vector<Line>::iterator iterLine = lines.begin();
		for (;iterLine != lines.end(); iterLine++)
		{
			DrawLine(*iterLine, clrLine);
		}

		COLORREF clrCircle = RGB(0,0,255);
		vector<Circle>::iterator iterCircle = circles.begin();
		for (;iterCircle != circles.end(); iterCircle++)
		{
			DrawCircle(*iterCircle, clrCircle);
		}
	}

	COLORREF clrBoundary = RGB(255,0,0);
	DrawBoundary(boundary, clrBoundary);

	if (hasOutCanvasData)
	{
		m_stc_drawing.SetWindowText("存在超出边界数据！！！");
	}
	else
	{
		m_btn_clip.EnableWindow(TRUE);
		m_stc_drawing.SetWindowText("图形绘制完成：");
		m_stc_info1.ShowWindow(SW_SHOW);
		CString strInfo1;
		strInfo1.Format("共有%s个图形和%s个边界，其中%s个图形与边界相交。",complexArray.GetAt(0),complexArray.GetAt(1),complexArray.GetAt(2));
		m_stc_info1.SetWindowText(strInfo1);
		m_stc_info2.ShowWindow(SW_SHOW);
		CString strInfo2;
		strInfo2.Format("共有%s个图形与边界无交点，其中%s个位于边界内，%s个位于边界外。",complexArray.GetAt(3),complexArray.GetAt(4),complexArray.GetAt(5));
		m_stc_info2.SetWindowText(strInfo2);
	}
}


void CDemo_ClipView_VCDlg::DrawLine(Line line, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.MoveTo(line.startpoint);
	dc.LineTo(line.endpoint);

	dc.SelectObject(penOld);
}
void CDemo_ClipView_VCDlg::DrawCircle(Circle circle, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.Arc(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.center.x + circle.radius, circle.center.y + circle.radius, 0, 0, 0, 0);

	dc.SelectObject(penOld);
}
void CDemo_ClipView_VCDlg::DrawBoundary(Boundary boundary, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);


	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	if (iter == boundary.vertexs.end()) return;
	CPoint startPoint = *iter;
	dc.MoveTo(startPoint);
	for (;iter != boundary.vertexs.end(); iter++)
	{
		dc.LineTo(*iter);
	}
	dc.LineTo(startPoint);

	dc.SelectObject(penOld);
}