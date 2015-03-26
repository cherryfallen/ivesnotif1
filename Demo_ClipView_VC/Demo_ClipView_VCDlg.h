
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


struct r_lineNum
{
	CPoint point;
	int num_line;
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
	vector<Line> lines;
	vector<Circle> circles;
	Boundary boundary;
	void DrawLine(Line line, COLORREF clr);
	void DrawCircle(Circle circle, COLORREF clr);
	void DrawBoundary(Boundary boundary, COLORREF clr);

private:
	void JudgeConvexPoint();
	vector<BOOL> convexPoint;
	vector<RECT> edgeRect;
	vector<IntersectPoint> intersectPoint;

	void dealConvex();
	void dealConcave();

	void ClearTestLines();


	void getInterpointArray(vector<r_lineNum>&,int);
	struct r_lineNum getInterpoint(double,int,int,int,int,int);
	bool isPointInBoundary(CPoint&);
	CPoint getMiddlePoint(vector<r_lineNum>&,int,int);
	double getAngle(long,long,long,long,double);
	void ClearPartialTestCaseData();
	void forCircleRun();
};
