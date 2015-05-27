
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
//#include "precise.h"

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
};

//dqc methods begins
void preprocessJudgeConvexPoint(vector<BOOL>&);
void preprocessNormalVector(vector<Vector>&,vector<BOOL>&);
void preprocessEdgeRect(const Boundary);

int crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2);
void dealConcave(vector<Line>&,Boundary&,int);
//dqc methods ends




//gs methods begins	
void dealConvex(vector<Line>&,Boundary&,int);
bool InBox(Line&);
int Multinomial(int,int,int,int,int,int);
CPoint CrossPoint(Line&,Line&);
bool IsOnline(CPoint&,Line&);
bool Intersect(CPoint&,CPoint&,Line&);
Line result(Line&);

//gs methods ends

//xh methods begins
void getInterpointArray(vector<XPoint>&,int,vector<Circle>&);
struct XPoint getInterpoint(double,int,int,int,int);
bool isPointInBoundary(XPoint&);
bool isPointInBoundary(CPoint&);
XPoint getMiddlePoint(vector<XPoint>&,int,int,vector<Circle>&);
double getAngle(double,double,double,double,double);
static void forCircleRun(vector<Circle>&,Boundary&,int);
//xh methods ends