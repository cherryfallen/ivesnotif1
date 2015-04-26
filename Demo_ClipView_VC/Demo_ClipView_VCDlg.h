
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
#include "precise.h"

struct Vector
{
	double x,y;
};
struct XPoint{
	double x;
	double y;
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
	CStringArray complexArray;
	double startTime;
	double endTime;
	double startMemory;
	double endMemory;
	BOOL hasOutCanvasData;

	void ClearTestCaseData();
	BOOL LoadTestCaseData(CString xmlPath, CString caseID);
	void DrawTestCase(CString xmlPath, CString caseID);
	void BeginTimeAndMemoryMonitor();
	void EndTimeAndMemoryMonitor();
	BOOL XmlNodeToPoint(pugi::xml_node node, CPoint& piont);
	int  SplitCStringToArray(CString str,CStringArray& strArray, CString split);
	BOOL IsPointOutCanvas(CPoint point);
	BOOL IsLineOutCanvas(Line line);
	BOOL IsCircleOutCanvas(Circle circle);
	BOOL IsBoundaryOutCanvas(Boundary boundary);

public:
	static double maxMemory;
	static bool m_bStopTimer;
	static int timerId;
	void DrawLine(Line line, COLORREF clr);
	void DrawCircle(Circle circle, COLORREF clr);
	void DrawBoundary(Boundary boundary, COLORREF clr);
	static DWORD WINAPI ThreadProc2(LPVOID lpParam);

};

//dqc methods begins
void preprocessJudgeConvexPoint(vector<BOOL>&);
void preprocessNormalVector(vector<Vector>&,vector<BOOL>&);
void preprocessEdgeRect(const Boundary);

//vector<BOOL> convexPoint;//多边形的点的凹凸性，true为凸点
//vector<RECT> edgeRect;//多边形的边的外矩形框
//vector<IntersectPoint> intersectPoint;//线段的所有交点（也包括线段的起点与终点）
int crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2);
void dealConcave(vector<Line>&,Boundary&);
//dqc methods ends




//gs methods begins	
void dealConvex(vector<Line>&,Boundary&);
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
static void forCircleRun(vector<Circle>&,Boundary&);
//xh methods ends
