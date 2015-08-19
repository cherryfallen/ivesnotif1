
// Demo_ClipView_VCDlg.h : 头文件
//
#pragma once
#include <vector>
using std::vector;
#include "pugiconfig.hpp"
#include "pugixml.hpp"
#include "GraphicEntity.h"
#include <time.h>
#include <Windows.h>
#include <Psapi.h>
#include "afxwin.h"
#include <algorithm>
#pragma comment(lib,"psapi.lib")

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

//是否测试某一类型。如果为0，将不载入也不处理该类型，以方便测试性能
#define TEST_LINES 1
#define TEST_CIRCLES 1

//是否绘出初始数据和裁剪结果数据。如果为0，将不绘出该数据，以方便测试性能
#define TEST_DRAW_INITIAL 1
#define TEST_DRAW_ANSWER 1

#define MAX_THREAD_NUMBER 16

//double相等可接受精度
#define DOUBLE_DEGREE 0.01

//线程检查间隔
#define THREAD_CHECK_INTERVAL 5
//最小的绘图大小
#define MINIMUM_DRAW_SIZE 100
//检查内存泄漏
#define _CRTDBG_MAP_ALLOC


struct Vector
{
	double x,y;
};
struct XPoint{
	double x;
	double y;
};
struct ThreadTime
{
	int i;
	double time;
};
struct  _param   {   //这个结构是用于保存每个进程需要画的圆、线、多边形等
	vector<Line> thread_lines;
	vector<Circle> thread_circles;
	Boundary boundary;
	int i;
};

struct  _arc2draw   {  //这个结构是用于保存一个需要画的弧线
	CRect rect;
	CPoint start_point;
	CPoint end_point;
};

struct IntersectPoint
{
public:
	bool isIntoPoly;
	int contexPoint;
	double t;
	CPoint point;
};


// CDemo_ClipView_VCDlg 对话框
class CDemo_ClipView_VCDlg : public CDialogEx
{
// 构造
public:
	CDemo_ClipView_VCDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DEMO_CLIPVIEW_VC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CButton m_btn_clip;
	CStatic m_stc_drawing;
	CStatic m_stc_info1;
	CStatic m_stc_info2;
	afx_msg void OnBnClickedBtnClip();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CString curPath;
	double startTime;
	double endTime;
	BOOL hasOutCanvasData;
	CList<int> beginTIdList;

	void ClearTestCaseData();
	BOOL LoadTestCaseData(CString xmlPath, CString caseID);
	void DrawTestCase(CString xmlPath, CString caseID);
	void BeginMonitor();
	void EndMonitor();
	BOOL XmlNodeToPoint(pugi::xml_node node, CPoint& piont);
	int  SplitCStringToArray(CString str,CStringArray& strArray, CString split);
	BOOL IsPointOutCanvas(CPoint point);
	BOOL IsLineOutCanvas(Line line);
	BOOL IsCircleOutCanvas(Circle circle);
	BOOL IsBoundaryOutCanvas(Boundary boundary);
	BOOL GetThreadIdList(CList<int>& tIdList);

public:
	void DrawLine(Line line, COLORREF clr);
	void DrawCircle(Circle circle, COLORREF clr);
	void DrawBoundary(Boundary boundary, COLORREF clr);
	static DWORD WINAPI ThreadProc2(LPVOID lpParam);
	static DWORD WINAPI ThreadProc_Paint(LPVOID lpParam);

	static const int INTIALIZE=0;

	static vector<Line> lines;                                     //用于存储读取到的所有线
	static vector<Circle> circles;									//用于存储读取到的所有圆
	static Boundary boundary;										//用于存储读取到的多边形窗口

	static int circle_in_bound;									//圆在多边形内的数量
	static int circle_inter_bound;								//圆与多边形相交的数量
	static int line_in_bound;
	static int line_out_bound;
	static int line_overlap_bound;

	//用于存储需要画的线的全局变量的容器
	static vector<Line> lines_to_draw[MAX_THREAD_NUMBER];			//用于存储所有要画的线
	static vector<_arc2draw> circles_to_draw[MAX_THREAD_NUMBER];    //用于存储所有要画的弧线
	static vector<Line> lines_drawing[MAX_THREAD_NUMBER];			//用于存储正在画的线
	static vector<_arc2draw> circles_drawing[MAX_THREAD_NUMBER];	//用于存储正在画的圆

	static CDC MemDC[MAX_THREAD_NUMBER];							//多重缓冲区
	static CBitmap MemBitmap[MAX_THREAD_NUMBER];
	static bool computeFinished;									//标记计算是否完成
	static CPen penCircle,penLine;
	static CRITICAL_SECTION critical_sections[MAX_THREAD_NUMBER];	//为每个线程分配一个临界区
	static int circle_number[2][MAX_THREAD_NUMBER];					//为各类圆在每个线程中统计的数目
	static int line_number[3][MAX_THREAD_NUMBER];					//为各类线在每个线程中统计的数目
	static HANDLE hThread[MAX_THREAD_NUMBER];						//用于存储线程句柄
	static HANDLE hThread_Paint[MAX_THREAD_NUMBER];					//用于存储线程句柄
	static int THREAD_NUMBER;										//计算线程数
	static int PAINT_THREAD_NUMBER;									//绘图线程数

	static bool isConvexPoly;										//多边形的凹凸性，以选择不同的算法
	static vector<BOOL> convexPoint;								//多边形的点的凹凸性，true为凸点
	static vector<RECT> edgeRect;									//多边形的边的外矩形框
	static vector<Vector> normalVector;								//多边形的边的内法向量
	

	//dqc methods begins
	static void preprocessJudgeConvexPoint(vector<BOOL>&);
	static void preprocessNormalVector(vector<Vector>&,vector<BOOL>&);
	static void preprocessEdgeRect(const Boundary);
	static inline bool isPointInLine(CPoint& p,CPoint& p1,CPoint& p2,int i);
	static bool isOverlap(Line l);
	static inline int crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2);
	static void dealConcave(vector<Line>&,Boundary&,int);
	static bool Compare(IntersectPoint,IntersectPoint);
	//dqc methods ends




	//gs methods begins	
	static void dealConvex(vector<Line>&,Boundary&,int);
	static bool InBox(Line&);
	static int Multinomial(int,int,int,int,int,int);
	static CPoint CrossPoint(Line&,Line&);
	static bool IsOnline(CPoint&,Line&);
	static bool Intersect(CPoint&,CPoint&,Line&);
	static Line result(Line&);

	//gs methods ends

	//xh methods begins
	static void getInterpointArray(vector<XPoint>&,int,vector<Circle>&);
	static struct XPoint getInterpoint(double,int,int,int,int);
	static bool isPointInBoundary(XPoint&);
	static bool isPointInBoundary(CPoint&);
	static XPoint getMiddlePoint(vector<XPoint>&,int,int,vector<Circle>&);
	static double getAngle(double,double,double,double,double);
	static void forCircleRun(vector<Circle>&,Boundary&,int);
	//xh methods ends

};
		
    

