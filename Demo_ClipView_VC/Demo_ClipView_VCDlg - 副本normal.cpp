
// Demo_ClipView_VCDlg.cpp : 实现文件

#include "stdafx.h"
#include "Demo_ClipView_VC.h"
#include "Demo_ClipView_VCDlg.h"
#include "afxdialogex.h"
#include <TlHelp32.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <process.h>
#include <omp.h>
#include <stdio.h>
using namespace std;



/*---------------------------------------------- main part begins------------------------------------------------------------------*/

int CDemo_ClipView_VCDlg::circle_in_bound = 0;									//圆在多边形内的数量
int CDemo_ClipView_VCDlg::circle_inter_bound = 0;								//圆与多边形相交的数量
int CDemo_ClipView_VCDlg::line_in_bound = 0;
int CDemo_ClipView_VCDlg::line_out_bound = 0;
int CDemo_ClipView_VCDlg::line_overlap_bound = 0;

vector<Line>CDemo_ClipView_VCDlg::lines_to_draw[MAX_THREAD_NUMBER];				//用于存储所有要画的线
vector<_arc2draw>CDemo_ClipView_VCDlg::circles_to_draw[MAX_THREAD_NUMBER];		//用于存储所有要画的弧线
vector<Line>CDemo_ClipView_VCDlg::lines_drawing;								//用于存储正在画的线
vector<_arc2draw>CDemo_ClipView_VCDlg::circles_drawing;							//用于存储正在画的圆
vector<Line>CDemo_ClipView_VCDlg::lines;										//用于存储读取到的所有线
vector<Circle>CDemo_ClipView_VCDlg::circles;									//用于存储读取到的所有圆
Boundary CDemo_ClipView_VCDlg::boundary;	

CRITICAL_SECTION CDemo_ClipView_VCDlg::critical_sections[MAX_THREAD_NUMBER];	//为每个线程分配一个临界区
CRITICAL_SECTION CDemo_ClipView_VCDlg::critical_circle_number[2];				//为圆在多边形内部和相交个数计数创建临界区
CRITICAL_SECTION CDemo_ClipView_VCDlg::critical_line_number[3];					//为线在多边形内部和相交个数计数创建临界区

bool CDemo_ClipView_VCDlg::isConvexPoly;										//多边形的凹凸性，以选择不同的算法
vector<BOOL>CDemo_ClipView_VCDlg::convexPoint;									//多边形的点的凹凸性，true为凸点
vector<RECT>CDemo_ClipView_VCDlg::edgeRect;										//多边形的边的外矩形框
vector<Vector>CDemo_ClipView_VCDlg::normalVector;


/*--------------------------------------------- methods part begins------------------------------------------------------------------*/
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
	int height = CANVAS_HEIGHT + INFO_HEIGHT + windowRect.Height() - clientRect.Height() + 10;
	SetWindowPos(&wndTopMost, 10, 10, width, height, SWP_NOZORDER);

	m_btn_clip.SetWindowPos(NULL, 50, CANVAS_HEIGHT + 15, 80, 30, SWP_NOZORDER);
	m_stc_drawing.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 20, 200, 20, SWP_NOZORDER);
	m_stc_info1.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 5, 450, 20, SWP_NOZORDER);
	m_stc_info2.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 25, 650, 40, SWP_NOZORDER);

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

void CDemo_ClipView_VCDlg::OnBnClickedBtnClip()
{
	BeginMonitor();
	//TODO 在此处完成裁剪算法和裁剪后显示程序，并修改需要显示的图形信息

	//开始 初始化结果信息
	CDemo_ClipView_VCDlg::circle_in_bound=0;
	CDemo_ClipView_VCDlg::circle_inter_bound=0;
	CDemo_ClipView_VCDlg::line_in_bound=0;
	CDemo_ClipView_VCDlg::line_out_bound=0;
	CDemo_ClipView_VCDlg::line_overlap_bound=0;
	//结束 初始化结果信息

	//开始 记录图形信息
	int lines_num = lines.size();
	int circles_num = circles.size();
	int boundarys_num = 1;
	//结束 记录图形信息

	//开始 处理boundary里面连续两点相同的异常情况，先进行处理清除此异常情况
	vector<CPoint>::iterator iter;
	for (iter = CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1; iter != CDemo_ClipView_VCDlg::boundary.vertexs.end();)
	{
		if ((*iter).x ==(*(iter-1)).x&&(*iter).y ==(*(iter-1)).y)
			iter = CDemo_ClipView_VCDlg::boundary.vertexs.erase(iter);
		else iter++;
	}
	CDemo_ClipView_VCDlg::boundary.vertexs[0].x=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].x;
	CDemo_ClipView_VCDlg::boundary.vertexs[0].y=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].y;
	//结束 处理boundary里面连续两点相同的异常情况，先进行处理清除此异常情况

	//开始 处理boundary里连续三点共线的情况
	for (iter = CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1; iter != CDemo_ClipView_VCDlg::boundary.vertexs.end();)
	{
		CPoint p1,p2,p3;
		if (iter==CDemo_ClipView_VCDlg::boundary.vertexs.end()-1)
		{
			p1=*(iter-1);
			p2=*(iter);
			p3=*(CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1);
		}
		else
		{
			p1=*(iter-1);
			p2=*iter;
			p3=*(iter+1);
		}

		if ((p2.x-p1.x)*(p3.y-p2.y)==(p3.x-p2.x)*(p2.y-p1.y))//三点共线
			iter = CDemo_ClipView_VCDlg::boundary.vertexs.erase(iter);
		else
			iter++;
	}
	CDemo_ClipView_VCDlg::boundary.vertexs[0].x=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].x;
	CDemo_ClipView_VCDlg::boundary.vertexs[0].y=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].y;
	//结束 处理boundary里连续三点共线的情况


	//开始 根据CPU数量确定计算线程数
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	int THREAD_NUMBER=info.dwNumberOfProcessors-1;
	if (THREAD_NUMBER>MAX_THREAD_NUMBER-1)
		THREAD_NUMBER=MAX_THREAD_NUMBER-1;
	if (THREAD_NUMBER<1)
		THREAD_NUMBER=1;
	//结束 根据CPU数量确定计算线程数

	//开始 保留足够的空间，以免之后空间不够时发生内存拷贝
	for (int i=0;i<THREAD_NUMBER;i++)
	{
		CDemo_ClipView_VCDlg::lines_to_draw[i].reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
		CDemo_ClipView_VCDlg::circles_to_draw[i].reserve(CDemo_ClipView_VCDlg::circles.size()*2/THREAD_NUMBER);
	}
	CDemo_ClipView_VCDlg::lines_drawing.reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
	CDemo_ClipView_VCDlg::circles_drawing.reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
	//结束 保留足够的空间，以免之后空间不够时发生内存拷贝

	//开始 初始化临界资源
	for (int i=0;i<THREAD_NUMBER;i++)
		InitializeCriticalSection(&CDemo_ClipView_VCDlg::critical_sections[i]);
	for (int i=0;i<2;i++)
		InitializeCriticalSection(&CDemo_ClipView_VCDlg::critical_circle_number[i]);
	for (int i=0;i<3;i++)
		InitializeCriticalSection(&CDemo_ClipView_VCDlg::critical_line_number[i]);

	//结束 初始化临界资源

	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));		//填充整个画布为黑色

	COLORREF clrBoundary = RGB(255,0,0);								//边界的颜色为红色
	COLORREF clrLine = RGB(0,255,0);									//线的颜色为绿色
	COLORREF clrCircle = RGB(0,0,255);									//圆的颜色为蓝色
	
	//开始 为画线和画圆分别生成一个Cpen
	CPen penLine,penCircle;												
	penLine.CreatePen(PS_SOLID, 1, clrLine);
	penCircle.CreatePen(PS_SOLID, 1, clrCircle);
	//结束 为画线和画圆分别生成一个Cpen

	DrawBoundary(boundary, clrBoundary);								//画多边形边界


	HANDLE hThread[MAX_THREAD_NUMBER];								//用于存储线程句柄
	DWORD  dwThreadID[MAX_THREAD_NUMBER];								//用于存储线程的ID
	_param* Info = new _param[MAX_THREAD_NUMBER];						//传递给线程处理函数的参数

	int nThreadLines=(int)(lines.size()/THREAD_NUMBER);					//前n-1个线程需要处理的线的条数（线程数为n）
	int nThreadCircles = (int)(circles.size()/THREAD_NUMBER);			//前n-1个线程需要处理的圆的个数（线程数为n）

	//开始 为每个线程分配需要处理的线和圆
	for(int i=0;i<THREAD_NUMBER;i++){
		int upperNumber=(i+1)*nThreadLines;
		if (i==THREAD_NUMBER-1)
			upperNumber=CDemo_ClipView_VCDlg::lines.size();
		for (int j = i*nThreadLines; j < upperNumber; j++)
			Info[i].thread_lines.push_back(CDemo_ClipView_VCDlg::lines[j]);

		upperNumber=(i+1)*nThreadCircles;
		if (i==THREAD_NUMBER-1)
			upperNumber=CDemo_ClipView_VCDlg::circles.size();
		for (int j = i*nThreadCircles; j < upperNumber; j++)
			Info[i].thread_circles.push_back(CDemo_ClipView_VCDlg::circles[j]);
	
		Info[i].i = i;
		Info[i].boundary = CDemo_ClipView_VCDlg::boundary;
	}
	//结束 为每个线程分配需要处理的线和圆


	//开始 对多边形边界的信息进行预处理
	preprocessJudgeConvexPoint(convexPoint);
	preprocessNormalVector(normalVector,convexPoint);
	preprocessEdgeRect(boundary);
	//结束 对多边形边界的信息进行预处理



	//开始 创建多线程进行计算
	for (int i = 0;i<THREAD_NUMBER;i++)
		hThread[i] = CreateThread(NULL,0,ThreadProc2,&Info[i],0,&(dwThreadID[i]));
	//结束 创建多线程进行计算



	//开始 当线程没有全部返回时对已经计算完的线和圆进行绘制显示
	while (WaitForMultipleObjects(THREAD_NUMBER,hThread,true,
		THREAD_CHECK_INTERVAL)!=WAIT_OBJECT_0)//若线程没有完全返回，则循环继续
	{
		if (!TEST_DRAW_ANSWER) continue;									//若不用画图，则退出本次循环

		//开始 画线		
		dc.SelectObject(&penLine);
		for (int i=0;i<THREAD_NUMBER;i++)
		{			
			if (lines_to_draw[i].size()>MINIMUM_DRAW_SIZE)					//检查是否产生了待绘数据，有足够多的话就开始绘图，以节约时间
			{
				EnterCriticalSection(&critical_sections[i]);				//进入对临界资源lines_to_draw[i]进行访问
				lines_drawing.swap(lines_to_draw[i]);						//交换绘图容器（交换时为空容器）与待绘图容器
				LeaveCriticalSection(&critical_sections[i]);				//离开对临界资源lines_to_draw[i]进行访问

				//开始 对第k线程交换出来的线进行绘制
				for (vector<Line>::iterator iter=lines_drawing.begin();
					iter!=lines_drawing.end(); iter++)
				{
					dc.MoveTo((*iter).startpoint);
					dc.LineTo((*iter).endpoint);
				}
				//结束 对第k线程交换出来的线进行绘制
				lines_drawing.clear();										//清空绘图容器
			}
		}
		//结束 画线

		//开始 画圆
		dc.SelectObject(&penCircle);
		for (int i=0;i<THREAD_NUMBER;i++)
		{
			if (circles_to_draw[i].size()>MINIMUM_DRAW_SIZE)				//检查是否产生了待绘数据，有足够多的话就开始绘图，以节约时间
			{
				EnterCriticalSection(&critical_sections[i]);				//进入对临界资源critical_sections[i]进行访问
				circles_drawing.swap(circles_to_draw[i]);					//交换绘图容器（交换时为空容器）与待绘图容器
				LeaveCriticalSection(&critical_sections[i]);				//离开对临界资源critical_sections[i]进行访问

				//开始 对第k线程交换出来的圆进行绘制
				for (vector<_arc2draw>::iterator iter=circles_drawing.begin();iter!= circles_drawing.end();iter++)
					dc.Arc(&((*iter).rect),(*iter).start_point,(*iter).end_point);
				//结束 对第k线程交换出来的圆进行绘制
				circles_drawing.clear();									//清空绘图容器
			}
		}
		//结束 画圆

	}
	//结束 当线程没有全部返回时对已经计算完的线和圆进行绘制显示


	//开始 画剩下的线和圆弧
	if (TEST_DRAW_ANSWER)
	{
		dc.SelectObject(&penLine);
		for (int i=0;i<THREAD_NUMBER;i++)
			for (vector<Line>::iterator iter=lines_to_draw[i].begin();iter!=lines_to_draw[i].end(); iter++)
			{
				dc.MoveTo((*iter).startpoint);
				dc.LineTo((*iter).endpoint);
			}

		dc.SelectObject(&penCircle);
		for (int i=0;i<THREAD_NUMBER;i++)
			for (vector<_arc2draw>::iterator iter= circles_to_draw[i].begin();iter!= circles_to_draw[i].end(); iter++)
				dc.Arc(&((*iter).rect),(*iter).start_point,(*iter).end_point);
	}
	//结束 画剩下的线和圆弧


	//开始 计算一些数据
	int circle_out_bound = circles_num-circle_inter_bound-circle_in_bound;
	int line_inter_bound = lines_num-line_in_bound-line_out_bound-line_overlap_bound;
	//结束 计算一些数据
	

	//开始 输出图形复杂程度
	CString text;
	text.Format("共有%d条线，%d个圆和%d个边界，其中%d个图形在多边形内部，%d个图形在多边形外部。共有%d个图形和多边形相交，%d个图形和多边形重合",lines_num,circles_num,boundarys_num,line_in_bound+circle_in_bound,line_out_bound+circle_out_bound,line_inter_bound+circle_inter_bound,line_overlap_bound);
	m_stc_info2.SetWindowText(text);
	//结束 输出图形复杂程度


	//开始 释放临界资源和堆空间
	for (int i=0;i<THREAD_NUMBER;i++)
	{
		DeleteCriticalSection(&critical_sections[i]);
		vector<Line>().swap(lines_to_draw[i]);
		vector<_arc2draw>().swap(circles_to_draw[i]);
	}
	
	for (int i=0;i<3;i++)
		DeleteCriticalSection(&critical_line_number[i]);
	for (int i=0;i<2;i++)
		DeleteCriticalSection(&critical_circle_number[i]);

	vector<Line>().swap(lines_drawing);
	vector<_arc2draw>().swap(circles_drawing);
	vector<Line>().swap(lines);
	vector<Circle>().swap(circles);
	vector<BOOL>().swap(convexPoint);
	vector<RECT>().swap(edgeRect);
	vector<Vector>().swap(normalVector);


	for(int i=0;i<THREAD_NUMBER;i++){
		vector<Line>().swap(Info[i].thread_lines);
		vector<Circle>().swap(Info[i].thread_circles);
	}
	delete [] Info;
    Info = NULL;

	penLine.DeleteObject();
	penCircle.DeleteObject();

	//结束 释放临界资源和堆空间
	
	_CrtDumpMemoryLeaks();										//输出泄漏信息
	
	
	EndMonitor();
}

/*
*功能：对部分线和圆进行求交计算的线程函数
*参数：传递给每个线程的待处理的线和圆的容器
*思路：对传递过来的圆和先分别跟多边形窗口进行求交，
*	   获得需要绘制的线段和弧线，并存入一个待绘制的容器中
*/
DWORD WINAPI CDemo_ClipView_VCDlg::ThreadProc2(LPVOID lpParam)  
{  
	_param  * Info = ( _param *)lpParam; 

	//开始 进行线的裁剪
	if (TEST_LINES)                                                       
	{
		if (isConvexPoly)
			dealConvex(Info->thread_lines,Info->boundary,Info->i);
		else
			dealConcave(Info->thread_lines,Info->boundary,Info->i);

	}
	//结束 进行线的裁剪

	//开始 进行圆的裁剪
	if (TEST_CIRCLES)
		forCircleRun(Info->thread_circles,Info->boundary,Info->i);
	//结束 进行圆的裁剪

	return 0;                                                             //线程返回
}






/*----------------------------------------------- 凸多边形线裁剪算法 starts--------------------------------------------------------*/

/*
*功能：判断线段是否在包围盒里，若在包围盒里则返回true，否则返回false
*参数：boundary为裁剪边界  line为所判断的线段
*思路：遍历多边形的顶点，以最小的x,y和最大的x,y值分别为包围盒的对角线端点
*/
bool CDemo_ClipView_VCDlg::InBox(Line& line){
	bool isVisible=true;
	int xl=INTIALIZE,xr=-INTIALIZE,yt=-INTIALIZE,yb=INTIALIZE;					//对包围盒的边界值进行初始化
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
*功能：过点(x1,y1)(x2,y2)一般式直线方程为(y2-y1)x+(x1-x2)y+x2y1-x1y2=0，
*      根据上述多项式判断(x,y)与(x1,y1)(x,2,y2)线段的位置,返回多项式值
*参数：(x1,y1),(x2,y2)为该线段两端端点的坐标值  (x,y）为所判断的点的坐标值
*思路：将被判断的点的坐标代入多项式中，得知其正负号
*/
int CDemo_ClipView_VCDlg::Multinomial(int x1,int y1,int x2,int y2,int x,int y){
	int result=(y2-y1)*x+(x1-x2)*y+x2*y1-x1*y2;
	return result;
}


/*
*功能：返回line1,line2两条直线的交点
*思路：根据直线的一般式方程，求直线直接的交点
*/
CPoint CDemo_ClipView_VCDlg::CrossPoint(Line& line1,Line& line2){
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
*思路：将该直线的两个端点和被判断的端点传入Multinomial()函数中，
*      若此多项式值为0则说明点在线段上
*/
bool CDemo_ClipView_VCDlg::IsOnline(CPoint& pt,Line& line){
	if(Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt.x,pt.y)==0)
		return 1;
	else 
		return 0;
}


/*
*功能：判断由pt1,pt2两个端点所在线段与线段line是否相交，
*      若相交则返回true，否则返回false
*思路：分别将pt1,line的两个端点和pt2，line的两个端点传入Multinomial()函数中，
*      若返回的两个多项式值异号则说明pt1,pt2所在线段和line相交
*/
bool CDemo_ClipView_VCDlg::Intersect(CPoint& pt1,CPoint& pt2,Line& line){
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
Line CDemo_ClipView_VCDlg::result(Line& line){
	Line line_result;
	line_result.startpoint.x=line_result.startpoint.y=INTIALIZE;
	line_result.endpoint.x=line_result.endpoint.y=INTIALIZE;

	bool bspt=isPointInBoundary(line.startpoint);								//startpoint是否在多边形内，若是返回值为true

	bool bept=isPointInBoundary(line.endpoint);									//endpoint是否在多边形内，若是返回值为true
	
	if(bspt&&bept)																//若两点都在多边形内则绘制该直线
		return line; 
	
	else if((bspt&&(!bept))||((!bspt)&&bept)){									//若startpoint在多边形内，但endpoint在多边形外
		if(bspt&&(!bept))
			line_result.startpoint=line.startpoint;
		else if((!bspt)&&bept)
			line_result.startpoint=line.endpoint;

		//开始 求交点
		int n=boundary.vertexs.size();
		int i=0;
		while(i<n-1){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			
			if(Intersect(side.startpoint,side.endpoint,line)){					//过点(x1,y1)(x2,y2)一般式直线方程 
																				//(y2-y1)x+(x1-x2)y+x2y1-x1y2=0
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=CrossPoint(line,side);
						break;
				}
				else i++;
			}
			else i++;
		}
		//结束 求交点

		return line_result;														//绘制交点到startpoint的直线
	}

	else if((!bspt)&&(!bept)){												    //若startpoint和endpoint均在多边形外
		int n=boundary.vertexs.size();
		int i=0,nCrossPoint=0;
		while(i<(n-1)&&nCrossPoint!=2){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			bool isIntersect=Intersect(side.startpoint,side.endpoint,line);
			if(isIntersect&&line_result.startpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.startpoint=p;
						nCrossPoint++;
						i++;
				}
				else i++;
			}
			else if(isIntersect&&line_result.endpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
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


void CDemo_ClipView_VCDlg::dealConvex(vector<Line>& lines,Boundary& boundary,int threadNumber)
{
	//开始 处理凸多边形窗口
	for(int i=0;i<lines.size();i++){
		bool overlap=isOverlap(lines[i]);
		if (overlap)
		{
				EnterCriticalSection(&critical_line_number[2]); 
				line_overlap_bound++;
				LeaveCriticalSection(&critical_line_number[2]); 
		}
		int size=boundary.vertexs.size()-1;
		bool haveIntersect=false;
		for (int ii=0;ii<size;ii++)
		{
			CPoint p1=boundary.vertexs[ii];
		    CPoint p2=boundary.vertexs[ii+1];
			if(Intersect(p1,p2,lines[i])){
				haveIntersect=true;
				break;
			}
		}
		
		//开始 没有交点的情况
		if (!haveIntersect)
		{
			if (isPointInBoundary(lines[i].startpoint))							//如果在多边形内
			{
				Line l=lines[i];
				EnterCriticalSection(&critical_sections[threadNumber]); 
				lines_to_draw[threadNumber].push_back(lines[i]);
				LeaveCriticalSection(&critical_sections[threadNumber]);				
				
				if (!overlap)
				{
					EnterCriticalSection(&critical_line_number[0]); 
					line_in_bound++;
					LeaveCriticalSection(&critical_line_number[0]); 
				}
			}
			else if (!overlap)

			{
				EnterCriticalSection(&critical_line_number[1]); 
				line_out_bound++;
				LeaveCriticalSection(&critical_line_number[1]); 
			}
			continue;
		}
		//结束 没有交点的情况

		if(InBox(lines[i])){													//先判断线段是否在包围盒内，若明显不在则不显示该线段，
																				//否则继续以下步骤
			Line line=result(lines[i]);
			if(line.startpoint.x!=INTIALIZE &&
				line.startpoint.y!=INTIALIZE &&
				line.endpoint.x!=INTIALIZE &&
				line.endpoint.y!=INTIALIZE)
				lines_to_draw[threadNumber].push_back(line);
		}
	}
	//结束 处理凸多边形窗口
}

/*----------------------------------------------- 凸多边形线裁剪算法 ends----------------------------------------------------------*/





/*----------------------------------------------- 任意多边形线裁剪算法 starts------------------------------------------------------*/

/*
*功能：计算两个向量的叉积
*参数：a1,a2为第一个向量的起点与终点，b1,b2为第二个向量的起点与终点
*/
inline int CDemo_ClipView_VCDlg::crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2)
{
	int ux=a2.x-a1.x;
	int uy=a2.y-a1.y;
	int vx=b2.x-b1.x;
	int vy=b2.y-b1.y;
	return ux*vy-uy*vx;
}


/*
*功能：判断一个点是否在一条线段上（不包括延长）
*参数：p为该点，p1、p2为该线段的两个端点，i为该线段对应的边的序号
*思路：先判断点是否在线段的矩形框内，不是的话直接返回false；再判断(P-P1)X(P2-P1)是否等于0，等于0表示在线段上。
*/
inline bool CDemo_ClipView_VCDlg::isPointInLine(CPoint& p,CPoint& p1,CPoint& p2,int i)
{
	if (p.x<edgeRect[i].left || p.x>edgeRect[i].right || 
		p.y<edgeRect[i].bottom || p.y>edgeRect[i].top)
		return false;
	if (!crossMulti(p1,p,p1,p2)==0)							//判断叉积是否为0
		return false;
	return true;
}

/*
*功能：判断某一条线段是否与多边形的边界重叠，因为程序需要在最后显示重叠的线段数量
*思路：先判断是否平行，不平行就跳过；再判断是否有一个端点在另一条线段上，有的话就说明重叠
*/
bool CDemo_ClipView_VCDlg::isOverlap(Line l)
{
	int size=boundary.vertexs.size()-1;
	for (int i=0;i<size;i++)
	{
		CPoint p1=boundary.vertexs[i];
		CPoint p2=boundary.vertexs[i+1];

		if (!((p2.x-p1.x)*(l.endpoint.y-l.startpoint.y)==(p2.y-p1.y)*(l.endpoint.x-l.startpoint.x)))
			continue;

		bool isP1InLine=false,isP2InLine=false,isP3InLine=true;
		isP1InLine=isPointInLine(l.startpoint,p1,p2,i);
		if (!isP1InLine)
			isP2InLine=isPointInLine(l.endpoint,p1,p2,i);

		if (!isP1InLine && !isP2InLine)
		{
			if (p1.x<min(l.startpoint.x,l.endpoint.x) || p1.x>max(l.startpoint.x,l.endpoint.x) || 
				p1.y<min(l.startpoint.y,l.endpoint.y) || p1.y>max(l.startpoint.y,l.endpoint.y))
				isP3InLine=false;
			if (isP3InLine && !crossMulti(l.startpoint,p1,l.startpoint,l.endpoint)==0)
				isP3InLine=false;
		}


		if (isP1InLine || isP2InLine || isP3InLine)
			return true;
	}
	return false;
}


/*
*功能：用于交点排序的比较函数（按t值得从小到大排序）
*参数：a,b分别为两个交点
*/
bool CDemo_ClipView_VCDlg::Compare(IntersectPoint a,IntersectPoint b)
{
	if (a.t<b.t)
		return true;
	else
		return false;
}

/*
*功能：判断多边形每个点是凹点还是凸点
*思路：求相邻两边的向量的叉积，已知凸点的值与凹点的值正负性不同，
*      并且x坐标最大的点一定是凸点。先计算相邻两边的向量的叉积并保存，
*	   同时找出x最大的点。再通过与x最大的点比较正负性得出点的凹凸性。
*/
void CDemo_ClipView_VCDlg::preprocessJudgeConvexPoint(vector<BOOL>& convexPoint)
{
	int dotnum=boundary.vertexs.size()-1;										//多边形顶点数
	vector<int> z;
	int max=-1,maxnum=-1;
	for (int i=0;i<dotnum;i++)
	{																				
		CPoint* p1;																//当前点为p2,其前一点为p1,后一点为p3
		if (i==0)
			p1=&boundary.vertexs[dotnum-1];
		else
			p1=&boundary.vertexs[i-1];
		CPoint* p2=&boundary.vertexs[i];
		CPoint* p3=&boundary.vertexs[i+1];
		int crossmulti=crossMulti(*p1,*p2,*p2,*p3);								//计算相邻两条边的叉积（即p1p2与p2p3）

		z.push_back(crossmulti);
		if (p2->x>max)															//找出x最大的点，记录
		{
			max=p2->x;
			maxnum=i;
		}
	}

	isConvexPoly=true;
	//开始 通过与x最大的点比较正负性得出点的凹凸性
	for (int i=0;i<dotnum;i++)  
		if ((z[maxnum]>0 && z[i]>0)||(z[maxnum]<0 && z[i]<0))
			convexPoint.push_back(true);
		else
		{
			convexPoint.push_back(false);
			isConvexPoly=false;
		}
	//结束 通过与x最大的点比较正负性得出点的凹凸性
	convexPoint.push_back(convexPoint[0]);

}

/*
*功能：计算多边形的边的内法向量
*思路：绕多边形的边界依次计算。对于某一条边，先随意构造出一个与当前边垂直的法向量。
*	   若该法向量与前一条边的向量的点积为正，则将法向量反向。
*	   如果这两条边的公共顶点为凹点，则将该法向量再反向。
*/
void CDemo_ClipView_VCDlg::preprocessNormalVector(vector<Vector>& normalVector,vector<BOOL>& convexPoint)
{

	int edgenum=boundary.vertexs.size()-1;											//多边形边数

	for (int i=0;i<edgenum;i++)
	{
		CPoint p1=boundary.vertexs[i];
		CPoint p2=boundary.vertexs[i+1];											//p1p2为多边形的边

		//开始 求内法向量
		CPoint p0;																	//p0为p1前面一个顶点
		if (i==0)
			p0=boundary.vertexs[edgenum-1];
		else
			p0=boundary.vertexs[i-1];

		double n1,n2;		
		if (p2.y-p1.y==0)															//先任意算出一个法向量
		{
			n2=1;
			n1=0;
		}
		else
		{
			n1=1;
			n2=-(double)(p2.x-p1.x)/(p2.y-p1.y);
		}
		if ((p1.x-p0.x)*n1+(p1.y-p0.y)*n2>0)										//与前一条边点乘，如果方向错了，使法向量反向
		{
			n1=-n1;
			n2=-n2;
		}
		if (!convexPoint[i])														//如果是凹点，法向量再反向
		{
			n1=-n1;
			n2=-n2;
		}

		Vector nv;
		nv.x=n1;
		nv.y=n2;
		normalVector.push_back(nv);
		//结束 求内法向量
	}
}


/*
*功能：计算多边形的边的外矩形框
*思路：某线段的外矩形框是以该线段为对角线的矩形，
*	   用于快速排除线段与边不相交的情况（当边的外矩形框与线段的外矩形框不相交时）。   
*/
void CDemo_ClipView_VCDlg::preprocessEdgeRect(const Boundary boundary)
{
	int edgenum=boundary.vertexs.size()-1;
	for (int i=0;i<edgenum;i++)
	{
		RECT r;
		r.bottom=min(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.top=max(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.left=min(boundary.vertexs[i].x,boundary.vertexs[i+1].x);
		r.right=max(boundary.vertexs[i].x,boundary.vertexs[i+1].x);

		edgeRect.push_back(r);
	}
}



/*
*功能：处理凹多边形的裁剪
*思路：先预处理判断出每个点的凹凸性，算出所有多边形的边的外矩形框。
*	   然后枚举每条线段与每条多边形的边，先通过快速排除法
*      排除明显不可能相交的边，再通过跨立试验进一步判断是否有交点。
*	   接着用改进的cyrus-beck算法算出交点的值，以及交点的性质
*     （出点还是入点），并将线段的起点作为入点，终点作为出点。
*	   然后处理当线段与多边形的顶点相交的特殊情况。
*	   最后寻找所有符合“入点-出点”的线段作为显示的线段。
*/
void CDemo_ClipView_VCDlg::dealConcave(vector<Line>& lines,Boundary& boundary,int threadNumber)
{
	vector<IntersectPoint> intersectPoint;										//线段的所有交点（也包括线段的起点与终点）

	int edgenum=boundary.vertexs.size()-1;
	int linenum=lines.size();
	RECT r;
	for (int i =0;i<linenum;i++)												//枚举每条线段
	{
		r.bottom=min(lines[i].startpoint.y,lines[i].endpoint.y);
		r.top=max(lines[i].startpoint.y,lines[i].endpoint.y);
		r.left=min(lines[i].startpoint.x,lines[i].endpoint.x);
		r.right=max(lines[i].startpoint.x,lines[i].endpoint.x);



		intersectPoint.clear();													//将线段起点当作入点放在首位
		IntersectPoint ip1;
		ip1.isIntoPoly=true;
		ip1.t=0;
		ip1.point=lines[i].startpoint;
		intersectPoint.push_back(ip1);

		BOOL haveIntersect=false;
		for (int j=0;j<edgenum;j++)												//枚举每条多边形的边
		{
			//开始 快速排除法
			if (r.left>edgeRect[j].right || r.right<edgeRect[j].left ||
				r.top<edgeRect[j].bottom || r.bottom>edgeRect[j].top)
				continue;
			//结束 快速排除法


			//开始 跨立试验
			CPoint p1=boundary.vertexs[j];
			CPoint p2=boundary.vertexs[j+1];									//p1p2为多边形的边
			CPoint q1=lines[i].startpoint;
			CPoint q2=lines[i].endpoint;										//q1q2为线段
			long long a= ((long long)crossMulti(p1,q1,p1,p2))*(crossMulti(p1,p2,p1,q2));
			long long b= ((long long)crossMulti(q1,p1,q1,q2))*(crossMulti(q1,q2,q1,p2));
			if (!(a>=0 && b>=0))												//不相交就跳过这条边
				continue;
			//结束 跨立试验

			//开始 取出内法向量
			double n1=normalVector[j].x;
			double n2=normalVector[j].y;
			//结束 取出内法向量

			//开始 cyrus-beck算法 算出参数t
			double t1=n1*(q2.x-q1.x)+n2*(q2.y-q1.y);
			double t2=n1*(q1.x-p1.x)+n2*(q1.y-p1.y);
			double t=-t2/t1;
			CPoint pt;
			pt.x=(LONG)((q2.x-q1.x)*t+q1.x);
			pt.y=(LONG)((q2.y-q1.y)*t+q1.y);
			if (t>=-0.000000001 && t<=1.000000001)								//如果算出的参数t满足 t>=0 && t<=1, 说明交点在线段上，将它加入交点集
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
			//结束 cyrus-beck算法 算出参数t

		}


		bool overlap=isOverlap(lines[i]);
		if (overlap)
		{
				EnterCriticalSection(&critical_line_number[2]); 
				line_overlap_bound++;
				LeaveCriticalSection(&critical_line_number[2]); 
		}

		//开始 没有交点的情况
		if (!haveIntersect)
		{
			if (isPointInBoundary(lines[i].startpoint))							//如果在多边形内
			{
				Line l=lines[i];
				

				EnterCriticalSection(&critical_sections[threadNumber]); 
				lines_to_draw[threadNumber].push_back(lines[i]);
				LeaveCriticalSection(&critical_sections[threadNumber]);				
				
				if (!overlap)
				{
					EnterCriticalSection(&critical_line_number[0]); 
					line_in_bound++;
					LeaveCriticalSection(&critical_line_number[0]); 
				}
			}
			else if (!overlap)

			{
				EnterCriticalSection(&critical_line_number[1]); 
				line_out_bound++;
				LeaveCriticalSection(&critical_line_number[1]); 
			}
			continue;
		}
		//结束 没有交点的情况

		//开始 将线段终点当作出点放在末位
		IntersectPoint ip2;
		ip2.isIntoPoly=false;
		ip2.t=1;
		ip2.point=lines[i].endpoint;
		intersectPoint.push_back(ip2);
		//结束 将线段终点当作出点放在末位

		std::sort(intersectPoint.begin()+1,intersectPoint.end()-1,Compare);		//按t从小到大排序


		/*开始 处理
		如果有两点t值相同（即线段与多边形的顶点相交的特殊情况），
		并且一个是入点，一个是出点，则根据顶点的凹凸性重新安排入、出
		（不能动第一个点和最后的点，即线段的端点）*/
		vector<IntersectPoint>::iterator iter = intersectPoint.begin();
		iter++;
		for (;iter!= intersectPoint.end()&&(iter+1)!= intersectPoint.end()
			&&(iter+2)!= intersectPoint.end();iter++ )
			if ( ((*iter).isIntoPoly ^ (*(iter+1)).isIntoPoly )
				&&abs((*iter).t-(*(iter+1)).t)<1e-9)  
			{
				//开始 得到该顶点序号
				int contexPointNumber=max((*iter).contexPoint,(*(iter+1)).contexPoint);
				if ( ((*iter).contexPoint==edgenum-1 && (*(iter+1)).contexPoint==0)
					|| ((*(iter+1)).contexPoint==edgenum-1&& (*iter).contexPoint==0) )
					contexPointNumber=0;
				//结束 得到该顶点序号

				if (convexPoint[contexPointNumber]&& !(*iter).isIntoPoly)		//如果是凸点，安排为先进后出
				{
					(*iter).isIntoPoly=true;
					(*(iter+1)).isIntoPoly=false;
				}
				else if (!convexPoint[contexPointNumber]
						&& (*iter).isIntoPoly)									//如果是凹点，安排为先出后进
				{
					(*iter).isIntoPoly=false;
					(*(iter+1)).isIntoPoly=true;
				}

			}


			//开始 保存交点
			int size=(int)intersectPoint.size()-1;
			for (int j=0;j<size;j++)
			{
				if (intersectPoint[j].isIntoPoly && !intersectPoint[j+1].isIntoPoly)
				{
					Line l;
					l.startpoint=intersectPoint[j].point;
					l.endpoint=intersectPoint[j+1].point;
					EnterCriticalSection(&critical_sections[threadNumber]); 
					lines_to_draw[threadNumber].push_back(l);
					LeaveCriticalSection(&critical_sections[threadNumber]); 
					j++;
				}
			}
			//结束 保存交点

		/*结束 处理
		如果有两点t值相同（即线段与多边形的顶点相交的特殊情况），
		并且一个是入点，一个是出点，则根据顶点的凹凸性重新安排入、出
		（不能动第一个点和最后的点，即线段的端点）*/
	}

}


/*----------------------------------------------- 任意多边形线裁剪算法 ends--------------------------------------------------------*/





/*----------------------------------------------- 任意多边形圆裁剪算法 starts------------------------------------------------------*/

/*
*功能：计算一个容器的圆与多边形窗口相交需要绘制的弧形，并存储在circles_to_draw容器中
*思路：通过计算圆与多边形的交点，并通过相邻交点的顺时针的圆上的交点是否在多边形内，
*	   判断弧线是否需要绘制   
*/
void  CDemo_ClipView_VCDlg::forCircleRun(vector<Circle>& circles,Boundary& boundary,int threadNumber)
{

	for(unsigned int i = 0;i<circles.size();i++){									//再次开始遍历每一个圆
		vector<XPoint> point_Array;													//用于存储每个圆与多边形窗口的交点
		getInterpointArray(point_Array,i,circles);									//获得存储交点的容器

		//开始 讨论没有交点以及相切的情况
		if(point_Array.size()==0||point_Array.size()==1)							//将完全跟多边形不相交以及相切的情况的圆排除，
		{
			if (point_Array.size()==1)
			{
				EnterCriticalSection(&critical_circle_number[1]);  
				circle_inter_bound++;
				LeaveCriticalSection(&critical_circle_number[1]);
			}
			XPoint mm;
			mm.x = circles[i].center.x;
			mm.y = circles[i].center.y;
			bool inBoundary = isPointInBoundary(mm);								//圆心若在多边形内
			/*if (inBoundary==false&&point_Array.size()==0)
			{
				EnterCriticalSection(&critical_circle_number[1]);  
				circle_inter_bound++;
				LeaveCriticalSection(&critical_circle_number[1]);
			}*/
			if (inBoundary==true)
			{
				long _x = boundary.vertexs[0].x-circles[i].center.x;
				long _y = boundary.vertexs[0].y-circles[i].center.y;
				bool t = (_x*_x+_y*_y)>(circles[i].radius*circles[i].radius);
				/*if (t == false && point_Array.size()==0)
				{
					EnterCriticalSection(&critical_circle_number[1]);  
					circle_inter_bound++;
				LeaveCriticalSection(&critical_circle_number[1]);
				}*/
				
				if (t==true)														//若圆的半径比较小
				{
					if (point_Array.size()==0)
					{
						EnterCriticalSection(&critical_circle_number[0]);  
						circle_in_bound++;
						LeaveCriticalSection(&critical_circle_number[0]);
					}
					//开始 画整个圆
					CRect rect(circles[i].center.x - circles[i].radius,circles[i].center.y - 
						       circles[i].radius,circles[i].center.x + circles[i].radius,
							   circles[i].center.y + circles[i].radius);
					CPoint start_point,end_point;
					start_point.x = 0; start_point.y = 0;
					end_point.x = 0; end_point.y = 0;
					/*CClientDC dc(dlg);
					dc.Arc(&rect, start_point, end_point);*/
					struct _arc2draw arc2draw ;
					arc2draw.rect = rect;
					arc2draw.start_point = start_point;
					arc2draw.end_point = end_point;
					EnterCriticalSection(&critical_sections[threadNumber]);  
					circles_to_draw[threadNumber].push_back(arc2draw);
					LeaveCriticalSection(&critical_sections[threadNumber]); 
					//结束 画整个圆
				}
			}
		}
		//结束 讨论没有交点以及相切的情况


		//开始 讨论一般的情况
		else if (point_Array.size()>1)
		{
			EnterCriticalSection(&critical_circle_number[1]);  
			circle_inter_bound++;
			LeaveCriticalSection(&critical_circle_number[1]);

			vector<XPoint>::iterator pos;
			point_Array.push_back(point_Array[0]);									//为了便于讨论和计算，在整个交点容器的最后加入第一个交点
			
			/*开始 处理当圆刚好过多边形的某一点时，
			  会出现该点被记录两次的情况，故需要进行排除*/
			for (pos = point_Array.begin()+1; pos != point_Array.end(); pos++)
			{
				if (abs((*pos).x -(*(pos-1)).x)<0.1 
					&& abs((*pos).y -(*(pos-1)).y)<0.1)
				{
					pos = point_Array.erase(pos);
					pos--;
				}
			}
			/*结束 处理当圆刚好过多边形的某一点时，
			  会出现该点被记录两次的情况，故需要进行排除*/

			//开始 对交点容器中的每一个交点进行遍历
			for (unsigned int j = 0; j < point_Array.size()-1; j++)
			{
				XPoint mid_point = getMiddlePoint(point_Array,i,j,circles);				//获得两交点的在圆上的中间点
				bool mid_in_bound = isPointInBoundary(mid_point);						//判断中间点是否在多边形窗口内
				if(mid_in_bound == true)												//在多边形窗口内，则画整个圆弧 
				{				
					int start_x = circles[i].center.x-circles[i].radius;
					int start_y = circles[i].center.y-circles[i].radius;
					int end_x =  circles[i].center.x+circles[i].radius;
					int end_y = circles[i].center.y+circles[i].radius;

					CRect rect(start_x,start_y,end_x,end_y);
					CPoint tmp,tmp2;
					long tmp_xy =(long)(point_Array[j].x);
					if (point_Array[j].x-tmp_xy>0.5)
					{
						tmp.x = tmp_xy+1;
					}
					else
						tmp.x = tmp_xy;

					tmp_xy =(long)(point_Array[j].y);
					if (point_Array[j].y-tmp_xy>0.5)
					{
						tmp.y = tmp_xy+1;
					}
					else
						tmp.y = tmp_xy;
					tmp_xy =(long)(point_Array[j+1].x);
					if (point_Array[j+1].x-tmp_xy>0.5)
					{
						tmp2.x = tmp_xy+1;
					}
					else
						tmp2.x = tmp_xy;

					tmp_xy =(long)(point_Array[j+1].y);
					if (point_Array[j+1].y-tmp_xy>0.5)
					{
						tmp2.y = tmp_xy+1;
					}
					else
						tmp2.y = tmp_xy;
					if (!(tmp.x==tmp2.x&&tmp.y==tmp2.y))
					{
						struct _arc2draw arc2draw ;
						arc2draw.rect = rect;
						arc2draw.start_point = tmp;
						arc2draw.end_point = tmp2;
						EnterCriticalSection(&critical_sections[threadNumber]);  
						circles_to_draw[threadNumber].push_back(arc2draw);
						LeaveCriticalSection(&critical_sections[threadNumber]); 
					}


				}
			}
			//结束 对交点容器中的每一个交点进行遍历

		}
		//结束 讨论一般的情况

	}
}


/*
*功能：给出圆的圆点的坐标和圆上某点的坐标以及半径的长度，
*      通过圆的参数方程，获得该点的角度
*思路：通过三角函数即可求得。
*/
double CDemo_ClipView_VCDlg::getAngle(double x1,double x2,double y1,double y2,double r)
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
XPoint CDemo_ClipView_VCDlg::getMiddlePoint(vector<XPoint>& point_Array,int i,int j,vector<Circle>& circles2)
{
	double a1 = getAngle(point_Array[j].x,circles2[i].center.x,point_Array[j].y,circles2[i].center.y,circles2[i].radius);
	double a2 = getAngle(point_Array[j+1].x,circles2[i].center.x,point_Array[j+1].y,circles2[i].center.y,circles2[i].radius);
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
	XPoint point ;
	point.x = x;
	point.y = y;
	return point;
}


/*
*函数重载
*功能：判断点是否在多边形窗口内
*思路：根据交点法求得某点是否在多边形的窗口内部。
*      通过该点取平行于y轴的直线l，
*	   先判断直线是否穿过多边形窗口的点。
*      若穿过，判断与该点相邻的两边是否在l的两边，
*	   若在，则交点数加一，若在同一边，则交点数加二。
*	   然后判断多边形窗口的每一条边是否与直线l有交点，
*	   若有，则交点数加一。最后若交点数为偶数，
*      则该点在多边形窗口外部，若为奇数，则该点在多边形窗口内部。
*/
bool CDemo_ClipView_VCDlg::isPointInBoundary(CPoint& point)
{
	XPoint xp;
	xp.x=point.x;
	xp.y=point.y;
	bool ans=isPointInBoundary(xp);
	return ans;
}


/*
*功能：判断点是否在多边形窗口内
*思路：根据交点法求得某点是否在多边形的窗口内部。通过该点取平行于y轴的直线L，
*	   先判断直线是否穿过多边形窗口的点。若穿过，判断与该点相邻的两点p1，p2是否在L的两边，
*	   若在，则交点数加一，若在同一边，则交点数加二，若两点中特定方向的一点p1也在直线L上，那么取该方向的下一点p3，
*      若p3和p2在直线L的两边，则交点数加一，否则不加（这个方法必须确保多边形没有连续三点共线）。
*	   然后判断多边形窗口的每一条边是否与直线l有交点（不考虑多边形的端点），若有，则交点数加一。
*	   最后若交点数为偶数，则该点在多边形窗口外部，若为奇数，则该点在多边形窗口内部。 
*/
bool CDemo_ClipView_VCDlg::isPointInBoundary(XPoint& point)
{

	int interNum = 0;												//交点数
	long x1 = point.x;												//（x1，y1）是判断的点，（x2，y2）是过（x1，y1）平行于y轴的无穷小的点
	long y1 = point.y;
	long x2 = x1;
	long y2 = 0;
	for (unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		//开始 处理平行于y轴的直线L过多边形的端点的情况
		if ((abs(boundary.vertexs[i].x-point.x)<
			DOUBLE_DEGREE)&&(boundary.vertexs[i].y<point.y))		//对边界点在线上和连续两边界点在线上的情况进行讨论
		{
			if (i==(boundary.vertexs.size()-1))						//当该点是多边形的最后一个点的时候
			{
				double between1 =boundary.vertexs[i-1].x-point.x;	//它的上一个点
				double between2 =boundary.vertexs[1].x-point.x;		//它的下一个点
				if (abs(between2)<DOUBLE_DEGREE)					//当它的下一个点也是在直线L上，忽略
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)						//当它的上一个点也是在直线L上，计算该方向上的下一个点
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))					//p3和p2在直线L的两边，则交点数加一
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))						//判断与该点相邻的两点p1，p2是否在L的两边，若在，则交点数加一
				{
					interNum++;
				}

			}
			else if (i==(boundary.vertexs.size()-2))				//当该点是多边形的倒数第二个点的时候
			{
				double between1 =boundary.vertexs[i-1].x-point.x;
				double between2 =boundary.vertexs[i+1].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))
				{
					interNum++;
				}

			}
			else if (i==1)											//当该点是多边形的第二个点的时候
			{
				double between1 =boundary.vertexs[0].x-point.x;
				double between2 =boundary.vertexs[2].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[boundary.vertexs.size()-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))
				{
					interNum++;
				}

			}
			else													//当该点在多边形的点的中间位置时
			{
				double between1 =boundary.vertexs[i-1].x-point.x;
				double between2 =boundary.vertexs[i+1].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
			}
		}
	}
	//结束 处理平行于y轴的直线L过多边形的端点的情况

	//开始 处理平行于y轴的直线L与多变形相交的情况
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		double between1 =boundary.vertexs[i].x-point.x;
		double between2 =boundary.vertexs[i-1].x-point.x;
		if (abs(between1)<DOUBLE_DEGREE||abs(between2)<DOUBLE_DEGREE)
		{
			continue;
		}
		if((between1>0&&between2<0)||(between1<0&&between2>0))
		{
			long x3 = boundary.vertexs[i].x;
		    long x4 = boundary.vertexs[i-1].x;
			long y3 = boundary.vertexs[i].y;
			long y4 = boundary.vertexs[i-1].y;
			long a = y4-y3;
			long b = x3-x4;
			long c = x4*y3-x3*y4;
			double isInter1 = a*point.x+b*point.y+c;
			double isInter2 = a*point.x+b*y2+c;
			if((isInter1<0&&isInter2>0)||(isInter1>0&&isInter2<0))
			{
				interNum++;
			}

		}
	}
	//结束 处理平行于y轴的直线L与多变形相交的情况

	
	if(interNum%2!=0)
	{
		return true;												//交点为奇数，则点在多边形内
	}
	else
	{
		return false;												//交点为偶数，则点在多边形外
	}

}


/*
*功能：通过直线参数方程的参数，直线的两交点的坐标，获得该直线上的一个点。
*思路：通过直线的参数方程可以求得。
*/
struct XPoint CDemo_ClipView_VCDlg::getInterpoint(double t,int x1,int x2,int y1,int y2)
{
	double x = x1+(x2-x1)*t;
	double y = y1+(y2-y1)*t;
	struct XPoint pit;
	pit.x = x;
	pit.y = y;

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
void CDemo_ClipView_VCDlg::getInterpointArray(vector<XPoint>& point_Array,int circle_num,vector<Circle>& circle2)
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
				struct XPoint pit= getInterpoint(t,x1,x2,y1,y2);
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
				struct XPoint pit = getInterpoint(t1,x1,x2,y1,y2);
				point_Array.push_back(pit);
				if (t2>=0&&t2<=1)
				{
					struct XPoint pit = getInterpoint(t2,x1,x2,y1,y2);
					point_Array.push_back(pit);
				}

			}
			else
			{
				if (t2>=0&&t2<=1)
				{
					struct XPoint pit = getInterpoint(t2,x1,x2,y1,y2);
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

		//开始 将所有的交点按照顺时针排序
		vector<XPoint> y_up_0;
		vector<XPoint> y_down_0;
		for (int i = 0; i < point_Array.size(); i++)
		{
			if (point_Array[i].y <= circle2[circle_num].center.y)
			{
				y_down_0.push_back(point_Array[i]);
			}
			else
			{
				y_up_0.push_back(point_Array[i]);
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
						XPoint tmp;
						tmp.x = y_down_0[j].x;
						tmp.y = y_down_0[j].y;
						y_down_0[j] = y_down_0[j+1];
						y_down_0[j+1] = tmp;
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
						XPoint tmp;
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
				point_Array[i] = y_down_0[i];
			}
		}
		if (y_up_0.size()>0)
		{
			for (int i = 0; i < y_up_0.size(); i++)
			{
				point_Array[y_down_0.size()+i] = y_up_0[i];
			}
		}
		//结束 将所有的交点按照顺时针排序

	}

}

/*----------------------------------------------- 任意多边形圆裁剪算法 ends----------------------------------------------------*/


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
	for (pugi::xml_node entity = testCase.first_child(); entity; entity = entity.next_sibling())
	{
		pugi::xml_attribute type = entity.attribute("Type");
		CString typeValue(type.value());
		if (typeValue.CompareNoCase("Line") == 0)
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
		else if(typeValue.CompareNoCase("Circle") == 0)
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
		else if (typeValue.CompareNoCase("Polygon") == 0)
		{
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
	}
	return TRUE;
}
void CDemo_ClipView_VCDlg::ClearTestCaseData()
{
	hasOutCanvasData = FALSE;
	boundary.vertexs.clear();
	circles.clear();
	lines.clear();
	if (beginTIdList.GetSize() > 0)
	{
		beginTIdList.RemoveAll();
	}
	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}

void CDemo_ClipView_VCDlg::BeginMonitor()
{
	m_btn_clip.EnableWindow(FALSE);
	m_stc_drawing.SetWindowText("裁剪中...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	GetThreadIdList(beginTIdList);
	startTime = clock();
}
void CDemo_ClipView_VCDlg::EndMonitor()
{
	endTime = clock();
	double useTimeS = (endTime - startTime)/1000;
	m_stc_drawing.ShowWindow(SW_HIDE);
	m_stc_info1.ShowWindow(SW_SHOW);
	CString strTime;
	strTime.Format("图形裁剪完毕，耗时：%.3lf 秒！", useTimeS);
	m_stc_info1.SetWindowText(strTime);
	m_stc_info2.ShowWindow(SW_SHOW);
	CList<int> endTIdList;
	GetThreadIdList(endTIdList);
	POSITION pos = endTIdList.GetHeadPosition();
	while(pos)
	{
		int tId = endTIdList.GetAt(pos);
		CString strId;
		strId.Format("%d", tId);
		if (!beginTIdList.Find(tId))
		{
			HANDLE th = OpenThread(THREAD_QUERY_INFORMATION, FALSE, tId);
			if (th)
			{
				DWORD exitCode = 0;
				GetExitCodeThread(th, &exitCode);
				CloseHandle(th);
				if (exitCode == STILL_ACTIVE)
				{
					MessageBox(_T("新建线程仍在运行:") + strId, _T("线程消息"), MB_OK);
					break;
				}
			}
			else
			{
				MessageBox(_T("新建线程无法访问:") + strId, _T("线程消息"), MB_OK);
				break;
			}
		}
		endTIdList.GetNext(pos);
	}
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

		COLORREF clrBoundary = RGB(255,0,0);
		DrawBoundary(boundary, clrBoundary);

	}

	if (hasOutCanvasData)
	{
		m_stc_drawing.SetWindowText("存在超出边界数据！！！");
	}
	else
	{
		m_btn_clip.EnableWindow(TRUE);
		m_stc_drawing.SetWindowText("图形绘制完毕！");
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
	penUse.DeleteObject();
}
void CDemo_ClipView_VCDlg::DrawCircle(Circle circle, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.Arc(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.center.x + circle.radius, circle.center.y + circle.radius, 0, 0, 0, 0);

	dc.SelectObject(penOld);
	penUse.DeleteObject();
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
	penUse.DeleteObject();
}

BOOL CDemo_ClipView_VCDlg::GetThreadIdList(CList<int>& tIdList)
{
	int pID = _getpid();
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pID);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	te32.dwSize = sizeof(THREADENTRY32);
	BOOL bGetThread = Thread32First(hThreadSnap, &te32);
	while(bGetThread)
	{
		if (te32.th32OwnerProcessID == pID)
		{
			tIdList.AddTail(te32.th32ThreadID);
		}
		bGetThread = Thread32Next(hThreadSnap, &te32);
	}
	CloseHandle(hThreadSnap);
	return TRUE;
}